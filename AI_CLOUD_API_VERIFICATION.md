# AI对话框云端API支持验证

## ✅ 验证结果：完全支持

AI对话框（AIAssistantPanel）已经完全支持云端API，会自动使用启动时配置的AI服务。

## 🔍 验证过程

### 1. OllamaClient初始化
**位置**: `MainWindow::MainWindow()`
```cpp
m_ollamaClient = new OllamaClient(this);
```

### 2. 配置加载
**位置**: `MainWindow::loadConfiguration()`
```cpp
if (config.useCloudApi()) {
    // 使用云端API
    m_ollamaClient->setCloudMode(true);
    m_ollamaClient->setApiKey(config.cloudApiKey());
} else {
    // 使用本地Ollama
    m_ollamaClient->setCloudMode(false);
    m_ollamaClient->setBaseUrl(config.ollamaUrl());
    m_ollamaClient->setModel(config.ollamaModel());
}
```

### 3. AIAssistantPanel使用同一实例
**位置**: `MainWindow::setupUI()`
```cpp
m_aiAssistantPanel = new AIAssistantPanel(m_ollamaClient, this);
```

### 4. 发送消息时自动选择模式
**位置**: `OllamaClient::sendChatMessage()`
```cpp
if (m_cloudMode) {
    // 云端API使用OpenAI格式
    QJsonArray messages;
    // ... 添加系统提示词和用户消息
    json["model"] = m_model;
    json["messages"] = messages;
    json["stream"] = true;
    url = QUrl(m_baseUrl + "/v1/chat/completions");
    
    // 添加API Key认证
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", 
            QString("Bearer %1").arg(m_apiKey).toUtf8());
    }
} else {
    // 本地Ollama格式
    json["model"] = m_model;
    json["prompt"] = fullPrompt;
    json["stream"] = true;
    url = QUrl(m_baseUrl + "/api/generate");
}
```

## 📋 配置流程

### 启动时选择云端API
1. 用户启动程序
2. 显示AI配置对话框
3. 用户选择"☁️ 云端API"标签页
4. 输入API Key
5. 点击"确定"

### 配置保存
```cpp
config.setCloudApiKey(apiKey);
config.setUseCloudMode(true);
config.save();

m_ollamaClient->setCloudMode(true);
m_ollamaClient->setApiKey(apiKey);
```

### AI对话框自动使用云端
- AIAssistantPanel使用同一个 `m_ollamaClient` 实例
- 发送消息时自动检测 `m_cloudMode`
- 自动使用云端API格式和API Key认证

## 🎯 使用场景

### 场景1：启动时选择云端API
```
1. 启动程序
2. AI配置对话框 → 选择"云端API"
3. 输入API Key: sk-xxxxx
4. 确定
5. 导入题目
6. 在AI导师面板输入问题
   → 自动使用云端API（DeepSeek）
   → 使用保存的API Key认证
   → 流式输出AI回复
```

### 场景2：启动时选择本地Ollama
```
1. 启动程序
2. AI配置对话框 → 选择"本地Ollama"
3. 选择模型: qwen2.5:7b
4. 确定
5. 导入题目
6. 在AI导师面板输入问题
   → 自动使用本地Ollama
   → 连接 http://localhost:11434
   → 流式输出AI回复
```

### 场景3：切换模式
```
1. 设置 → AI配置
2. 切换到"云端API"标签页
3. 输入新的API Key
4. 保存
   → OllamaClient自动切换到云端模式
   → AI对话框立即使用新配置
```

## 🔧 技术细节

### 配置同步
```cpp
// ConfigManager保存配置
{
  "cloudApiKey": "sk-xxxxx",
  "ollamaModel": "qwen2.5:7b",
  "useCloudMode": true  // 当前使用云端模式
}

// OllamaClient状态
m_cloudMode = true
m_apiKey = "sk-xxxxx"
m_baseUrl = "https://api.deepseek.com"
m_model = "deepseek-chat"
```

### 请求格式

**云端API请求**:
```json
POST https://api.deepseek.com/v1/chat/completions
Headers:
  Authorization: Bearer sk-xxxxx
  Content-Type: application/json
Body:
{
  "model": "deepseek-chat",
  "messages": [
    {"role": "system", "content": "你是一位编程导师..."},
    {"role": "user", "content": "这道题我不会做"}
  ],
  "stream": true
}
```

**本地Ollama请求**:
```json
POST http://localhost:11434/api/generate
Headers:
  Content-Type: application/json
Body:
{
  "model": "qwen2.5:7b",
  "prompt": "系统提示词\n\n用户消息",
  "stream": true
}
```

## ✅ 验证清单

- [x] OllamaClient支持云端模式
- [x] 启动时正确加载配置
- [x] AIAssistantPanel使用同一OllamaClient实例
- [x] sendChatMessage自动选择正确的API
- [x] 云端模式下添加API Key认证
- [x] 流式输出支持云端API
- [x] 配置切换立即生效

## 🎉 结论

**AI对话框完全支持云端API！**

- 启动时选择云端API → AI对话框自动使用云端
- 启动时选择本地Ollama → AI对话框自动使用本地
- 配置切换 → AI对话框立即跟随
- 无需额外配置，完全自动同步

用户只需要在启动时或设置中配置一次，所有AI功能（导入、分析、对话）都会使用相同的配置。
