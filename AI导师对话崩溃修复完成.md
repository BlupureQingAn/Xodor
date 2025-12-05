# AI导师对话崩溃修复完成

## 问题描述

用户反馈：AI导师一对话就说AI连接失败，然后应用崩溃。

## 问题诊断

### 崩溃原因分析

#### 1. 空指针访问 ⚠️

在 `onErrorOccurred` 方法中，当AI连接失败时：

```cpp
void AIAssistantPanel::onErrorOccurred(const QString &error)
{
    // 问题：没有检查 m_chatContainer 和 m_chatLayout 是否已初始化
    ChatBubbleWidget *bubble = new ChatBubbleWidget(errorMsg, false, m_chatContainer);
    m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);  // 可能崩溃
}
```

**崩溃场景**：
- AI连接失败时立即触发 `error` 信号
- `onErrorOccurred` 被调用
- 如果UI还没有完全初始化，`m_chatContainer` 或 `m_chatLayout` 可能为空
- 访问空指针导致崩溃

#### 2. 缺少异常处理

`sendChatMessage` 方法没有异常处理：

```cpp
void AIAssistantPanel::sendChatMessage(const QString &message)
{
    // 问题：没有 try-catch 保护
    m_aiClient->sendChatMessage(fullMessage, systemPrompt);  // 可能抛出异常
}
```

#### 3. 缺少调试日志

没有足够的日志输出，难以诊断问题。

## 解决方案

### 1. 加强空指针检查 ✅

**文件**：`src/ui/AIAssistantPanel.cpp`

```cpp
void AIAssistantPanel::onErrorOccurred(const QString &error)
{
    qWarning() << "[AIAssistantPanel] Error occurred:" << error;
    
    // ✅ 检查UI组件是否已初始化
    if (!m_chatContainer || !m_chatLayout) {
        qCritical() << "[AIAssistantPanel] Chat container or layout not initialized!";
        QMessageBox::critical(this, "AI连接错误", 
            QString("AI连接失败：%1\n\n请检查AI配置或网络连接。").arg(error));
        return;
    }
    
    // ✅ 如果正在接收消息，先结束当前消息
    if (m_isReceivingMessage) {
        m_isReceivingMessage = false;
        if (m_currentAssistantBubble) {
            // 更新当前气泡显示错误
            m_currentAssistantBubble->setContent(QString("❌ 错误: %1").arg(error));
            m_currentAssistantBubble = nullptr;
        } else {
            // 创建新的错误气泡
            QString errorMsg = QString("❌ 错误: %1").arg(error);
            ChatBubbleWidget *bubble = new ChatBubbleWidget(errorMsg, false, m_chatContainer);
            bubble->setFontScale(m_fontScale);
            m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
        }
    } else {
        // 创建错误消息气泡
        QString errorMsg = QString("❌ 错误: %1").arg(error);
        ChatBubbleWidget *bubble = new ChatBubbleWidget(errorMsg, false, m_chatContainer);
        bubble->setFontScale(m_fontScale);
        
        // 插入到布局中
        m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
    }
    
    scrollToBottom();
}
```

**关键改进**：
- ✅ 检查 `m_chatContainer` 和 `m_chatLayout` 是否为空
- ✅ 如果UI未初始化，显示错误对话框而不是崩溃
- ✅ 正确处理正在接收消息时的错误
- ✅ 添加调试日志

### 2. 添加异常处理 ✅

**文件**：`src/ui/AIAssistantPanel.cpp`

```cpp
void AIAssistantPanel::sendChatMessage(const QString &message)
{
    qDebug() << "[AIAssistantPanel] Sending chat message, length:" << message.length();
    
    if (!m_aiClient) {
        qCritical() << "[AIAssistantPanel] AI client is null!";
        onErrorOccurred("AI客户端未初始化");
        return;
    }
    
    if (!m_hasQuestion) {
        qWarning() << "[AIAssistantPanel] No question selected";
        onErrorOccurred("请先选择一道题目");
        return;
    }
    
    // ✅ 添加异常处理
    try {
        // 构建系统提示词
        QString systemPrompt = buildSystemPrompt();
        
        // 构建完整消息（包含题目上下文）
        QString fullMessage = QString("【当前题目】\n%1\n\n【题目描述】\n%2\n\n【学生的问题】\n%3")
            .arg(m_currentQuestion.title())
            .arg(m_currentQuestion.description())
            .arg(message);
        
        qDebug() << "[AIAssistantPanel] Full message length:" << fullMessage.length();
        qDebug() << "[AIAssistantPanel] Calling sendChatMessage...";
        
        // 发送消息
        m_aiClient->sendChatMessage(fullMessage, systemPrompt);
        
        qDebug() << "[AIAssistantPanel] Message sent successfully";
    } catch (const std::exception &e) {
        qCritical() << "[AIAssistantPanel] Exception in sendChatMessage:" << e.what();
        onErrorOccurred(QString("发送消息时发生错误：%1").arg(e.what()));
    } catch (...) {
        qCritical() << "[AIAssistantPanel] Unknown exception in sendChatMessage";
        onErrorOccurred("发送消息时发生未知错误");
    }
}
```

**关键改进**：
- ✅ 使用 try-catch 捕获异常
- ✅ 添加详细的调试日志
- ✅ 异常时调用 `onErrorOccurred` 显示友好错误信息

### 3. 错误处理流程

```
用户发送消息
    ↓
sendChatMessage()
    ↓
检查 m_aiClient 是否为空
    ├─ 是 → onErrorOccurred("AI客户端未初始化")
    └─ 否 → 继续
    ↓
检查是否选择了题目
    ├─ 否 → onErrorOccurred("请先选择一道题目")
    └─ 是 → 继续
    ↓
try {
    构建消息
    ↓
    m_aiClient->sendChatMessage()
    ↓
    成功
} catch (异常) {
    ↓
    onErrorOccurred(异常信息)
}
    ↓
onErrorOccurred(error)
    ↓
检查 m_chatContainer 和 m_chatLayout
    ├─ 为空 → 显示错误对话框
    └─ 不为空 → 创建错误气泡
    ↓
显示错误信息（不崩溃）
```

## 防崩溃机制

### 1. 空指针检查

在访问指针前检查是否为空：

```cpp
if (!m_chatContainer || !m_chatLayout) {
    // 显示错误对话框，不访问空指针
    QMessageBox::critical(this, "AI连接错误", error);
    return;
}
```

### 2. 异常捕获

使用 try-catch 捕获所有异常：

```cpp
try {
    // 可能抛出异常的代码
    m_aiClient->sendChatMessage(fullMessage, systemPrompt);
} catch (const std::exception &e) {
    // 处理标准异常
    onErrorOccurred(QString("错误：%1").arg(e.what()));
} catch (...) {
    // 处理未知异常
    onErrorOccurred("未知错误");
}
```

### 3. 状态检查

在执行操作前检查状态：

```cpp
if (!m_aiClient) {
    onErrorOccurred("AI客户端未初始化");
    return;
}

if (!m_hasQuestion) {
    onErrorOccurred("请先选择一道题目");
    return;
}
```

### 4. 调试日志

添加详细的调试日志，便于诊断问题：

```cpp
qDebug() << "[AIAssistantPanel] Sending chat message";
qWarning() << "[AIAssistantPanel] Error occurred:" << error;
qCritical() << "[AIAssistantPanel] AI client is null!";
```

## 可能的AI连接失败原因

### 1. Ollama未启动

**症状**：连接失败，提示"网络错误"

**解决**：
- 启动Ollama服务
- 检查Ollama是否在运行：`ollama list`

### 2. 模型未下载

**症状**：连接成功但无响应

**解决**：
- 下载模型：`ollama pull qwen2.5-coder:7b`
- 检查模型列表：`ollama list`

### 3. 端口被占用

**症状**：连接失败，提示"端口错误"

**解决**：
- 检查端口11434是否被占用
- 修改Ollama端口配置

### 4. 云端API配置错误

**症状**：云端模式连接失败

**解决**：
- 检查API Key是否正确
- 检查API URL是否正确
- 检查网络连接

## 测试验证

### 测试场景1：AI连接失败（UI已初始化）

1. 启动程序
2. 选择题目
3. 打开AI导师面板
4. 停止Ollama服务
5. 发送消息

**预期结果**：
- ✅ 显示错误气泡："❌ 错误: 网络错误..."
- ✅ 不崩溃
- ✅ 可以继续使用其他功能

### 测试场景2：AI连接失败（UI未初始化）

1. 启动程序
2. 立即打开AI导师面板
3. 在UI完全加载前发送消息

**预期结果**：
- ✅ 显示错误对话框
- ✅ 不崩溃
- ✅ 提示检查AI配置

### 测试场景3：正常对话

1. 启动程序
2. 确保Ollama运行
3. 选择题目
4. 打开AI导师面板
5. 发送消息

**预期结果**：
- ✅ 显示用户消息气泡
- ✅ 显示AI回复气泡
- ✅ 流式输出正常
- ✅ 对话历史保存

### 调试日志示例

```
[AIAssistantPanel] Sending chat message, length: 15
[AIAssistantPanel] Full message length: 234
[AIAssistantPanel] Calling sendChatMessage...
[OllamaClient] 本地Ollama模式 - 发送请求到: http://localhost:11434/api/chat
[OllamaClient] 网络错误发生: 7 Connection refused
[AIAssistantPanel] Error occurred: 网络错误: Connection refused
[AIAssistantPanel] Chat container and layout are initialized
```

## 修改文件清单

| 文件 | 修改内容 | 状态 |
|------|---------|------|
| `src/ui/AIAssistantPanel.cpp` | 加强空指针检查 | ✅ |
| `src/ui/AIAssistantPanel.cpp` | 添加异常处理 | ✅ |
| `src/ui/AIAssistantPanel.cpp` | 添加调试日志 | ✅ |
| 编译状态 | 编译成功，无错误 | ✅ |

## 总结

AI导师对话崩溃问题已修复：

### ✅ 防崩溃机制
- 空指针检查：访问指针前检查是否为空
- 异常捕获：使用try-catch捕获所有异常
- 状态检查：执行操作前检查状态
- 友好错误：显示友好的错误信息而不是崩溃

### ✅ 错误处理
- UI未初始化时显示错误对话框
- 正在接收消息时正确处理错误
- 显示详细的错误信息
- 添加调试日志便于诊断

### ✅ 用户体验
- 即使AI连接失败也不会崩溃
- 显示友好的错误提示
- 可以继续使用其他功能
- 提供解决建议

现在AI导师对话功能应该能够稳定运行，即使遇到AI连接失败也不会崩溃！
