#include "AIConnectionChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>

AIConnectionChecker::AIConnectionChecker(QObject *parent)
    : QObject(parent)
    , m_pendingChecks(0)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void AIConnectionChecker::checkOllamaConnection(const QString &baseUrl, const QString &model)
{
    m_pendingChecks++;
    m_checkingModel = model;
    
    // 首先检查服务是否在线（检查 /api/tags 端点）
    QUrl url(baseUrl + "/api/tags");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);  // 5秒超时
    
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, baseUrl, model]() {
        handleOllamaReply(reply);
    });
}

void AIConnectionChecker::checkCloudApiConnection(const QString &apiKey, const QString &apiUrl)
{
    if (apiKey.isEmpty()) {
        m_status.cloudApiAvailable = false;
        m_status.cloudApiError = "未配置API Key";
        emit cloudApiCheckCompleted(false, m_status.cloudApiError);
        checkIfAllCompleted();
        return;
    }
    
    m_pendingChecks++;
    
    // 发送一个简单的测试请求
    QJsonObject message;
    message["role"] = "user";
    message["content"] = "test";
    
    QJsonArray messages;
    messages.append(message);
    
    QJsonObject json;
    json["model"] = "gpt-3.5-turbo";
    json["messages"] = messages;
    json["max_tokens"] = 5;
    
    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    request.setTransferTimeout(10000);  // 10秒超时
    
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleCloudApiReply(reply);
    });
}

void AIConnectionChecker::handleOllamaReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        m_status.ollamaAvailable = false;
        
        // 根据错误类型提供详细信息
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                m_status.ollamaError = "连接被拒绝\n\n"
                                      "Ollama服务未运行\n"
                                      "请在终端执行：ollama serve";
                break;
                
            case QNetworkReply::HostNotFoundError:
                m_status.ollamaError = "找不到服务器\n\n"
                                      "请检查服务地址配置";
                break;
                
            case QNetworkReply::TimeoutError:
                m_status.ollamaError = "连接超时\n\n"
                                      "服务响应缓慢或网络不稳定";
                break;
                
            case QNetworkReply::ContentNotFoundError:
                m_status.ollamaError = "API端点不存在\n\n"
                                      "Ollama版本可能过旧\n"
                                      "请更新到最新版本";
                break;
                
            default:
                m_status.ollamaError = QString("连接失败：%1").arg(reply->errorString());
                break;
        }
        
        qWarning() << "Ollama connection failed:" << m_status.ollamaError;
        emit ollamaCheckCompleted(false, m_status.ollamaError);
    } else {
        // 解析响应，检查模型是否存在
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonArray models = obj["models"].toArray();
        
        bool modelFound = false;
        for (const QJsonValue &value : models) {
            QJsonObject modelObj = value.toObject();
            QString modelName = modelObj["name"].toString();
            
            // 移除可能的标签（如 :latest）
            if (modelName.contains(':')) {
                modelName = modelName.split(':').first();
            }
            
            if (modelName == m_checkingModel || 
                modelName.startsWith(m_checkingModel + ":")) {
                modelFound = true;
                m_status.ollamaModel = modelObj["name"].toString();
                break;
            }
        }
        
        // 收集所有可用模型
        QStringList availableModels;
        for (const QJsonValue &value : models) {
            QString name = value.toObject()["name"].toString();
            availableModels.append(name);
        }
        m_status.availableModels = availableModels;
        
        if (modelFound) {
            // 配置的模型存在
            m_status.ollamaAvailable = true;
            m_status.ollamaError = "";
            
            QString successMsg = QString("✅ Ollama连接成功\n模型：%1")
                                .arg(m_status.ollamaModel);
            
            qInfo() << "Ollama connection successful:" << m_status.ollamaModel;
            emit ollamaCheckCompleted(true, successMsg);
        } else if (!availableModels.isEmpty()) {
            // 配置的模型不存在，但有其他可用模型
            m_status.ollamaAvailable = false;
            m_status.needModelSelection = true;
            m_status.ollamaError = QString("配置的模型 '%1' 未安装\n\n"
                                          "但检测到以下可用模型：\n%2\n\n"
                                          "您可以：\n"
                                          "1. 选择使用已安装的模型\n"
                                          "2. 下载配置的模型：ollama pull %1")
                                  .arg(m_checkingModel)
                                  .arg(availableModels.join("\n"));
            
            qWarning() << "Ollama model not found, but other models available:" << availableModels;
            emit ollamaCheckCompleted(false, m_status.ollamaError);
        } else {
            // 没有任何模型
            m_status.ollamaAvailable = false;
            m_status.needModelSelection = false;
            m_status.ollamaError = QString("未安装任何模型\n\n"
                                          "请先下载模型，例如：\n"
                                          "ollama pull qwen\n"
                                          "ollama pull llama2\n"
                                          "ollama pull codellama");
            
            qWarning() << "No Ollama models installed";
            emit ollamaCheckCompleted(false, m_status.ollamaError);
        }
    }
    
    reply->deleteLater();
    checkIfAllCompleted();
}

void AIConnectionChecker::handleCloudApiReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        m_status.cloudApiAvailable = false;
        
        if (reply->error() == QNetworkReply::AuthenticationRequiredError ||
            reply->error() == QNetworkReply::ContentAccessDenied) {
            m_status.cloudApiError = "API Key无效或已过期";
        } else if (reply->error() == QNetworkReply::TimeoutError) {
            m_status.cloudApiError = "连接超时";
        } else {
            m_status.cloudApiError = QString("连接失败：%1").arg(reply->errorString());
        }
        
        qWarning() << "Cloud API connection failed:" << m_status.cloudApiError;
        emit cloudApiCheckCompleted(false, m_status.cloudApiError);
    } else {
        m_status.cloudApiAvailable = true;
        m_status.cloudApiError = "";
        
        qInfo() << "Cloud API connection successful";
        emit cloudApiCheckCompleted(true, "✅ 云端API连接成功");
    }
    
    reply->deleteLater();
    checkIfAllCompleted();
}

void AIConnectionChecker::checkIfAllCompleted()
{
    m_pendingChecks--;
    
    if (m_pendingChecks <= 0) {
        emit allChecksCompleted(m_status);
    }
}
