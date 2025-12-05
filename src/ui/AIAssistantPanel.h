#ifndef AIASSISTANTPANEL_H
#define AIASSISTANTPANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialog>
#include <QMessageBox>
#include "../ai/AIAssistant.h"
#include "../core/Question.h"

class OllamaClient;
class ChatBubbleWidget;

// 使用AIAssistant.h中定义的ChatMessage结构体

class AIAssistantPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AIAssistantPanel(OllamaClient *aiClient, QWidget *parent = nullptr);
    
    // 设置当前题目上下文
    void setQuestionContext(const Question &question);
    
    // 清空对话历史
    void clearHistory();
    
    // 刷新对话（开始新对话）
    void refreshChat();
    
    // 查看历史记录
    void viewHistory();
    
    // 设置当前代码（用于错误诊断）
    void setCurrentCode(const QString &code);
    
    // 主动提供帮助（代码错误时调用）
    void offerHelp(const QString &message);
    
signals:
    void assistantReady();
    void assistantError(const QString &error);
    void requestCurrentCode();  // 请求获取当前代码
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    void onSendMessage();
    void onStopGeneration();  // 终止输出
    void onAnalyzeCode();  // 分析代码快捷按钮
    void onGetHint();      // 思路快捷按钮
    void onExplainConcept(); // 知识点快捷按钮
    void onStreamingChunk(const QString &chunk);  // 流式数据块
    void onStreamingFinished();  // 流式完成
    void onErrorOccurred(const QString &error);
    void onNewChat();  // 新对话
    void onViewHistory();  // 查看历史
    
private:
    void setupUI();
    void appendUserMessage(const QString &message);
    void startAssistantMessage();  // 开始AI消息（准备接收流式数据）
    void appendToAssistantMessage(const QString &chunk);  // 追加流式数据
    void finishAssistantMessage();  // 完成AI消息
    void sendChatMessage(const QString &message);
    void loadConversationHistory();
    void loadConversationById(const QString &questionId);  // 根据ID加载对话
    void saveConversationHistory();
    QString buildSystemPrompt();  // 构建费曼学习法系统提示词
    QString formatMessageContent(const QString &content);  // 格式化消息内容（支持代码块）
    void scrollToBottom();  // 滚动到底部
    void updateAllBubbleScales();  // 更新所有气泡的缩放
    
    // UI组件
    QScrollArea *m_scrollArea;     // 滚动区域
    QWidget *m_chatContainer;      // 消息容器
    QVBoxLayout *m_chatLayout;     // 消息布局
    QTextEdit *m_inputField;       // 改为 QTextEdit 支持多行
    QPushButton *m_sendButton;
    QPushButton *m_stopButton;     // 终止输出
    QPushButton *m_analyzeButton;  // 分析代码
    QPushButton *m_hintButton;     // 思路
    QPushButton *m_conceptButton;  // 知识点
    QPushButton *m_newChatButton;  // 新对话
    QPushButton *m_historyButton;  // 历史记录
    qreal m_fontScale;             // 字体缩放比例
    
    // 核心组件
    OllamaClient *m_aiClient;
    
    // 当前题目信息
    Question m_currentQuestion;
    bool m_hasQuestion;
    QString m_currentCode;
    
    // 对话历史
    QVector<ChatMessage> m_messages;
    QString m_currentAssistantMessage;  // 当前正在接收的AI消息
    bool m_isReceivingMessage;  // 是否正在接收流式消息
    ChatBubbleWidget *m_currentAssistantBubble;  // 当前AI消息的气泡
    
    // 费曼学习法相关
    int m_questionCount;  // AI提问次数
    QString m_userLevel;  // 用户水平评估
};

#endif // AIASSISTANTPANEL_H
