#include "OllamaClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>
#include <QTimer>
#include <QEventLoop>

OllamaClient::OllamaClient(QObject *parent)
    : AIService(parent)
    , m_baseUrl("http://localhost:11434")
    , m_model("qwen2.5-coder:7b")  // 默认使用qwen2.5-coder
    , m_cloudMode(false)
    , m_currentReply(nullptr)
    , m_isAborting(false)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &OllamaClient::handleNetworkReply);
}

void OllamaClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

void OllamaClient::setModel(const QString &model)
{
    m_model = model;
}

void OllamaClient::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void OllamaClient::setCloudMode(bool enabled)
{
    m_cloudMode = enabled;
    qDebug() << "[OllamaClient] Cloud mode set to:" << enabled;
    
    // 注意：不在这里设置baseUrl和model
    // 这些应该由调用者根据配置设置
    // 这样可以支持不同的云端API提供商
}

void OllamaClient::analyzeCode(const QString &questionDesc, const QString &code)
{
    QString prompt = QString(
        "你是一位经验丰富的编程导师。请分析以下C++代码，并提供详细的反馈。\n\n"
        "【题目】\n%1\n\n"
        "【学生代码】\n```cpp\n%2\n```\n\n"
        "请按以下格式提供分析：\n\n"
        "## 代码思路\n"
        "简要说明代码的核心思路和算法。\n\n"
        "## 代码优点\n"
        "列出代码中做得好的地方。\n\n"
        "## 改进建议\n"
        "提供具体的优化建议，包括：\n"
        "- 时间复杂度优化\n"
        "- 空间复杂度优化\n"
        "- 代码可读性\n"
        "- 边界条件处理\n\n"
        "## 涉及知识点\n"
        "列出代码涉及的数据结构、算法和编程技巧。\n\n"
        "## 参考代码（可选）\n"
        "如果有更优的实现方式，提供简短的代码示例。"
    ).arg(questionDesc, code);
    
    sendRequest(prompt, "code_analysis");
}

void OllamaClient::generateQuestions(const QJsonObject &params)
{
    QString prompt = "根据学习的题库，生成一套模拟题，包含题目描述、难度、测试用例和参考答案。";
    sendRequest(prompt, "generate_questions");
}

void OllamaClient::parseQuestionBank(const QStringList &mdFiles)
{
    QString prompt = "分析以下Markdown题库文件，提取题目结构、难度、标签等信息：\n\n";
    for (const auto &file : mdFiles) {
        prompt += file + "\n\n";
    }
    sendRequest(prompt, "parse_bank");
}

void OllamaClient::sendCustomPrompt(const QString &prompt, const QString &context)
{
    sendRequest(prompt, context);
}

QStringList OllamaClient::getAvailableModels()
{
    QStringList models;
    
    // 始终使用本地Ollama URL检测模型，不受当前模式影响
    QString ollamaUrl = "http://localhost:11434";
    
    // 创建独立的网络管理器，避免干扰主请求
    QNetworkAccessManager tempManager;
    QNetworkRequest request(QUrl(ollamaUrl + "/api/tags"));
    request.setTransferTimeout(5000);
    
    qDebug() << "[OllamaClient] 检测本地模型，URL:" << ollamaUrl;
    
    QNetworkReply *reply = tempManager.get(request);
    
    // 同步等待响应（最多5秒）
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(5000);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        qDebug() << "[OllamaClient] getAvailableModels 响应:" << data.left(200);
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonArray modelsArray = doc.object()["models"].toArray();
            for (const QJsonValue &val : modelsArray) {
                QString modelName = val.toObject()["name"].toString();
                if (!modelName.isEmpty()) {
                    models.append(modelName);
                }
            }
        }
    }
    
    reply->deleteLater();
    qDebug() << "[OllamaClient] 检测到模型:" << models;
    return models;
}

void OllamaClient::sendRequest(const QString &prompt, const QString &context)
{
    QJsonObject json;
    QUrl url;
    
    if (m_cloudMode) {
        // 云端API使用OpenAI格式
        QJsonArray messages;
        QJsonObject message;
        message["role"] = "user";
        message["content"] = prompt;
        messages.append(message);
        
        json["model"] = m_model;
        json["messages"] = messages;
        json["stream"] = true;
        
        url = QUrl(m_baseUrl + "/v1/chat/completions");
        
        qDebug() << "[OllamaClient] 云端API模式 - 发送请求到:" << url.toString();
    } else {
        // 本地Ollama格式 - 使用新的 /api/chat 端点
        QJsonArray messages;
        QJsonObject message;
        message["role"] = "user";
        message["content"] = prompt;
        messages.append(message);
        
        json["model"] = m_model;
        json["messages"] = messages;
        json["stream"] = true;
        
        url = QUrl(m_baseUrl + "/api/chat");
        
        qDebug() << "[OllamaClient] 本地Ollama模式 - 发送请求到:" << url.toString();
    }
    
    qDebug() << "[OllamaClient] 模型:" << m_model;
    qDebug() << "[OllamaClient] Context:" << context;
    qDebug() << "[OllamaClient] Prompt长度:" << prompt.length();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 如果是云端模式，添加API Key
    if (m_cloudMode && !m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
        qDebug() << "[OllamaClient] 已添加API Key认证";
    }
    
    // 设置超时（5分钟，因为AI处理可能需要较长时间）
    request.setTransferTimeout(300000);
    
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    reply->setProperty("context", context);
    reply->setProperty("fullResponse", QString());  // 存储完整响应
    
    // 连接readyRead信号以处理流式数据
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        QString context = reply->property("context").toString();
        QString fullResponse = reply->property("fullResponse").toString();
        
        // 读取新数据
        QByteArray newData = reply->readAll();
        
        // 流式响应是多个JSON对象，每行一个
        QList<QByteArray> lines = newData.split('\n');
        
        for (const QByteArray &line : lines) {
            if (line.trimmed().isEmpty()) continue;
            
            // 云端API的流式响应以"data: "开头
            QByteArray jsonLine = line;
            if (m_cloudMode && line.startsWith("data: ")) {
                jsonLine = line.mid(6);  // 去掉"data: "前缀
                if (jsonLine.trimmed() == "[DONE]") {
                    qDebug() << "[OllamaClient] 云端API流式响应完成";
                    continue;
                }
            }
            
            QJsonDocument doc = QJsonDocument::fromJson(jsonLine);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString chunk;
                
                if (m_cloudMode) {
                    // 云端API格式: choices[0].delta.content
                    QJsonArray choices = obj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject choice = choices[0].toObject();
                        QJsonObject delta = choice["delta"].toObject();
                        chunk = delta["content"].toString();
                    }
                } else {
                    // 本地Ollama格式
                    // 新API (/api/chat): message.content
                    // 旧API (/api/generate): response
                    if (obj.contains("message")) {
                        QJsonObject message = obj["message"].toObject();
                        chunk = message["content"].toString();
                    } else if (obj.contains("response")) {
                        chunk = obj["response"].toString();
                    }
                }
                
                if (!chunk.isEmpty()) {
                    fullResponse += chunk;
                    qDebug() << "[OllamaClient] 收到数据块:" << chunk.left(50);
                    
                    // 发射流式进度信号
                    emit streamProgress(context, fullResponse.length(), fullResponse);
                }
                
                // 检查是否完成
                bool done = obj["done"].toBool();
                if (done) {
                    qDebug() << "[OllamaClient] 流式响应完成，总长度:" << fullResponse.length();
                }
            }
        }
        
        // 更新存储的完整响应
        reply->setProperty("fullResponse", fullResponse);
    });
    
    // 连接finished信号
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        qDebug() << "[OllamaClient] Reply finished信号触发!";
    });
    
    // 连接错误信号
    connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError code) {
        qDebug() << "[OllamaClient] 网络错误发生:" << code << reply->errorString();
    });
    
    qDebug() << "[OllamaClient] 请求已发送，等待响应...";
    qDebug() << "[OllamaClient] Reply对象:" << reply;
}

void OllamaClient::handleNetworkReply(QNetworkReply *reply)
{
    qDebug() << "[OllamaClient] ========== handleNetworkReply 被调用 ==========";
    qDebug() << "[OllamaClient] Reply对象:" << reply;
    qDebug() << "[OllamaClient] URL:" << reply->url().toString();
    
    // 如果正在终止或这不是当前请求，忽略它
    if (m_isAborting) {
        qDebug() << "[OllamaClient] 正在终止请求，忽略回调";
        reply->deleteLater();
        return;
    }
    
    QString context = reply->property("context").toString();
    
    qDebug() << "[OllamaClient] 收到响应, Context:" << context;
    qDebug() << "[OllamaClient] 错误码:" << reply->error();
    qDebug() << "[OllamaClient] 错误信息:" << reply->errorString();
    
    if (reply->error() != QNetworkReply::NoError) {
        // 忽略用户主动取消的错误
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            qDebug() << "[OllamaClient] 请求已被用户取消 (handleNetworkReply, context:" << context << ")";
            reply->deleteLater();
            return;
        }
        
        QString errorMsg;
        
        // 根据错误类型提供友好的错误信息
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "无法连接到Ollama服务\n\n"
                          "请检查：\n"
                          "1. Ollama服务是否正在运行（ollama serve）\n"
                          "2. 服务地址是否正确（默认：http://localhost:11434）\n"
                          "3. 防火墙是否阻止连接";
                break;
                
            case QNetworkReply::HostNotFoundError:
                errorMsg = "找不到Ollama服务器\n\n"
                          "请检查服务地址配置是否正确";
                break;
                
            case QNetworkReply::TimeoutError:
                errorMsg = "请求超时\n\n"
                          "可能原因：\n"
                          "1. 网络连接不稳定\n"
                          "2. Ollama服务响应缓慢\n"
                          "3. 模型正在加载中";
                break;
                
            case QNetworkReply::ContentNotFoundError:
                errorMsg = "API端点不存在\n\n"
                          "可能原因：\n"
                          "1. Ollama版本过旧，请更新到最新版本\n"
                          "2. API地址配置错误";
                break;
                
            default:
                errorMsg = QString("网络请求失败\n\n"
                                  "错误信息：%1\n\n"
                                  "请检查Ollama服务状态").arg(reply->errorString());
                break;
        }
        
        emit error(errorMsg);
        reply->deleteLater();
        return;
    }
    
    // 从属性中获取完整响应（流式模式）
    QString response = reply->property("fullResponse").toString();
    
    qDebug() << "[OllamaClient] 完整响应长度:" << response.length();
    qDebug() << "[OllamaClient] 响应前100字符:" << response.left(100);
    
    if (context == "code_analysis" || context == "custom" || context == "question_parse" || context == "ai_judge") {
        qDebug() << "[OllamaClient] 发送 codeAnalysisReady 信号 (context:" << context << ")";
        emit codeAnalysisReady(response);
    } else if (context == "generate_questions") {
        // 解析生成的题目JSON
        emit questionsGenerated(QJsonArray());
    } else if (context == "parse_bank") {
        emit questionBankParsed(QJsonArray());
    }
    
    reply->deleteLater();
}


void OllamaClient::sendChatMessage(const QString &message, const QString &systemPrompt)
{
    // 如果有正在进行的请求，先终止它
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    QJsonObject json;
    QUrl url;
    
    if (m_cloudMode) {
        // 云端API使用OpenAI格式
        QJsonArray messages;
        
        // 添加系统提示词
        if (!systemPrompt.isEmpty()) {
            QJsonObject systemMsg;
            systemMsg["role"] = "system";
            systemMsg["content"] = systemPrompt;
            messages.append(systemMsg);
        }
        
        // 添加用户消息
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = message;
        messages.append(userMsg);
        
        json["model"] = m_model;
        json["messages"] = messages;
        json["stream"] = true;
        json["max_tokens"] = 800;  // 适度限制，允许复杂问题展开
        
        url = QUrl(m_baseUrl + "/v1/chat/completions");
        
        qDebug() << "[OllamaClient] 云端聊天模式 - 发送消息";
    } else {
        // 本地Ollama格式 - 使用新的 /api/chat 端点
        QJsonArray messages;
        
        // 添加系统提示词
        if (!systemPrompt.isEmpty()) {
            QJsonObject systemMsg;
            systemMsg["role"] = "system";
            systemMsg["content"] = systemPrompt;
            messages.append(systemMsg);
        }
        
        // 添加用户消息
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = message;
        messages.append(userMsg);
        
        json["model"] = m_model;
        json["messages"] = messages;
        json["stream"] = true;
        
        // 添加参数限制输出长度（给AI更多灵活性）
        QJsonObject options;
        options["num_predict"] = 800;  // 适度限制，允许复杂问题展开
        json["options"] = options;
        
        url = QUrl(m_baseUrl + "/api/chat");
        
        qDebug() << "[OllamaClient] 本地聊天模式 - 发送消息";
    }
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 如果是云端模式，添加API Key
    if (m_cloudMode && !m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    }
    
    request.setTransferTimeout(300000);
    
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    m_currentReply = reply;  // 记录当前请求
    reply->setProperty("context", "chat");
    reply->setProperty("fullResponse", QString());
    
    // 连接readyRead信号以处理流式数据
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        QString context = reply->property("context").toString();
        QString fullResponse = reply->property("fullResponse").toString();
        
        // 读取新数据
        QByteArray newData = reply->readAll();
        
        // 流式响应是多个JSON对象，每行一个
        QList<QByteArray> lines = newData.split('\n');
        
        for (const QByteArray &line : lines) {
            if (line.trimmed().isEmpty()) continue;
            
            // 云端API的流式响应以"data: "开头
            QByteArray jsonLine = line;
            if (m_cloudMode && line.startsWith("data: ")) {
                jsonLine = line.mid(6);
                if (jsonLine.trimmed() == "[DONE]") {
                    emit streamingFinished();
                    continue;
                }
            }
            
            QJsonDocument doc = QJsonDocument::fromJson(jsonLine);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString chunk;
                
                if (m_cloudMode) {
                    // 云端API格式
                    QJsonArray choices = obj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject choice = choices[0].toObject();
                        QJsonObject delta = choice["delta"].toObject();
                        chunk = delta["content"].toString();
                    }
                } else {
                    // 本地Ollama格式
                    // 新API (/api/chat): message.content
                    // 旧API (/api/generate): response
                    if (obj.contains("message")) {
                        QJsonObject message = obj["message"].toObject();
                        chunk = message["content"].toString();
                    } else if (obj.contains("response")) {
                        chunk = obj["response"].toString();
                    }
                }
                
                if (!chunk.isEmpty()) {
                    fullResponse += chunk;
                    
                    // 只为非AI判题的请求发送流式数据块信号
                    // AI判题使用 "ai_judge" context，不应该显示在AI导师面板
                    if (context != "ai_judge") {
                        emit streamingChunk(chunk);
                    }
                }
                
                // 检查是否完成
                bool done = obj["done"].toBool();
                if (done) {
                    // 只为非AI判题的请求发送完成信号
                    if (context != "ai_judge") {
                        emit streamingFinished();
                    }
                }
            }
        }
        
        // 更新存储的完整响应
        reply->setProperty("fullResponse", fullResponse);
    });
    
    // 错误处理
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError code) {
        QString context = reply->property("context").toString();
        
        // 忽略用户主动取消的错误
        if (code == QNetworkReply::OperationCanceledError) {
            qDebug() << "[OllamaClient] 请求已被用户取消 (context:" << context << ")";
            reply->deleteLater();
            return;
        }
        
        // 只为聊天context发送错误信号到UI
        // 其他context（如ai_judge）的错误不应该显示在聊天界面
        if (context == "chat") {
            QString errorMsg;
            
            // 根据错误类型提供友好的错误信息
            switch (code) {
                case QNetworkReply::ConnectionRefusedError:
                    errorMsg = "无法连接到AI服务\n\n"
                              "请检查：\n"
                              "1. AI服务是否正在运行\n"
                              "2. 服务地址是否正确\n"
                              "3. 防火墙是否阻止连接";
                    break;
                    
                case QNetworkReply::HostNotFoundError:
                    errorMsg = "找不到AI服务器\n\n"
                              "请检查服务地址配置是否正确";
                    break;
                    
                case QNetworkReply::TimeoutError:
                    errorMsg = "请求超时\n\n"
                              "可能原因：\n"
                              "1. 网络连接不稳定\n"
                              "2. AI服务响应缓慢\n"
                              "3. 模型正在加载中";
                    break;
                    
                case QNetworkReply::ContentNotFoundError:
                    errorMsg = "API端点不存在\n\n"
                              "可能原因：\n"
                              "1. Ollama版本过旧，请更新到最新版本\n"
                              "2. API地址配置错误\n"
                              "3. 请在设置中检测并选择正确的模型";
                    break;
                    
                default:
                    errorMsg = QString("网络请求失败\n\n"
                                      "错误信息：%1\n\n"
                                      "请检查AI服务状态").arg(reply->errorString());
                    break;
            }
            
            emit error(errorMsg);
        } else {
            // 非聊天context的错误只记录日志，不显示在UI
            qWarning() << "[OllamaClient] 网络错误 (context:" << context << "):" << reply->errorString();
        }
        
        reply->deleteLater();
    });
    
    // 完成后清理
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // 只有在不是终止状态时才处理
        if (!m_isAborting && m_currentReply == reply) {
            m_currentReply = nullptr;
        }
    });
}

void OllamaClient::abortCurrentRequest()
{
    if (m_currentReply && !m_isAborting) {
        qDebug() << "[OllamaClient] 终止当前请求";
        m_isAborting = true;  // 设置终止标志
        
        // 保存指针并清空成员变量（这样handleNetworkReply会忽略它）
        QNetworkReply *replyToAbort = m_currentReply;
        m_currentReply = nullptr;
        
        // 断开所有信号连接，避免触发任何回调
        replyToAbort->disconnect();
        
        // 终止请求
        replyToAbort->abort();
        replyToAbort->deleteLater();
        
        m_isAborting = false;  // 重置标志
        
        qDebug() << "[OllamaClient] 请求已终止";
    }
}
