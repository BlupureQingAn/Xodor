# 模式重命名与崩溃修复

## 改进内容

### 1. 模式名称调整

#### 之前的命名
- **编辑模式**: 主界面，编写代码的地方
- **刷题模式**: 题库列表视图

#### 现在的命名（更符合直觉）
- **刷题模式**: 主界面，编写代码的地方
- **题库列表**: 题库列表视图

#### 修改位置
**文件**: `src/ui/MainWindow.cpp`

1. **工具栏按钮**:
   - `📊 刷题模式` → `📊 题库列表`
   - `✏️ 编辑模式` → `✏️ 刷题模式`

2. **菜单项**:
   - `刷题模式(&P)` → `题库列表(&P)`
   - `编辑模式(&E)` → `刷题模式(&E)`

3. **状态栏消息**:
   - `已切换到刷题模式` → `已切换到题库列表`
   - `已切换到编辑模式` → `已切换到刷题模式`

4. **注释**:
   - `// === 刷题模式 ===` → `// === 题库列表 ===`
   - `// 刷题模式信号` → `// 题库列表信号`
   - `// 切换到编辑模式` → `// 切换到刷题模式`

### 2. 题库列表点击崩溃修复

#### 问题分析
题库列表点击后软件崩溃，可能原因：
1. Question对象无效
2. 指针未初始化
3. 索引越界
4. 缺少安全检查

#### 解决方案
**文件**: `src/ui/MainWindow.cpp` - `setupConnections()`

添加了完整的安全检查和错误处理：

```cpp
connect(m_practiceWidget, &PracticeWidget::questionSelected, this, [this](const Question &question) {
    qDebug() << "[MainWindow] Question selected:" << question.id() << question.title();
    
    // 1. 验证题目有效性
    if (question.id().isEmpty()) {
        qWarning() << "[MainWindow] Invalid question (empty id)";
        return;
    }
    
    // 2. 切换到刷题模式
    m_stackedWidget->setCurrentIndex(0);
    
    // 3. 在题库中找到题目索引
    bool found = false;
    for (int i = 0; i < m_questionBank->count(); ++i) {
        if (m_questionBank->allQuestions()[i].id() == question.id()) {
            m_currentQuestionIndex = i;
            
            // 4. 安全地设置题目（检查指针）
            if (m_questionPanel) {
                m_questionPanel->setQuestion(question);
            }
            
            // 5. 加载保存的代码（而不是清空）
            loadSavedCode(question.id());
            
            // 6. 更新题目列表选中状态（检查指针）
            if (m_questionListWidget) {
                m_questionListWidget->setCurrentQuestion(i);
            }
            
            statusBar()->showMessage(QString("已选择题目: %1").arg(question.title()), 3000);
            found = true;
            break;
        }
    }
    
    // 7. 未找到题目的处理
    if (!found) {
        qWarning() << "[MainWindow] Question not found:" << question.id();
        statusBar()->showMessage("题目未找到", 3000);
    }
});
```

#### 关键改进

1. **题目验证**: 检查question.id()是否为空
2. **指针检查**: 使用前检查m_questionPanel和m_questionListWidget
3. **代码加载**: 调用loadSavedCode()而不是setCode("")，保留用户之前的代码
4. **调试日志**: 添加qDebug和qWarning，便于追踪问题
5. **错误处理**: 题目未找到时给出提示而不是崩溃

## 用户体验改进

### 命名更直观
- **刷题模式**: 用户主要工作的地方，编写代码、运行测试
- **题库列表**: 查看所有题目，选择要做的题

### 更安全的切换
- 点击题库列表中的题目不会崩溃
- 自动加载之前保存的代码
- 给出清晰的状态反馈

### 调试友好
- 添加了详细的日志输出
- 便于定位问题
- 便于后续维护

## 工作流程

### 正常流程
```
启动程序（刷题模式）
    ↓
点击"题库列表"按钮 (Ctrl+P)
    ↓
显示所有题目
    ↓
双击某个题目
    ↓
验证题目有效性
    ↓
切换回刷题模式
    ↓
加载题目和保存的代码
    ↓
开始编写/继续编写代码
```

### 异常处理
```
双击题目
    ↓
题目ID为空？
    → 是：记录警告，不切换
    → 否：继续
    ↓
在题库中找到？
    → 否：显示"题目未找到"
    → 是：正常加载
    ↓
指针有效？
    → 否：跳过该操作
    → 是：正常执行
```

## 快捷键

- **Ctrl+P**: 切换到题库列表
- **Ctrl+E**: 切换到刷题模式

## 文件修改清单
- ✅ `src/ui/MainWindow.cpp` - 重命名模式，修复崩溃

## 编译状态
✅ 编译成功，无错误，无警告

## 测试要点
1. 点击"题库列表"按钮，应该显示题目列表
2. 双击题目，应该切换到刷题模式并加载题目
3. 不应该崩溃
4. 应该加载之前保存的代码（如果有）
5. 状态栏应该显示"已选择题目: XXX"
