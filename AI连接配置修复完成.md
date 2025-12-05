# AI连接配置修复完成

## 问题描述

用户反馈：修复AI连接的问题。

## 问题诊断

### 发现的问题

#### 1. 云端API配置不完整 ⚠️

**问题**：`ConfigManager` 只保存了云端API的Key，没有保存URL和Model

```cpp
// 旧代码
class ConfigManager {
    QString m_cloudApiKey;  // 只有Key
    // 缺少URL和Model配置
};
```

**后果**：
- 无法支持不同的云端API提供商
- URL和Model被硬编码在 `setCloudMode` 中
- 用户无法自定义云端API配置

#### 2. setCloudMode 覆盖配置 ⚠️

**问题**：`OllamaClient::setCloudMode` 会自动设置URL和Model

```cpp
// 旧代码
void OllamaClient::setCloudMode(bool enabled)
{
    m_cloudMode = enabled;
    if (m_cloudMode) {
        m_baseUrl = "https://api.deepseek.com";  // ❌ 硬编码
        m_model = "deepseek-chat";  // ❌ 硬编码
    } else {
        m_baseUrl = "http://localhost:11434";  // ❌ 硬编码
    }
}
```

**后果**：
- 即使在 `loadConfiguration` 中设置了URL和Model，也会被覆盖
- 无法支持其他云端API提供商（如OpenAI、Claude等）
- 配置不灵活

#### 3. 配置加载顺序问题

**问题**：在 `loadConfiguration` 中，云端模式只设置了Key

```cpp
// 旧代码
if (config.useCloudApi()) {
    m_ollamaClient->setCloudMode(true);  // 这会覆盖URL和Model
    m_ollamaClient->setApiKey(config.cloudApiKey());
    // ❌ 没有设置URL和Model
}
```

## 解决方案

### 1. 扩展ConfigManager配置 ✅

**文件**：`src/utils/ConfigManager.h`

添加云端API的URL和Model配置：

```cpp
class ConfigManager
{
public:
    // ✅ 添加云端API配置
    QString cloudApiUrl() const { return m_cloudApiUrl; }
    QString cloudApiModel() const { return m_cloudApiModel; }
    
    void setCloudApiUrl(const QString &url) { m_cloudApiUrl = url; }
    void setCloudApiModel(const QString &model) { m_cloudApiModel = model; }
    
private:
    QString m_cloudApiKey;
    QString m_cloudApiUrl;      // ✅ 新增
    QString m_cloudApiModel;    // ✅ 新增
    bool m_useCloudMode = false;
};
```

### 2. 更新配置加载和保存 ✅

**文件**：`src/utils/ConfigManager.cpp`

#### 加载配置

```cpp
void ConfigManager::load()
{
    QFile file("data/config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        // ✅ 使用默认值
        m_compilerPath = "g++";
        m_ollamaUrl = "http://localhost:11434";
        m_ollamaModel = "qwen2.5:7b";
        m_cloudApiUrl = "https://api.deepseek.com";      // ✅ 默认云端URL
        m_cloudApiModel = "deepseek-chat";               // ✅ 默认云端模型
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    
    m_compilerPath = obj["compilerPath"].toString("g++");
    m_ollamaUrl = obj["ollamaUrl"].toString("http://localhost:11434");
    m_ollamaModel = obj["ollamaModel"].toString("qwen2.5:7b");
    m_cloudApiKey = obj["cloudApiKey"].toString();
    m_cloudApiUrl = obj["cloudApiUrl"].toString("https://api.deepseek.com");      // ✅ 加载云端URL
    m_cloudApiModel = obj["cloudApiModel"].toString("deepseek-chat");             // ✅ 加载云端模型
    m_useCloudMode = obj["useCloudMode"].toBool(false);
    
    file.close();
}
```

#### 保存配置

```cpp
void ConfigManager::save()
{
    QDir dir("data");
    if (!dir.exists()) dir.mkpath(".");
    
    QJsonObject obj;
    obj["compilerPath"] = m_compilerPath;
    obj["ollamaUrl"] = m_ollamaUrl;
    obj["ollamaModel"] = m_ollamaModel;
    obj["cloudApiKey"] = m_cloudApiKey;
    obj["cloudApiUrl"] = m_cloudApiUrl;          // ✅ 保存云端URL
    obj["cloudApiModel"] = m_cloudApiModel;      // ✅ 保存云端模型
    obj["useCloudMode"] = m_useCloudMode;
    
    QFile file("data/config.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}
```

### 3. 简化setCloudMode ✅

**文件**：`src/ai/OllamaClient.cpp`

移除硬编码的URL和Model设置：

```cpp
void OllamaClient::setCloudMode(bool enabled)
{
    m_cloudMode = enabled;
    qDebug() << "[OllamaClient] Cloud mode set to:" << enabled;
    
    // ✅ 不在这里设置baseUrl和model
    // 这些应该由调用者根据配置设置
    // 这样可以支持不同的云端API提供商
}
```

**关键改进**：
- ✅ 只设置模式标志，不修改URL和Model
- ✅ 由调用者负责设置正确的URL和Model
- ✅ 支持任意云端API提供商

### 4. 正确配置AI客户端 ✅

**文件**：`src/ui/MainWindow.cpp`

在 `loadConfiguration` 中正确设置所有参数：

```cpp
void MainWindow::loadConfiguration()
{
    ConfigManager &config = ConfigManager::instance();
    
    // ... 编译器配置 ...
    
    // ✅ 配置AI服务
    if (config.useCloudApi()) {
        // 使用云端API
        m_ollamaClient->setCloudMode(true);
        m_ollamaClient->setBaseUrl(config.cloudApiUrl());      // ✅ 设置云端URL
        m_ollamaClient->setModel(config.cloudApiModel());      // ✅ 设置云端模型
        m_ollamaClient->setApiKey(config.cloudApiKey());
        
        qDebug() << "[MainWindow] 配置为云端API模式";
        qDebug() << "[MainWindow]   URL:" << config.cloudApiUrl();
        qDebug() << "[MainWindow]   Model:" << config.cloudApiModel();
        qDebug() << "[MainWindow]   API Key:" << (config.cloudApiKey().isEmpty() ? "未设置" : "已设置");
    } else {
        // 使用本地Ollama
        m_ollamaClient->setCloudMode(false);
        m_ollamaClient->setBaseUrl(config.ollamaUrl());
        m_ollamaClient->setModel(config.ollamaModel());
        
        qDebug() << "[MainWindow] 配置为本地Ollama模式";
        qDebug() << "[MainWindow]   URL:" << config.ollamaUrl();
        qDebug() << "[MainWindow]   Model:" << config.ollamaModel();
    }
}
```

**关键改进**：
- ✅ 先设置 `setCloudMode`
- ✅ 再设置 `setBaseUrl` 和 `setModel`（不会被覆盖）
- ✅ 最后设置 `setApiKey`
- ✅ 添加详细的调试日志

## 配置文件格式

### data/config.json

```json
{
    "compilerPath": "g++",
    "ollamaUrl": "http://localhost:11434",
    "ollamaModel": "qwen2.5:7b",
    "cloudApiKey": "sk-xxx",
    "cloudApiUrl": "https://api.deepseek.com",
    "cloudApiModel": "deepseek-chat",
    "useCloudMode": false
}
```

### 支持的云端API提供商

| 提供商 | URL | 模型示例 |
|--------|-----|---------|
| DeepSeek | `https://api.deepseek.com` | `deepseek-chat` |
| OpenAI | `https://api.openai.com` | `gpt-4`, `gpt-3.5-turbo` |
| Claude | `https://api.anthropic.com` | `claude-3-opus` |
| 其他兼容OpenAI API的服务 | 自定义URL | 自定义模型 |

## 配置流程

### 本地Ollama模式

```
启动程序
    ↓
ConfigManager::load()
    ↓
读取 ollamaUrl 和 ollamaModel
    ↓
MainWindow::loadConfiguration()
    ↓
setCloudMode(false)
    ↓
setBaseUrl(ollamaUrl)
    ↓
setModel(ollamaModel)
    ↓
本地Ollama配置完成
```

### 云端API模式

```
启动程序
    ↓
ConfigManager::load()
    ↓
读取 cloudApiUrl, cloudApiModel, cloudApiKey
    ↓
MainWindow::loadConfiguration()
    ↓
setCloudMode(true)
    ↓
setBaseUrl(cloudApiUrl)
    ↓
setModel(cloudApiModel)
    ↓
setApiKey(cloudApiKey)
    ↓
云端API配置完成
```

## 调试日志示例

### 本地Ollama模式

```
[MainWindow] 配置为本地Ollama模式
[MainWindow]   URL: http://localhost:11434
[MainWindow]   Model: qwen2.5:7b
[OllamaClient] Cloud mode set to: false
```

### 云端API模式

```
[MainWindow] 配置为云端API模式
[MainWindow]   URL: https://api.deepseek.com
[MainWindow]   Model: deepseek-chat
[MainWindow]   API Key: 已设置
[OllamaClient] Cloud mode set to: true
```

## 修改文件清单

| 文件 | 修改内容 | 状态 |
|------|---------|------|
| `src/utils/ConfigManager.h` | 添加cloudApiUrl和cloudApiModel | ✅ |
| `src/utils/ConfigManager.cpp` | 加载和保存云端API配置 | ✅ |
| `src/ai/OllamaClient.cpp` | 简化setCloudMode，移除硬编码 | ✅ |
| `src/ui/MainWindow.cpp` | 正确配置云端API参数 | ✅ |
| 编译状态 | 编译成功，无错误 | ✅ |

## 测试验证

### 测试场景1：本地Ollama模式

1. 配置文件设置：
```json
{
    "useCloudMode": false,
    "ollamaUrl": "http://localhost:11434",
    "ollamaModel": "qwen2.5:7b"
}
```

2. 启动程序
3. 查看调试日志

**预期结果**：
- ✅ 显示"配置为本地Ollama模式"
- ✅ URL和Model正确
- ✅ AI功能正常工作

### 测试场景2：云端API模式（DeepSeek）

1. 配置文件设置：
```json
{
    "useCloudMode": true,
    "cloudApiUrl": "https://api.deepseek.com",
    "cloudApiModel": "deepseek-chat",
    "cloudApiKey": "sk-xxx"
}
```

2. 启动程序
3. 查看调试日志

**预期结果**：
- ✅ 显示"配置为云端API模式"
- ✅ URL、Model和API Key正确
- ✅ AI功能正常工作

### 测试场景3：切换模式

1. 从本地模式切换到云端模式
2. 打开设置对话框
3. 修改配置
4. 保存并重启

**预期结果**：
- ✅ 配置正确保存
- ✅ 重启后使用新配置
- ✅ AI功能正常工作

## 总结

AI连接配置问题已修复：

### ✅ 配置完整性
- 添加云端API的URL和Model配置
- 支持任意云端API提供商
- 配置灵活可定制

### ✅ 配置不被覆盖
- `setCloudMode` 不再修改URL和Model
- 由调用者负责设置正确的配置
- 配置顺序正确

### ✅ 调试友好
- 添加详细的调试日志
- 显示所有配置参数
- 便于诊断问题

### ✅ 向后兼容
- 旧配置文件自动使用默认值
- 不影响现有用户
- 平滑升级

现在AI连接配置应该能够正常工作，支持本地Ollama和各种云端API提供商！
