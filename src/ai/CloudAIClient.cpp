#include "CloudAIClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>

CloudAIClient::CloudAIClient(QObject *parent)
    : AIService(parent)
    , m_apiUrl("https://api.openai.com/v1/chat/completions")
{
    m_networkManager = new QNetworkAccessManager(this);
}

void CloudAIClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void CloudAIClient::setApiUrl(const QString &url)
{
    m_apiUrl = url;
}

void CloudAIClient::analyzeCode(const QString &questionDesc, const QString &code)
{
    if (m_apiKey.isEmpty()) {
        emit error("API Key未设置");
        return;
    }
    
    QString prompt = QString("分析以下C++代码的思路、优化建议和涉及的知识点：\n\n题目：%1\n\n代码：\n%2")
                        .arg(questionDesc, code);
    
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    
    QJsonArray messages;
    messages.append(message);
    
    QJsonObject json;
    json["model"] = "gpt-3.5-turbo";
    json["messages"] = messages;
    
    QNetworkRequest request{QUrl(m_apiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QString content = choices[0].toObject()["message"].toObject()["content"].toString();
                emit codeAnalysisReady(content);
            }
        }
        reply->deleteLater();
    });
}

void CloudAIClient::generateQuestions(const QJsonObject &params)
{
    QString prompt = "根据学习的题库，生成一套模拟题，包含题目描述、难度、测试用例和参考答案。以JSON格式返回。";
    
    // TODO: 实现完整的题目生成逻辑
    emit questionsGenerated(QJsonArray());
}

void CloudAIClient::parseQuestionBank(const QStringList &mdFiles)
{
    // TODO: 实现题库解析
    emit questionBankParsed(QJsonArray());
}
