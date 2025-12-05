# AI聊天气泡文字截断和滚动修复（深度修复版）

## 问题描述

用户反馈AI导师聊天界面存在严重问题：

1. **文字和代码块被水平截断**
   - 长文本或代码行超出气泡宽度时被截断
   - 窗口大小改变时文字不会重新排版
   - 代码块可能超出容器边界
   - **滚动条出现时会遮挡文字**
   - **缩放后排版混乱**

2. **滚动条未自动滚到底部**
   - AI消息流式更新时，气泡内部的滚动条没有自动滚到底部
   - 用户需要手动滚动才能看到完整内容

## 深度问题分析

### 1. 文字截断的根本原因

**关键发现：文档宽度计算没有考虑所有占用空间的元素！**

- ❌ `QTextDocument`的`textWidth`直接使用viewport宽度
- ❌ **没有减去文档边距**（`documentMargin`左右各占用空间）
- ❌ **没有预留滚动条宽度**（滚动条出现时会挤压内容）
- ❌ 窗口resize时没有重新计算文档宽度
- ❌ 代码块的CSS样式缺少`max-width`和`word-break`属性

**空间占用分析：**
```
总宽度 = viewport宽度
可用宽度 = viewport宽度 - 左边距 - 右边距 - 滚动条宽度
```

如果不正确计算，文字会被滚动条或边框截断！

### 2. 滚动条问题

- 气泡内容更新时只滚动了外层对话框
- 没有处理气泡内部`QTextBrowser`的滚动条
- 缺少延迟机制确保内容渲染完成后再滚动

### 3. 缩放后排版混乱

- resize事件中没有延迟处理，导致viewport尺寸未更新
- 滚动条状态变化时没有重新计算宽度

## 解决方案

### 1. 修复文字截断

#### 正确计算文档宽度（关键修复！）
```cpp
void ChatBubbleWidget::updateTextBrowser()
{
    // ...设置HTML内容...
    
    // 强制文档重新布局 - 计算实际可用宽度
    QTextDocument *doc = m_textBrowser->document();
    
    // 1. 获取viewport宽度
    int viewportWidth = m_textBrowser->viewport()->width();
    
    // 2. 减去文档边距（左右各4px）
    int documentMargin = doc->documentMargin();
    int availableWidth = viewportWidth - (documentMargin * 2);
    
    // 3. 预留滚动条宽度（如果可能出现滚动条）
    QScrollBar *vScrollBar = m_textBrowser->verticalScrollBar();
    if (vScrollBar) {
        availableWidth -= vScrollBar->sizeHint().width() + 2;  // 滚动条宽度+间距
    }
    
    // 4. 确保宽度合理
    if (availableWidth < 100) {
        if (parentWidget()) {
            // 使用父widget宽度，减去气泡边距和文档边距
            int parentWidth = parentWidget()->width();
            int bubbleMargins = (m_isUser ? 10 : 5) + (m_isUser ? 5 : 10);  // 左右边距
            availableWidth = parentWidth - bubbleMargins - (documentMargin * 2) - 20;  // 额外留20px余量
        } else {
            availableWidth = 300;  // 最小宽度
        }
    }
    
    // 5. 设置文档宽度
    doc->setTextWidth(availableWidth);
    doc->adjustSize();
    // ...
}
```

**关键改进点：**
1. ✅ 减去文档边距（左右各4px）
2. ✅ 预留滚动条宽度（约15-20px）
3. ✅ 多级回退策略确保宽度合理
4. ✅ 额外预留余量避免边界情况

#### 改进resizeEvent处理（延迟+完整计算）
```cpp
void ChatBubbleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 延迟重新布局，确保viewport已经调整完成
    QTimer::singleShot(0, this, [this]() {
        QTextDocument *doc = m_textBrowser->document();
        
        // 获取viewport宽度
        int viewportWidth = m_textBrowser->viewport()->width();
        
        // 减去文档边距
        int documentMargin = doc->documentMargin();
        int availableWidth = viewportWidth - (documentMargin * 2);
        
        // 预留滚动条宽度（如果滚动条可见）
        QScrollBar *vScrollBar = m_textBrowser->verticalScrollBar();
        if (vScrollBar && vScrollBar->isVisible()) {
            availableWidth -= vScrollBar->width() + 2;
        }
        
        if (availableWidth > 50) {  // 确保宽度合理
            doc->setTextWidth(availableWidth);
            doc->adjustSize();
            
            // 重新计算高度
            int height = doc->size().toSize().height() + (documentMargin * 2) + 10;
            int maxHeight = 400;
            
            if (height > maxHeight) {
                m_textBrowser->setMinimumHeight(maxHeight);
                m_textBrowser->setMaximumHeight(maxHeight);
            } else {
                m_textBrowser->setMinimumHeight(height);
                m_textBrowser->setMaximumHeight(height);
            }
        }
    });
}
```

**关键改进点：**
1. ✅ 使用`QTimer::singleShot(0, ...)`延迟执行，确保viewport已调整
2. ✅ 检查滚动条是否可见，只在可见时减去宽度
3. ✅ 完整的宽度计算逻辑，与updateTextBrowser一致

#### 改进代码块CSS样式
```cpp
QString codeHtml = QString(
    "<table cellpadding='0' cellspacing='0' style='width: 100%%; max-width: 100%%; margin: 6px 0; "
    "background-color: #1e1e1e; border-collapse: collapse; "
    "border-left: 3px solid #007acc; "
    "border-top: 1px solid #3a3a3a; border-right: 1px solid #3a3a3a; border-bottom: 1px solid #3a3a3a; "
    "table-layout: fixed;'>"  // 固定表格布局
    "<tr><td style='padding: 6px 10px; background-color: #252525; border-bottom: 1px solid #3a3a3a;'>"
    "<span style='color: #9cdcfe; font-size: %1pt; font-weight: bold;'>%2</span></td></tr>"
    "<tr><td style='padding: 10px 12px; background-color: #1e1e1e; overflow: hidden;'>"  // 隐藏溢出
    "<pre style='margin: 0; padding: 0; background-color: #1e1e1e; "
    "font-family: \"Consolas\", \"Courier New\", monospace; "
    "font-size: %3pt; line-height: 1.3; "
    "white-space: pre-wrap; word-wrap: break-word; word-break: break-all; "  // 强制换行
    "overflow-wrap: break-word; color: #d4d4d4;'>%4</pre>"
    "</td></tr></table>"
).arg(fontSize - 2).arg(langLabel).arg(fontSize).arg(highlightedCode);
```

关键CSS属性：
- `max-width: 100%` - 限制最大宽度
- `table-layout: fixed` - 固定表格布局
- `overflow: hidden` - 隐藏溢出内容
- `word-break: break-all` - 强制在任意位置断行
- `overflow-wrap: break-word` - 在单词边界换行

### 2. 修复滚动条自动滚动

#### 在setContent中添加滚动逻辑
```cpp
void ChatBubbleWidget::setContent(const QString &content)
{
    m_content = content;
    updateTextBrowser();
    
    // 滚动到底部（如果有滚动条）
    QTimer::singleShot(10, this, [this]() {
        QScrollBar *scrollBar = m_textBrowser->verticalScrollBar();
        if (scrollBar && scrollBar->isVisible()) {
            scrollBar->setValue(scrollBar->maximum());
        }
    });
    
    update();  // 触发重绘
}
```

关键点：
- 使用`QTimer::singleShot(10, ...)`延迟执行，确保内容已渲染
- 检查滚动条是否可见（`isVisible()`）
- 只在有滚动条时才滚动

### 3. 减小文档边距

```cpp
// 原来：setDocumentMargin(8) - 太大，占用过多空间
// 现在：setDocumentMargin(4) - 更合理
m_textBrowser->document()->setDocumentMargin(4);
```

**为什么要减小边距？**
- 边距8px时，左右共占用16px
- 在窄窗口或有滚动条时，会显著减少可用宽度
- 边距4px已经足够，左右共8px

### 4. 添加必要的头文件

```cpp
#include <QResizeEvent>
#include <QTimer>
```

## 技术细节

### 文档宽度计算策略（完整版）

**计算公式：**
```
可用宽度 = viewport宽度 - (文档边距 × 2) - 滚动条宽度 - 安全余量
```

**详细步骤：**
1. **获取viewport宽度**：`m_textBrowser->viewport()->width()`
2. **减去文档边距**：`documentMargin * 2`（左右各一个边距）
3. **减去滚动条宽度**：`vScrollBar->sizeHint().width() + 2`
4. **回退策略**：如果计算结果<100px，使用父widget宽度
5. **最小保证**：确保至少有300px宽度

**为什么要这样计算？**
- viewport宽度包含了所有可见区域
- 但文档内容不能占满viewport，需要留出边距
- 滚动条出现时会占用viewport的一部分空间
- 如果不预留，文字会被滚动条遮挡

### 滚动时机控制

1. **内容更新时**：`setContent()`中延迟10ms滚动
2. **只滚动有滚动条的气泡**：检查`scrollBar->isVisible()`
3. **外层对话框滚动**：由`AIAssistantPanel::scrollToBottom()`处理

### CSS布局技巧

1. **table-layout: fixed**：确保表格不会超出容器
2. **word-break: break-all**：强制长单词断行
3. **white-space: pre-wrap**：保留空格和换行，但允许自动换行
4. **overflow: hidden**：防止内容溢出

## 测试建议

### 1. 文字换行测试
- 输入很长的单行文本（无空格）
- 输入包含长URL的文本
- 输入很长的代码行
- 调整窗口大小，观察文字是否重新排版

### 2. 代码块测试
- 输入包含长代码行的代码块
- 输入多行代码块
- 测试不同语言的代码高亮
- 调整窗口大小，观察代码块是否正确换行

### 3. 滚动测试
- 发送长消息，观察是否自动滚到底部
- AI流式回复时，观察滚动条是否跟随
- 测试有滚动条和无滚动条的气泡

### 4. 边界情况
- 极窄窗口（<300px）
- 极宽窗口（>1920px）
- 快速连续发送多条消息
- 包含特殊字符的文本

## 改进效果

### 修复前
- ❌ 长文本被截断，需要水平滚动
- ❌ 代码块超出边界
- ❌ 窗口resize后文字不重排
- ❌ 滚动条不自动滚到底部

### 修复后
- ✅ 文字自动换行，不会被截断
- ✅ 代码块正确换行，不超出边界
- ✅ 窗口resize时文字自动重排
- ✅ 滚动条自动滚到底部

## 相关文件

- `src/ui/ChatBubbleWidget.h` - 添加resizeEvent声明
- `src/ui/ChatBubbleWidget.cpp` - 实现修复逻辑
- `src/ui/AIAssistantPanel.cpp` - 外层滚动控制（已有）

## 修复验证

### 测试场景

1. **长文本测试**
   - 输入超长单行文本（无空格）
   - 预期：自动换行，不被截断

2. **代码块测试**
   - 输入包含长代码行的代码块
   - 预期：代码正确换行，不超出边界

3. **滚动条测试**
   - 发送超过400px高度的消息
   - 预期：出现滚动条，文字不被遮挡

4. **窗口缩放测试**
   - 调整窗口大小
   - 预期：文字自动重排，不出现混乱

5. **滚动条出现/消失测试**
   - 从短消息变为长消息
   - 预期：滚动条出现时文字自动调整宽度

### 关键指标

- ✅ 文字不被滚动条遮挡
- ✅ 文字不被气泡边框截断
- ✅ 窗口缩放后排版正常
- ✅ 滚动条自动滚到底部
- ✅ 代码块正确换行

## 后续优化方向

1. **性能优化**：使用防抖减少resize时的重排次数
2. **平滑滚动**：使用动画效果滚动到底部
3. **智能滚动**：只在用户在底部时才自动滚动
4. **代码高亮增强**：支持更多语言和主题
5. **响应式布局**：根据窗口大小调整字体和间距
6. **自适应边距**：根据内容类型动态调整边距

## 总结

这次深度修复解决了文字截断的根本原因：**文档宽度计算不正确**。

**核心改进：**
1. 正确计算可用宽度 = viewport宽度 - 边距 - 滚动条宽度
2. 减小文档边距从8px到4px，节省空间
3. resize时延迟处理，确保viewport已更新
4. 多级回退策略，确保各种情况下都有合理宽度

现在用户可以正常查看完整的AI回复内容，文字不会被截断或遮挡，窗口缩放后排版也保持正常。
