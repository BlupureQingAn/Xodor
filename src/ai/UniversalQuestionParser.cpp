#include "UniversalQuestionParser.h"
#include <QDebug>
#include <QRegularExpression>

UniversalQuestionParser::UniversalQuestionParser()
{
    // 初始化题目分隔符模式（支持中英文）
    m_questionPatterns = {
        QRegularExpression("^#{1,3}\\s*(.+)$"),                    // Markdown 标题
        QRegularExpression("^第\\s*[0-9一二三四五六七八九十]+\\s*[题道]"),  // 第X题
        QRegularExpression("^[0-9]+[.、)]\\s*(.+)$"),              // 1. 题目
        QRegularExpression("^Problem\\s+[0-9]+", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^Question\\s+[0-9]+", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^题目\\s*[0-9]+"),
        QRegularExpression("^-{3,}$|^={3,}$|^\\*{3,}$")           // 分隔线
    };
    
    // 初始化输入模式
    m_inputPatterns = {
        QRegularExpression("^输入[：:：]", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^Input[：:：]?", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^【输入】"),
        QRegularExpression("^\\[输入\\]"),
        QRegularExpression("输入格式"),
        QRegularExpression("输入样例")
    };
    
    // 初始化输出模式
    m_outputPatterns = {
        QRegularExpression("^输出[：:：]", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^Output[：:：]?", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^【输出】"),
        QRegularExpression("^\\[输出\\]"),
        QRegularExpression("输出格式"),
        QRegularExpression("输出样例")
    };
    
    // 初始化示例模式
    m_examplePatterns = {
        QRegularExpression("^示例\\s*[0-9]*[：:：]?"),
        QRegularExpression("^Example\\s*[0-9]*[：:：]?", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^测试用例\\s*[0-9]*"),
        QRegularExpression("^Test\\s+Case\\s*[0-9]*", QRegularExpression::CaseInsensitiveOption)
    };
    
    // 初始化难度模式
    m_difficultyPatterns = {
        QRegularExpression("难度[：:：]?\\s*(简单|中等|困难|Easy|Medium|Hard)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("Difficulty[：:：]?\\s*(简单|中等|困难|Easy|Medium|Hard)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("^(简单|中等|困难|Easy|Medium|Hard)$", QRegularExpression::CaseInsensitiveOption)
    };
    
    // 初始化标签模式
    m_tagPatterns = {
        QRegularExpression("标签[：:：]?\\s*(.+)"),
        QRegularExpression("Tags?[：:：]?\\s*(.+)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("分类[：:：]?\\s*(.+)"),
        QRegularExpression("Category[：:：]?\\s*(.+)", QRegularExpression::CaseInsensitiveOption)
    };
    
    // 初始化限制模式
    m_limitPatterns = {
        QRegularExpression("时间限制[：:：]?\\s*(.+)"),
        QRegularExpression("Time\\s+Limit[：:：]?\\s*(.+)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("内存限制[：:：]?\\s*(.+)"),
        QRegularExpression("Memory\\s+Limit[：:：]?\\s*(.+)", QRegularExpression::CaseInsensitiveOption)
    };
}

ParsePattern UniversalQuestionParser::analyzeFormat(const QString &content)
{
    ParsePattern pattern;
    
    // 检测题目分隔符
    pattern.questionSeparators = detectQuestionSeparators(content);
    
    // 检测测试数据格式
    detectTestCaseFormat(content, pattern);
    
    // 检测元信息格式
    detectMetadataFormat(content, pattern);
    
    // 判断是否包含多道题目
    int questionCount = 0;
    QStringList lines = content.split('\n');
    for (const QString &line : lines) {
        if (isQuestionBoundary(line)) {
            questionCount++;
        }
    }
    pattern.hasMultipleQuestions = (questionCount > 1);
    
    qDebug() << "Format analysis:" 
             << "Multiple questions:" << pattern.hasMultipleQuestions
             << "Question separators:" << pattern.questionSeparators.size();
    
    return pattern;
}

QStringList UniversalQuestionParser::detectQuestionSeparators(const QString &content)
{
    QStringList separators;
    QStringList lines = content.split('\n');
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        for (const QRegularExpression &pattern : m_questionPatterns) {
            if (pattern.match(trimmed).hasMatch()) {
                if (!separators.contains(trimmed.left(10))) {
                    separators.append(trimmed.left(10));
                }
                break;
            }
        }
    }
    
    return separators;
}

void UniversalQuestionParser::detectTestCaseFormat(const QString &content, ParsePattern &pattern)
{
    QStringList lines = content.split('\n');
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        // 检测输入关键词
        for (const QRegularExpression &inputPattern : m_inputPatterns) {
            if (inputPattern.match(trimmed).hasMatch()) {
                if (!pattern.inputKeywords.contains(trimmed.left(10))) {
                    pattern.inputKeywords.append(trimmed.left(10));
                }
            }
        }
        
        // 检测输出关键词
        for (const QRegularExpression &outputPattern : m_outputPatterns) {
            if (outputPattern.match(trimmed).hasMatch()) {
                if (!pattern.outputKeywords.contains(trimmed.left(10))) {
                    pattern.outputKeywords.append(trimmed.left(10));
                }
            }
        }
        
        // 检测示例关键词
        for (const QRegularExpression &examplePattern : m_examplePatterns) {
            if (examplePattern.match(trimmed).hasMatch()) {
                if (!pattern.exampleKeywords.contains(trimmed.left(10))) {
                    pattern.exampleKeywords.append(trimmed.left(10));
                }
            }
        }
    }
    
    pattern.hasStructuredExamples = !pattern.exampleKeywords.isEmpty();
}

void UniversalQuestionParser::detectMetadataFormat(const QString &content, ParsePattern &pattern)
{
    QStringList lines = content.split('\n');
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        // 检测难度关键词
        for (const QRegularExpression &diffPattern : m_difficultyPatterns) {
            if (diffPattern.match(trimmed).hasMatch()) {
                if (!pattern.difficultyKeywords.contains(trimmed.left(10))) {
                    pattern.difficultyKeywords.append(trimmed.left(10));
                }
            }
        }
        
        // 检测标签关键词
        for (const QRegularExpression &tagPattern : m_tagPatterns) {
            if (tagPattern.match(trimmed).hasMatch()) {
                if (!pattern.tagKeywords.contains(trimmed.left(10))) {
                    pattern.tagKeywords.append(trimmed.left(10));
                }
            }
        }
        
        // 检测限制关键词
        for (const QRegularExpression &limitPattern : m_limitPatterns) {
            if (limitPattern.match(trimmed).hasMatch()) {
                if (!pattern.limitKeywords.contains(trimmed.left(10))) {
                    pattern.limitKeywords.append(trimmed.left(10));
                }
            }
        }
    }
}

bool UniversalQuestionParser::isQuestionBoundary(const QString &line)
{
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) return false;
    
    for (const QRegularExpression &pattern : m_questionPatterns) {
        if (pattern.match(trimmed).hasMatch()) {
            return true;
        }
    }
    
    return false;
}

QStringList UniversalQuestionParser::splitMultipleQuestions(const QString &content)
{
    QStringList questions;
    QStringList lines = content.split('\n');
    QString currentQuestion;
    
    for (const QString &line : lines) {
        if (isQuestionBoundary(line) && !currentQuestion.isEmpty()) {
            // 遇到新题目边界，保存当前题目
            questions.append(currentQuestion.trimmed());
            currentQuestion.clear();
        }
        currentQuestion += line + "\n";
    }
    
    // 添加最后一道题目
    if (!currentQuestion.trimmed().isEmpty()) {
        questions.append(currentQuestion.trimmed());
    }
    
    qDebug() << "Split into" << questions.size() << "questions";
    return questions;
}

Question UniversalQuestionParser::parseSingleQuestion(const QString &content)
{
    Question question;
    
    // 提取元信息
    QuestionMetadata metadata = extractMetadata(content);
    question.setTitle(metadata.title);
    question.setDifficulty(metadata.difficulty);
    question.setTags(metadata.tags);
    question.setDescription(metadata.description);
    
    // 提取测试数据
    QVector<TestCase> testCases = extractTestCases(content);
    question.setTestCases(testCases);
    
    // 生成唯一ID
    QString id = metadata.title.toLower().replace(" ", "_");
    id = id.replace(QRegularExpression("[^a-z0-9_]"), "");
    question.setId(id);
    
    return question;
}

QuestionMetadata UniversalQuestionParser::extractMetadata(const QString &content)
{
    QuestionMetadata metadata;
    QStringList lines = content.split('\n');
    
    QString descriptionText;
    bool inDescription = false;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        
        // 提取标题（第一个非空行或第一个标题）
        if (metadata.title.isEmpty()) {
            if (line.startsWith('#')) {
                metadata.title = line.remove(QRegularExpression("^#+\\s*")).trimmed();
                continue;
            } else if (!line.isEmpty() && isQuestionBoundary(line)) {
                metadata.title = line;
                continue;
            }
        }
        
        // 提取难度
        for (const QRegularExpression &pattern : m_difficultyPatterns) {
            QRegularExpressionMatch match = pattern.match(line);
            if (match.hasMatch()) {
                QString diffText = match.captured(1).isEmpty() ? match.captured(0) : match.captured(1);
                metadata.difficulty = parseDifficulty(diffText);
            }
        }
        
        // 提取标签
        for (const QRegularExpression &pattern : m_tagPatterns) {
            QRegularExpressionMatch match = pattern.match(line);
            if (match.hasMatch()) {
                QString tagText = match.captured(1);
                metadata.tags = parseTags(tagText);
            }
        }
        
        // 提取时间限制
        if (line.contains("时间限制") || line.contains("Time Limit")) {
            metadata.timeLimit = line;
        }
        
        // 提取内存限制
        if (line.contains("内存限制") || line.contains("Memory Limit")) {
            metadata.memoryLimit = line;
        }
        
        // 提取描述（题目描述部分）
        if (line.contains("题目描述") || line.contains("Description") || 
            line.contains("问题") || line.contains("Problem")) {
            inDescription = true;
            continue;
        }
        
        if (inDescription) {
            if (line.contains("输入") || line.contains("Input") || 
                line.contains("示例") || line.contains("Example")) {
                inDescription = false;
            } else if (!line.isEmpty()) {
                descriptionText += line + "\n";
            }
        }
    }
    
    metadata.description = descriptionText.trimmed();
    
    // 如果没有提取到标题，使用默认值
    if (metadata.title.isEmpty()) {
        metadata.title = "未命名题目";
    }
    
    return metadata;
}

QVector<TestCase> UniversalQuestionParser::extractTestCases(const QString &content)
{
    QVector<TestCase> testCases;
    QStringList lines = content.split('\n');
    
    QStringList inputs;
    QStringList outputs;
    QString currentInput;
    QString currentOutput;
    bool inInput = false;
    bool inOutput = false;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        // 检测输入开始
        bool isInputLine = false;
        for (const QRegularExpression &pattern : m_inputPatterns) {
            if (pattern.match(trimmed).hasMatch()) {
                isInputLine = true;
                break;
            }
        }
        
        // 检测输出开始
        bool isOutputLine = false;
        for (const QRegularExpression &pattern : m_outputPatterns) {
            if (pattern.match(trimmed).hasMatch()) {
                isOutputLine = true;
                break;
            }
        }
        
        if (isInputLine) {
            // 保存之前的输入
            if (!currentInput.isEmpty()) {
                inputs.append(currentInput.trimmed());
                currentInput.clear();
            }
            inInput = true;
            inOutput = false;
            
            // 检查是否在同一行包含输入内容
            QString inputContent = trimmed;
            inputContent.remove(QRegularExpression("^输入[：:：]?"));
            inputContent.remove(QRegularExpression("^Input[：:：]?", QRegularExpression::CaseInsensitiveOption));
            if (!inputContent.trimmed().isEmpty()) {
                currentInput = inputContent.trimmed();
            }
        } else if (isOutputLine) {
            // 保存之前的输出
            if (!currentOutput.isEmpty()) {
                outputs.append(currentOutput.trimmed());
                currentOutput.clear();
            }
            inOutput = true;
            inInput = false;
            
            // 检查是否在同一行包含输出内容
            QString outputContent = trimmed;
            outputContent.remove(QRegularExpression("^输出[：:：]?"));
            outputContent.remove(QRegularExpression("^Output[：:：]?", QRegularExpression::CaseInsensitiveOption));
            if (!outputContent.trimmed().isEmpty()) {
                currentOutput = outputContent.trimmed();
            }
        } else if (!trimmed.isEmpty()) {
            // 累积输入或输出内容
            if (inInput) {
                currentInput += (currentInput.isEmpty() ? "" : "\n") + trimmed;
            } else if (inOutput) {
                currentOutput += (currentOutput.isEmpty() ? "" : "\n") + trimmed;
            }
        } else {
            // 空行可能表示一组测试数据结束
            if (inInput && !currentInput.isEmpty()) {
                inputs.append(currentInput.trimmed());
                currentInput.clear();
                inInput = false;
            }
            if (inOutput && !currentOutput.isEmpty()) {
                outputs.append(currentOutput.trimmed());
                currentOutput.clear();
                inOutput = false;
            }
        }
    }
    
    // 保存最后的输入输出
    if (!currentInput.isEmpty()) {
        inputs.append(currentInput.trimmed());
    }
    if (!currentOutput.isEmpty()) {
        outputs.append(currentOutput.trimmed());
    }
    
    // 配对输入输出
    testCases = pairInputOutput(inputs, outputs);
    
    qDebug() << "Extracted" << testCases.size() << "test cases";
    return testCases;
}

QVector<TestCase> UniversalQuestionParser::pairInputOutput(const QStringList &inputs, const QStringList &outputs)
{
    QVector<TestCase> testCases;
    int count = qMin(inputs.size(), outputs.size());
    
    for (int i = 0; i < count; ++i) {
        TestCase testCase;
        testCase.input = inputs[i];
        testCase.expectedOutput = outputs[i];
        testCase.description = generateTestCaseDescription(i + 1, count);
        testCases.append(testCase);
    }
    
    return testCases;
}

QString UniversalQuestionParser::generateTestCaseDescription(int index, int total)
{
    if (index == 1) {
        return "基本测试";
    } else if (index == total) {
        return "边界测试";
    } else {
        return QString("测试用例 %1").arg(index);
    }
}

Difficulty UniversalQuestionParser::parseDifficulty(const QString &text)
{
    QString lower = text.toLower();
    if (lower.contains("简单") || lower.contains("easy")) {
        return Difficulty::Easy;
    } else if (lower.contains("中等") || lower.contains("medium")) {
        return Difficulty::Medium;
    } else if (lower.contains("困难") || lower.contains("hard")) {
        return Difficulty::Hard;
    }
    return Difficulty::Medium;  // 默认中等
}

QStringList UniversalQuestionParser::parseTags(const QString &text)
{
    QStringList tags;
    
    // 按常见分隔符拆分
    QStringList separators = {",", "，", ";", "；", "|", "/", " "};
    QString temp = text;
    
    for (const QString &sep : separators) {
        temp = temp.replace(sep, "|");
    }
    
    tags = temp.split('|', Qt::SkipEmptyParts);
    
    // 清理标签
    for (QString &tag : tags) {
        tag = tag.trimmed();
    }
    
    tags.removeAll("");
    return tags;
}

QString UniversalQuestionParser::cleanText(const QString &text)
{
    QString cleaned = text.trimmed();
    cleaned.remove(QRegularExpression("^[#*-]+\\s*"));
    return cleaned;
}

QVector<Question> UniversalQuestionParser::parseContent(const QString &content, const ParsePattern &pattern)
{
    QVector<Question> questions;
    
    if (pattern.hasMultipleQuestions) {
        // 拆分多道题目
        QStringList questionContents = splitMultipleQuestions(content);
        for (const QString &qContent : questionContents) {
            Question q = parseSingleQuestion(qContent);
            if (!q.title().isEmpty()) {
                questions.append(q);
            }
        }
    } else {
        // 单道题目
        Question q = parseSingleQuestion(content);
        if (!q.title().isEmpty()) {
            questions.append(q);
        }
    }
    
    return questions;
}
