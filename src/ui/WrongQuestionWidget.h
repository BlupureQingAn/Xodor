#ifndef WRONGQUESTIONWIDGET_H
#define WRONGQUESTIONWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include "../core/WrongQuestionBook.h"

class WrongQuestionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WrongQuestionWidget(QWidget *parent = nullptr);
    
    void loadWrongQuestions();
    
signals:
    void questionSelected(const QString &questionTitle);
    
private slots:
    void showAllQuestions();
    void showUnresolvedQuestions();
    void clearWrongQuestions();
    void onMarkButtonClicked();
    void onQuestionDoubleClicked(int row, int column);
    
private:
    void setupUI();
    void displayQuestions(const QVector<WrongQuestionRecord> &questions);
    
    QTableWidget *m_wrongQuestionsTable;
    QPushButton *m_showAllBtn;
    QPushButton *m_showUnresolvedBtn;
    QPushButton *m_clearBtn;
    QLabel *m_statsLabel;
};

#endif // WRONGQUESTIONWIDGET_H
