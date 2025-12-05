# AI导师输出优化

## 问题描述
1. AI导师回复太长，说太多废话
2. Markdown格式处理不完整，有未处理的符号
3. 需要更简洁、更直接的回答风格

## 解决方案

### 1. System Prompt优化
**位置**：`src/ui/AIAssistantPanel.cpp` - `buildSystemPrompt()`

**修改前的问题**：
- Prompt太长，包含大量指导性文字
- 强调"温柔耐心"导致回复冗长
- 没有明确的长度限制

**修改后的特点**：
```
【核心原则】
1. 🎯 准确第一：确保技术建议完全正确
2. 💡 简洁直接：少说废话，多给代码
3. 🔍 仔细验证：分析代码前必须手动模拟执行

【回答要求】
⚠️ 重要限制：
- 回答控制在200字以内（代码不计入字数）
- 直接指出问题，不要铺垫
- 用代码示例说明，不要长篇大论
- 一次只讲一个核心问题

【回答格式】
1. 一句话说明问题（如果有）
2. 给出修改后的代码
3. 一句话解释原因
```

**示例对比**：

修改前：
```
你好！我来帮你看看代码。首先，我注意到你的代码整体结构不错，
说明你对基本语法有一定的理解。不过，我发现了一个小问题...
（继续长篇大论）
```

修改后：
```
循环条件错了，应该是 `i < n`：
```cpp
for (int i = 0; i < n; i++) {  // 修改这里
    // ...
}
```
数组下标从0到n-1。
```

### 2. 输出长度限制
**位置**：`src/ai/OllamaClient.cpp` - `sendChatMessage()`

**本地模式（Ollama）**：
```cpp
QJsonObject options;
options["num_predict"] = 500;  // 限制生成的token数量
json["options"] = options;
```

**云端模式（OpenAI兼容）**：
```cpp
json["max_tokens"] = 500;  // 限制输出长度
```

**说明**：
- 500 tokens ≈ 200-300个中文字
- 代码块不计入限制（由AI自行控制）
- 既保证回答完整，又避免冗长

### 3. 增强Markdown格式处理
**位置**：`src/ui/ChatBubbleWidget.cpp` - `formatMarkdown()`

**新增支持的格式**：

#### 粗体
- `**text**` → 已支持
- `__text__` → 新增支持

#### 斜体
- `*text*` → 已支持
- `_text_` → 新增支持（避免与下划线冲突）

#### 删除线
- `~~text~~` → 新增支持

**实现代码**：
```cpp
// 粗体（支持 **text** 和 __text__）
result.replace(QRegularExpression("\\*\\*([^\\*]+)\\*\\*"), 
              "<b style='color: #ffd700; letter-spacing: normal;'>\\1</b>");
result.replace(QRegularExpression("__([^_]+)__"), 
              "<b style='color: #ffd700; letter-spacing: normal;'>\\1</b>");

// 斜体（支持 *text* 和 _text_）
result.replace(QRegularExpression("\\*([^\\*]+)\\*"), 
              "<i style='color: #e8e8e8;'>\\1</i>");
result.replace(QRegularExpression("(?<!_)_([^_]+)_(?!_)"), 
              "<i style='color: #e8e8e8;'>\\1</i>");

// 删除线（支持 ~~text~~）
result.replace(QRegularExpression("~~([^~]+)~~"), 
              "<s style='color: #888;'>\\1</s>");
```

**正则表达式说明**：
- `(?<!_)` - 负向后查找，确保前面不是下划线
- `(?!_)` - 负向前查找，确保后面不是下划线
- 避免 `__text__` 被误识别为两个斜体

## 优势

### 回答质量
1. **更简洁**：200字限制，直击要点
2. **更实用**：多代码少废话，学生更容易理解
3. **更高效**：一次只讲一个问题，避免信息过载

### 用户体验
1. **快速获取答案**：不用看长篇大论
2. **代码优先**：直接看修改方案
3. **节省时间**：减少阅读负担

### 技术实现
1. **API层面限制**：通过`num_predict`/`max_tokens`控制
2. **Prompt层面引导**：明确要求简洁
3. **格式处理完善**：支持更多Markdown语法

## 测试建议

### 测试场景
1. **代码分析**：看AI是否直接指出问题
2. **思路提示**：看AI是否简洁给出思路
3. **知识点讲解**：看AI是否控制在200字内

### 预期效果
- 回复长度：100-300字（不含代码）
- 代码示例：清晰、完整、可运行
- 格式显示：所有Markdown符号正确渲染

## 修改文件
- `src/ui/AIAssistantPanel.cpp` - 优化System Prompt
- `src/ai/OllamaClient.cpp` - 添加输出长度限制
- `src/ui/ChatBubbleWidget.cpp` - 增强Markdown处理

### 4. 数学公式支持
**位置**：`src/ui/ChatBubbleWidget.cpp` - `formatMarkdown()`

**支持的格式**：

#### 行内公式
- `$formula$` → 斜体、金色、浅灰背景
- 示例：`$x^2 + y^2 = r^2$`

#### 块级公式
- `$$formula$$` → 独立块、左侧金色边框
- 示例：
```
$$
E = mc^2
$$
```

**实现特点**：
1. 在HTML转义之前处理，避免公式符号被转义
2. 使用占位符机制，避免与其他格式冲突
3. 块级公式：深色背景 + 金色左边框
4. 行内公式：浅灰背景 + 金色文字
5. 使用 Times New Roman 字体，更适合数学符号

**正则表达式**：
```cpp
// 块级公式：$$...$$
QRegularExpression blockMathRegex("\\$\\$([^$]+)\\$\\$");

// 行内公式：$...$（但不匹配$$）
QRegularExpression inlineMathRegex("(?<!\\$)\\$([^$\\n]+)\\$(?!\\$)");
```

**渲染效果**：
- 行内：`$x + y$` → <span style="color: #ffd700; background: #2a2a2a; font-style: italic;">x + y</span>
- 块级：独立一行，左侧金色边框，更醒目

**注意**：
- 这是简单的文本渲染，不是真正的LaTeX渲染
- 适合简单的数学表达式
- 复杂公式建议使用代码块展示

## 完成时间
2024-12-06
