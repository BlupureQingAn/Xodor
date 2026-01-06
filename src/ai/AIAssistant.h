#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include "../core/Question.h"

class OllamaClient;

// 聊天消息
struct ChatMessage {
    QString role;          // "user" or "assistant"
    QString content;       // 消息内容
    QDateTime timestamp;   // 时间戳
    
    QString toJson() const;
    static ChatMessage fromJson(const QString &json);
};

// AI 助手
class AIAssistant : public QObject
{
    Q_OBJECT
public:
    explicit AIAssistant(OllamaClient *aiClient, QObject *parent = nullptr);
    
    // 提问
    void askQuestion(const QString &question, const Question &currentQuestion);
    
    // 获取思路提示（不含具体代码）
    void getHint(const Question &currentQuestion);
    
    // 知识点讲解
    void explainConcept(const QString &conceptName, const Question &currentQuestion);
    
    // 错误诊断
    void diagnoseError(const QString &code, const QString &errorMessage, 
                      const Question &currentQuestion);
    
    // 获取历史对话
    QVector<ChatMessage> getChatHistory() const { return m_chatHistory; }
    
    // 清空历史
    void clearHistory();
    
    // 保存/加载历史
    void saveHistory(const QString &questionId);
    void loadHistory(const QString &questionId);
    
signals:
    void responseReady(const QString &response);
    void error(const QString &errorMessage);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    QString buildQuestionPrompt(const QString &userInput, const Question &question);
    QString buildHintPrompt(const Question &question);
    QString buildConceptPrompt(const QString &conceptName, const Question &question);
    QString buildDiagnosePrompt(const QString &code, const QString &errorMessage, 
                               const Question &question);
    
    void addMessage(const QString &role, const QString &content);
    QString getHistoryFilePath(const QString &questionId) const;
    
    OllamaClient *m_aiClient;
    QVector<ChatMessage> m_chatHistory;
    QString m_currentQuestionId;
};

#endif // AIASSISTANT_H
