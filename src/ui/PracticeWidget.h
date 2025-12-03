#ifndef PRACTICEWIDGET_H
#define PRACTICEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../core/Question.h"
#include "../core/QuestionBank.h"

// 刷题系统主界面
class PracticeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PracticeWidget(QuestionBank *questionBank, QWidget *parent = nullptr);
    
    void refreshQuestionList();
    
signals:
    void questionSelected(const Question &question);
    
private slots:
    void onFilterChanged();
    void onSearchTextChanged(const QString &text);
    void onQuestionDoubleClicked(int row, int column);
    void onRefreshClicked();
    void onResetProgressClicked();
    
private:
    void setupUI();
    void updateStatistics();
    void loadQuestions();
    QString getStatusText(const QString &questionId) const;
    QString getStatusIcon(const QString &questionId) const;
    
    QuestionBank *m_questionBank;
    
    // UI组件
    QLabel *m_statsLabel;
    QLabel *m_progressLabel;
    QLineEdit *m_searchEdit;
    QComboBox *m_difficultyFilter;
    QComboBox *m_tagFilter;
    QComboBox *m_statusFilter;
    QTableWidget *m_questionTable;
    QPushButton *m_refreshBtn;
    QPushButton *m_resetProgressBtn;
    
    // 筛选条件
    QString m_currentSearchText;
    Difficulty m_currentDifficulty;
    QString m_currentTag;
    int m_currentStatus; // -1=全部, 0=未开始, 1=进行中, 2=已完成, 3=已掌握
};

#endif // PRACTICEWIDGET_H
