#ifndef AIANALYSISPANEL_H
#define AIANALYSISPANEL_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>

class AIAnalysisPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AIAnalysisPanel(QWidget *parent = nullptr);
    
    void setAnalysis(const QString &analysis);
    void setVisible(bool visible);
    
signals:
    void requestAnalysis();
    
private:
    void setupUI();
    
    QTextBrowser *m_analysisBrowser;
    QPushButton *m_analyzeBtn;
    QPushButton *m_toggleBtn;
};

#endif // AIANALYSISPANEL_H
