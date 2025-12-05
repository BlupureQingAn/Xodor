# 测试用例修复工具统一重构

## 概述

将原来的单题目修复（TestCaseFixerDialog）和批量修复（BatchTestCaseFixerDialog）合并为一个统一的对话框，简化交互流程，提升用户体验。

## 主要改进

### 1. 统一的对话框

**之前：**
- `TestCaseFixerDialog` - 单题目修复
- `BatchTestCaseFixerDialog` - 批量修复
- 两个独立的类，代码重复

**现在：**
- 统一的 `TestCaseFixerDialog`
- 支持两种构造模式：
  ```cpp
  // 单题目修复模式
  TestCaseFixerDialog(const Question &question, const QString &questionFilePath, 
                     OllamaClient *aiClient, QWidget *parent = nullptr);
  
  // 批量修复模式
  TestCaseFixerDialog(QuestionBank *questionBank, OllamaClient *aiClient, 
                     QWidget *parent = nullptr);
  ```

### 2. 自动化流程

**之前的流程：**
1. 打开对话框
2. 点击"检测问题"按钮
3. 查看检测结果
4. 点击"AI修复"按钮
5. 等待AI响应
6. 点击"保存修复"按钮
7. 手动刷新题库

**现在的流程：**
1. 打开对话框 → **自动检测问题**
2. 点击"一键修复"按钮 → **自动完成修复和保存**
3. **自动刷新题库**

### 3. 简化的UI

#### 单题目模式
```
┌─────────────────────────────────────┐
│ 修复题目：题目名称                    │
│ ⚠️ 发现 3 个测试用例存在问题          │
│ [进度条]（隐藏）                      │
├─────────────────────────────────────┤
│ 测试用例：                            │
│ ✅ 【测试用例 1/5】                   │
│ ⚠️ 【测试用例 2/5】← 有问题           │
│ ...                                  │
├─────────────────────────────────────┤
│ [🚀 一键修复] [⏹ 停止] [关闭]        │
├─────────────────────────────────────┤
│ 处理日志：                            │
│ 检测到 3 个测试用例需要修复。         │
│ 点击'一键修复'按钮开始自动修复。      │
└─────────────────────────────────────┘
```

#### 批量修复模式
```
┌─────────────────────────────────────┐
│ 批量修复测试用例                      │
│ ⚠️ 发现 15 个题目需要修复             │
│ [=========>          ] 5/15          │
├─────────────────────────────────────┤
│ 需要修复的题目：                      │
│ 1. 题目A (2个问题)                   │
│ 2. 题目B (3个问题)                   │
│ ...                                  │
├─────────────────────────────────────┤
│ [🚀 一键修复] [⏹ 停止] [关闭]        │
├─────────────────────────────────────┤
│ 处理日志：                            │
│ [1/15] 正在修复：题目A                │
│ ✅ 成功修复并保存 2 个测试用例        │
│ [2/15] 正在修复：题目B                │
│ ...                                  │
└─────────────────────────────────────┘
```

### 4. 智能状态提示

**状态标签动态更新：**
- 🔍 正在扫描题库...
- ⚠️ 发现 N 个问题
- 🚀 正在调用AI修复...
- 🤖 AI正在生成修复方案...
- ✅ 修复完成
- ❌ 修复失败

**日志信息简洁明了：**
- 不显示完整的AI提示词
- 不显示AI原始响应
- 只显示关键的处理步骤和结果

### 5. 自动刷新机制

**统一的信号：**
```cpp
signals:
    void questionsFixed();  // 题目修复完成信号
```

**MainWindow 自动响应：**
```cpp
connect(dialog, &TestCaseFixerDialog::questionsFixed, this, [this]() {
    onRefreshQuestionBank();  // 自动刷新题库
});
```

## 技术实现

### 模式识别

```cpp
private:
    bool m_isBatchMode;  // 模式标识
```

根据构造函数自动设置模式：
- 单题目模式：`m_isBatchMode = false`
- 批量模式：`m_isBatchMode = true`

### 自动检测

**单题目模式：**
```cpp
// 构造函数中自动检测
m_problematicIndices = detectProblematicTestCases(m_question);
displayTestCases();
```

**批量模式：**
```cpp
// 构造函数中自动扫描
scanAllQuestions();
```

### 一键修复

```cpp
void TestCaseFixerDialog::onAutoFix()
{
    if (m_isBatchMode) {
        // 批量修复：逐个处理
        fixNextQuestion();
    } else {
        // 单题目修复：直接修复
        QString prompt = generateFixPrompt(m_question, m_problematicIndices);
        m_aiClient->sendChatMessage(prompt, "");
    }
}
```

### 自动保存和刷新

```cpp
void TestCaseFixerDialog::applyAIFix()
{
    // 应用修复
    targetQuestion->setTestCases(testCases);
    
    // 保存
    if (saveFixedQuestion(*targetQuestion, targetFilePath)) {
        // 发送信号通知主窗口刷新
        emit questionsFixed();
        
        QMessageBox::information(this, "完成", "修复完成！题库将自动刷新。");
    }
}
```

## 代码简化

### 删除的文件
- `src/ui/BatchTestCaseFixerDialog.h` - 不再需要
- `src/ui/BatchTestCaseFixerDialog.cpp` - 不再需要

### 合并的功能
- 问题检测逻辑
- AI修复逻辑
- 保存逻辑
- UI布局逻辑

### 代码行数对比
- **之前：** TestCaseFixerDialog (450行) + BatchTestCaseFixerDialog (350行) = 800行
- **现在：** TestCaseFixerDialog (570行)
- **减少：** 230行代码（约29%）

## 用户体验提升

### 交互步骤减少

**单题目修复：**
- 之前：7步（打开 → 检测 → 查看 → 修复 → 等待 → 保存 → 刷新）
- 现在：2步（打开 → 一键修复）
- **减少：71%**

**批量修复：**
- 之前：6步（打开 → 扫描 → 查看 → 开始 → 等待 → 刷新）
- 现在：2步（打开 → 一键修复）
- **减少：67%**

### 自动化程度提升

1. **自动检测** - 打开对话框立即显示问题
2. **自动修复** - 一键完成所有操作
3. **自动保存** - 无需手动保存
4. **自动刷新** - 题库自动更新

### 错误处理改进

- AI调用失败自动跳过（批量模式）
- 保存失败自动跳过（批量模式）
- 友好的错误提示
- 可随时停止操作

## 使用示例

### 单题目修复

```cpp
// 在 MainWindow 中
void MainWindow::onFixTestCases()
{
    Question currentQuestion = m_questionBank->getQuestion(m_currentQuestionIndex);
    QString questionFilePath = "data/questions/" + currentQuestion.id() + ".json";
    
    TestCaseFixerDialog *dialog = new TestCaseFixerDialog(
        currentQuestion, questionFilePath, m_ollamaClient, this);
    
    connect(dialog, &TestCaseFixerDialog::questionsFixed, this, [this]() {
        onRefreshQuestionBank();  // 自动刷新
    });
    
    dialog->exec();
}
```

### 批量修复

```cpp
// 在 MainWindow 中
void MainWindow::onBatchFixTestCases()
{
    TestCaseFixerDialog *dialog = new TestCaseFixerDialog(
        m_questionBank, m_ollamaClient, this);
    
    connect(dialog, &TestCaseFixerDialog::questionsFixed, this, [this]() {
        onRefreshQuestionBank();  // 自动刷新
    });
    
    dialog->exec();
}
```

## 测试要点

1. **单题目模式**
   - 打开对话框是否自动检测问题
   - 一键修复是否正常工作
   - 修复后是否自动刷新题库
   - 无问题时是否正确提示

2. **批量模式**
   - 打开对话框是否自动扫描
   - 题目列表是否正确显示
   - 批量修复是否逐个处理
   - 进度条是否正确更新
   - 修复完成后是否自动刷新

3. **错误处理**
   - AI调用失败是否正确处理
   - 保存失败是否正确处理
   - 停止按钮是否正常工作

4. **UI响应**
   - 状态标签是否实时更新
   - 日志信息是否清晰
   - 按钮状态是否正确切换

## 优势总结

1. **代码更简洁** - 减少29%代码量
2. **交互更简单** - 减少67-71%操作步骤
3. **体验更流畅** - 自动化程度大幅提升
4. **维护更容易** - 统一的代码逻辑
5. **功能更强大** - 保留所有原有功能

## 后续优化建议

1. 添加修复预览功能
2. 支持选择性修复（批量模式）
3. 添加修复历史记录
4. 支持撤销修复操作
5. 优化AI提示词以提高修复成功率
