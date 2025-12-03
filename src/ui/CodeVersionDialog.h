#ifndef CODEVERSIONDIALOG_H
#define CODEVERSIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include "../core/CodeVersionManager.h"

// 代码版本历史对话框
class CodeVersionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CodeVersionDialog(const QString &questionId, const QString &questionTitle,
                              CodeVersionManager *versionManager, QWidget *parent = nullptr);
    
    // 获取选中的版本代码（用于恢复）
    QString getSelectedVersionCode() const;
    bool hasSelectedVersion() const { return !m_selectedVersionId.isEmpty(); }
    
signals:
    void versionRestored(const QString &code);
    
private slots:
    void onVersionSelected(QListWidgetItem *item);
    void onRestoreClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    
private:
    void setupUI();
    void loadVersions();
    void updatePreview();
    QString formatVersionItem(const CodeVersion &version) const;
    QString getStatusIcon(bool testPassed) const;
    
    CodeVersionManager *m_versionManager;
    QString m_questionId;
    QString m_questionTitle;
    QString m_selectedVersionId;
    QVector<CodeVersion> m_versions;
    
    // UI 组件
    QLabel *m_titleLabel;
    QLabel *m_countLabel;
    QListWidget *m_versionList;
    QTextEdit *m_codePreview;
    QPushButton *m_restoreBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_closeBtn;
};

#endif // CODEVERSIONDIALOG_H
