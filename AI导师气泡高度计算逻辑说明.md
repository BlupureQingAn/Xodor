# AI导师气泡高度计算逻辑说明

## 问题描述

用户反馈：预设按钮（如"知识点"）点击后的用户消息气泡底部有多余空白，且一行内字数越多气泡高度越高。

## 调试分析

通过添加调试日志，发现以下情况：

### 测试结果

1. **输入 "123456789" (9个字符)**
   - 实际行数：1行
   - 气泡高度：39px
   - 结论：正常显示

2. **输入 "1234567890·123" (13个字符)**
   - 实际行数：2行
   - 气泡高度：58px
   - 结论：文本自动换行到第二行

## 根本原因

**这不是bug，而是正常的文本换行行为**：

1. 气泡宽度有限（受布局约束）
2. 当文本长度超过气泡宽度时，QTextBrowser会自动换行
3. 换行后行数增加，气泡高度相应增加
4. 高度计算公式：`高度 = 行数 × 固定行高 + 边距`

## 当前实现

### 高度计算逻辑（`adjustHeight()`）

```cpp
// 1. 遍历所有文本块，统计实际行数
int totalLines = 0;
QTextBlock block = doc->begin();
while (block.isValid()) {
    QTextLayout *textLayout = block.layout();
    if (textLayout) {
        totalLines += textLayout->lineCount();
    }
    block = block.next();
}

// 2. 使用固定行高计算
int fontSize = qRound(11 * m_fontScale);
int fixedLineHeight = qRound(fontSize * 1.5);

// 3. 计算总高度
int textBrowserHeight = totalLines * fixedLineHeight + margin * 2 + 8;
```

### 关键特性

1. **自动换行**：`setLineWrapMode(QTextEdit::WidgetWidth)`
2. **固定行高**：`fontSize * 1.5`，确保每行高度一致
3. **精确计算**：使用 `QTextLayout::lineCount()` 获取实际行数
4. **滚动条支持**：超过600px显示内部滚动条

## 解决方案（如果用户仍认为是问题）

如果用户希望更多文字能在一行内显示，可以考虑：

### 方案1：增加气泡最小宽度
```cpp
// 在 ChatBubbleWidget 构造函数中
setMinimumWidth(400);  // 增加最小宽度
```

### 方案2：减小字体大小
```cpp
// 修改 formatUserMessage 中的字体大小
int fontSize = qRound(10 * m_fontScale);  // 从11改为10
```

### 方案3：调整布局边距
```cpp
// 减小左右边距，给文本更多空间
layout->setContentsMargins(isUser ? 8 : 5, 6, isUser ? 5 : 8, 6);
```

## 结论

当前的气泡高度计算逻辑是**正确且精确**的：
- 文本确实换行了（从1行变成2行）
- 气泡高度根据实际行数计算
- 没有多余的空白或错误的高度

如果用户觉得文本换行太早，这是**布局宽度限制**的问题，而不是高度计算的bug。
