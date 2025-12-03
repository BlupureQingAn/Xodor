#include "QuestionParser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

QuestionParser::QuestionParser(QObject *parent)
    : QObject(parent)
{
}

QVector<Question> QuestionParser::parseMarkdownFile(const QString &filePath)
{
    QVector<Question> questions;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return questions;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
    
    // 策略1: 如果文件名包含题号，直接解析整个文件为一道题
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.baseName();
    if (QRegularExpression(R"(^\d+[_\-\.]|第\d+题|题目\d+)").match(fileName).hasMatch()) {
        Question q = extractQuestion(content);
        if (!q.title().isEmpty()) {
            questions.append(q);
            return questions;
        }
    }
    
    // 策略2: 按一级标题分割（# 标题）
    QRegularExpression headerRegex("(?=^#\\s+[^#])", QRegularExpression::MultilineOption);
    QStringList sections = content.split(headerRegex, Qt::SkipEmptyParts);
    
    // 如果只有一个section，说明是单题文件
    if (sections.size() == 1) {
        Question q = extractQuestion(sections.first());
        if (!q.title().isEmpty()) {
            questions.append(q);
        }
        return questions;
    }
    
    // 策略3: 多题文件，智能识别每个section
    for (const QString &section : sections) {
        if (section.trimmed().isEmpty()) continue;
        
        QString firstLine = section.split('\n').first().trimmed();
        firstLine.remove(QRegularExpression("^#+\\s*"));
        
        // 宽松的题目识别规则
        bool isQuestion = true;  // 默认认为是题目
        
        // 排除规则：明显不是题目的内容
        if (firstLine.contains(QRegularExpression(R"(^(?:目录|索引|说明|介绍|前言|附录|参考|版本|更新|changelog|readme|license)$)", 
                                                  QRegularExpression::CaseInsensitiveOption))) {
            isQuestion = false;
        }
        
        // 如果标题太短（少于2个字符），可能不是题目
        if (firstLine.length() < 2) {
            isQuestion = false;
        }
        
        if (isQuestion) {
            Question q = extractQuestion(section);
            // 只要有标题就添加，不强制要求测试用例（可能在描述中）
            if (!q.title().isEmpty()) {
                questions.append(q);
            }
        }
    }
    
    return questions;
}

QVector<Question> QuestionParser::parseWithAI(const QString &content)
{
    // AI辅助解析 - 用于复杂或非标准格式的题目
    // 这个函数会被MainWindow在导入失败时调用
    
    QVector<Question> questions;
    
    // 构建AI提示词
    QString prompt = R"(
请分析以下Markdown文本，提取其中的编程题目信息。

要求：
1. 识别所有题目（忽略目录、说明等非题目内容）
2. 对每道题提取：标题、难度、描述、测试用例
3. 测试用例格式：输入和期望输出

文本内容：
---
%1
---

请以JSON格式返回，格式如下：
{
  "questions": [
    {
      "title": "题目标题",
      "difficulty": "简单/中等/困难",
      "description": "题目描述",
      "testCases": [
        {"input": "输入数据", "output": "期望输出"}
      ]
    }
  ]
}
)";
    
    prompt = prompt.arg(content);
    
    // 注意：这里需要同步等待AI响应，或者使用信号槽异步处理
    // 当前返回空，实际使用时需要集成OllamaClient
    
    return questions;
}

Question QuestionParser::extractQuestion(const QString &markdown)
{
    Question q;
    
    QStringList lines = markdown.split('\n');
    if (lines.isEmpty()) return q;
    
    // 提取标题
    QString title = lines.first().trimmed();
    title.remove(QRegularExpression("^#+\\s*"));
    q.setTitle(title);
    
    // 生成唯一ID
    q.setId(QString("q_%1").arg(qHash(title)));
    
    // 提取描述和测试用例
    QString description;
    QVector<TestCase> testCases;
    bool inCodeBlock = false;
    bool inTestCase = false;
    bool inDescription = false;
    QString currentInput, currentOutput;
    QString codeBlockContent;
    
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i];
        QString trimmedLine = line.trimmed();
        
        // 跳过空行
        if (trimmedLine.isEmpty()) {
            if (inDescription) {
                description += "\n";
            }
            continue;
        }
        
        // 检测代码块
        if (trimmedLine.startsWith("```")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeBlockContent.clear();
            } else {
                inCodeBlock = false;
                // 代码块结束，可能是测试用例
                if (!codeBlockContent.isEmpty() && inTestCase) {
                    if (currentInput.isEmpty()) {
                        currentInput = codeBlockContent.trimmed();
                    } else {
                        currentOutput = codeBlockContent.trimmed();
                        if (!currentInput.isEmpty() && !currentOutput.isEmpty()) {
                            TestCase tc;
                            tc.input = currentInput;
                            tc.expectedOutput = currentOutput;
                            testCases.append(tc);
                            currentInput.clear();
                            currentOutput.clear();
                        }
                    }
                }
                codeBlockContent.clear();
            }
            continue;
        }
        
        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }
        
        // 检测难度标签
        if (trimmedLine.contains(QRegularExpression(R"(难度[:：])", QRegularExpression::CaseInsensitiveOption))) {
            if (trimmedLine.contains("简单") || trimmedLine.contains("easy", Qt::CaseInsensitive)) {
                q.setDifficulty(Difficulty::Easy);
            } else if (trimmedLine.contains("困难") || trimmedLine.contains("hard", Qt::CaseInsensitive)) {
                q.setDifficulty(Difficulty::Hard);
            } else {
                q.setDifficulty(Difficulty::Medium);
            }
            continue;
        }
        
        // 检测题目描述部分
        if (trimmedLine.contains(QRegularExpression(R"(^##\s*题目描述|^##\s*Description)", QRegularExpression::CaseInsensitiveOption))) {
            inDescription = true;
            inTestCase = false;
            continue;
        }
        
        // 检测示例/测试用例标题
        if (trimmedLine.contains(QRegularExpression(R"(^##\s*示例|^##\s*Example|^###\s*示例)", QRegularExpression::CaseInsensitiveOption))) {
            inTestCase = true;
            inDescription = false;
            continue;
        }
        
        // 检测其他section（提示、进阶等）
        if (trimmedLine.startsWith("##") && 
            (trimmedLine.contains("提示") || trimmedLine.contains("进阶") || 
             trimmedLine.contains("约束") || trimmedLine.contains("限制"))) {
            inTestCase = false;
            inDescription = false;
            // 这些内容也加入描述
            description += "\n\n" + line + "\n";
            inDescription = true;
            continue;
        }
        
        // 检测输入输出（多种格式）
        // 格式1: "输入: xxx" 或 "Input: xxx"
        QRegularExpression inputRegex(R"(^(?:输入|input|Input)[:：]\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression outputRegex(R"(^(?:输出|output|Output|期望输出)[:：]\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
        
        QRegularExpressionMatch inputMatch = inputRegex.match(trimmedLine);
        QRegularExpressionMatch outputMatch = outputRegex.match(trimmedLine);
        
        if (inputMatch.hasMatch()) {
            currentInput = inputMatch.captured(1).trimmed();
            inTestCase = true;
        } else if (outputMatch.hasMatch()) {
            currentOutput = outputMatch.captured(1).trimmed();
            if (!currentInput.isEmpty() && !currentOutput.isEmpty()) {
                TestCase tc;
                tc.input = currentInput;
                tc.expectedOutput = currentOutput;
                testCases.append(tc);
                currentInput.clear();
                currentOutput.clear();
            }
        } else if (inDescription) {
            description += line + "\n";
        }
    }
    
    q.setDescription(description.trimmed());
    q.setTestCases(testCases);
    q.setType(QuestionType::Code);
    
    // 如果没有设置难度，默认为中等
    if (q.difficulty() != Difficulty::Easy && 
        q.difficulty() != Difficulty::Medium && 
        q.difficulty() != Difficulty::Hard) {
        q.setDifficulty(Difficulty::Medium);
    }
    
    return q;
}
