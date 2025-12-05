#ifndef QUESTIONBANKPANEL_H
#define QUESTIONBANKPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSet>
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
    
    // 获取和恢复展开状态
    QStringList getExpandedPaths() const;
    void restoreExpandedPaths(const QStringList &paths);
    QString getSelectedQuestionPath() const;
    
    // 获取和恢复难度筛选状态
    QSet<Difficulty> getActiveDifficultyFilters() const;
    void restoreDifficultyFilters(const QSet<Difficulty> &filters);
    
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
    void onDifficultyFilterChanged();
    
private:
    void setupUI();
    void applyFilters();
    
    // UI组件
    QLineEdit *m_searchEdit;
    QuestionBankTreeWidget *m_treeWidget;
    QCheckBox *m_easyCheckBox;
    QCheckBox *m_mediumCheckBox;
    QCheckBox *m_hardCheckBox;
    
    // 筛选状态
    QSet<Difficulty> m_activeDifficultyFilters;
};

#endif // QUESTIONBANKPANEL_H
