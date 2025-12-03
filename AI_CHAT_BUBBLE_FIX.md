# AI导师对话气泡优化

## 问题描述
1. AI导师界面的对话气泡消失
2. 字体大小不合适（太大）
3. AI回复没有左对齐
4. 对话气泡不能适应面板大小调整
5. 代码内容没有使用代码块格式

## 优化方案

### 1. 响应式气泡设计
- 使用 `max-width: 75%` 替代固定边距，气泡自动适应面板宽度
- 用户消息：蓝色渐变气泡，右对齐
- AI消息：绿色渐变气泡，左对齐
- 圆角：18px（更现代的设计）
- 字体大小：10pt（更舒适的阅读体验）

### 2. 代码块支持
新增 `formatMessageContent()` 函数处理：
- 多行代码块：```language\ncode\n```
  - 深色背景 (#1a1a1a)
  - 语言标签显示
  - 等宽字体 (Consolas)
  - 自动换行和滚动
- 行内代码：`code`
  - 灰色背景高亮
  - 等宽字体

### 3. 流式输出优化
- 每次接收数据时重新渲染完整HTML
- 支持代码块的实时渲染
- 保持滚动位置在底部

### 4. 样式统一
所有消息（新消息、历史消息）使用相同的渲染逻辑：
- 时间戳：9pt灰色字体
- 内容：10pt白色字体
- 行高：1.6（更好的可读性）
- 自动换行：word-wrap: break-word

## 修改文件
- `src/ui/AIAssistantPanel.cpp`: 
  - 重构消息渲染逻辑
  - 添加代码块格式化支持
  - 优化响应式布局
- `src/ui/AIAssistantPanel.h`: 
  - 添加 `formatMessageContent()` 方法声明

## 技术细节

### 代码块正则表达式
```cpp
// 多行代码块
QRegularExpression codeBlockRegex("```([^\\n]*)\\n([\\s\\S]*?)```");

// 行内代码
QRegularExpression("`([^`]+)`")
```

### HTML结构
```html
<div style='text-align: left/right;'>
  <div style='display: inline-block; max-width: 75%;'>
    <div>时间戳</div>
    <div>内容（支持代码块）</div>
  </div>
</div>
```

## 测试建议
1. 发送普通文本消息，检查气泡样式和对齐
2. 发送包含代码的消息，验证代码块渲染
3. 调整面板宽度，确认气泡自适应
4. 测试流式输出时的实时渲染效果
5. 切换题目检查历史记录加载
