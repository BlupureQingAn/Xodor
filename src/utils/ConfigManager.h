#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QJsonObject>

class ConfigManager
{
public:
    static ConfigManager& instance();
    
    void load();
    void save();
    
    QString compilerPath() const { return m_compilerPath; }
    QString ollamaUrl() const { return m_ollamaUrl; }
    QString ollamaModel() const { return m_ollamaModel; }
    QString cloudApiKey() const { return m_cloudApiKey; }
    bool useCloudMode() const { return m_useCloudMode; }
    
    // 判断当前使用哪种AI模式
    bool useCloudApi() const { return m_useCloudMode; }
    bool useLocalOllama() const { return !m_useCloudMode; }
    
    void setCompilerPath(const QString &path) { m_compilerPath = path; }
    void setOllamaUrl(const QString &url) { m_ollamaUrl = url; }
    void setOllamaModel(const QString &model) { m_ollamaModel = model; }
    void setCloudApiKey(const QString &key) { m_cloudApiKey = key; }
    void setUseCloudMode(bool useCloud) { m_useCloudMode = useCloud; }
    
private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    QString m_compilerPath;
    QString m_ollamaUrl;
    QString m_ollamaModel;
    QString m_cloudApiKey;
    bool m_useCloudMode = false;  // 当前使用的模式：false=本地，true=云端
};

#endif // CONFIGMANAGER_H
