#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QWidget>
#include <QTableWidget>

class HistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryWidget(QWidget *parent = nullptr);
    
    void loadHistory();
    
private:
    void setupUI();
    
    QTableWidget *m_historyTable;
};

#endif // HISTORYWIDGET_H
