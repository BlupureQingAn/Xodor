#ifndef QUESTIONBANKMANAGERDIALOG_H
#define QUESTIONBANKMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "../core/QuestionBankManager.h"

class OllamaClient;

// 题库管理对话框
class QuestionBankManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QuestionBankManagerDialog(OllamaClient *aiClient, QWidget *parent = nullptr);
    
    QString getSelectedBankId() const { return m_selectedBankId; }
    
signals:
    void bankSelected(const QString &bankId);
    void bankDeleted(const QString &bankId);
    void bankRefreshed(const QString &bankId);
    
private slots:
    void onBankListChanged();
    void onBankSelectionChanged();
    void onSwitchBank();
    void onDeleteBank();
    void onRefreshBank();
    void onRenameBank();
    void onImportNewBank();
    
private:
    void setupUI();
    void refreshBankList();
    void updateButtons();
    QString formatBankInfo(const QuestionBankInfo &info) const;
    
    OllamaClient *m_aiClient;
    QString m_selectedBankId;
    
    // UI组件
    QListWidget *m_bankList;
    QLabel *m_infoLabel;
    QPushButton *m_switchBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_renameBtn;
    QPushButton *m_importBtn;
    QPushButton *m_closeBtn;
};

#endif // QUESTIONBANKMANAGERDIALOG_H
