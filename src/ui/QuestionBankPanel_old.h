#ifndef QUESTIONBANKPANEL_H
#define QUESTIONBANKPANEL_H

#include <QWidget>
#include <QLineEdit>
#include "../core/Question.h"

class QuestionBankTreeWidget;

// 题库面板 - 使用树形结构展示题库
class QuestionBankPanel : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionBankPanel(QWidget *parent = nullptr);
    
    // 刷新题库树
    void refreshBankTree();
    
    // 展开指定题库
    void expandBank(const QString &bankPath);
    
    // 选中指定题目
    void selectQuestion(const QString &questionPath);
    
public slots:
    void updateQuestionStatus(const QString &questionId);

signals:
    // 题目被选中（通过文件路径）
    void questionFileSelected(const QString &filePath, const Question &question);
    
    // 题库被选中
    void bankSelected(const QString &bankPath);
    
private slots:
    void onSearchTextChanged(const QString &text);
    void onQuestionSelected(const QString &filePath, const Question &question);
    void onBankSelected(const QString &bankPath);
    
private:
    void setupUI();
    
    // UI组件
    QLineEdit *m_searchEdit;
    QuestionBankTreeWidget *m_treeWidget;
};

#endif // QUESTIONBANKPANEL_H
