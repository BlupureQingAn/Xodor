# AI对话历史题目过滤修复

## 问题描述
在查看AI对话历史时，会显示所有题目的对话记录，而不是只显示当前题目的对话。这可能导致：
1. 用户误加载其他题目的对话
2. 对话列表混乱，难以找到想要的记录
3. 不符合用户预期（应该只看当前题目的历史）

## 解决方案

### 1. 添加题目ID过滤
**文件**: `src/ui/ChatHistoryDialog.h`, `src/ui/ChatHistoryDialog.cpp`

在 `ChatHistoryDialog` 中添加当前题目ID字段和设置方法：

```cpp
class ChatHistoryDialog : public QDialog
{
    // ...
public:
    // 设置当前题目ID，只显示该题目的对话历史
    void setCurrentQuestionId(const QString &questionId);
    
private:
    QString m_currentQuestionId;  // 当前题目ID，用于过滤
    // ...
};
```

实现过滤逻辑：
```cpp
void ChatHistoryDialog::setCurrentQuestionId(const QString &questionId)
{
    m_currentQuestionId = questionId;
    qDebug() << "[ChatHistoryDialog] Set current question filter:" << questionId;
    
    // 重新加载对话列表（只显示当前题目的对话）
    loadConversationList();
}
```

### 2. 在加载时应用过滤
**文件**: `src/ui/ChatHistoryDialog.cpp`

在 `loadConversationList()` 中添加过滤逻辑：

```cpp
for (const QFileInfo &fileInfo : files) {
    // ... 读取文件和解析 ...
    
    ConversationInfo info;
    info.questionId = obj["questionId"].toString();
    // ...
    
    // 如果设置了当前题目ID，只显示该题目的对话
    if (!m_currentQuestionId.isEmpty() && info.questionId != m_currentQuestionId) {
        qDebug() << "[ChatHistoryDialog] Skipping conversation for different question:" << info.questionId;
        continue;  // 跳过其他题目的对话
    }
    
    m_conversations.append(info);
    // ... 添加到列表 ...
}
```

### 3. 在AI助手面板中设置过滤
**文件**: `src/ui/AIAssistantPanel.cpp`

修改 `viewHistory()` 方法，传入当前题目ID：

```cpp
void AIAssistantPanel::viewHistory()
{
    // 检查是否有当前题目
    if (!m_hasQuestion) {
        QMessageBox::information(this, "提示", "请先选择一道题目");
        return;
    }
    
    ChatHistoryDialog dialog(this);
    
    // 设置当前题目ID，只显示当前题目的对话历史
    dialog.setCurrentQuestionId(m_currentQuestion.id());
    
    // ... 连接信号 ...
    
    dialog.exec();
}
```

## 修改的文件
1. `src/ui/ChatHistoryDialog.h` - 添加题目ID过滤字段和方法声明
2. `src/ui/ChatHistoryDialog.cpp` - 实现过滤逻辑
3. `src/ui/AIAssistantPanel.cpp` - 在打开历史对话时设置当前题目ID

## 功能特点

### 1. 智能过滤
- 只显示当前题目的对话历史
- 自动跳过其他题目的对话记录
- 保持对话列表的整洁和相关性

### 2. 用户友好
- 如果没有选择题目，提示用户先选择题目
- 清晰的调试日志，便于追踪过滤过程
- 不影响原有的加载和删除功能

### 3. 数据安全
- 不会误加载其他题目的对话
- 删除操作仍然只影响当前显示的对话
- 保持数据的完整性和一致性

## 使用场景

### 场景1：查看当前题目的历史对话
1. 用户正在做题目A
2. 点击"📜 历史"按钮
3. 只显示题目A的历史对话记录
4. 不会看到题目B、C等其他题目的对话

### 场景2：切换题目后查看历史
1. 用户从题目A切换到题目B
2. 点击"📜 历史"按钮
3. 只显示题目B的历史对话记录
4. 题目A的对话不会出现在列表中

### 场景3：没有选择题目
1. 用户刚打开程序，还没选择题目
2. 点击"📜 历史"按钮
3. 显示提示："请先选择一道题目"
4. 防止显示所有题目的混乱对话列表

## 技术细节

### 过滤逻辑
```cpp
// 伪代码
for each conversation file:
    load conversation info
    if currentQuestionId is set:
        if conversation.questionId != currentQuestionId:
            skip this conversation
    add to list
```

### 数据流程
1. 用户点击"📜 历史" → `viewHistory()`
2. 检查是否有当前题目 → `m_hasQuestion`
3. 创建对话框 → `ChatHistoryDialog`
4. 设置题目过滤 → `setCurrentQuestionId(m_currentQuestion.id())`
5. 加载对话列表 → `loadConversationList()`
6. 应用过滤 → 只添加匹配的对话
7. 显示对话框 → `dialog.exec()`

### 调试输出
```
[ChatHistoryDialog] Set current question filter: question_001
[ChatHistoryDialog] Skipping conversation for different question: question_002
[ChatHistoryDialog] Skipping conversation for different question: question_003
```

## 测试步骤
1. 选择题目A，与AI对话几轮
2. 切换到题目B，与AI对话几轮
3. 在题目B中点击"📜 历史"
4. 验证只显示题目B的对话记录
5. 切换回题目A，点击"📜 历史"
6. 验证只显示题目A的对话记录
7. 关闭所有题目，点击"📜 历史"
8. 验证显示提示"请先选择一道题目"

## 预期结果
- ✅ 历史对话列表只显示当前题目的记录
- ✅ 不会显示其他题目的对话
- ✅ 没有选择题目时给出友好提示
- ✅ 加载和删除功能正常工作
- ✅ 调试日志清晰，便于追踪

## 编译状态
✅ 编译成功，无错误无警告

## 用户体验改进
- **更清晰**：只看到相关的对话记录
- **更安全**：不会误加载其他题目的对话
- **更高效**：快速找到想要的历史记录
- **更友好**：没有题目时给出明确提示

## 后续优化建议
1. 添加"查看所有对话"选项（高级功能）
2. 支持按日期范围筛选对话
3. 添加对话搜索功能
4. 支持导出当前题目的所有对话
5. 添加对话统计信息（总消息数、总对话数等）
