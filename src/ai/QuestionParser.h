#ifndef QUESTIONPARSER_H
#define QUESTIONPARSER_H

#include <QObject>
#include <QString>
#include "../core/Question.h"

class QuestionParser : public QObject
{
    Q_OBJECT
public:
    explicit QuestionParser(QObject *parent = nullptr);
    
    QVector<Question> parseMarkdownFile(const QString &filePath);
    QVector<Question> parseWithAI(const QString &content);
    
private:
    Question extractQuestion(const QString &markdown);
};

#endif // QUESTIONPARSER_H
