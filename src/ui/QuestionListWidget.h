#ifndef QUESTIONLISTWIDGET_H
#define QUESTIONLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include "../core/Question.h"

class QuestionListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionListWidget(QWidget *parent = nullptr);
    
    void setQuestions(const QVector<Question> &questions);
    void setCurrentQuestion(int index);
    QVector<int> getSelectedIndices() const;
    
signals:
    void questionSelected(int index);
    void questionsDeleteRequested(const QVector<int> &indices);
    
private slots:
    void onSearchTextChanged(const QString &text);
    void onDifficultyFilterChanged(int index);
    void onQuestionItemClicked(QListWidgetItem *item);
    void onDeleteSelected();
    void onSelectAll();
    
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    
private:
    void setupUI();
    void updateQuestionList();
    void filterQuestions();
    
    QLineEdit *m_searchEdit;
    QComboBox *m_difficultyFilter;
    QListWidget *m_questionList;
    
    QVector<Question> m_allQuestions;
    QVector<int> m_filteredIndices;
};

#endif // QUESTIONLISTWIDGET_H
