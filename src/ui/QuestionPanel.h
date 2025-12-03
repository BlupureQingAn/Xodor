#ifndef QUESTIONPANEL_H
#define QUESTIONPANEL_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>
#include "../core/Question.h"

class QuestionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionPanel(QWidget *parent = nullptr);
    
    void setQuestion(const Question &question);
    
signals:
    void runTests();
    void nextQuestion();
    void previousQuestion();
    
private:
    void setupUI();
    QString convertMarkdownToHtml(const QString &markdown);
    
    QTextBrowser *m_questionBrowser;
    QPushButton *m_runTestsBtn;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
};

#endif // QUESTIONPANEL_H
