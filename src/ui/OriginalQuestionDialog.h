#ifndef ORIGINALQUESTIONDIALOG_H
#define ORIGINALQUESTIONDIALOG_H

#include <QDialog>
#include <QTextBrowser>
#include <QPushButton>
#include <QTabWidget>
#include "../core/Question.h"

class OriginalQuestionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OriginalQuestionDialog(const Question &question, QWidget *parent = nullptr);
    
signals:
    void practiceRequested();
    
private:
    void setupUI();
    void displayQuestion();
    
    Question m_question;
    QTextBrowser *m_descriptionBrowser;
    QTextBrowser *m_answerBrowser;
    QTextBrowser *m_testCasesBrowser;
    QPushButton *m_practiceBtn;
    QPushButton *m_closeBtn;
};

#endif // ORIGINALQUESTIONDIALOG_H
