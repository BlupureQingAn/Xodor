#ifndef PRACTICEWIDGET_H
#define PRACTICEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "../core/Question.h"
#include "../core/QuestionBank.h"

class PracticeStatsPanel;  // 前向声明

// 刷题系统主界面
class PracticeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PracticeWidget(QuestionBank *questionBank, QWidget *parent = nullptr);
    
    void refreshQuestionList();
    
signals:
    void questionSelected(const Question &question);
    void reloadQuestionBankRequested();  // 请求重新加载题库
    void switchBankRequested();  // 请求切换题库
    
private slots:
    void onFilterChanged();
    void onSearchTextChanged(const QString &text);
    void onQuestionDoubleClicked(int row, int column);
    void onRefreshClicked();
    void onResetProgressClicked();
    void onRandomQuestionClicked();
    void onRecommendQuestionClicked();
    void onSwitchBankClicked();
    void onExportProgressClicked();
    void onBatchMarkClicked();
    void onHeaderClicked(int logicalIndex);
    void onQuestionStatusUpdated(const QString &questionId);
    
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void updateStatistics();
    void loadQuestions();
    void updateBankSelector();
    QString getStatusText(const QString &questionId) const;
    QString getStatusIcon(const QString &questionId) const;
    Question getRandomQuestion() const;
    Question getRecommendedQuestion() const;
    void exportProgressReport();
    void batchMarkStatus();
    
    // 题库加载辅助方法
    QVector<Question> loadQuestionsFromBank(const QString &bankPath) const;
    void loadQuestionsRecursive(const QString &dirPath, QVector<Question> &questions) const;
    
    QuestionBank *m_questionBank;
    
    // UI组件
    QComboBox *m_bankSelector;
    QLabel *m_statsLabel;
    QProgressBar *m_progressBar;
    QLineEdit *m_searchEdit;
    QComboBox *m_difficultyFilter;
    QComboBox *m_tagFilter;
    QComboBox *m_statusFilter;
    QComboBox *m_sortCombo;
    QTableWidget *m_questionTable;
    QPushButton *m_refreshBtn;
    QPushButton *m_resetProgressBtn;
    QPushButton *m_randomBtn;
    QPushButton *m_recommendBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_batchMarkBtn;
    PracticeStatsPanel *m_statsPanel;  // 统计面板
    
    // 筛选条件
    QString m_currentSearchText;
    Difficulty m_currentDifficulty;
    QString m_currentTag;
    int m_currentStatus; // -1=全部, 0=未开始, 1=进行中, 2=已完成, 3=已掌握
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif // PRACTICEWIDGET_H
