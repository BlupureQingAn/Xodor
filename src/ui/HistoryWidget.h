#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>

class HistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryWidget(QWidget *parent = nullptr);
    
private slots:
    void loadHistory();
    
private:
    void setupUI();
    
    QTableWidget *m_historyTable;
    QLabel *m_totalLabel;
    QLabel *m_completedLabel;
    QLabel *m_accuracyLabel;
};

#endif // HISTORYWIDGET_H
