#include "QuestionBank.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

QuestionBank::QuestionBank(QObject *parent)
    : QObject(parent)
{
}

void QuestionBank::loadFromDirectory(const QString &dirPath)
{
    m_questions.clear();
    
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const auto &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const auto &val : arr) {
                    m_questions.append(Question(val.toObject()));
                }
            } else {
                m_questions.append(Question(doc.object()));
            }
            
            file.close();
        }
    }
    
    emit questionsLoaded(m_questions.size());
}

void QuestionBank::addQuestion(const Question &question)
{
    m_questions.append(question);
}

Question QuestionBank::getQuestion(const QString &id) const
{
    for (const auto &q : m_questions) {
        if (q.id() == id) return q;
    }
    return Question();
}

void QuestionBank::clear()
{
    m_questions.clear();
}

void QuestionBank::removeQuestion(int index)
{
    if (index >= 0 && index < m_questions.size()) {
        m_questions.remove(index);
    }
}
