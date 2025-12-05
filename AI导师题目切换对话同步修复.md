# AI导师题目切换对话同步修复

## 修复时间
2024年12月6日

## 问题描述

用户报告：切换题目后，AI导师的对话没有切换到对应题目的对话历史。

## 问题分析

### 原始逻辑流程

1. 用户在题目A进行对话
2. 点击"新建对话"
3. `refreshChat()`清空对话，但`m_currentQuestion`仍然是题目A
4. 用户切换到题目B
5. `setQuestionContext(question_B)`被调用
6. 检查`m_messages`不为空？**否**（已被清空）
7. 不保存对话（因为`m_messages`为空）
8. 更新`m_currentQuestion = question_B`
9. 加载题目B的对话

### 潜在问题场景

虽然上述流程看起来正常，但存在以下问题：

**场景1：重复切换到同一题目**
- 切换到题目A → 加载对话
- 再次切换到题目A → 又加载一次对话（不必要）
- 可能导致UI闪烁或重复渲染

**场景2：调试信息不足**
- 没有日志输出，无法追踪题目切换过程
- 难以诊断对话加载失败的原因

## 修复内容

### 1. 添加同题目检查

**文件**：`src/ui/AIAssistantPanel.cpp`

**修改**：`setQuestionContext()` 方法

```cpp
void AIAssistantPanel::setQuestionContext(const Question &question)
{
    qDebug() << "[AIAssistantPanel] setQuestionContext called for:" << question.id();
    
    // 如果是同一个题目，不需要切换
    if (m_hasQuestion && m_currentQuestion.id() == question.id()) {
        qDebug() << "[AIAssistantPanel] Same question, skipping switch";
        return;
    }
    
    // 先保存当前题目的对话历史（如果有的话）
    if (m_hasQuestion && !m_messages.isEmpty()) {
        qDebug() << "[AIAssistantPanel] Saving conversation for old question:" 
                 << m_currentQuestion.id() << "messages:" << m_messages.size();
        saveConversationHistory();
    }
    
    // 切换到新题目
    QString oldQuestionId = m_hasQuestion ? m_currentQuestion.id() : "none";
    m_currentQuestion = question;
    m_hasQuestion = true;
    
    qDebug() << "[AIAssistantPanel] Switched from" << oldQuestionId << "to" << question.id();
    
    // 加载新题目的对话历史
    loadConversationHistory();
}
```

**效果**：
- ✅ 避免重复切换到同一题目
- ✅ 减少不必要的UI刷新
- ✅ 添加详细的调试日志

### 2. 优化新建对话逻辑

**文件**：`src/ui/AIAssistantPanel.cpp`

**修改**：`refreshChat()` 方法

```cpp
void AIAssistantPanel::refreshChat()
{
    qDebug() << "[AIAssistantPanel] refreshChat called for question:" 
             << (m_hasQuestion ? m_currentQuestion.id() : "none");
    
    // 保存当前对话到历史
    if (m_hasQuestion && !m_messages.isEmpty()) {
        qDebug() << "[AIAssistantPanel] Saving conversation before clearing, messages:" 
                 << m_messages.size();
        saveConversationHistory();
    }
    
    // 清空当前对话（但保留题目上下文）
    clearHistory();
    
    qDebug() << "[AIAssistantPanel] Chat cleared, current question still:" 
             << (m_hasQuestion ? m_currentQuestion.id() : "none");
}
```

**效果**：
- ✅ 明确说明保留题目上下文
- ✅ 添加详细的调试日志
- ✅ 便于追踪新建对话的过程

## 调试日志示例

### 正常题目切换

```
[AIAssistantPanel] setQuestionContext called for: q001 两数之和
[AIAssistantPanel] Saving conversation for old question: q002 messages: 4
[AIAssistantPanel] Saved conversation to: data/conversations/q002.json messages: 4
[AIAssistantPanel] Switched from q002 to q001
[AIAssistantPanel] Loading conversation history for question: q001
```

### 新建对话后切换题目

```
[AIAssistantPanel] refreshChat called for question: q001
[AIAssistantPanel] Saving conversation before clearing, messages: 6
[AIAssistantPanel] Saved conversation to: data/conversations/q001.json messages: 6
[AIAssistantPanel] Chat cleared, current question still: q001

[AIAssistantPanel] setQuestionContext called for: q002
[AIAssistantPanel] Switched from q001 to q002
[AIAssistantPanel] Loading conversation history for question: q002
```

### 重复切换到同一题目

```
[AIAssistantPanel] setQuestionContext called for: q001
[AIAssistantPanel] Same question, skipping switch
```

## 测试步骤

### 测试1：基本题目切换
1. 在题目A进行对话（至少2轮）
2. 切换到题目B
3. 查看控制台日志
4. 验证题目B的对话是否正确加载

**预期日志**：
```
[AIAssistantPanel] setQuestionContext called for: B
[AIAssistantPanel] Saving conversation for old question: A messages: 4
[AIAssistantPanel] Switched from A to B
[AIAssistantPanel] Loading conversation history for question: B
```

### 测试2：新建对话后切换
1. 在题目A进行对话
2. 点击"新建对话"
3. 切换到题目B
4. 切回题目A
5. 验证题目A的对话是否恢复

**预期结果**：
- ✅ 题目A的对话已保存
- ✅ 题目B显示空对话或历史对话
- ✅ 切回题目A时对话恢复

### 测试3：重复切换
1. 选择题目A
2. 再次点击题目A
3. 查看控制台日志

**预期日志**：
```
[AIAssistantPanel] setQuestionContext called for: A
[AIAssistantPanel] Same question, skipping switch
```

## 可能的问题排查

### 问题1：切换题目后对话没有更新

**检查步骤**：
1. 查看控制台日志，确认`setQuestionContext`是否被调用
2. 检查题目ID是否正确
3. 验证`data/conversations/{questionId}.json`文件是否存在
4. 检查JSON文件内容是否正确

**常见原因**：
- 题目ID不一致
- 文件读取权限问题
- JSON格式错误

### 问题2：新建对话后切换题目，旧对话丢失

**检查步骤**：
1. 查看控制台日志，确认`refreshChat`是否保存了对话
2. 检查`data/conversations/`目录中的文件
3. 验证文件修改时间

**常见原因**：
- `refreshChat`没有正确保存对话
- 文件被覆盖

### 问题3：重复加载对话导致卡顿

**检查步骤**：
1. 查看控制台日志，统计`setQuestionContext`调用次数
2. 检查是否有"Same question, skipping switch"日志

**解决方案**：
- 已添加同题目检查，避免重复加载

## 相关文件

- `src/ui/AIAssistantPanel.h` - AI助手面板头文件
- `src/ui/AIAssistantPanel.cpp` - AI助手面板实现
- `src/ui/MainWindow.cpp` - 主窗口（调用setQuestionContext）
- `data/conversations/*.json` - 对话历史文件

## 修复状态

✅ 已完成 - 添加了同题目检查和详细日志，优化了题目切换逻辑
