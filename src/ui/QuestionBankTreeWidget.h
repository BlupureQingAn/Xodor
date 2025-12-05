#ifndef QUESTIONBANKTREEWIDGET_H
#define QUESTIONBANKTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <QVector>
#include "../core/Question.h"

// 节点类型
enum class TreeNodeType {
    Root,           // 根节点（基础题库）
    Bank,           // 题库文件夹
    QuestionFile    // 题目文件
};

// 题库树形控件
class QuestionBankTreeWidget : public QTreeWidget
{
    Q_OBJECT
    
public:
    explicit QuestionBankTreeWidget(QWidget *parent = nullptr);
    
    // 加载题库树
    void loadBankTree();
    
    // 展开指定题库
    void expandBank(const QString &bankPath);
    
    // 选中指定题目
    void selectQuestion(const QString &questionPath);
    
    // 刷新树
    void refreshTree();
    
    // 更新题目状态
    void updateQuestionStatus(const QString &questionId);
    
    // 获取和恢复展开状态
    QStringList getExpandedPaths() const;
    void restoreExpandedPaths(const QStringList &paths);
    QString getSelectedQuestionPath() const;
    
signals:
    // 题目被选中
    void questionSelected(const QString &filePath, const Question &question);
    
    // 题库被选中
    void bankSelected(const QString &bankPath);
    
private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onCustomContextMenu(const QPoint &pos);
    
    // 右键菜单操作
    void onAddQuestion();
    void onEditQuestion();
    void onDeleteQuestion();
    void onDeleteBank();
    
private:
    void setupUI();
    void loadRootNode();
    void loadBankNode(QTreeWidgetItem *parentItem, const QString &bankPath);
    void loadQuestionFiles(QTreeWidgetItem *bankItem, const QString &bankPath);
    
    // 辅助函数
    TreeNodeType getNodeType(QTreeWidgetItem *item) const;
    QString getNodePath(QTreeWidgetItem *item) const;
    int countQuestionsInBank(const QString &bankPath) const;
    Question loadQuestionFromFile(const QString &filePath) const;
    QString getQuestionStatusIcon(const QString &questionId) const;
    
    // 根节点
    QTreeWidgetItem *m_rootItem;
};

#endif // QUESTIONBANKTREEWIDGET_H
