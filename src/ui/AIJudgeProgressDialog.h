#ifndef AIJUDGEPROGRESSDIALOG_H
#define AIJUDGEPROGRESSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QWidget>
#include <QPainter>
#include <QTimer>

// 自定义红色进度条
class RedProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit RedProgressBar(QWidget *parent = nullptr)
        : QWidget(parent), m_value(0)
    {
        setFixedHeight(8);
        setMinimumWidth(250);
        
        // 启动动画定时器
        m_animationTimer = new QTimer(this);
        connect(m_animationTimer, &QTimer::timeout, this, [this]() {
            m_value = (m_value + 2) % 100;
            update();
        });
        m_animationTimer->start(30);  // 30ms更新一次
    }
    
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 绘制背景
        painter.setBrush(QColor("#2d2d2d"));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 4, 4);
        
        // 绘制进度条（红色渐变）
        if (m_value > 0) {
            int progressWidth = width() * m_value / 100;
            
            QLinearGradient gradient(0, 0, progressWidth, 0);
            gradient.setColorAt(0, QColor("#660000"));
            gradient.setColorAt(1, QColor("#aa0000"));
            
            painter.setBrush(gradient);
            painter.drawRoundedRect(0, 0, progressWidth, height(), 4, 4);
        }
    }
    
private:
    int m_value;
    QTimer *m_animationTimer;
};

// AI判题进度对话框
class AIJudgeProgressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIJudgeProgressDialog(QWidget *parent = nullptr);
    
    void setMessage(const QString &message);
    
private:
    QLabel *m_iconLabel;
    QLabel *m_messageLabel;
    RedProgressBar *m_progressBar;
};

#endif // AIJUDGEPROGRESSDIALOG_H
