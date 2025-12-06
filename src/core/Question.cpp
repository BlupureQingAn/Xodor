#include "Question.h"
#include "MarkdownQuestionParser.h"
#include <QUuid>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>

Question::Question()
    : m_type(QuestionType::Code)
    , m_difficulty(Difficulty::Medium)
{
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

Question::Question(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_title = json["title"].toString();
    
    QString typeStr = json["type"].toString();
    if (typeStr == "choice") m_type = QuestionType::Choice;
    else if (typeStr == "fill") m_type = QuestionType::Fill;
    else m_type = QuestionType::Code;
    
    QString diffStr = json["difficulty"].toString();
    if (diffStr == "easy") m_difficulty = Difficulty::Easy;
    else if (diffStr == "hard") m_difficulty = Difficulty::Hard;
    else m_difficulty = Difficulty::Medium;
    
    QJsonArray tagsArray = json["tags"].toArray();
    for (const auto &tag : tagsArray) {
        m_tags.append(tag.toString());
    }
    
    m_description = json["description"].toString();
    
    QJsonArray optsArray = json["options"].toArray();
    for (const auto &opt : optsArray) {
        m_options.append(opt.toString());
    }
    
    QJsonArray casesArray = json["testCases"].toArray();
    for (const auto &caseVal : casesArray) {
        QJsonObject caseObj = caseVal.toObject();
        TestCase tc;
        tc.input = caseObj["input"].toString();
        tc.expectedOutput = caseObj["output"].toString();
        tc.description = caseObj["description"].toString();
        tc.isAIGenerated = caseObj["isAIGenerated"].toBool(false);
        m_testCases.append(tc);
    }
    
    m_referenceAnswer = json["referenceAnswer"].toString();
}

QJsonObject Question::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["title"] = m_title;
    
    QString typeStr = "code";
    if (m_type == QuestionType::Choice) typeStr = "choice";
    else if (m_type == QuestionType::Fill) typeStr = "fill";
    json["type"] = typeStr;
    
    QString diffStr = "medium";
    if (m_difficulty == Difficulty::Easy) diffStr = "easy";
    else if (m_difficulty == Difficulty::Hard) diffStr = "hard";
    json["difficulty"] = diffStr;
    
    QJsonArray tagsArray;
    for (const auto &tag : m_tags) {
        tagsArray.append(tag);
    }
    json["tags"] = tagsArray;
    
    json["description"] = m_description;
    
    QJsonArray optsArray;
    for (const auto &opt : m_options) {
        optsArray.append(opt);
    }
    json["options"] = optsArray;
    
    QJsonArray casesArray;
    for (const auto &tc : m_testCases) {
        QJsonObject caseObj;
        caseObj["input"] = tc.input;
        caseObj["output"] = tc.expectedOutput;
        caseObj["description"] = tc.description;
        caseObj["isAIGenerated"] = tc.isAIGenerated;
        casesArray.append(caseObj);
    }
    json["testCases"] = casesArray;
    
    json["referenceAnswer"] = m_referenceAnswer;
    
    return json;
}

// Markdown支持方法实现
Question Question::fromMarkdownFile(const QString &filePath)
{
    return MarkdownQuestionParser::parseFromFile(filePath);
}

Question Question::fromMarkdown(const QString &content)
{
    return MarkdownQuestionParser::parseFromContent(content);
}

bool Question::saveAsMarkdown(const QString &filePath) const
{
    QString markdown = toMarkdown();
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << markdown;
    file.close();
    
    return true;
}

QString Question::toMarkdown() const
{
    return MarkdownQuestionParser::generateMarkdown(*this);
}
