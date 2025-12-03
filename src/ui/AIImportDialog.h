#ifndef AIIMPORTDIALOG_H
#define AIIMPORTDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "../core/Question.h"

class OllamaClient;

// AI驱动的智能题库导入对话框
class AIImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIImportDialog(const QString &folderPath, OllamaClient *aiClient, QWidget *parent = nullptr);
    
    QVector<Question> getImportedQuestions() const { return m_questions; }
    bool isSuccess() const { return m_success; }
    
signals:
    void importProgress(int current, int total, const QString &message);
    void importFinished(bool success);
    
private slots:
    void startImport();
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    void setupUI();
    void scanFiles();
    void sendToAI();
    QString buildAIPrompt();
    void parseAIResponse(const QString &response);
    
    QString m_folderPath;
    OllamaClient *m_aiClient;
    QVector<Question> m_questions;
    QStringList m_fileContents;
    QStringList m_fileNames;
    bool m_success;
    int m_currentStep;
    
    // UI组件
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QTextEdit *m_logText;
    QPushButton *m_cancelBtn;
    QPushButton *m_closeBtn;
};

#endif // AIIMPORTDIALOG_H
