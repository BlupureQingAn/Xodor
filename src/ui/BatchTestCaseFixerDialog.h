#ifndef BATCHTESTCASEFIXERDIALOG_H
#define BATCHTESTCASEFIXERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "../core/Question.h"

class OllamaClient;
class QuestionBank;

class BatchTestCaseFixerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BatchTestCaseFixerDialog(QuestionBank *questionBank, OllamaClient *aiClient, 
                                     QWidget *parent = nullptr);
    
signals:
    void batchFixCompleted();  // 批量修复完成信号
    
private slots:
    void onScanQuestions();
    void onStartBatchFix();
    void onStopBatchFix();
    void onAIChunk(const QString &chunk);
    void onAIFinished();
    void onAIError(const QString &error);
    
private:
    void setupUI();
    void scanAllQuestions();
    void fixNextQuestion();
    QString generateFixPrompt(const Question &question, const QVector<int> &problematicIndices);
    bool saveFixedQuestion(const Question &question, const QString &filePath);
    QVector<int> detectProblematicTestCases(const Question &question);
    
    QuestionBank *m_questionBank;
    OllamaClient *m_aiClient;
    
    QListWidget *m_questionList;
    QTextEdit *m_logView;
    QPushButton *m_scanButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    
    struct QuestionToFix {
        QString id;
        QString title;
        QString filePath;
        QVector<int> problematicIndices;
    };
    
    QVector<QuestionToFix> m_questionsToFix;
    int m_currentIndex;
    bool m_isFixing;
    QString m_currentAIResponse;
    Question m_currentQuestion;
};

#endif // BATCHTESTCASEFIXERDIALOG_H
