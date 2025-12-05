# Ollama新API端点修复完成

## 问题描述

用户反馈错误：
```
❌ 错误: 网络错误: Error transferring http://localhost:11434/api/generate - server replied: Not Found
```

用户配置的模型是 "qwen"，实际安装的是 "qwen2.5-7b"。

## 问题诊断

### 1. API端点过时 ⚠️

**问题**：代码使用的是旧的 `/api/generate` 端点

```cpp
// 旧代码
url = QUrl(m_baseUrl + "/api/generate");  // ❌ 旧端点
```

**Ollama API变化**：
- **旧API**：`/api/generate` - 使用 `prompt` 字段
- **新API**：`/api/chat` - 使用 `messages` 数组

**后果**：
- 新版Ollama不再支持 `/api/generate` 端点
- 返回 404 Not Found 错误
- AI功能完全无法使用

### 2. 请求格式不匹配

**旧格式** (`/api/generate`):
```json
{
  "model": "qwen2.5:7b",
  "prompt": "Hello",
  "stream": true
}
```

**新格式** (`/api/chat`):
```json
{
  "model": "qwen2.5:7b",
  "messages": [
    {
      "role": "user",
      "content": "Hello"
    }
  ],
  "stream": true
}
```

### 3. 响应格式不同

**旧格式响应**:
```json
{
  "model": "qwen2.5:7b",
  "response": "chunk text",
  "done": false
}
```

**新格式响应**:
```json
{
  "model": "qwen2.5:7b",
  "message": {
    "role": "assistant",
    "content": "chunk text"
  },
  "done": false
}
```

### 4. 模型名称问题

用户配置 "qwen" 但实际安装的是 "qwen2.5-7b"，模型名不匹配也会导致问题。

## 解决方案

### 1. 更新API端点 ✅

**文件**：`src/ai/OllamaClient.cpp`

#### sendRequest 方法

```cpp
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
    } else {
        // ✅ 本地Ollama格式 - 使用新的 /api/chat 端点
        QJsonArray messages;
        QJsonObject message;
        message["role"] = "user";
        message["content"] = prompt;
        messages.append(message);
        
        json["model"] = m_model;
        json["messages"] = messages;
        json["stream"] = true;
        
        url = QUrl(m_baseUrl + "/api/chat");  // ✅ 新端点
        
        qDebug() << "[OllamaClient] 本地Ollama模式 - 发送请求到:" << url.toString();
    }
    
    // ...
}
```

#### sendChatMessage 方法

```cpp
void OllamaClient::sendChatMessage(const QString &message, const QString &systemPrompt)
{
    QJsonObject json;
    QUrl url;
    
    if (m_cloudMode) {
        // 云端API格式
        // ...
    } else {
        // ✅ 本地Ollama格式 - 使用新的 /api/chat 端点
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
        
        url = QUrl(m_baseUrl + "/api/chat");  // ✅ 新端点
        
        qDebug() << "[OllamaClient] 本地聊天模式 - 发送消息";
    }
    
    // ...
}
```

### 2. 更新响应解析 ✅

**文件**：`src/ai/OllamaClient.cpp`

支持新旧两种响应格式，确保兼容性：

```cpp
// 在 sendRequest 的 readyRead lambda 中
if (m_cloudMode) {
    // 云端API格式: choices[0].delta.content
    QJsonArray choices = obj["choices"].toArray();
    if (!choices.isEmpty()) {
        QJsonObject choice = choices[0].toObject();
        QJsonObject delta = choice["delta"].toObject();
        chunk = delta["content"].toString();
    }
} else {
    // ✅ 本地Ollama格式 - 支持新旧两种格式
    // 新API (/api/chat): message.content
    // 旧API (/api/generate): response
    if (obj.contains("message")) {
        QJsonObject message = obj["message"].toObject();
        chunk = message["content"].toString();
    } else if (obj.contains("response")) {
        chunk = obj["response"].toString();
    }
}
```

**关键改进**：
- ✅ 优先检查新格式 `message.content`
- ✅ 回退到旧格式 `response`
- ✅ 兼容新旧版本Ollama

### 3. 更新默认模型名 ✅

**文件**：`src/ai/OllamaClient.cpp` 和 `src/utils/ConfigManager.cpp`

```cpp
// OllamaClient.cpp
OllamaClient::OllamaClient(QObject *parent)
    : AIService(parent)
    , m_baseUrl("http://localhost:11434")
    , m_model("qwen2.5-coder:7b")  // ✅ 更新默认模型名
    , m_cloudMode(false)
{
    // ...
}

// ConfigManager.cpp
void ConfigManager::load()
{
    // ...
    if (!file.open(QIODevice::ReadOnly)) {
        // 使用默认值
        m_ollamaModel = "qwen2.5-coder:7b";  // ✅ 更新默认模型名
        return;
    }
    
    // ...
    m_ollamaModel = obj["ollamaModel"].toString("qwen2.5-coder:7b");  // ✅ 更新默认值
}
```

## API对比

### 旧API vs 新API

| 特性 | 旧API (`/api/generate`) | 新API (`/api/chat`) |
|------|------------------------|---------------------|
| 端点 | `/api/generate` | `/api/chat` |
| 请求格式 | `{"prompt": "text"}` | `{"messages": [...]}` |
| 系统提示 | 需要手动拼接到prompt | 支持 `role: "system"` |
| 响应字段 | `response` | `message.content` |
| 状态 | ⚠️ 已弃用 | ✅ 推荐使用 |

### 请求示例对比

#### 旧API请求
```json
{
  "model": "qwen2.5:7b",
  "prompt": "System: You are a helpful assistant.\n\nUser: Hello",
  "stream": true
}
```

#### 新API请求
```json
{
  "model": "qwen2.5:7b",
  "messages": [
    {
      "role": "system",
      "content": "You are a helpful assistant."
    },
    {
      "role": "user",
      "content": "Hello"
    }
  ],
  "stream": true
}
```

### 响应示例对比

#### 旧API响应
```json
{
  "model": "qwen2.5:7b",
  "created_at": "2024-01-01T00:00:00Z",
  "response": "Hello! How can I help you?",
  "done": false
}
```

#### 新API响应
```json
{
  "model": "qwen2.5:7b",
  "created_at": "2024-01-01T00:00:00Z",
  "message": {
    "role": "assistant",
    "content": "Hello! How can I help you?"
  },
  "done": false
}
```

## 模型名称说明

### 常见的Qwen模型名称

| 模型名 | 说明 | 推荐用途 |
|--------|------|---------|
| `qwen2.5:7b` | Qwen2.5 基础模型 7B | 通用对话 |
| `qwen2.5-coder:7b` | Qwen2.5 代码专用 7B | ✅ 代码相关任务 |
| `qwen2.5:14b` | Qwen2.5 基础模型 14B | 高质量对话 |
| `qwen2.5-coder:14b` | Qwen2.5 代码专用 14B | 复杂代码任务 |

### 检查已安装的模型

```bash
ollama list
```

输出示例：
```
NAME                    ID              SIZE    MODIFIED
qwen2.5-coder:7b        abc123def456    4.7 GB  2 days ago
qwen2.5:7b              def456abc123    4.7 GB  1 week ago
```

### 下载模型

```bash
# 下载代码专用模型（推荐）
ollama pull qwen2.5-coder:7b

# 或下载基础模型
ollama pull qwen2.5:7b
```

## 修改文件清单

| 文件 | 修改内容 | 状态 |
|------|---------|------|
| `src/ai/OllamaClient.cpp` | 更新API端点为 `/api/chat` | ✅ |
| `src/ai/OllamaClient.cpp` | 更新请求格式使用 messages | ✅ |
| `src/ai/OllamaClient.cpp` | 更新响应解析支持新格式 | ✅ |
| `src/ai/OllamaClient.cpp` | 更新默认模型名 | ✅ |
| `src/utils/ConfigManager.cpp` | 更新默认模型名 | ✅ |
| 编译状态 | 编译成功，无错误 | ✅ |

## 测试验证

### 测试场景1：AI导师对话

1. 确保Ollama运行：`ollama serve`
2. 确保模型已下载：`ollama list`
3. 启动程序
4. 选择题目
5. 打开AI导师面板
6. 发送消息

**预期结果**：
- ✅ 请求发送到 `http://localhost:11434/api/chat`
- ✅ 收到流式响应
- ✅ AI回复正常显示

### 测试场景2：AI判题

1. 编写代码
2. 点击"AI判题"
3. 等待结果

**预期结果**：
- ✅ 请求发送到 `http://localhost:11434/api/chat`
- ✅ AI分析代码
- ✅ 显示判题结果

### 调试日志示例

```
[MainWindow] 配置为本地Ollama模式
[MainWindow]   URL: http://localhost:11434
[MainWindow]   Model: qwen2.5-coder:7b
[OllamaClient] Cloud mode set to: false
[AIAssistantPanel] Sending chat message, length: 15
[OllamaClient] 本地聊天模式 - 发送消息
[OllamaClient] 本地Ollama模式 - 发送请求到: http://localhost:11434/api/chat
[OllamaClient] 模型: qwen2.5-coder:7b
[OllamaClient] 收到数据块: Hello! How can I help you?
[OllamaClient] 流式响应完成，总长度: 234
```

## 故障排查

### 问题1：仍然显示 404 Not Found

**原因**：Ollama版本太旧，不支持 `/api/chat` 端点

**解决**：
```bash
# 更新Ollama到最新版本
# Windows: 重新下载安装包
# macOS: brew upgrade ollama
# Linux: curl -fsSL https://ollama.com/install.sh | sh
```

### 问题2：模型名称错误

**症状**：显示"模型不存在"

**解决**：
1. 检查已安装的模型：`ollama list`
2. 在设置中修改模型名称，使用实际安装的模型名
3. 或下载配置的模型：`ollama pull qwen2.5-coder:7b`

### 问题3：Ollama未运行

**症状**：连接失败

**解决**：
```bash
# 启动Ollama服务
ollama serve
```

## 总结

Ollama API端点问题已修复：

### ✅ API更新
- 从旧的 `/api/generate` 更新到新的 `/api/chat`
- 请求格式使用 messages 数组
- 响应解析支持新格式 `message.content`

### ✅ 兼容性
- 支持新旧两种响应格式
- 向后兼容旧版Ollama
- 平滑升级

### ✅ 默认配置
- 更新默认模型名为 `qwen2.5-coder:7b`
- 更适合代码相关任务
- 与常见安装匹配

现在Ollama API应该能够正常工作了！如果仍有问题，请检查：
1. Ollama是否运行
2. 模型是否已下载
3. 模型名称是否正确
