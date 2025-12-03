# Bug修复：崩溃和清空题库

## 🐛 问题描述

### 问题1：点击刷题面板崩溃

**现象**：
- 点击"刷题模式"按钮后程序崩溃
- 没有错误提示，直接退出

**根本原因**：
```cpp
PracticeWidget::PracticeWidget(QuestionBank *questionBank, QWidget *parent)
{
    setupUI();
    loadQuestions();      // ❌ 此时题库可能为空
    updateStatistics();   // ❌ 访问空题库导致崩溃
}
```

在构造函数中直接调用`loadQuestions()`和`updateStatistics()`，但此时：
1. 题库可能还没有加载任何题目
2. `m_questionBank`可能为空指针
3. 访问空题库导致崩溃

### 问题2：无法删除导入的题库

**现象**：
- 导入题库后无法清空
- 没有清空题库的菜单选项
- 只能重启程序

**根本原因**：
- 缺少清空题库的功能
- 没有提供UI入口

## ✅ 修复方案

### 修复1：延迟加载题库

#### 修改前
```cpp
PracticeWidget::PracticeWidget(QuestionBank *questionBank, QWidget *parent)
{
    setupUI();
    loadQuestions();      // 立即加载
    updateStatistics();   // 立即统计
}
```

#### 修改后
```cpp
PracticeWidget::PracticeWidget(QuestionBank *questionBank, QWidget *parent)
{
    setupUI();
    
    // 延迟加载，避免初始化时崩溃
    // loadQuestions() 和 updateStatistics() 会在 refreshQuestionList() 中调用
    
    // 连接进度管理器信号
    connect(&ProgressManager::instance(), &ProgressManager::statisticsChanged,
            this, &PracticeWidget::updateStatistics);
}
```

#### 安全检查增强
```cpp
void PracticeWidget::refreshQuestionList()
{
    // 安全检查
    if (!m_questionBank) {
        qWarning() << "QuestionBank is null in refreshQuestionList()";
        m_statsLabel->setText("题库未加载");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    
    loadQuestions();
    updateStatistics();
}
```

### 修复2：添加清空题库功能

#### 菜单项添加

```cpp
// 文件菜单
QMenu *fileMenu = menuBar()->addMenu("文件(&F)");

QAction *importAction = fileMenu->addAction("导入题库(&I)...");
QAction *refreshAction = fileMenu->addAction("刷新题库(&R)");
QAction *reloadAction = fileMenu->addAction("重新加载题库(&L)...");
QAction *clearAction = fileMenu->addAction("清空题库(&C)...");  // ✅ 新增
```

#### 快捷键

```
Ctrl+Shift+C - 清空题库
```

#### 实现逻辑

```cpp
void MainWindow::onClearQuestionBank()
{
    // 1. 检查题库是否为空
    if (m_questionBank->count() == 0) {
        QMessageBox::information(this, "提示", "当前没有题库");
        return;
    }
    
    // 2. 确认对话框
    QMessageBox msgBox;
    msgBox.setText("确定要清空当前题库吗？");
    msgBox.setInformativeText(
        QString("当前有 %1 道题目，清空后将无法恢复。\n\n"
                "注意：这不会删除原始文件，只是清空程序中的题库。")
        .arg(m_questionBank->count())
    );
    
    if (msgBox.exec() == QMessageBox::Yes) {
        // 3. 清空题库
        m_questionBank->clear();
        m_currentQuestionIndex = -1;
        
        // 4. 清空UI
        m_questionPanel->setQuestion(Question());
        m_codeEditor->setCode("");
        m_aiPanel->setAnalysis("");
        m_questionListWidget->setQuestions(QVector<Question>());
        
        // 5. 刷新刷题模式
        m_practiceWidget->refreshQuestionList();
        
        // 6. 清空会话
        SessionManager::instance().clearSession();
        
        // 7. 提示完成
        QMessageBox::information(this, "完成", 
            "题库已清空。\n\n您可以重新导入题库。");
    }
}
```

## 📊 修复效果

### 崩溃问题

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| 空题库切换刷题模式 | ❌ 崩溃 | ✅ 正常显示"题库未加载" |
| 有题库切换刷题模式 | ❌ 可能崩溃 | ✅ 正常显示 |
| 导入后切换 | ❌ 可能崩溃 | ✅ 正常显示 |

### 清空题库

| 功能 | 修复前 | 修复后 |
|------|--------|--------|
| 清空题库 | ❌ 无此功能 | ✅ 菜单项+快捷键 |
| 确认提示 | - | ✅ 详细提示 |
| UI更新 | - | ✅ 全部清空 |
| 会话清理 | - | ✅ 自动清理 |

## 🎨 UI改进

### 清空题库对话框

```
┌─────────────────────────────────────┐
│ ⚠️  清空题库                        │
├─────────────────────────────────────┤
│ 确定要清空当前题库吗？              │
│                                     │
│ 当前有 15 道题目，清空后将无法恢复。│
│                                     │
│ 注意：这不会删除原始文件，          │
│ 只是清空程序中的题库。              │
│                                     │
│              [是(Y)] [否(N)]        │
└─────────────────────────────────────┘
```

### 完成提示

```
┌─────────────────────────────────────┐
│ ℹ️  完成                            │
├─────────────────────────────────────┤
│ 题库已清空。                        │
│                                     │
│ 您可以重新导入题库。                │
│                                     │
│                  [确定]             │
└─────────────────────────────────────┘
```

## 🔧 技术细节

### 延迟初始化模式

```cpp
// 构造函数：只创建UI，不加载数据
PracticeWidget::PracticeWidget(...)
{
    setupUI();  // 创建UI组件
    // 不调用 loadQuestions()
}

// 显示时：才加载数据
void MainWindow::onSwitchToPracticeMode()
{
    m_stackedWidget->setCurrentIndex(1);
    m_practiceWidget->refreshQuestionList();  // 这里才加载
}
```

### 安全检查模式

```cpp
void PracticeWidget::loadQuestions()
{
    // 第一层检查：指针
    if (!m_questionBank) {
        qWarning() << "QuestionBank is null";
        return;
    }
    
    // 第二层检查：数量
    if (m_questionBank->count() == 0) {
        qDebug() << "QuestionBank is empty";
        return;
    }
    
    // 安全：可以访问
    for (const auto &question : m_questionBank->allQuestions()) {
        // ...
    }
}
```

### 清空操作的完整性

```cpp
void onClearQuestionBank()
{
    // 1. 数据层清空
    m_questionBank->clear();
    m_currentQuestionIndex = -1;
    
    // 2. UI层清空
    m_questionPanel->setQuestion(Question());
    m_codeEditor->setCode("");
    m_aiPanel->setAnalysis("");
    m_questionListWidget->setQuestions(QVector<Question>());
    
    // 3. 刷题模式清空
    m_practiceWidget->refreshQuestionList();
    
    // 4. 会话清空
    SessionManager::instance().clearSession();
    
    // 5. 状态栏提示
    statusBar()->showMessage("题库已清空", 3000);
}
```

## 📝 使用说明

### 清空题库

**方法1：菜单**
```
文件 → 清空题库
```

**方法2：快捷键**
```
Ctrl+Shift+C
```

**流程：**
1. 点击"清空题库"
2. 确认对话框显示当前题目数量
3. 点击"是"确认清空
4. 所有UI自动清空
5. 显示完成提示

### 重新导入

清空后可以：
1. 点击"导入题库"
2. 选择新的题库文件夹
3. 使用AI智能导入或手动解析
4. 开始新的刷题

## ✅ 验证清单

- [x] 空题库切换刷题模式不崩溃
- [x] 有题库切换刷题模式正常
- [x] 清空题库菜单项添加
- [x] 清空题库快捷键工作
- [x] 清空后UI全部更新
- [x] 清空后会话清理
- [x] 清空后可以重新导入
- [x] 编译成功
- [x] 运行测试通过

## 🔮 后续优化

### 短期

1. [ ] 添加"导出题库"功能
2. [ ] 添加"题库备份"功能
3. [ ] 添加"题库统计"功能

### 长期

1. [ ] 支持多题库管理
2. [ ] 支持题库切换
3. [ ] 支持题库合并
4. [ ] 支持题库分享

---

**版本**: 1.7.1  
**更新日期**: 2024年  
**状态**: ✅ 已完成并测试
