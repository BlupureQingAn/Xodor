#ifndef QUESTIONBANKMANAGERDIALOG_H
#define QUESTIONBANKMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "../core/QuestionBankManager.h"
#include "../core/Question.h"

class OllamaClient;

// 题库管理对话框
class QuestionBankManagerDialog : public QDialog
{
    Q_OBJECT
    
public:
    // 题库进度统计
    struct BankProgress {
        int totalCount = 0;
        int completedCount = 0;
        int masteredCount = 0;
        int inProgressCount = 0;
        int notStartedCount = 0;
    };
    
    explicit QuestionBankManagerDialog(OllamaClient *aiClient, QWidget *parent = nullptr);
    
    QString getSelectedBankId() const { return m_selectedBankId; }
    
signals:
    void bankDeleted(const QString &bankId);
    void bankRefreshed(const QString &bankId);
    
private slots:
    void onBankListChanged();
    void onBankSelectionChanged();
    void onViewBankDetails();
    void onDeleteBank();
    void onRefreshBank();
    void onRenameBank();
    void onExportPath();
    void onImportNewBank();
    
private:
    void setupUI();
    void refreshBankList();
    void updateButtons();
    QString formatBankInfo(const QuestionBankInfo &info) const;
    int countQuestionsInDirectory(const QString &dirPath) const;
    void countFilesRecursive(const QString &dirPath, int &count) const;
    
    // 进度统计相关
    BankProgress calculateBankProgress(const QString &bankPath) const;
    QVector<Question> loadQuestionsFromPath(const QString &dirPath) const;
    void loadQuestionsRecursive(const QString &dirPath, QVector<Question> &questions) const;
    
    OllamaClient *m_aiClient;
    QString m_selectedBankId;
    
    // UI组件
    QListWidget *m_bankList;
    QLabel *m_infoLabel;
    QPushButton *m_viewBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_renameBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_importBtn;
    QPushButton *m_closeBtn;
};

#endif // QUESTIONBANKMANAGERDIALOG_H
