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
    
    qInfo() << "=== Ollama连接检测开始 ===";
    qInfo() << "检测URL:" << url.toString();
    qInfo() << "配置的模型:" << model;
    
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
    m_pendingChecks++;
    
    if (apiKey.isEmpty()) {
        m_status.cloudApiAvailable = false;
        m_status.cloudApiError = "未配置API Key";
        qInfo() << "Cloud API check: No API key configured";
        emit cloudApiCheckCompleted(false, m_status.cloudApiError);
        checkIfAllCompleted();
        return;
    }
    
    // 简单验证：检查API Key格式
    // 大多数API Key都是以 "sk-" 开头或者是长字符串
    if (apiKey.length() < 10) {
        m_status.cloudApiAvailable = false;
        m_status.cloudApiError = "API Key格式可能不正确（长度过短）";
        qWarning() << "Cloud API check: API key too short";
        emit cloudApiCheckCompleted(false, m_status.cloudApiError);
        checkIfAllCompleted();
        return;
    }
    
    // 如果配置了API Key，就认为可用
    // 实际的连接验证会在真正使用时进行
    m_status.cloudApiAvailable = true;
    m_status.cloudApiError = "";
    
    qInfo() << "Cloud API check: API key configured (length:" << apiKey.length() << ")";
    qInfo() << "Cloud API connection assumed available (will verify on actual use)";
    emit cloudApiCheckCompleted(true, "✅ 云端API已配置");
    checkIfAllCompleted();
}

void AIConnectionChecker::handleOllamaReply(QNetworkReply *reply)
{
    qInfo() << "=== Ollama响应接收 ===";
    qInfo() << "HTTP状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qInfo() << "错误代码:" << reply->error();
    qInfo() << "错误信息:" << reply->errorString();
    
    if (reply->error() != QNetworkReply::NoError) {
        m_status.ollamaAvailable = false;
        
        qWarning() << "Ollama连接失败，错误详情:";
        qWarning() << "  URL:" << reply->url().toString();
        qWarning() << "  错误代码:" << reply->error();
        qWarning() << "  错误信息:" << reply->errorString();
        
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
        
        qInfo() << "Ollama服务响应成功";
        qInfo() << "响应数据长度:" << data.size() << "字节";
        qDebug() << "响应内容:" << data;
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            qWarning() << "JSON解析失败！响应不是有效的JSON";
            m_status.ollamaAvailable = false;
            m_status.ollamaError = "服务器响应格式错误";
            emit ollamaCheckCompleted(false, m_status.ollamaError);
            reply->deleteLater();
            checkIfAllCompleted();
            return;
        }
        
        QJsonObject obj = doc.object();
        QJsonArray models = obj["models"].toArray();
        
        qInfo() << "=== 模型检测 ===";
        qInfo() << "配置的模型:" << m_checkingModel;
        qInfo() << "检测到的模型数量:" << models.size();
        
        bool modelFound = false;
        QString foundModelName;
        
        qInfo() << "开始匹配模型...";
        
        for (const QJsonValue &value : models) {
            QJsonObject modelObj = value.toObject();
            QString fullModelName = modelObj["name"].toString();  // 如 "qwen2.5:7b"
            
            qDebug() << "  检查模型:" << fullModelName;
            
            QString baseModelName = fullModelName;
            
            // 移除标签（如 :latest, :7b）
            if (baseModelName.contains(':')) {
                baseModelName = baseModelName.split(':').first();
            }
            
            QString checkingBase = m_checkingModel;
            if (checkingBase.contains(':')) {
                checkingBase = checkingBase.split(':').first();
            }
            
            qDebug() << "    完整名称:" << fullModelName;
            qDebug() << "    基础名称:" << baseModelName;
            qDebug() << "    配置基础:" << checkingBase;
            
            // 更宽松的匹配逻辑
            // 1. 完全匹配：qwen2.5:7b == qwen2.5:7b
            // 2. 基础名称匹配：qwen == qwen2.5
            // 3. 前缀匹配：qwen 匹配 qwen2.5, qwen-plus 等
            bool match1 = (fullModelName == m_checkingModel);
            bool match2 = (baseModelName == checkingBase);
            bool match3 = baseModelName.startsWith(checkingBase);
            bool match4 = fullModelName.startsWith(m_checkingModel + ":");
            
            qDebug() << "    完全匹配?" << match1;
            qDebug() << "    基础匹配?" << match2;
            qDebug() << "    前缀匹配?" << match3;
            qDebug() << "    标签匹配?" << match4;
            
            if (match1 || match2 || match3 || match4) {
                modelFound = true;
                foundModelName = fullModelName;
                m_status.ollamaModel = fullModelName;
                
                qInfo() << "✅ 找到匹配的模型!";
                qInfo() << "  配置的模型:" << m_checkingModel;
                qInfo() << "  找到的模型:" << fullModelName;
                qInfo() << "  匹配方式:" << (match1 ? "完全匹配" : match2 ? "基础匹配" : match3 ? "前缀匹配" : "标签匹配");
                break;
            }
        }
        
        if (!modelFound) {
            qWarning() << "❌ 未找到匹配的模型";
            qWarning() << "  配置的模型:" << m_checkingModel;
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
            // 自动使用第一个可用的模型
            m_status.ollamaAvailable = true;
            m_status.ollamaModel = availableModels.first();
            m_status.needModelSelection = true;
            m_status.ollamaError = "";
            
            QString infoMsg = QString("✅ Ollama连接成功\n"
                                     "自动使用模型：%1\n"
                                     "（配置的模型 '%2' 未找到）")
                             .arg(m_status.ollamaModel)
                             .arg(m_checkingModel);
            
            qInfo() << "Ollama connection successful (auto-selected):" << m_status.ollamaModel;
            qInfo() << "  Configured model not found:" << m_checkingModel;
            qInfo() << "  Available models:" << availableModels;
            emit ollamaCheckCompleted(true, infoMsg);
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



void AIConnectionChecker::checkIfAllCompleted()
{
    m_pendingChecks--;
    
    if (m_pendingChecks <= 0) {
        emit allChecksCompleted(m_status);
    }
}
