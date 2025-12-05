#ifndef CHATBUBBLEDELEGATE_H
#define CHATBUBBLEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>
#include <QStyleOptionViewItem>
#include <QModelIndex>

class ChatBubbleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatBubbleDelegate(QObject *parent = nullptr);
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    
    // 字体缩放
    void setFontScale(qreal scale);
    qreal fontScale() const { return m_fontScale; }
    
signals:
    void sizeChanged();  // 当缩放改变时发出信号
    
private:
    QSize calculateSize(const QString &text, const QString &role, int maxWidth) const;
    void drawBubble(QPainter *painter, const QRectF &rect, const QColor &color, bool isUser) const;
    QString formatMarkdown(const QString &content) const;
    
    qreal m_fontScale = 1.0;  // 字体缩放比例
};

#endif // CHATBUBBLEDELEGATE_H
