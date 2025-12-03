#ifndef AIIMPORTDIALOG_NEW_H
#define AIIMPORTDIALOG_NEW_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include "../ai/AIQuestionBankImporter.h"

class OllamaClient;

// AI驱动的导入对话框
class AIImportDialogNew : public QDialog
{
    Q_OBJECT
public:
    explicit AIImportDialogNew(OllamaClient *aiClient, QWidget *parent = nullptr);
    
    // 开始导入
    void startImport(const QString &sourcePath, const QString &categoryName);
    
private slots:
    void onStageChanged(ImportStage stage, const QString &message);
    void onProgressUpdated(int percentage, const QString &message);
    void onImportComplete(const QString &categoryName, int questionCount);
    void onImportFailed(const QString &error);
    void onCancelClicked();
    
private:
    void setupUI();
    QString getStageText(ImportStage stage);
    QString getStageEmoji(ImportStage stage);
    
    AIQuestionBankImporter *m_importer;
    
    // UI组件
    QLabel *m_titleLabel;
    QLabel *m_stageLabel;
    QProgressBar *m_progressBar;
    QTextEdit *m_logText;
    QPushButton *m_cancelBtn;
    QPushButton *m_closeBtn;
};

#endif // AIIMPORTDIALOG_NEW_H
