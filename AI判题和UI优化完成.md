# AI判题和UI优化完成

## 修改内容

### 1. AI判题配置确认 ✅

**问题：** AI判题调用的AI应该使用设置中的AI配置

**现状分析：**
```cpp
// MainWindow.cpp - setupUI()
m_aiJudge = new AIJudge(m_ollamaClient, this);
```

**结论：** ✅ **已经正确配置**

AI判题使用的是 `m_ollamaClient`，而这个客户端在 `loadConfiguration()` 方法中会根据用户设置进行配置：

```cpp
void MainWindow::loadConfiguration()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 配置AI服务
    if (config.useCloudApi()) {
        // 使用云端API
        m_ollamaClient->setCloudMode(true);
        m_ollamaClient->setBaseUrl(config.cloudApiUrl());
        m_ollamaClient->setModel(config.cloudApiModel());
        m_ollamaClient->setApiKey(config.cloudApiKey());
    } else {
        // 使用本地Ollama
        m_ollamaClient->setCloudMode(false);
        m_ollamaClient->setBaseUrl(config.ollamaUrl());
        m_ollamaClient->setModel(config.ollamaModel());
    }
}
```

**工作流程：**
```
用户在设置中配置AI
  ↓
保存配置到 ConfigManager
  ↓
MainWindow::loadConfiguration() 读取配置
  ↓
配置 m_ollamaClient
  ↓
AI判题使用配置好的 m_ollamaClient
```

**验证：**
- ✅ AI判题和AI导师使用同一个 `m_ollamaClient` 实例
- ✅ 配置更改后立即生效（通过 `aiConfigChanged` 信号）
- ✅ 支持本地Ollama和云端API两种模式

### 2. 判题进度对话框居中显示 ✅

**问题：** 判题的加载进度条应该手动居中绘制

**修改前：**
```cpp
if (!m_aiJudgeProgressDialog) {
    m_aiJudgeProgressDialog = new QProgressDialog(this);
    m_aiJudgeProgressDialog->setWindowTitle("AI判题中");
    m_aiJudgeProgressDialog->setLabelText("正在分析代码...");
    m_aiJudgeProgressDialog->setRange(0, 0);
    m_aiJudgeProgressDialog->setModal(true);
    m_aiJudgeProgressDialog->setCancelButton(nullptr);
}
m_aiJudgeProgressDialog->show();
```

**修改后：**
```cpp
if (!m_aiJudgeProgressDialog) {
    m_aiJudgeProgressDialog = new QProgressDialog(this);
    m_aiJudgeProgressDialog->setWindowTitle("AI判题中");
    m_aiJudgeProgressDialog->setLabelText("正在分析代码...");
    m_aiJudgeProgressDialog->setRange(0, 0);  // 不确定进度
    m_aiJudgeProgressDialog->setModal(true);
    m_aiJudgeProgressDialog->setCancelButton(nullptr);  // 不允许取消
    m_aiJudgeProgressDialog->setMinimumWidth(300);
    m_aiJudgeProgressDialog->setMinimumHeight(120);
}

// 手动居中对话框
QRect parentRect = this->geometry();
QSize dialogSize = m_aiJudgeProgressDialog->sizeHint();
int x = parentRect.x() + (parentRect.width() - dialogSize.width()) / 2;
int y = parentRect.y() + (parentRect.height() - dialogSize.height()) / 2;
m_aiJudgeProgressDialog->move(x, y);

m_aiJudgeProgressDialog->show();
```

**改进点：**

1. **设置最小尺寸**
   ```cpp
   m_aiJudgeProgressDialog->setMinimumWidth(300);
   m_aiJudgeProgressDialog->setMinimumHeight(120);
   ```
   - 确保对话框有合适的大小
   - 避免内容被挤压

2. **手动计算居中位置**
   ```cpp
   QRect parentRect = this->geometry();
   QSize dialogSize = m_aiJudgeProgressDialog->sizeHint();
   int x = parentRect.x() + (parentRect.width() - dialogSize.width()) / 2;
   int y = parentRect.y() + (parentRect.height() - dialogSize.height()) / 2;
   m_aiJudgeProgressDialog->move(x, y);
   ```
   - 获取主窗口的位置和大小
   - 获取对话框的建议大小
   - 计算居中位置：`(父窗口宽度 - 对话框宽度) / 2`
   - 使用 `move()` 设置对话框位置

**居中计算公式：**
```
对话框X坐标 = 主窗口X + (主窗口宽度 - 对话框宽度) / 2
对话框Y坐标 = 主窗口Y + (主窗口高度 - 对话框高度) / 2
```

**效果：**
- ✅ 对话框始终显示在主窗口中央
- ✅ 即使主窗口移动或调整大小，每次显示都会重新居中
- ✅ 视觉效果更加专业和友好

### 3. AI导师气泡高度逻辑文档 ✅

**问题：** 目前AI导师对话框的气泡绘制的高度的逻辑是什么

**已创建文档：** `AI导师气泡高度计算逻辑说明.md`

**核心机制总结：**

#### 自适应高度计算
```cpp
void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    int docHeight = doc->size().toSize().height();
    int margin = doc->documentMargin();
    int height = docHeight + margin * 2 + 4;
    
    m_textBrowser->setFixedHeight(height);
    setMinimumHeight(height + 20);
}
```

#### 计算流程
```
1. 设置文档宽度 = 视口宽度
   ↓
2. Qt 自动计算文档高度（考虑换行）
   ↓
3. 加上文档边距（8px × 2）
   ↓
4. 设置 QTextBrowser 固定高度
   ↓
5. 设置 Widget 最小高度（加上布局边距）
```

#### 触发时机
1. **内容变化**：`setContent()` 调用时
2. **窗口大小变化**：`resizeEvent()` 宽度变化时
3. **字体缩放**：`setFontScale()` 调用时

#### 关键配置
```cpp
// 自动换行
m_textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
m_textBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

// 禁用滚动条
m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

// 尺寸策略
setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
```

#### 优势
- ✅ 短消息占用少量空间
- ✅ 长消息自动扩展
- ✅ 代码块完整显示
- ✅ 响应窗口大小变化
- ✅ 支持字体缩放

## 相关文件

### 修改的文件
1. **`src/ui/MainWindow.cpp`**
   - 添加进度对话框居中逻辑
   - 设置对话框最小尺寸

### 分析的文件
2. **`src/ai/AIJudge.cpp`**
   - AI判题实现
   - 使用 OllamaClient 进行判题

3. **`src/ui/ChatBubbleWidget.cpp`**
   - 聊天气泡实现
   - 自适应高度计算

### 创建的文档
4. **`AI导师气泡高度计算逻辑说明.md`**
   - 详细的高度计算逻辑说明
   - 包含示例和调试技巧

## 测试建议

### 测试1：AI判题配置
1. 打开设置（Ctrl+,）
2. 配置本地Ollama模型
3. 保存设置
4. 使用AI判题功能
5. 验证：使用配置的模型进行判题

### 测试2：进度对话框居中
1. 调整主窗口大小
2. 移动主窗口位置
3. 点击"AI判题"按钮
4. 验证：进度对话框显示在主窗口中央

### 测试3：气泡高度自适应
1. 在AI导师面板发送短消息
2. 验证：气泡高度适应短消息
3. 发送长消息（多行）
4. 验证：气泡高度自动扩展
5. 发送包含代码块的消息
6. 验证：代码块完整显示
7. 调整窗口宽度
8. 验证：气泡高度随宽度变化自动调整

### 测试4：字体缩放
1. 在AI导师面板按住Ctrl+滚轮
2. 放大/缩小字体
3. 验证：气泡高度随字体大小调整

## 技术细节

### 进度对话框居中算法

**坐标系统：**
- Qt 使用屏幕坐标系统
- 原点在屏幕左上角
- X轴向右，Y轴向下

**计算步骤：**
```cpp
// 1. 获取主窗口的屏幕坐标和大小
QRect parentRect = this->geometry();
// parentRect.x() - 主窗口左上角X坐标
// parentRect.y() - 主窗口左上角Y坐标
// parentRect.width() - 主窗口宽度
// parentRect.height() - 主窗口高度

// 2. 获取对话框的建议大小
QSize dialogSize = m_aiJudgeProgressDialog->sizeHint();
// dialogSize.width() - 对话框宽度
// dialogSize.height() - 对话框高度

// 3. 计算居中位置
int x = parentRect.x() + (parentRect.width() - dialogSize.width()) / 2;
int y = parentRect.y() + (parentRect.height() - dialogSize.height()) / 2;

// 4. 移动对话框到计算的位置
m_aiJudgeProgressDialog->move(x, y);
```

**示例计算：**
```
假设：
- 主窗口位置：(100, 100)
- 主窗口大小：800 × 600
- 对话框大小：300 × 120

计算：
- X = 100 + (800 - 300) / 2 = 100 + 250 = 350
- Y = 100 + (600 - 120) / 2 = 100 + 240 = 340

结果：对话框显示在 (350, 340)
```

### 气泡高度计算原理

**QTextDocument 布局引擎：**
- Qt 的 QTextDocument 内置强大的布局引擎
- 支持 HTML/CSS 样式
- 自动处理文本换行、表格、列表等

**高度计算依赖：**
1. **文档宽度**：必须先设置，才能计算换行后的高度
2. **内容复杂度**：代码块、表格等会增加高度
3. **字体大小**：影响行高和总高度
4. **行间距**：`line-height` CSS 属性

**性能优化：**
- 只在必要时重新计算（内容变化、宽度变化）
- 使用 Qt 的缓存机制
- 避免频繁的 `setHtml()` 调用

## 优势总结

### 1. AI判题配置
- ✅ 使用统一的AI客户端配置
- ✅ 支持本地和云端两种模式
- ✅ 配置更改立即生效

### 2. 进度对话框
- ✅ 始终居中显示
- ✅ 视觉效果专业
- ✅ 用户体验友好

### 3. 气泡高度
- ✅ 自适应内容长度
- ✅ 响应窗口大小变化
- ✅ 支持字体缩放
- ✅ 性能优化

## 后续优化建议

### 1. 进度对话框增强
- 显示实际进度（如果AI支持）
- 添加取消按钮（可选）
- 显示预计剩余时间

### 2. 气泡动画
- 添加气泡出现动画
- 流式输出时的打字机效果
- 滚动到新消息的平滑动画

### 3. AI判题反馈
- 显示判题详细过程
- 支持查看AI的分析思路
- 提供改进建议的可视化展示

## 总结

✅ **AI判题配置**：已确认使用设置中的AI配置，无需修改

✅ **进度对话框居中**：实现手动居中算法，确保对话框始终显示在主窗口中央

✅ **气泡高度逻辑**：创建详细文档说明自适应高度计算机制

所有修改已编译成功，可以直接使用！
