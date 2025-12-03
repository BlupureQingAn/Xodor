#ifndef SMARTIMPORTDIALOG_H
#define SMARTIMPORTDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include "../ai/SmartQuestionImporter.h"

class OllamaClient;

// 导入模式已移除，统一使用AI智能解析

class SmartImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SmartImportDialog(const QString &sourcePath, const QString &bankName,
                              OllamaClient *aiClient, QWidget *parent = nullptr);
    
    bool isSuccess() const { return m_success; }
    QVector<Question> getImportedQuestions() const;
    QString getAnalysisReportPath() const { return m_analysisReportPath; }
    
private slots:
    void onProgressUpdated(const ImportProgress &progress);
    void onLogMessage(const QString &message);
    void onImportCompleted(bool success, const QString &message);
    void onCancelClicked();
    
private:
    void setupUI();
    void startImport();
    
    SmartQuestionImporter *m_importer;
    QString m_sourcePath;
    QString m_bankName;
    QString m_targetPath;
    QString m_analysisReportPath;
    bool m_success;
    
    // UI组件
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QLabel *m_statsLabel;
    QProgressBar *m_progressBar;
    QTextEdit *m_logText;
    QPushButton *m_cancelBtn;
    QPushButton *m_closeBtn;
    QPushButton *m_viewReportBtn;
};

#endif // SMARTIMPORTDIALOG_H
