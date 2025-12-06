#ifndef QUESTION_H
#define QUESTION_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>

enum class QuestionType {
    Code,
    Choice,
    Fill
};

enum class Difficulty {
    Easy,
    Medium,
    Hard
};

struct TestCase {
    QString input;
    QString expectedOutput;
    QString description;  // 测试用例描述（如"基本测试"、"边界条件"等）
    bool isAIGenerated = false;  // 是否AI生成的测试数据
};

class Question
{
public:
    Question();
    explicit Question(const QJsonObject &json);
    
    QJsonObject toJson() const;
    
    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QuestionType type() const { return m_type; }
    Difficulty difficulty() const { return m_difficulty; }
    QStringList tags() const { return m_tags; }
    QString description() const { return m_description; }
    QStringList options() const { return m_options; }
    QVector<TestCase> testCases() const { return m_testCases; }
    QString referenceAnswer() const { return m_referenceAnswer; }
    
    void setId(const QString &id) { m_id = id; }
    void setTitle(const QString &title) { m_title = title; }
    void setType(QuestionType type) { m_type = type; }
    void setDifficulty(Difficulty diff) { m_difficulty = diff; }
    void setTags(const QStringList &tags) { m_tags = tags; }
    void setDescription(const QString &desc) { m_description = desc; }
    void setOptions(const QStringList &opts) { m_options = opts; }
    void setTestCases(const QVector<TestCase> &cases) { m_testCases = cases; }
    void setReferenceAnswer(const QString &ans) { m_referenceAnswer = ans; }
    
    // Markdown支持
    static Question fromMarkdownFile(const QString &filePath);
    static Question fromMarkdown(const QString &content);
    bool saveAsMarkdown(const QString &filePath) const;
    QString toMarkdown() const;
    
private:
    QString m_id;
    QString m_title;
    QuestionType m_type;
    Difficulty m_difficulty;
    QStringList m_tags;
    QString m_description;
    QStringList m_options;
    QVector<TestCase> m_testCases;
    QString m_referenceAnswer;
};

#endif // QUESTION_H
