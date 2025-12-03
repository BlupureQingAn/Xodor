# 刷题模式集成说明

## 🎯 重大改进

将刷题系统从独立窗口改为**集成到主界面**的双模式设计，提供更流畅的使用体验。

## 📐 架构设计

### 双模式切换

使用 `QStackedWidget` 实现两种模式的无缝切换：

```
MainWindow
├── QStackedWidget (中央窗口)
│   ├── [0] 编辑模式 (Normal Mode)
│   │   └── QSplitter
│   │       ├── QuestionPanel (题目面板)
│   │       ├── CodeEditor (代码编辑器)
│   │       └── AIAnalysisPanel (AI分析面板)
│   │
│   └── [1] 刷题模式 (Practice Mode)
│       └── PracticeWidget (刷题系统)
│           ├── 统计信息栏
│           ├── 筛选工具栏
│           └── 题目列表表格
│
└── QDockWidget (侧边栏)
    └── QuestionListWidget (题目列表)
```

## ✨ 功能特性

### 1. 模式切换

#### 工具栏按钮
- **📊 刷题模式** - 切换到刷题系统视图
- **✏️ 编辑模式** - 切换到代码编辑视图

#### 快捷键
- `Ctrl+P` - 切换到刷题模式
- `Ctrl+E` - 切换到编辑模式

#### 菜单入口
- 视图 → 刷题模式
- 视图 → 编辑模式

### 2. 无缝联动

#### 从刷题模式到编辑模式
1. 在刷题模式中双击题目
2. 自动切换到编辑模式
3. 加载选中的题目
4. 清空代码编辑器
5. 准备开始答题

#### 从编辑模式到刷题模式
1. 点击工具栏"刷题模式"按钮
2. 自动刷新题目列表
3. 显示最新进度统计
4. 保持筛选条件

### 3. 数据同步

- 测试完成后自动更新进度
- 刷题模式实时显示最新状态
- 编辑模式和刷题模式共享题库
- 进度数据持久化保存

## 🔧 技术实现

### 初始化顺序

```cpp
void MainWindow::setupUI()
{
    // 1. 先初始化题库（避免空指针）
    m_questionBank = new QuestionBank(this);
    m_ollamaClient = new OllamaClient(this);
    m_compilerRunner = new CompilerRunner(this);
    
    // 2. 创建堆叠窗口
    m_stackedWidget = new QStackedWidget(this);
    
    // 3. 创建编辑模式界面
    m_normalModeWidget = new QWidget(this);
    // ... 创建分割器和各个面板
    
    // 4. 创建刷题模式界面（传入已初始化的题库）
    m_practiceWidget = new PracticeWidget(m_questionBank, this);
    
    // 5. 添加到堆叠窗口
    m_stackedWidget->addWidget(m_normalModeWidget);  // index 0
    m_stackedWidget->addWidget(m_practiceWidget);    // index 1
    
    // 6. 设置为中央窗口
    setCentralWidget(m_stackedWidget);
}
```

### 模式切换

```cpp
void MainWindow::onSwitchToPracticeMode()
{
    m_stackedWidget->setCurrentIndex(1);  // 切换到刷题模式
    m_practiceWidget->refreshQuestionList();
    statusBar()->showMessage("已切换到刷题模式", 2000);
}

void MainWindow::onSwitchToNormalMode()
{
    m_stackedWidget->setCurrentIndex(0);  // 切换到编辑模式
    statusBar()->showMessage("已切换到编辑模式", 2000);
}
```

### 题目选择联动

```cpp
void MainWindow::setupConnections()
{
    // 刷题模式选择题目后自动切换到编辑模式
    connect(m_practiceWidget, &PracticeWidget::questionSelected, 
            this, [this](const Question &question) {
        // 切换到编辑模式
        m_stackedWidget->setCurrentIndex(0);
        
        // 加载题目
        for (int i = 0; i < m_questionBank->count(); ++i) {
            if (m_questionBank->allQuestions()[i].id() == question.id()) {
                m_currentQuestionIndex = i;
                m_questionPanel->setQuestion(question);
                m_codeEditor->setCode("");
                m_questionListWidget->setCurrentQuestion(i);
                statusBar()->showMessage(
                    QString("已选择题目: %1").arg(question.title()), 
                    3000
                );
                break;
            }
        }
    });
}
```

## 🐛 Bug修复

### 1. 空指针崩溃

**问题**：PracticeWidget 在题库未初始化时访问导致崩溃

**解决方案**：
```cpp
void PracticeWidget::loadQuestions()
{
    m_questionTable->setRowCount(0);
    
    // 添加空指针检查
    if (!m_questionBank) {
        qWarning() << "QuestionBank is null";
        return;
    }
    
    // 添加空题库检查
    if (m_questionBank->count() == 0) {
        qDebug() << "QuestionBank is empty";
        return;
    }
    
    // ... 继续加载
}
```

### 2. 初始化顺序

**问题**：PracticeWidget 在 QuestionBank 之前创建

**解决方案**：调整初始化顺序，先创建 QuestionBank，再创建 PracticeWidget

### 3. 统计信息显示

**问题**：题库为空时统计信息显示异常

**解决方案**：
```cpp
void PracticeWidget::updateStatistics()
{
    if (!m_questionBank) {
        m_statsLabel->setText("题库未加载");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    
    // ... 正常统计
}
```

## 🎨 UI优化

### 工具栏布局

```
[📊 刷题模式] [✏️ 编辑模式] | [📚 导入题库] [🔄 刷新] | [⬅ 上一题] [➡ 下一题] | [▶ 运行测试] [🤖 AI分析]
```

### 菜单结构

```
文件
  ├── 导入题库
  ├── 刷新题库
  ├── 重新加载题库
  └── 退出

视图 (新增)
  ├── 刷题模式 (Ctrl+P)
  └── 编辑模式 (Ctrl+E)

题目
  ├── 查看原题
  └── 生成模拟题

历史
  └── 查看做题记录

代码
  └── 插入模板

工具
  ├── 错题本
  └── 设置

帮助
  └── 关于
```

## 📊 使用流程

### 典型工作流程

```
1. 启动程序（默认编辑模式）
   ↓
2. 导入题库
   ↓
3. 切换到刷题模式 (Ctrl+P)
   ↓
4. 浏览题目列表，查看进度
   ↓
5. 筛选想做的题目（难度/题型/状态）
   ↓
6. 双击题目
   ↓
7. 自动切换到编辑模式
   ↓
8. 编写代码
   ↓
9. 运行测试
   ↓
10. 自动更新进度
   ↓
11. 切换回刷题模式查看进度 (Ctrl+P)
   ↓
12. 继续下一题
```

## 🔄 与原有功能的兼容

### 保留的功能
- ✅ 题目列表侧边栏（仍然可用）
- ✅ 上一题/下一题按钮
- ✅ 查看原题功能
- ✅ 错题本功能
- ✅ 历史记录功能
- ✅ AI分析功能

### 增强的功能
- 📊 刷题模式提供更全面的进度视图
- 🔍 更强大的筛选和搜索功能
- 📈 实时统计信息
- 🎯 智能状态判定

## 💡 使用建议

### 刷题模式适用场景
- 查看整体进度
- 筛选特定类型题目
- 规划刷题计划
- 查找未完成的题目
- 复习已掌握的题目

### 编辑模式适用场景
- 专注编写代码
- 查看题目详情
- 运行测试
- AI分析代码
- 连续做题

### 最佳实践
1. 用刷题模式**规划**要做的题目
2. 双击题目自动切换到编辑模式
3. 在编辑模式**专注**完成题目
4. 完成后切换回刷题模式**查看**进度
5. 重复以上流程

## 📝 快捷键总结

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+P` | 切换到刷题模式 |
| `Ctrl+E` | 切换到编辑模式 |
| `Ctrl+I` | 导入题库 |
| `F5` | 刷新题库 |
| `Ctrl+R` | 重新加载题库 |
| `Ctrl+Shift+V` | 查看原题 |
| `Ctrl+H` | 查看历史记录 |
| `Ctrl+W` | 打开错题本 |
| `Ctrl+,` | 打开设置 |
| `Ctrl+Q` | 退出程序 |

## 🎯 优势总结

### 相比独立窗口的优势

1. **更流畅的体验**
   - 无需打开/关闭窗口
   - 一键切换视图
   - 保持上下文

2. **更好的集成**
   - 共享工具栏和菜单
   - 统一的状态栏提示
   - 一致的快捷键

3. **更高的效率**
   - 快速切换模式
   - 自动联动跳转
   - 减少操作步骤

4. **更稳定的性能**
   - 避免多窗口管理
   - 统一的生命周期
   - 更少的内存占用

## 🔮 未来扩展

可以继续添加更多模式：
- 📖 学习模式（查看教程和文档）
- 🏆 竞赛模式（限时答题）
- 📊 统计模式（详细的数据分析）
- 🎓 复习模式（智能推荐复习题目）

只需在 QStackedWidget 中添加新的页面即可。

---

**版本**: 1.5.1  
**更新日期**: 2024年  
**状态**: ✅ 已完成并测试
