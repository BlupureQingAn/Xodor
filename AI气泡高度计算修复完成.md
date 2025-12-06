# AI气泡高度计算修复完成

## 问题描述
AI输出完成后，有滚动条的消息气泡突然变短，下面的信息被截断。

## 问题原因

### 根本原因
当气泡内容超过600px需要显示滚动条时，高度计算逻辑存在问题：

1. **第一次计算**：使用完整的viewport宽度计算文档高度
2. **限制高度**：发现超过600px，限制为600px
3. **显示滚动条**：滚动条出现，占用约10px宽度
4. **宽度变化**：viewport实际宽度减少，但文档宽度未更新
5. **内容被截断**：文档按旧宽度布局，但实际宽度更小，导致内容被截断

### 示意图
```
第一次计算（无滚动条）:
┌─────────────────────────┐
│  viewport width: 500px  │
│  内容按500px宽度布局    │
│  计算高度: 650px        │
└─────────────────────────┘

限制高度后（有滚动条）:
┌──────────────────────┬─┐
│ viewport: 490px      │█│ 滚动条10px
│ 但文档仍按500px布局  │█│
│ 导致右侧内容被截断   │█│
└──────────────────────┴─┘
```

## 解决方案

### 修改的文件
`src/ui/ChatBubbleWidget.cpp` - `adjustHeight()` 函数

### 修复逻辑

#### 1. 第一次计算
```cpp
doc->setTextWidth(m_textBrowser->viewport()->width());
int docHeight = qRound(layout->documentSize().height());
int textBrowserHeight = docHeight + margin * 2;
```

#### 2. 判断是否需要滚动条
```cpp
const int maxBubbleHeight = 600;
bool needsScrollBar = textBrowserHeight > maxBubbleHeight;
```

#### 3. 如果需要滚动条，重新计算
```cpp
if (needsScrollBar) {
    textBrowserHeight = maxBubbleHeight;
    
    // 重要：减去滚动条宽度，重新计算文档宽度
    int scrollBarWidth = 10;  // 8px滚动条 + 2px margin
    doc->setTextWidth(m_textBrowser->viewport()->width() - scrollBarWidth);
    
    // 重新计算高度（宽度变小，内容可能变高）
    docHeight = qRound(layout->documentSize().height());
    int newTextBrowserHeight = docHeight + margin * 2;
    
    // 确保不超过最大高度
    if (newTextBrowserHeight > maxBubbleHeight) {
        textBrowserHeight = maxBubbleHeight;
    } else {
        textBrowserHeight = newTextBrowserHeight;
    }
}
```

### 关键改进

1. **两次计算**：
   - 第一次：判断是否需要滚动条
   - 第二次：考虑滚动条宽度重新计算

2. **宽度调整**：
   - 减去滚动条宽度（10px）
   - 文档按新宽度重新布局

3. **高度验证**：
   - 重新计算后再次检查是否超过最大高度
   - 确保最终高度不超过600px

## 技术细节

### 滚动条宽度计算
```
滚动条样式设置：
- width: 8px
- margin: 2px

实际占用空间：8 + 2 = 10px
```

### 高度计算公式
```
文档高度 = documentSize().height()
文档边距 = documentMargin() = 4px
TextBrowser高度 = 文档高度 + 边距*2
Widget高度 = TextBrowser高度 + 布局边距(12px)
```

### 流程图
```
开始
  ↓
计算初始高度
  ↓
是否 > 600px? ──否──→ 直接设置高度
  ↓是
限制为600px
  ↓
减去滚动条宽度(10px)
  ↓
重新计算文档宽度
  ↓
重新计算高度
  ↓
是否 > 600px? ──是──→ 保持600px
  ↓否
使用新计算的高度
  ↓
设置最终高度
  ↓
结束
```

## 测试验证

### 测试场景1：短内容（< 600px）
- ✅ 不显示滚动条
- ✅ 高度自适应
- ✅ 内容完整显示

### 测试场景2：长内容（> 600px）
- ✅ 显示滚动条
- ✅ 高度限制为600px
- ✅ 内容不被截断
- ✅ 可以滚动查看全部内容

### 测试场景3：流式输出
- ✅ 输出过程中高度动态调整
- ✅ 超过600px后显示滚动条
- ✅ 输出完成后高度稳定
- ✅ 内容完整不截断

## 编译状态
✅ 编译成功，无错误无警告

## 完成时间
2024-12-06
