# AI导师对话切换诊断指南

## 问题描述
用户报告：切换题目后，AI导师的对话没有切换到对应题目的对话。

## 诊断步骤

### 步骤1：检查是否调用了setQuestionContext

**操作**：切换题目，查看控制台输出

**预期日志**：
```
[AIAssistantPanel] setQuestionContext called for: q002 题目标题
[AIAssistantPanel] Saving conversation for old question: q001 messages: 4
[AIAssistantPanel] Saved conversation to: data/conversations/q001.json messages: 4
[AIAssistantPanel] Switched from q001 to q002
[AIAssistantPanel] Loading conversation history for question: q002
```

**如果没有日志**：
- 问题：`setQuestionContext`没有被调用
- 原因：MainWindow的题目切换逻辑有问题
- 检查：`loadCurrentQuestion()`是否被调用

**如果有"Same question, skipping switch"日志**：
- 问题：题目ID相同，被跳过了
- 原因：可能是重复点击同一题目
- 解决：这是正常行为，不需要修复

### 步骤2：检查是否清空了旧对话

**预期日志**：
```
[AIAssistantPanel] clearHistory called, clearing 4 messages
[AIAssistantPanel] Cleared 4 widgets from layout
```

**如果没有清空**：
- 问题：`clearHistory()`没有被调用
- 原因：`loadConversationHistory()`逻辑有问题

### 步骤3：检查是否加载了新对话

**预期日志**：
```
[AIAssistantPanel] Found 6 messages in history
[AIAssistantPanel] Loaded message: user content length: 15
[AIAssistantPanel] Loaded message: assistant content length: 234
[AIAssistantPanel] Loaded message: user content length: 20
[AIAssistantPanel] Loaded message: assistant content length: 456
...
[AIAssistantPanel] Conversation loaded successfully, total messages: 6
```

**如果没有找到消息**：
```
[AIAssistantPanel] No conversation history found for question: q002
[AIAssistantPanel] clearHistory called, clearing 0 messages
```
- 这是正常的，说明该题目没有对话历史

**如果找到了消息但UI没有显示**：
- 问题：气泡创建或布局有问题
- 检查：`m_chatLayout`和`m_chatContainer`是否正常

### 步骤4：检查文件是否存在

**操作**：
1. 在题目A进行对话
2. 切换到题目B
3. 检查`data/conversations/`目录

**预期**：
- 应该有`{questionId}.json`文件
- 文件内容应该是有效的JSON

**检查命令**（Windows）：
```cmd
dir data\conversations
type data\conversations\q001.json
```

### 步骤5：检查题目ID

**操作**：查看控制台日志中的题目ID

**常见问题**：
- 题目ID为空或null
- 题目ID包含特殊字符（导致文件名无效）
- 题目ID不一致（保存和加载使用了不同的ID）

## 常见问题和解决方案

### 问题1：切换题目后对话没有变化

**症状**：
- 切换到题目B，但显示的还是题目A的对话
- 或者显示空白

**可能原因**：
1. **题目ID相同**：两个题目使用了相同的ID
   - 检查：查看日志中的题目ID
   - 解决：修改题目JSON文件，确保ID唯一

2. **UI没有刷新**：气泡创建了但没有显示
   - 检查：查看日志是否有"Loaded message"
   - 解决：强制刷新UI（已在代码中添加）

3. **文件读取失败**：对话文件损坏或权限问题
   - 检查：手动打开JSON文件
   - 解决：删除损坏的文件，重新生成

### 问题2：新建对话后切换题目，旧对话丢失

**症状**：
- 在题目A点击"新建对话"
- 切换到题目B再切回题目A
- 题目A的对话消失了

**原因**：
- `refreshChat()`保存了空对话，覆盖了原有对话

**解决方案**：
- 已修复：`refreshChat()`会先保存当前对话再清空
- 验证：查看日志中的"Saving conversation before clearing"

### 问题3：对话内容显示乱码

**症状**：
- 对话加载成功，但中文显示为乱码

**原因**：
- JSON文件编码不是UTF-8

**解决方案**：
- 已修复：使用`QIODevice::Text`标志
- 验证：用文本编辑器打开JSON文件，检查编码

## 完整的日志示例

### 正常的题目切换

```
[MainWindow] Loading question: q002 两数之和
[AIAssistantPanel] setQuestionContext called for: q002 两数之和
[AIAssistantPanel] Saving conversation for old question: q001 messages: 4
[AIAssistantPanel] Saved conversation to: data/conversations/q001.json messages: 4
[AIAssistantPanel] Switched from q001 to q002
[AIAssistantPanel] Loading conversation history for question: q002
[AIAssistantPanel] Found 6 messages in history
[AIAssistantPanel] Loaded message: user content length: 15
[AIAssistantPanel] Loaded message: assistant content length: 234
[AIAssistantPanel] Loaded message: user content length: 20
[AIAssistantPanel] Loaded message: assistant content length: 456
[AIAssistantPanel] Loaded message: user content length: 18
[AIAssistantPanel] Loaded message: assistant content length: 567
[AIAssistantPanel] Conversation loaded successfully, total messages: 6
[AIAssistantPanel] clearHistory called, clearing 0 messages
[AIAssistantPanel] Cleared 0 widgets from layout
```

### 切换到没有对话历史的题目

```
[AIAssistantPanel] setQuestionContext called for: q003 三数之和
[AIAssistantPanel] Saving conversation for old question: q002 messages: 6
[AIAssistantPanel] Saved conversation to: data/conversations/q002.json messages: 6
[AIAssistantPanel] Switched from q002 to q003
[AIAssistantPanel] Loading conversation history for question: q003
[AIAssistantPanel] No conversation history found for question: q003
[AIAssistantPanel] clearHistory called, clearing 0 messages
[AIAssistantPanel] Cleared 0 widgets from layout
```

### 重复切换到同一题目

```
[AIAssistantPanel] setQuestionContext called for: q002 两数之和
[AIAssistantPanel] Same question, skipping switch
```

## 如何报告问题

如果问题仍然存在，请提供以下信息：

1. **完整的控制台日志**（从切换题目开始）
2. **操作步骤**：
   - 在哪个题目进行了对话
   - 切换到哪个题目
   - 预期看到什么
   - 实际看到什么
3. **文件检查**：
   - `data/conversations/`目录中有哪些文件
   - 相关JSON文件的内容（如果不大的话）
4. **题目信息**：
   - 题目ID是什么
   - 题目标题是什么

## 临时解决方案

如果对话切换有问题，可以尝试：

1. **手动刷新**：
   - 点击"新建对话"
   - 再点击"📜"历史记录
   - 从历史记录中恢复对话

2. **重启程序**：
   - 关闭程序
   - 重新打开
   - 程序会自动恢复上次的题目和对话

3. **清空缓存**：
   - 备份`data/conversations/`目录
   - 删除有问题的JSON文件
   - 重新进行对话
