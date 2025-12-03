#ifndef EXAMREPORTDIALOG_H
#define EXAMREPORTDIALOG_H

#include <QDialog>
#include <QTextBrowser>
#include <QPushButton>
#include <QTabWidget>
#include "../core/ExamReportGenerator.h"

// 答题报告查看对话框
class ExamReportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExamReportDialog(const ExamReport &report, QWidget *parent = nullptr);
    
private slots:
    void onExportMarkdown();
    void onExportHtml();
    void onExportJson();
    void onPrint();
    
private:
    void setupUI();
    void displayReport();
    void displaySummary();
    void displayDetails();
    void displayAnalysis();
    
    ExamReport m_report;
    
    // UI组件
    QTabWidget *m_tabWidget;
    QTextBrowser *m_summaryBrowser;
    QTextBrowser *m_detailsBrowser;
    QTextBrowser *m_analysisBrowser;
    
    QPushButton *m_exportMdBtn;
    QPushButton *m_exportHtmlBtn;
    QPushButton *m_exportJsonBtn;
    QPushButton *m_printBtn;
    QPushButton *m_closeBtn;
};

#endif // EXAMREPORTDIALOG_H
