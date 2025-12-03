#ifndef ERRORLISTWIDGET_H
#define ERRORLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../utils/SyntaxChecker.h"

class OllamaClient;

class ErrorListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ErrorListWidget(QWidget *parent = nullptr);
    
    void setErrors(const QVector<SyntaxError> &errors);
    void setAIClient(OllamaClient *client) { m_aiClient = client; }
    
signals:
    void errorClicked(int line, int column);
    void fixRequested(const QString &code, const SyntaxError &error);
    void fixAllRequested(const QString &code, const QVector<SyntaxError> &errors);
    
private slots:
    void onItemClicked(QListWidgetItem *item);
    void onFixAllClicked();
    void onFixSelectedClicked();
    
private:
    void updateErrorCount();
    QString formatErrorMessage(const SyntaxError &error) const;
    
    QListWidget *m_listWidget;
    QLabel *m_countLabel;
    QPushButton *m_fixAllButton;
    QPushButton *m_fixSelectedButton;
    
    QVector<SyntaxError> m_errors;
    OllamaClient *m_aiClient;
};

#endif // ERRORLISTWIDGET_H
