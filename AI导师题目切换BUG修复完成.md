# AI导师题目切换BUG修复完成

## 修复时间
2024年12月6日

## 问题描述

用户报告：切换题目后，AI导师的对话没有切换到对应题目的对话历史。

## 问题根本原因

通过分析控制台日志发现：**`setQuestionContext`根本没有被调用**！

### 代码路径分析

程序中有两个题目加载路径：

#### 路径1：通过题库面板选择题目（有BUG）
```
用户点击题目
  ↓
QuestionBankPanel::onQuestionFileSelected
  ↓
MainWindow::onQuestionFileSelected  ← 这里没有调用setQuestionContext！
  ↓
加载题目、代码、保存会话
  ↓
❌ AI助手上下文没有更新
```

#### 路径2：通过导航按钮切换题目（正常）
```
用户点击上一题/下一题
  ↓
MainWindow::loadCurrentQuestion
  ↓
m_aiAssistantPanel->setQuestionContext  ← 这里有调用
  ↓
✅ AI助手上下文正常更新
```

### 为什么会有两个路径？

1. **路径1**：用户从题库面板的树形列表中点击题目
   - 这是最常用的方式
   - 直接传递Question对象
   - **之前忘记更新AI助手上下文**

2. **路径2**：用户使用导航按钮（上一题/下一题）
   - 通过索引查找题目
   - 调用`loadCurrentQuestion()`
   - 正确更新了AI助手上下文

## 修复内容

### 修复1：在onQuestionFileSelected中添加AI助手上下文更新

**文件**：`src/ui/MainWindow.cpp`

**位置**：`onQuestionFileSelected()` 方法

**修改**：
```cpp
void MainWindow::onQuestionFileSelected(const QString &filePath, const Question &question)
{
    // ... 前面的代码 ...
    
    // 7. 保存题库面板状态
    if (m_questionBankPanel) {
        QStringList expandedPaths = m_questionBankPanel->getExpandedPaths();
        QString selectedPath = m_questionBankPanel->getSelectedQuestionPath();
        SessionManager::instance().savePanelState(expandedPaths, selectedPath);
    }
    
    // 8. 更新AI助手的题目上下文 ← 新增
    if (m_aiAssistantPanel) {
        qDebug() << "[MainWindow] Updating AI assistant context for question:" << question.id();
        m_aiAssistantPanel->setQuestionContext(question);
    } else {
        qWarning() << "[MainWindow] m_aiAssistantPanel is null!";
    }
    
    // 更新状态栏
    statusBar()->showMessage(QString("✅ 已加载题目：%1").arg(question.title()), 3000);
}
```

### 修复2：添加调试日志

**文件**：`src/ui/MainWindow.cpp`

**位置**：`loadCurrentQuestion()` 方法

**修改**：
```cpp
// 更新AI助手的题目上下文
if (m_aiAssistantPanel) {
    qDebug() << "[MainWindow] Calling setQuestionContext for question:" << question.id();
    m_aiAssistantPanel->setQuestionContext(question);
} else {
    qWarning() << "[MainWindow] m_aiAssistantPanel is null!";
}
```

## 修复后的日志输出

### 通过题库面板选择题目

```
[MainWindow] Question file selected: "path/to/question.json" "q002"
[MainWindow] Saving current code for question: "q001"
[MainWindow] Setting question ID to editor: "q002"
[MainWindow] Loaded saved code for question: "q002" length: 210
[MainWindow] Found question in bank at index: 1
[MainWindow] Session saved - Bank: "data/基础题库" Question: "q002"
[MainWindow] Panel state saved - Expanded: 3 Selected: "path/to/question.json"
[MainWindow] Updating AI assistant context for question: q002  ← 新增
[AIAssistantPanel] setQuestionContext called for: q002 题目标题  ← 新增
[AIAssistantPanel] Saving conversation for old question: q001 messages: 4  ← 新增
[AIAssistantPanel] Switched from q001 to q002  ← 新增
[AIAssistantPanel] Loading conversation history for question: q002  ← 新增
```

### 通过导航按钮切换题目

```
[MainWindow] Loading question: q003 题目标题
[MainWindow] Calling setQuestionContext for question: q003  ← 新增
[AIAssistantPanel] setQuestionContext called for: q003 题目标题
[AIAssistantPanel] Saving conversation for old question: q002 messages: 6
[AIAssistantPanel] Switched from q002 to q003
[AIAssistantPanel] Loading conversation history for question: q003
```

## 测试验证

### 测试1：题库面板选择题目
1. 在题目A进行对话（至少2轮）
2. 从题库面板点击题目B
3. 查看控制台日志
4. 验证AI助手对话是否切换

**预期结果**：
- ✅ 控制台输出`[MainWindow] Updating AI assistant context`
- ✅ 控制台输出`[AIAssistantPanel] setQuestionContext called`
- ✅ AI助手显示题目B的对话（或空白）
- ✅ 切回题目A时对话恢复

### 测试2：导航按钮切换题目
1. 在题目A进行对话
2. 点击"下一题"按钮
3. 验证AI助手对话是否切换

**预期结果**：
- ✅ 控制台输出`[MainWindow] Calling setQuestionContext`
- ✅ AI助手对话正确切换

### 测试3：混合使用
1. 从题库面板选择题目A
2. 进行对话
3. 点击"下一题"到题目B
4. 进行对话
5. 从题库面板选择题目C
6. 进行对话
7. 依次切回题目A、B、C

**预期结果**：
- ✅ 所有题目的对话都正确保存和加载
- ✅ 无论使用哪种方式切换，对话都能正确更新

## 为什么之前没发现这个问题？

1. **测试不充分**：主要测试了导航按钮，没有测试题库面板选择
2. **代码重复**：两个路径的代码逻辑相似但不完全一致
3. **缺少日志**：没有足够的调试日志来追踪问题

## 经验教训

1. **统一代码路径**：相同功能应该调用相同的方法
2. **添加充分的日志**：关键操作都应该有日志输出
3. **全面测试**：测试所有可能的用户操作路径

## 后续优化建议

### 建议1：重构题目加载逻辑

将题目加载逻辑统一到一个方法中：

```cpp
void MainWindow::loadQuestion(const Question &question)
{
    // 1. 保存当前代码
    // 2. 加载新题目
    // 3. 更新编辑器
    // 4. 更新AI助手  ← 统一在这里处理
    // 5. 保存会话
}
```

然后两个路径都调用这个方法：
- `onQuestionFileSelected` → `loadQuestion`
- `loadCurrentQuestion` → `loadQuestion`

### 建议2：添加单元测试

为题目切换功能添加自动化测试，确保：
- 题目切换时AI助手上下文更新
- 对话历史正确保存和加载
- 两种切换方式行为一致

## 相关文件

- `src/ui/MainWindow.cpp` - 主窗口（题目加载逻辑）
- `src/ui/AIAssistantPanel.cpp` - AI助手面板（对话管理）
- `src/ui/QuestionBankPanel.cpp` - 题库面板（题目选择）

## 修复状态

✅ 已完成 - 题目切换时AI助手对话现在能正确更新了

## 感谢

感谢用户提供详细的日志输出，帮助快速定位问题！
