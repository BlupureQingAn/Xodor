#ifndef MOCKEXAMMANAGERDIALOG_H
#define MOCKEXAMMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include "../ai/MockExamGenerator.h"

class OllamaClient;
class QuestionBankManager;

// 模拟题管理对话框
class MockExamManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MockExamManagerDialog(const QVector<Question> &questions,
                                  OllamaClient *aiClient, 
                                  QWidget *parent = nullptr);
    
private slots:
    void onAnalyzeBank();
    void onGenerateExams();
    void onViewExam();
    void onDeleteExam();
    void onExportExam();
    
    void onProgressUpdated(int percentage, const QString &message);
    void onExamGenerated(const QVector<Question> &questions, int examIndex);
    void onGenerationComplete(int totalExams);
    void onGenerationError(const QString &error);
    
private:
    void setupUI();
    void loadExistingExams();
    void saveExam(const QVector<Question> &questions, int examIndex);
    QString getExamPath(const QString &category, int examIndex);
    
    QVector<Question> m_questions;
    OllamaClient *m_aiClient;
    MockExamGenerator *m_generator;
    
    // UI 组件
    QComboBox *m_categoryCombo;
    QLabel *m_patternLabel;
    QSpinBox *m_examCountSpinBox;
    QPushButton *m_analyzeBtn;
    QPushButton *m_generateBtn;
    
    QListWidget *m_examList;
    QPushButton *m_viewBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_exportBtn;
    
    QProgressBar *m_progressBar;
    QTextEdit *m_logText;
    
    ExamPattern m_currentPattern;
    QString m_currentCategory;
};

#endif // MOCKEXAMMANAGERDIALOG_H
