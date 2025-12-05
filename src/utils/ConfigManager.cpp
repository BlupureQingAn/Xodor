#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

void ConfigManager::load()
{
    QFile file("data/config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        // 使用默认值
        m_compilerPath = "g++";
        m_ollamaUrl = "http://localhost:11434";
        m_ollamaModel = "qwen2.5-coder:7b";  // 默认使用qwen2.5-coder
        m_cloudApiUrl = "https://api.deepseek.com";
        m_cloudApiModel = "deepseek-chat";
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    
    m_compilerPath = obj["compilerPath"].toString("g++");
    m_ollamaUrl = obj["ollamaUrl"].toString("http://localhost:11434");
    m_ollamaModel = obj["ollamaModel"].toString("qwen2.5-coder:7b");  // 默认使用qwen2.5-coder
    m_cloudApiKey = obj["cloudApiKey"].toString();
    m_cloudApiUrl = obj["cloudApiUrl"].toString("https://api.deepseek.com");
    m_cloudApiModel = obj["cloudApiModel"].toString("deepseek-chat");
    m_useCloudMode = obj["useCloudMode"].toBool(false);
    
    file.close();
}

void ConfigManager::save()
{
    QDir dir("data");
    if (!dir.exists()) dir.mkpath(".");
    
    QJsonObject obj;
    obj["compilerPath"] = m_compilerPath;
    obj["ollamaUrl"] = m_ollamaUrl;
    obj["ollamaModel"] = m_ollamaModel;
    obj["cloudApiKey"] = m_cloudApiKey;
    obj["cloudApiUrl"] = m_cloudApiUrl;
    obj["cloudApiModel"] = m_cloudApiModel;
    obj["useCloudMode"] = m_useCloudMode;
    
    QFile file("data/config.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}
