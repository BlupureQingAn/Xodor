# AI判题回复隐藏修复完成

## 问题描述

用户反馈：AI判题的回复不应该显示在AI导师对话框中，应该隐藏。

## 问题诊断

### 问题根源

AI判题和AI导师都使用同一个 `OllamaClient` 实例，并且都监听流式输出信号：

```cpp
// AI导师面板连接流式输出信号
connect(m_aiClient, &OllamaClient::streamingChunk,
        this, &AIAssistantPanel::onStreamingChunk);
connect(m_aiClient, &OllamaClient::streamingFinished,
        this, &AIAssistantPanel::onStreamingFinished);
```

当AI判题发送请求时：
```cpp
// AIJudge.cpp
m_aiClient->sendCustomPrompt(prompt, "custom");
```

这会触发流式输出信号，导致AI判题的响应显示在AI导师对话框中。

### 信号流程

```
AI判题请求
    ↓
OllamaClient::sendCustomPrompt(prompt, "custom")
    ↓
OllamaClient::sendRequest(prompt, "custom")
    ↓
流式响应到达
    ↓
emit streamingChunk(chunk)  ← 所有监听者都会收到
    ↓
AIAssistantPanel::onStreamingChunk()  ← AI导师面板显示
    ↓
用户看到AI判题的响应（不期望）
```

## 解决方案

### 1. 使用专用Context标识 ✅

为AI判题使用专用的context标识 `"ai_judge"`，区别于其他请求：

**文件**：`src/ai/AIJudge.cpp`

```cpp
void AIJudge::judgeCode(const Question &question, const QString &code)
{
    // ...
    
    qDebug() << "[AIJudge] Sending prompt to AI client...";
    // ✅ 使用特殊的context "ai_judge"，避免触发AI导师面板的流式输出
    m_aiClient->sendCustomPrompt(prompt, "ai_judge");
}
```

### 2. 条件性发送流式信号 ✅

在 `OllamaClient` 中，只为非AI判题的请求发送流式信号：

**文件**：`src/ai/OllamaClient.cpp`

#### sendChatMessage 方法中的lambda

```cpp
// 连接readyRead信号以处理流式数据
connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
    // ✅ 从reply属性获取context
    QString context = reply->property("context").toString();
    QString fullResponse = reply->property("fullResponse").toString();
    
    // 读取新数据
    QByteArray newData = reply->readAll();
    
    // 流式响应是多个JSON对象，每行一个
    QList<QByteArray> lines = newData.split('\n');
    
    for (const QByteArray &line : lines) {
        // ... 解析JSON ...
        
        if (!chunk.isEmpty()) {
            fullResponse += chunk;
            
            // ✅ 只为非AI判题的请求发送流式数据块信号
            // AI判题使用 "ai_judge" context，不应该显示在AI导师面板
            if (context != "ai_judge") {
                emit streamingChunk(chunk);
            }
        }
        
        // 检查是否完成
        bool done = obj["done"].toBool();
        if (done) {
            // ✅ 只为非AI判题的请求发送完成信号
            if (context != "ai_judge") {
                emit streamingFinished();
            }
        }
    }
    
    // 更新存储的完整响应
    reply->setProperty("fullResponse", fullResponse);
});
```

### 3. 确保AI判题仍能收到响应 ✅

在 `handleNetworkReply` 中，确保 `"ai_judge"` context也能触发 `codeAnalysisReady` 信号：

**文件**：`src/ai/OllamaClient.cpp`

```cpp
void OllamaClient::handleNetworkReply(QNetworkReply *reply)
{
    QString context = reply->property("context").toString();
    QString response = reply->property("fullResponse").toString();
    
    // ...
    
    // ✅ 添加 "ai_judge" 到触发条件
    if (context == "code_analysis" || context == "custom" || 
        context == "question_parse" || context == "ai_judge") {
        qDebug() << "[OllamaClient] 发送 codeAnalysisReady 信号 (context:" << context << ")";
        emit codeAnalysisReady(response);
    } else if (context == "generate_questions") {
        emit questionsGenerated(QJsonArray());
    } else if (context == "parse_bank") {
        emit questionBankParsed(QJsonArray());
    }
    
    reply->deleteLater();
}
```

## 修复后的信号流程

### AI判题流程（不显示在AI导师面板）

```
AI判题请求
    ↓
OllamaClient::sendCustomPrompt(prompt, "ai_judge")
    ↓
OllamaClient::sendRequest(prompt, "ai_judge")
    ↓
流式响应到达
    ↓
检查 context != "ai_judge"
    ↓
不发送 streamingChunk 信号  ← AI导师面板不会收到
    ↓
响应完成后
    ↓
emit codeAnalysisReady(response)  ← AIJudge收到完整响应
    ↓
AIJudge::onAIResponse()
    ↓
解析判题结果
    ↓
显示判题结果对话框（不在AI导师面板）
```

### AI导师对话流程（正常显示）

```
用户在AI导师面板发送消息
    ↓
OllamaClient::sendChatMessage(message, systemPrompt)
    ↓
context = "chat"
    ↓
流式响应到达
    ↓
检查 context != "ai_judge"  ← 通过检查
    ↓
emit streamingChunk(chunk)  ← AI导师面板收到
    ↓
AIAssistantPanel::onStreamingChunk()
    ↓
实时显示AI回复（正常）
```

## Context标识说明

| Context | 用途 | 流式输出 | 完成信号 |
|---------|------|---------|---------|
| `"chat"` | AI导师对话 | ✅ 发送 | ✅ 发送 |
| `"code_analysis"` | 代码分析 | ✅ 发送 | ✅ 发送 |
| `"custom"` | 自定义请求 | ✅ 发送 | ✅ 发送 |
| `"ai_judge"` | AI判题 | ❌ 不发送 | ❌ 不发送 |
| `"question_parse"` | 题目解析 | ✅ 发送 | ✅ 发送 |
| `"generate_questions"` | 生成题目 | ✅ 发送 | ✅ 发送 |

## 测试验证

### 测试场景1：AI判题不显示在AI导师面板

1. 打开AI导师面板
2. 选择一道题目
3. 编写代码
4. 点击"AI判题"
5. 观察AI导师面板

**预期结果**：
- ✅ AI导师面板不显示AI判题的响应
- ✅ AI判题结果显示在独立的对话框中
- ✅ AI导师面板保持原有内容不变

### 测试场景2：AI导师对话正常工作

1. 打开AI导师面板
2. 发送一条消息给AI导师
3. 观察响应

**预期结果**：
- ✅ AI导师面板实时显示AI回复
- ✅ 流式输出正常工作
- ✅ 对话历史正常保存

### 测试场景3：AI判题功能正常

1. 选择一道题目
2. 编写代码
3. 点击"AI判题"
4. 等待判题结果

**预期结果**：
- ✅ 显示进度对话框
- ✅ AI分析代码
- ✅ 显示判题结果对话框
- ✅ 题目状态正确更新

## 修改文件清单

| 文件 | 修改内容 | 状态 |
|------|---------|------|
| `src/ai/AIJudge.cpp` | 使用 "ai_judge" context | ✅ |
| `src/ai/OllamaClient.cpp` | 条件性发送流式信号 | ✅ |
| `src/ai/OllamaClient.cpp` | 添加 "ai_judge" 到codeAnalysisReady触发条件 | ✅ |
| `src/ai/OllamaClient.cpp` | 在sendChatMessage的lambda中获取context | ✅ |
| 编译状态 | 编译成功，无错误 | ✅ |

## 技术细节

### Lambda捕获问题

**问题**：Lambda函数中无法访问外部的context变量

**原因**：context是函数参数，不在lambda的捕获列表中

**解决**：从reply的property中获取context

```cpp
// ❌ 错误：context未捕获
connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
    // context 不可访问
    if (context != "ai_judge") {  // 编译错误
        emit streamingChunk(chunk);
    }
});

// ✅ 正确：从reply属性获取
connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
    QString context = reply->property("context").toString();
    if (context != "ai_judge") {  // 正确
        emit streamingChunk(chunk);
    }
});
```

### 信号选择性发送

通过检查context，可以选择性地发送信号：

```cpp
// 只为特定context发送信号
if (context != "ai_judge") {
    emit streamingChunk(chunk);
}

// 或者只为特定context发送
if (context == "chat" || context == "code_analysis") {
    emit streamingChunk(chunk);
}
```

## 总结

AI判题回复隐藏功能已完成：

### ✅ 问题修复
- AI判题使用专用context `"ai_judge"`
- 流式输出信号不会发送给AI导师面板
- AI判题结果只显示在独立对话框中

### ✅ 功能保持
- AI导师对话正常工作
- 流式输出正常显示
- AI判题功能正常工作

### ✅ 代码质量
- 使用context区分不同类型的请求
- 条件性发送信号，避免干扰
- 编译成功，无错误

现在AI判题的响应不会显示在AI导师对话框中了！
