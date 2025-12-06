#include "MarkdownQuestionParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

Question MarkdownQuestionParser::parseFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开MD文件:" << filePath;
        return Question();
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
    
    return parseFromContent(content);
}

Question MarkdownQuestionParser::parseFromContent(const QString &content)
{
    Question q;
    
    // 提取Front Matter
    QJsonObject metadata = extractFrontMatter(content);
    
    if (metadata.isEmpty()) {
        qWarning() << "MD文件缺少Front Matter元数据";
        return q;
    }
    
    // 设置基本信息
    q.setId(metadata["id"].toString());
    q.setTitle(metadata["title"].toString());
    
    // 解析类型
    QString typeStr = metadata["type"].toString("code");
    if (typeStr == "choice") {
        q.setType(QuestionType::Choice);
    } else if (typeStr == "fill") {
        q.setType(QuestionType::Fill);
    } else {
        q.setType(QuestionType::Code);
    }
    
    // 解析难度
    QString diffStr = metadata["difficulty"].toString("medium");
    if (diffStr == "easy") {
        q.setDifficulty(Difficulty::Easy);
    } else if (diffStr == "hard") {
        q.setDifficulty(Difficulty::Hard);
    } else {
        q.setDifficulty(Difficulty::Medium);
    }
    
    // 解析标签
    QJsonArray tagsArray = metadata["tags"].toArray();
    QStringList tags;
    for (const QJsonValue &tagVal : tagsArray) {
        tags.append(tagVal.toString());
    }
    q.setTags(tags);
    
    // 移除Front Matter，获取题目描述
    QString description = removeFrontMatter(content);
    q.setDescription(description);
    
    // 解析测试用例
    QVector<TestCase> testCases = parseTestCases(description);
    q.setTestCases(testCases);
    
    return q;
}

QJsonObject MarkdownQuestionParser::extractFrontMatter(const QString &content)
{
    // 匹配 --- 包裹的Front Matter
    QRegularExpression frontMatterPattern(
        R"(^---\s*\n(.*?)\n---\s*\n)",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatch match = frontMatterPattern.match(content);
    if (!match.hasMatch()) {
        return QJsonObject();
    }
    
    QString yamlContent = match.captured(1);
    return parseYamlFrontMatter(yamlContent);
}

QVector<TestCase> MarkdownQuestionParser::parseTestCases(const QString &content)
{
    QVector<TestCase> testCases;
    
    // 匹配测试用例格式1：### 测试用例N：描述
    QRegularExpression testCasePattern(
        R"(###?\s*测试用例\s*\d*[：:]\s*(.+?)\n\*\*输入[：:]\*\*\s*\n```[^\n]*\n(.*?)\n```\s*\n\*\*输出[：:]\*\*\s*\n```[^\n]*\n(.*?)\n```)",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatchIterator it = testCasePattern.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        TestCase tc;
        tc.description = match.captured(1).trimmed();
        tc.input = match.captured(2).trimmed();
        tc.expectedOutput = match.captured(3).trimmed();
        tc.isAIGenerated = false;
        testCases.append(tc);
    }
    
    // 如果没有匹配到，尝试格式2：**输入样例N：**
    if (testCases.isEmpty()) {
        QRegularExpression samplePattern(
            R"(\*\*输入样例\s*\d*[：:]\*\*\s*\n```[^\n]*\n(.*?)\n```\s*\n\*\*输出样例\s*\d*[：:]\*\*\s*\n```[^\n]*\n(.*?)\n```)",
            QRegularExpression::DotMatchesEverythingOption
        );
        
        QRegularExpressionMatchIterator it2 = samplePattern.globalMatch(content);
        int index = 1;
        while (it2.hasNext()) {
            QRegularExpressionMatch match = it2.next();
            TestCase tc;
            tc.description = QString("样例%1").arg(index++);
            tc.input = match.captured(1).trimmed();
            tc.expectedOutput = match.captured(2).trimmed();
            tc.isAIGenerated = false;
            testCases.append(tc);
        }
    }
    
    return testCases;
}

QString MarkdownQuestionParser::generateMarkdown(const Question &question)
{
    QString markdown;
    
    // 生成Front Matter
    QJsonObject metadata;
    metadata["id"] = question.id();
    metadata["title"] = question.title();
    
    QString typeStr = "code";
    if (question.type() == QuestionType::Choice) typeStr = "choice";
    else if (question.type() == QuestionType::Fill) typeStr = "fill";
    metadata["type"] = typeStr;
    
    QString diffStr = "medium";
    if (question.difficulty() == Difficulty::Easy) diffStr = "easy";
    else if (question.difficulty() == Difficulty::Hard) diffStr = "hard";
    metadata["difficulty"] = diffStr;
    
    QJsonArray tagsArray;
    for (const QString &tag : question.tags()) {
        tagsArray.append(tag);
    }
    metadata["tags"] = tagsArray;
    
    markdown += generateYamlFrontMatter(metadata);
    markdown += "\n";
    
    // 添加题目描述
    markdown += question.description();
    
    // 如果描述中没有测试用例，添加测试用例部分
    if (!question.description().contains("## 测试用例") && 
        !question.description().contains("##测试用例")) {
        markdown += "\n\n## 测试用例\n\n";
        
        int index = 1;
        for (const TestCase &tc : question.testCases()) {
            markdown += QString("### 测试用例%1：%2\n").arg(index++).arg(tc.description);
            markdown += "**输入：**\n```\n";
            markdown += tc.input;
            markdown += "\n```\n\n";
            markdown += "**输出：**\n```\n";
            markdown += tc.expectedOutput;
            markdown += "\n```\n\n";
        }
    }
    
    return markdown;
}

QString MarkdownQuestionParser::removeFrontMatter(const QString &content)
{
    QRegularExpression frontMatterPattern(
        R"(^---\s*\n.*?\n---\s*\n)",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    QString result = content;
    result.remove(frontMatterPattern);
    return result.trimmed();
}

QJsonObject MarkdownQuestionParser::parseYamlFrontMatter(const QString &yamlContent)
{
    QJsonObject json;
    
    // 简单的YAML解析（支持基本的key: value格式）
    QStringList lines = yamlContent.split('\n');
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty() || trimmedLine.startsWith('#')) {
            continue;
        }
        
        // 解析 key: value
        int colonPos = trimmedLine.indexOf(':');
        if (colonPos > 0) {
            QString key = trimmedLine.left(colonPos).trimmed();
            QString value = trimmedLine.mid(colonPos + 1).trimmed();
            
            // 移除引号
            if (value.startsWith('"') && value.endsWith('"')) {
                value = value.mid(1, value.length() - 2);
            } else if (value.startsWith('\'') && value.endsWith('\'')) {
                value = value.mid(1, value.length() - 2);
            }
            
            // 处理数组 [item1, item2]
            if (value.startsWith('[') && value.endsWith(']')) {
                value = value.mid(1, value.length() - 2);
                QStringList items = value.split(',');
                QJsonArray array;
                for (QString item : items) {
                    item = item.trimmed();
                    // 移除引号
                    if (item.startsWith('"') && item.endsWith('"')) {
                        item = item.mid(1, item.length() - 2);
                    } else if (item.startsWith('\'') && item.endsWith('\'')) {
                        item = item.mid(1, item.length() - 2);
                    }
                    array.append(item);
                }
                json[key] = array;
            } else {
                json[key] = value;
            }
        }
    }
    
    return json;
}

QString MarkdownQuestionParser::generateYamlFrontMatter(const QJsonObject &metadata)
{
    QString yaml = "---\n";
    
    // 按顺序输出字段
    QStringList keys = {"id", "title", "type", "difficulty", "tags"};
    
    for (const QString &key : keys) {
        if (!metadata.contains(key)) {
            continue;
        }
        
        QJsonValue value = metadata[key];
        
        if (value.isArray()) {
            // 数组格式
            QJsonArray array = value.toArray();
            yaml += key + ": [";
            QStringList items;
            for (const QJsonValue &item : array) {
                items.append('"' + item.toString() + '"');
            }
            yaml += items.join(", ");
            yaml += "]\n";
        } else {
            // 字符串格式
            yaml += key + ": \"" + value.toString() + "\"\n";
        }
    }
    
    yaml += "---\n";
    return yaml;
}
