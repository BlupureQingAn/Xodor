#ifndef CHATBUBBLEWIDGET_H
#define CHATBUBBLEWIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

class ChatBubbleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatBubbleWidget(const QString &content, bool isUser, QWidget *parent = nullptr);
    
    void setContent(const QString &content);
    QString content() const;
    bool isUser() const { return m_isUser; }
    
    void setFontScale(qreal scale);
    qreal fontScale() const { return m_fontScale; }
    
    void forceUpdate();  // 强制更新布局和内容
    
protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private:
    QString formatMarkdown(const QString &content);
    QString formatUserMessage(const QString &content);
    void adjustHeight();
    
    QTextBrowser *m_textBrowser;
    QString m_content;
    bool m_isUser;
    qreal m_fontScale;
};

#endif // CHATBUBBLEWIDGET_H
