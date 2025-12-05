# AI导师并发消息崩溃最终修复

## 问题描述
在AI输出过程中发送新消息或点击快捷按钮（分析代码、思路、知识点）会导致程序崩溃。

## 根本原因
当AI正在输出时（`m_isReceivingMessage = true`），如果用户触发新的操作：
1. 会创建新的消息气泡
2. 但旧的`m_currentAssistantBubble`指针还在使用中
3. 导致指针混乱和内存访问错误

## 解决方案
**统一策略**：在所有可能触发新消息的操作中，如果检测到正在接收AI消息，先调用`onStopGeneration()`终止当前输出。

这相当于用户先点击"终止"按钮，然后再执行新操作，逻辑清晰且安全。

## 修改内容

### 1. `onSendMessage()` - 发送消息
```cpp
void AIAssistantPanel::onSendMessage()
{
    QString message = m_inputField->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
    }
    
    // 清空输入框
    m_inputField->clear();
    
    // 显示用户消息
    appendUserMessage(message);
    
    // 发送到AI
    sendChatMessage(message);
}
```

### 2. `onAnalyzeCode()` - 分析代码按钮
```cpp
void AIAssistantPanel::onAnalyzeCode()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
    }
    
    // 发出信号请求更新当前代码
    emit requestCurrentCode();
    
    // 稍微延迟一下，确保代码已更新
    QTimer::singleShot(50, this, [this]() {
        if (m_currentCode.isEmpty()) {
            QMessageBox::warning(this, "提示", "代码编辑器为空，请先编写代码");
            return;
        }
        
        // 模拟用户点击"分析代码"
        QString message = "请帮我分析一下代码";
        appendUserMessage(message);
        
        // 构建包含代码的消息
        QString fullMessage = QString("我的代码如下：\n```cpp\n%1\n```\n\n请帮我分析一下").arg(m_currentCode);
        sendChatMessage(fullMessage);
    });
}
```

### 3. `onGetHint()` - 思路按钮
```cpp
void AIAssistantPanel::onGetHint()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
    }
    
    QString message = "我不知道怎么做，能给我一些思路吗？";
    appendUserMessage(message);
    sendChatMessage(message);
}
```

### 4. `onExplainConcept()` - 知识点按钮
```cpp
void AIAssistantPanel::onExplainConcept()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
    }
    
    QString message = "这道题涉及哪些知识点？能讲解一下吗？";
    appendUserMessage(message);
    sendChatMessage(message);
}
```

## `onStopGeneration()` 的工作原理
```cpp
void AIAssistantPanel::onStopGeneration()
{
    if (!m_aiClient) {
        return;
    }
    
    qDebug() << "[AIAssistantPanel] 用户请求终止输出";
    
    // 1. 终止AI客户端的当前请求
    m_aiClient->abortCurrentRequest();
    
    // 2. 如果正在接收消息，添加终止标记并完成消息
    if (m_isReceivingMessage && m_currentAssistantBubble) {
        m_currentAssistantMessage += "\n\n⏹ **输出已终止**";
        m_currentAssistantBubble->setContent(m_currentAssistantMessage);
        finishAssistantMessage();
        // finishAssistantMessage() 会恢复按钮状态
    } else {
        // 3. 如果没有正在接收的消息，手动恢复按钮状态
        m_stopButton->setVisible(false);
        m_sendButton->setVisible(true);
    }
}
```

## 优势
1. **逻辑清晰**：相当于用户先点击终止，再执行新操作
2. **代码简洁**：不需要复杂的状态管理
3. **用户友好**：保留已输出的内容，添加"输出已终止"标记
4. **安全可靠**：避免指针混乱和内存错误

## 测试场景
1. ✅ AI输出时发送新消息 → 终止当前输出，发送新消息
2. ✅ AI输出时点击"分析代码" → 终止当前输出，分析代码
3. ✅ AI输出时点击"思路" → 终止当前输出，获取思路
4. ✅ AI输出时点击"知识点" → 终止当前输出，讲解知识点
5. ✅ 手动点击"终止"按钮 → 正常终止，保留已输出内容

## 修改文件
- `src/ui/AIAssistantPanel.cpp`
  - `onSendMessage()` - 添加终止检查
  - `onAnalyzeCode()` - 添加终止检查
  - `onGetHint()` - 修改为调用`onStopGeneration()`
  - `onExplainConcept()` - 修改为调用`onStopGeneration()`

## 完成时间
2024-12-06
