#ifndef TESTCASEFIXERDIALOG_H
#define TESTCASEFIXERDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QCheckBox>
#include "../core/Question.h"

class OllamaClient;
class QuestionBank;

class TestCaseFixerDialog : public QDialog
{
    Q_OBJECT
public:
    // 统一的构造函数
    explicit TestCaseFixerDialog(QuestionBank *questionBank, OllamaClient *aiClient, 
                                QWidget *parent = nullptr);
    
signals:
    void questionsFixed();  // 题目修复完成信号（自动刷新）
    
private slots:
    void onScanSelected();  // 扫描选中的题目（AI分析）
    void onSelectAll();     // 全选
    void onSelectNone();    // 取消全选
    void onFixSelected();   // 修复选中的有问题的题目
    void onStopFix();       // 停止操作
    void onAIChunk(const QString &chunk);
    void onAIFinished();
    void onAIError(const QString &error);
    void onQuestionItemChanged(QListWidgetItem *item);  // 题目选择变化
    
private:
    void setupUI();
    void loadAllQuestions();  // 加载所有题目到列表
    QString findValidBankPath();  // 智能查找有效的题库路径
    void scanNextQuestion();  // 扫描下一个题目（AI分析）
    void fixNextQuestion();   // 修复下一个题目
    QString generateScanPrompt(const Question &question);  // 生成扫描提示词
    QString generateFixPrompt(const Question &question, const QVector<int> &indices);
    bool saveFixedQuestion(const Question &question, const QString &filePath);
    void applyAIScanResult();  // 应用AI扫描结果
    void applyAIFix();  // 应用AI修复结果
    void updateStatusLabel();  // 更新状态标签
    
    QuestionBank *m_questionBank;
    OllamaClient *m_aiClient;        // 传入的客户端（用于获取配置）
    OllamaClient *m_privateClient;   // 私有客户端（独立使用，不影响主界面）
    
    struct QuestionItem {
        QString id;
        QString title;
        QString filePath;
        bool hasIssues;
        QVector<int> problematicIndices;
    };
    
    QVector<QuestionItem> m_allQuestions;      // 所有题目
    QVector<int> m_selectedIndices;            // 选中的题目索引
    QVector<QuestionItem> m_questionsToFix;    // 需要修复的题目
    int m_currentScanIndex;                    // 当前扫描索引
    int m_currentFixIndex;                     // 当前修复索引
    bool m_isScanning;                         // 是否正在扫描
    bool m_isFixing;                           // 是否正在修复
    Question m_currentQuestion;
    QString m_currentAIResponse;
    
    enum OperationMode {
        Idle,
        Scanning,
        Fixing
    };
    OperationMode m_currentMode;
    
    // UI组件
    QListWidget *m_questionList;
    QTextEdit *m_detailView;
    QTextEdit *m_logView;
    QPushButton *m_scanButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_fixButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_selectionLabel;
};

#endif // TESTCASEFIXERDIALOG_H
