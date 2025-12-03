#ifndef QUESTIONLISTWIDGET_H
#define QUESTIONLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include "../core/Question.h"

// 显示模式
enum class ListMode {
    Questions,      // 题目列表模式
    QuestionBanks   // 题库列表模式
};

class QuestionListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionListWidget(QWidget *parent = nullptr);
    
    // 题目列表模式
    void setQuestions(const QVector<Question> &questions);
    void setCurrentQuestion(int index);
    QVector<int> getSelectedIndices() const;
    
    // 题库列表模式
    void setMode(ListMode mode);
    void refreshBankList();
    QString getSelectedBankPath() const { return m_selectedBankPath; }
    
signals:
    void questionSelected(int index);
    void questionsDeleteRequested(const QVector<int> &indices);
    void bankSelected(const QString &bankPath);
    void bankLoadRequested(const QString &bankPath);
    
private slots:
    void onSearchTextChanged(const QString &text);
    void onDifficultyFilterChanged(int index);
    void onQuestionItemClicked(QListWidgetItem *item);
    void onDeleteSelected();
    void onSelectAll();
    void onLoadBank();
    void onDeleteBank();
    void onRenameBank();
    
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    
private:
    void setupUI();
    void updateQuestionList();
    void filterQuestions();
    void updateBankList();
    QString getBankInfo(const QString &bankPath) const;
    
    ListMode m_mode;
    QLineEdit *m_searchEdit;
    QComboBox *m_difficultyFilter;
    QListWidget *m_questionList;
    QPushButton *m_loadBankBtn;
    QPushButton *m_deleteBankBtn;
    QPushButton *m_renameBankBtn;
    
    QVector<Question> m_allQuestions;
    QVector<int> m_filteredIndices;
    QString m_selectedBankPath;
};

#endif // QUESTIONLISTWIDGET_H
