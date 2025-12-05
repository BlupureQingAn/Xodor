# AI导师气泡高度计算逻辑说明

## 概述

AI导师对话框中的聊天气泡（`ChatBubbleWidget`）使用自适应高度机制，根据内容自动计算和调整高度。

## 核心机制

### 1. 自适应高度计算

**文件：`src/ui/ChatBubbleWidget.cpp`**

**关键方法：`adjustHeight()`**

```cpp
void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    int docHeight = doc->size().toSize().height();
    int margin = doc->documentMargin();
    int height = docHeight + margin * 2 + 4;
    
    m_textBrowser->setFixedHeight(height);
    setMinimumHeight(height + 20);  // 加上布局边距
}
```

### 2. 高度计算流程

```
1. 获取 QTextDocument
   ↓
2. 设置文档宽度为视口宽度
   doc->setTextWidth(viewport()->width())
   ↓
3. 让 Qt 自动计算文档高度
   docHeight = doc->size().toSize().height()
   ↓
4. 加上文档边距
   height = docHeight + margin * 2 + 4
   ↓
5. 设置 QTextBrowser 固定高度
   m_textBrowser->setFixedHeight(height)
   ↓
6. 设置 Widget 最小高度
   setMinimumHeight(height + 20)
```

### 3. 触发时机

高度重新计算在以下情况触发：

#### 3.1 内容变化时
```cpp
void ChatBubbleWidget::setContent(const QString &content)
{
    m_content = content;
    
    // 格式化内容
    QString html = m_isUser ? formatUserMessage(content) : formatMarkdown(content);
    m_textBrowser->setHtml(html);
    
    // 自动调整高度
    adjustHeight();
}
```

#### 3.2 窗口大小变化时
```cpp
void ChatBubbleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 宽度变化时重新计算高度
    if (event->oldSize().width() != event->size().width() && event->size().width() > 0) {
        adjustHeight();
    }
}
```

#### 3.3 字体缩放时
```cpp
void ChatBubbleWidget::setFontScale(qreal scale)
{
    if (qAbs(m_fontScale - scale) > 0.01) {
        m_fontScale = scale;
        setContent(m_content);  // 重新设置内容，触发 adjustHeight()
    }
}
```

## 关键配置

### 1. 文本换行设置

```cpp
// 关键：让 QTextBrowser 根据 widget 宽度自动换行
m_textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
m_textBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
```

**作用：**
- `WidgetWidth`：根据控件宽度自动换行
- `WrapAtWordBoundaryOrAnywhere`：优先在单词边界换行，必要时在任意位置换行

### 2. 尺寸策略

```cpp
// Widget 尺寸策略
setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

// QTextBrowser 尺寸策略
m_textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
```

**作用：**
- `Expanding`（水平）：尽可能占用可用宽度
- `Minimum`（垂直）：使用最小必要高度

### 3. 滚动条设置

```cpp
m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
```

**作用：**
- 禁用滚动条，让内容完全展开
- 高度由内容决定，不限制在固定高度内

### 4. 文档边距

```cpp
m_textBrowser->document()->setDocumentMargin(8);
```

**作用：**
- 在文本周围添加8像素的内边距
- 计算高度时需要加上 `margin * 2`

## 高度计算示例

### 示例1：短文本

**内容：** "你好，我是AI导师"

**计算过程：**
```
1. 文档宽度 = 视口宽度（例如 400px）
2. 文档高度 = 单行文本高度（例如 20px）
3. 文档边距 = 8px
4. QTextBrowser 高度 = 20 + 8*2 + 4 = 40px
5. Widget 最小高度 = 40 + 20 = 60px
```

### 示例2：长文本（需要换行）

**内容：** "这是一段很长的文本，会根据气泡宽度自动换行..."

**计算过程：**
```
1. 文档宽度 = 视口宽度（例如 400px）
2. 文本自动换行成3行
3. 文档高度 = 3行 × 行高（例如 3 × 20 = 60px）
4. 文档边距 = 8px
5. QTextBrowser 高度 = 60 + 8*2 + 4 = 80px
6. Widget 最小高度 = 80 + 20 = 100px
```

### 示例3：包含代码块

**内容：** 包含 Markdown 代码块

**计算过程：**
```
1. 文档宽度 = 视口宽度（例如 400px）
2. 代码块使用 <pre> 标签，保持原始格式
3. 代码块高度 = 代码行数 × 行高
4. 文档高度 = 普通文本高度 + 代码块高度
5. 加上边距和布局间距
6. 最终高度 = 所有内容高度之和
```

## 布局边距

### Widget 布局边距

```cpp
QVBoxLayout *layout = new QVBoxLayout(this);
layout->setContentsMargins(isUser ? 10 : 5, 10, isUser ? 5 : 10, 10);
```

**说明：**
- 用户消息：左10px，右5px，上下10px
- AI消息：左5px，右10px，上下10px
- 这些边距会影响最终的 Widget 高度

### 气泡绘制区域

```cpp
void ChatBubbleWidget::paintEvent(QPaintEvent *event)
{
    QRectF bubbleRect = rect().adjusted(m_isUser ? 5 : 0, 5, m_isUser ? 0 : -5, -5);
    // ...
}
```

**说明：**
- 气泡矩形比 Widget 矩形小一圈
- 用于绘制阴影和边框效果

## 响应式调整

### 1. 窗口宽度变化

```cpp
void ChatBubbleWidget::resizeEvent(QResizeEvent *event)
{
    // 宽度变化时重新计算高度
    if (event->oldSize().width() != event->size().width()) {
        adjustHeight();
    }
}
```

**流程：**
```
窗口宽度变化
  ↓
气泡宽度变化
  ↓
文本换行位置变化
  ↓
文档高度变化
  ↓
重新计算气泡高度
```

### 2. 字体缩放（Ctrl+滚轮）

```cpp
void ChatBubbleWidget::setFontScale(qreal scale)
{
    m_fontScale = scale;
    setContent(m_content);  // 重新渲染
}
```

**流程：**
```
Ctrl+滚轮
  ↓
改变 m_fontScale
  ↓
重新格式化内容（字体大小变化）
  ↓
文档高度变化
  ↓
重新计算气泡高度
```

## 行高和间距

### 用户消息

```cpp
QString formatUserMessage(const QString &content)
{
    int fontSize = qRound(11 * m_fontScale);
    return QString("<div style='color: #f0f0f0; font-size: %1pt; line-height: 1.5; letter-spacing: normal;'>%2</div>")
           .arg(fontSize).arg(escaped);
}
```

**行高：** `line-height: 1.5`（字体大小的1.5倍）

### AI消息

```cpp
QString formatMarkdown(const QString &content)
{
    int fontSize = qRound(11 * m_fontScale);
    return QString("<div style='color: #f0f0f0; font-size: %1pt; line-height: 1.5; letter-spacing: normal;'>%2</div>")
           .arg(fontSize).arg(result);
}
```

**行高：** `line-height: 1.5`（与用户消息一致）

### 代码块

```cpp
"<pre style='line-height: 1.4; white-space: pre-wrap; word-wrap: break-word;'>%1</pre>"
```

**行高：** `line-height: 1.4`（代码块稍微紧凑）

## 特殊元素的高度

### 1. 标题

```cpp
// 二级标题
"<div style='font-size: 12pt; font-weight: bold; margin: 10px 0 5px 0;'>\\1</div>"

// 三级标题
"<div style='font-size: 11.5pt; font-weight: bold; margin: 8px 0 4px 0;'>\\1</div>"
```

**额外高度：** 上下边距（margin）

### 2. 列表项

```cpp
// 有序列表
"<div style='margin: 2px 0 2px 12px;'>...</div>"

// 无序列表
"<div style='margin: 2px 0 2px 12px;'>...</div>"
```

**额外高度：** 上下2px边距

### 3. 代码块

```cpp
"<table style='margin: 6px 0; ...'>...</table>"
```

**额外高度：** 上下6px边距 + 表格内边距

## 高度计算的优势

### 1. 自适应内容
- ✅ 短消息占用少量空间
- ✅ 长消息自动扩展
- ✅ 代码块完整显示

### 2. 响应式设计
- ✅ 窗口大小变化时自动调整
- ✅ 字体缩放时重新计算
- ✅ 保持内容完整可见

### 3. 性能优化
- ✅ 只在必要时重新计算
- ✅ 使用 Qt 的文档布局引擎
- ✅ 避免不必要的重绘

## 潜在问题和解决方案

### 问题1：高度计算不准确

**原因：**
- 文档宽度设置不正确
- 边距计算遗漏

**解决方案：**
```cpp
// 确保文档宽度正确
doc->setTextWidth(m_textBrowser->viewport()->width());

// 包含所有边距
int height = docHeight + margin * 2 + 4;
```

### 问题2：内容被截断

**原因：**
- 使用了固定高度
- 启用了滚动条

**解决方案：**
```cpp
// 禁用滚动条
m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

// 使用自适应高度
m_textBrowser->setFixedHeight(calculatedHeight);
```

### 问题3：窗口大小变化时高度不更新

**原因：**
- 没有监听 resizeEvent

**解决方案：**
```cpp
void ChatBubbleWidget::resizeEvent(QResizeEvent *event)
{
    if (event->oldSize().width() != event->size().width()) {
        adjustHeight();
    }
}
```

## 调试技巧

### 1. 打印高度信息

```cpp
void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    int docHeight = doc->size().toSize().height();
    int margin = doc->documentMargin();
    int height = docHeight + margin * 2 + 4;
    
    qDebug() << "Viewport width:" << m_textBrowser->viewport()->width();
    qDebug() << "Document height:" << docHeight;
    qDebug() << "Margin:" << margin;
    qDebug() << "Final height:" << height;
    
    m_textBrowser->setFixedHeight(height);
    setMinimumHeight(height + 20);
}
```

### 2. 检查文档内容

```cpp
qDebug() << "Document HTML:" << m_textBrowser->toHtml();
qDebug() << "Document size:" << doc->size();
```

### 3. 验证尺寸策略

```cpp
qDebug() << "Size policy:" << sizePolicy().horizontalPolicy() << sizePolicy().verticalPolicy();
qDebug() << "Minimum size:" << minimumSize();
qDebug() << "Size hint:" << sizeHint();
```

## 总结

ChatBubbleWidget 的高度计算机制：

1. **核心原理**：使用 QTextDocument 的自动布局引擎计算内容高度
2. **触发时机**：内容变化、窗口大小变化、字体缩放
3. **关键设置**：自动换行、禁用滚动条、自适应尺寸策略
4. **响应式**：根据窗口宽度自动调整高度
5. **性能**：只在必要时重新计算，避免频繁重绘

这种机制确保了聊天气泡能够：
- ✅ 完整显示所有内容
- ✅ 自适应不同长度的消息
- ✅ 响应窗口大小变化
- ✅ 支持字体缩放
- ✅ 保持良好的性能
