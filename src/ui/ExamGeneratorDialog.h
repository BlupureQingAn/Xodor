#ifndef EXAMGENERATORDIALOG_H
#define EXAMGENERATORDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include "../core/Question.h"

class OllamaClient;

class ExamGeneratorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExamGeneratorDialog(const QVector<Question> &existingQuestions,
                                OllamaClient *aiClient, QWidget *parent = nullptr);
    
    QVector<Question> getGeneratedQuestions() const { return m_generatedQuestions; }
    bool isSuccess() const { return m_success; }
    
private slots:
    void onGenerateClicked();
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    void setupUI();
    QString buildPrompt();
    void parseAIResponse(const QString &response);
    
    OllamaClient *m_aiClient;
    QVector<Question> m_existingQuestions;
    QVector<Question> m_generatedQuestions;
    bool m_success;
    
    // UI组件
    QSpinBox *m_countSpinBox;
    QComboBox *m_difficultyCombo;
    QCheckBox *m_includeTestsCheckBox;
    QTextEdit *m_logText;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_generateBtn;
    QPushButton *m_closeBtn;
};

#endif // EXAMGENERATORDIALOG_H
