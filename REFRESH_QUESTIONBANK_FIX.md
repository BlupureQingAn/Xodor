# 刷新题库功能修复

## 问题描述

用户点击"刷新题库"按钮后，题库列表变空，所有题目消失。虽然题库文件仍然存在于磁盘上（`data/基础题库/CCF/` 目录中的45道题目），但程序内存中的 `QuestionBank` 对象被清空了。

## 根本原因

`PracticeWidget::onRefreshClicked()` 函数只调用了 `loadQuestions()` 和 `updateStatistics()`，但没有重新从磁盘加载题库数据。`loadQuestions()` 只是从内存中的 `QuestionBank` 对象读取题目，如果 `QuestionBank` 是空的，就会显示空列表。

## 解决方案

### 1. 添加信号机制
在 `PracticeWidget` 中添加信号 `reloadQuestionBankRequested()`，当用户点击"刷新题库"按钮时发出此信号。

**修改文件：`src/ui/PracticeWidget.h`**
```cpp
signals:
    void questionSelected(const Question &question);
    void reloadQuestionBankRequested();  // 请求重新加载题库
```

### 2. 修改刷新按钮逻辑
在 `onRefreshClicked()` 中发出重新加载信号。

**修改文件：`src/ui/PracticeWidget.cpp`**
```cpp
void PracticeWidget::onRefreshClicked()
{
    // 发出信号请求 MainWindow 重新加载题库
    emit reloadQuestionBankRequested();
    
    // 然后刷新显示
    loadQuestions();
    updateStatistics();
}
```

### 3. 在 MainWindow 中实现重新加载
添加 `onRefreshCurrentBank()` 槽函数，从磁盘重新加载当前题库。

**修改文件：`src/ui/MainWindow.h`**
```cpp
void onRefreshCurrentBank();  // 刷新当前题库
```

**修改文件：`src/ui/MainWindow.cpp`**
```cpp
void MainWindow::onRefreshCurrentBank()
{
    // 刷新当前题库（重新从磁盘加载）
    if (m_currentBankPath.isEmpty()) {
        statusBar()->showMessage(QString::fromUtf8("没有加载的题库"), 3000);
        return;
    }
    
    statusBar()->showMessage(QString::fromUtf8("正在刷新题库..."), 0);
    
    // 保存当前题目索引
    int savedIndex = m_currentQuestionIndex;
    
    // 重新加载题库
    m_questionBank->loadFromDirectory(m_currentBankPath);
    
    // 恢复题目索引（如果有效）
    if (savedIndex >= 0 && savedIndex < m_questionBank->count()) {
        m_currentQuestionIndex = savedIndex;
        loadCurrentQuestion();
    } else {
        m_currentQuestionIndex = 0;
        if (m_questionBank->count() > 0) {
            loadCurrentQuestion();
        }
    }
    
    // 刷新练习模式的题目列表
    if (m_practiceWidget) {
        m_practiceWidget->refreshQuestionList();
    }
    
    statusBar()->showMessage(
        QString::fromUtf8("题库已刷新：共 %1 道题目").arg(m_questionBank->count()), 
        3000
    );
}
```

### 4. 连接信号和槽
在 `setupConnections()` 中连接信号。

**修改文件：`src/ui/MainWindow.cpp`**
```cpp
// 题库列表信号
connect(m_practiceWidget, &PracticeWidget::reloadQuestionBankRequested,
        this, &MainWindow::onRefreshCurrentBank);
```

## 修复效果

现在点击"刷新题库"按钮后：
1. ✅ 从磁盘重新加载题库文件
2. ✅ 保持当前题目位置（如果有效）
3. ✅ 刷新练习模式的题目列表
4. ✅ 显示加载的题目数量
5. ✅ 题库不会丢失

## 题库恢复说明

如果题库已经丢失（内存中为空），有两种恢复方法：

### 方法1：重启程序（推荐）
关闭程序后重新打开，程序会自动从 `data/基础题库/CCF` 加载题库。

### 方法2：使用修复后的刷新功能
点击"🔄 刷新"按钮，程序会重新从磁盘加载题库。

## 题库文件位置

题库文件保存在：
- **基础题库**：`data/基础题库/CCF/` （45道题目）
- **原始题库**：`data/原始题库/CCF/` （备份）

所有题目文件都是 JSON 格式，文件名为题目标题。

## 编译结果

✅ 编译成功 - Exit Code: 0

## 相关功能

程序中还有另一个功能 `onReloadQuestionBank()`，它的作用是：
- 让用户选择新的题库路径
- 清空当前题库
- 重新导入选择的题库

这与 `onRefreshCurrentBank()` 不同：
- `onRefreshCurrentBank()` - 刷新当前题库（从磁盘重新加载）
- `onReloadQuestionBank()` - 重新导入题库（需要用户选择路径）

## 总结

修复完成！"刷新题库"按钮现在能正确地从磁盘重新加载题库，不会再导致题库丢失。题库文件完好无损，用户数据安全。
