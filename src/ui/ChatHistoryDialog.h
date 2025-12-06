#ifndef CHATHISTORYDIALOG_H
#define CHATHISTORYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

struct ConversationInfo {
    QString questionId;
    QString questionTitle;
    QDateTime lastModified;
    int messageCount;
    QString filePath;
};

class ChatHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChatHistoryDialog(QWidget *parent = nullptr);
    
    QString getSelectedConversationId() const { return m_selectedId; }
    
    // 设置当前题目ID，只显示该题目的对话历史
    void setCurrentQuestionId(const QString &questionId);
    
signals:
    void conversationSelected(const QString &questionId);
    void conversationDeleted(const QString &questionId);
    
private slots:
    void onLoadClicked();
    void onDeleteClicked();
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem *item);
    
private:
    void loadConversationList();
    void updateButtonStates();
    QString formatDateTime(const QDateTime &dt) const;
    
    QListWidget *m_listWidget;
    QPushButton *m_loadButton;
    QPushButton *m_deleteButton;
    QPushButton *m_closeButton;
    QLabel *m_infoLabel;
    
    QString m_selectedId;
    QString m_currentQuestionId;  // 当前题目ID，用于过滤
    QVector<ConversationInfo> m_conversations;
};

#endif // CHATHISTORYDIALOG_H
