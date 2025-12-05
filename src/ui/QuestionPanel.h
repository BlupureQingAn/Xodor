#ifndef QUESTIONPANEL_H
#define QUESTIONPANEL_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QWheelEvent>
#include "../core/Question.h"

// 支持Ctrl+滚轮缩放的QTextBrowser
class ZoomableTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit ZoomableTextBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {}
    
protected:
    void wheelEvent(QWheelEvent *event) override
    {
        // Ctrl+滚轮缩放
        if (event->modifiers() & Qt::ControlModifier) {
            int delta = event->angleDelta().y();
            if (delta > 0) {
                zoomIn(1);  // 放大
            } else if (delta < 0) {
                zoomOut(1);  // 缩小
            }
            event->accept();
        } else {
            // 正常滚动
            QTextBrowser::wheelEvent(event);
        }
    }
};

class QuestionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionPanel(QWidget *parent = nullptr);
    
    void setQuestion(const Question &question);
    
signals:
    void runTests();
    void aiJudgeRequested();  // AI判题请求信号
    void nextQuestion();
    void previousQuestion();
    
private:
    void setupUI();
    QString convertMarkdownToHtml(const QString &markdown);
    
    ZoomableTextBrowser *m_questionBrowser;
    QPushButton *m_runTestsBtn;
    QPushButton *m_aiJudgeBtn;  // AI判题按钮
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
};

#endif // QUESTIONPANEL_H
