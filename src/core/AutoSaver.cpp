#include "AutoSaver.h"
#include "CodeVersionManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

AutoSaver::AutoSaver(QObject *parent)
    : QObject(parent)
    , m_versionManager(nullptr)
{
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &AutoSaver::performSave);
}

void AutoSaver::setQuestionId(const QString &id)
{
    m_questionId = id;
    qDebug() << "[AutoSaver] Question ID set to:" << id;
}

void AutoSaver::setContent(const QString &content)
{
    m_content = content;
}

void AutoSaver::triggerSave()
{
    m_debounceTimer->start();
}

void AutoSaver::forceSave()
{
    qDebug() << "[AutoSaver] Force save called";
    performSave();
}

void AutoSaver::setVersionManager(CodeVersionManager *versionManager)
{
    m_versionManager = versionManager;
}

void AutoSaver::saveVersion(bool testPassed, int passedTests, int totalTests)
{
    if (m_questionId.isEmpty() || m_content.isEmpty()) return;
    
    if (m_versionManager) {
        QString testResult = QString("%1/%2").arg(passedTests).arg(totalTests);
        m_versionManager->saveVersion(m_questionId, m_content, testPassed, testResult);
        emit versionSaved(m_questionId);
    }
}

void AutoSaver::performSave()
{
    if (m_questionId.isEmpty()) {
        qDebug() << "[AutoSaver] WARNING: questionId is empty, cannot save!";
        return;
    }
    
    QDir dir("data/user_answers");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 保存为 .cpp 文件（纯文本格式）
    QString filePath = QString("data/user_answers/%1.cpp").arg(m_questionId);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_content.toUtf8());
        file.close();
        qDebug() << "[AutoSaver] Saved code to:" << filePath << "length:" << m_content.length();
        emit saved(m_questionId, m_content);
    } else {
        qDebug() << "[AutoSaver] ERROR: Failed to open file for writing:" << filePath;
    }
}
