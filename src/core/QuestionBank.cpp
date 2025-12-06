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
    if (!dir.exists()) {
        qWarning() << "Question bank directory does not exist:" << dirPath;
        emit questionsLoaded(0);
        return;
    }
    
    // 递归加载所有 JSON 文件
    loadFromDirectoryRecursive(dirPath);
    
    qDebug() << "Loaded" << m_questions.size() << "questions from" << dirPath;
    emit questionsLoaded(m_questions.size());
}

void QuestionBank::loadFromDirectoryRecursive(const QString &dirPath)
{
    QDir dir(dirPath);
    
    // 优先加载MD文件（新格式）
    QStringList mdFilters;
    mdFilters << "*.md";
    QFileInfoList mdFiles = dir.entryInfoList(mdFilters, QDir::Files);
    
    for (const auto &fileInfo : mdFiles) {
        QString fileName = fileInfo.fileName().toLower();
        // 跳过README等非题目文件
        if (fileName.contains("readme") || 
            fileName.contains("拆分规则") ||
            fileName.contains("出题模式") ||
            fileName.contains("规律")) {
            continue;
        }
        
        Question q = Question::fromMarkdownFile(fileInfo.absoluteFilePath());
        if (!q.id().isEmpty()) {
            m_questions.append(q);
        }
    }
    
    // 如果当前目录没有MD文件，尝试加载JSON文件（向后兼容）
    if (mdFiles.isEmpty()) {
        QStringList jsonFilters;
        jsonFilters << "*.json";
        QFileInfoList jsonFiles = dir.entryInfoList(jsonFilters, QDir::Files);
        
        for (const auto &fileInfo : jsonFiles) {
            QFile file(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                
                if (doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (const auto &val : arr) {
                        m_questions.append(Question(val.toObject()));
                    }
                } else if (doc.isObject()) {
                    m_questions.append(Question(doc.object()));
                }
                
                file.close();
            }
        }
    }
    
    // 递归扫描子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        loadFromDirectoryRecursive(subDirInfo.absoluteFilePath());
    }
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
