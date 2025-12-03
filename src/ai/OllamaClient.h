#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include "AIService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class OllamaClient : public AIService
{
    Q_OBJECT
public:
    explicit OllamaClient(QObject *parent = nullptr);
    
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);
    QString currentModel() const { return m_model; }
    
    // 云端API支持
    void setApiKey(const QString &apiKey);
    void setCloudMode(bool enabled);
    bool isCloudMode() const { return m_cloudMode; }
    
    void analyzeCode(const QString &questionDesc, const QString &code) override;
    void generateQuestions(const QJsonObject &params) override;
    void parseQuestionBank(const QStringList &mdFiles) override;
    
    // 通用方法：发送自定义prompt
    void sendCustomPrompt(const QString &prompt, const QString &context = "custom");
    
    // 流式对话方法
    void sendChatMessage(const QString &message, const QString &systemPrompt = "");
    
    // 获取可用模型列表
    QStringList getAvailableModels();

signals:
    // 流式输出信号
    void streamingChunk(const QString &chunk);
    void streamingFinished();
    
private slots:
    void handleNetworkReply(QNetworkReply *reply);
    
private:
    void sendRequest(const QString &prompt, const QString &context);
    
    QNetworkAccessManager *m_networkManager;
    QString m_baseUrl;
    QString m_model;
    QString m_apiKey;
    bool m_cloudMode;
};

#endif // OLLAMACLIENT_H
