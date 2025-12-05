# 测试用例修复工具 UI 简化

## 修复内容

隐藏了 TestCaseFixerDialog 中的 AI 响应详细内容，改为后台处理并显示简洁的处理日志。

## 主要改进

### 1. UI 优化
- **日志区域高度**：从 150px 减小到 120px，更加紧凑
- **不显示完整提示词**：调用 AI 时只显示简洁的状态信息
- **不显示 AI 原始响应**：流式响应时只显示进度，不显示完整内容

### 2. AI 响应处理优化

#### onFixWithAI()
```cpp
// 修改前：显示完整提示词
m_aiResponseView->setPlainText(QString("正在调用AI...\n\n提示词：\n%1\n\n等待AI响应...").arg(prompt));

// 修改后：简洁的状态显示
m_aiResponseView->setPlainText(QString("🚀 正在调用AI...\n\n准备修复 %1 个测试用例\n\n请稍候...").arg(m_problematicIndices.size()));
```

#### onAIChunk()
```cpp
// 修改前：实时显示完整 AI 响应
m_aiResponseView->setPlainText(QString("AI响应：\n\n%1").arg(m_currentAIResponse));

// 修改后：只显示进度信息
m_aiResponseView->setPlainText(QString("🤖 AI正在处理中...\n\n已接收：%1 字符\n\n请稍候，AI正在分析测试用例并生成修复方案。").arg(responseLength));
```

#### onAIFinished()
```cpp
// 修改前：不解析响应，只显示完成状态
m_statusLabel->setText("状态：AI修复完成，请检查并保存");

// 修改后：解析响应并显示修复方案数量
if (match.hasMatch()) {
    int fixCount = doc.array().size();
    logMessage = QString("✅ AI修复完成！\n\n生成了 %1 个测试用例的修复方案。\n\n请点击'保存修复'按钮应用修复。").arg(fixCount);
}
```

#### onSaveFixed()
```cpp
// 修改前：从 UI 文本框读取响应
QString aiResponse = m_aiResponseView->toPlainText();
QRegularExpressionMatch match = jsonRegex.match(aiResponse);

// 修改后：从后台保存的完整响应读取
QRegularExpressionMatch match = jsonRegex.match(m_currentAIResponse);
```

### 3. 用户体验改进

**修改前的流程：**
1. 点击"AI修复" → 显示完整提示词（可能很长）
2. AI 响应中 → 实时显示所有 AI 输出（包括思考过程）
3. 完成后 → 用户需要手动检查 JSON 格式

**修改后的流程：**
1. 点击"AI修复" → 显示简洁状态："准备修复 N 个测试用例"
2. AI 响应中 → 显示进度："已接收 X 字符"，状态栏显示动画点
3. 完成后 → 自动解析并显示："生成了 N 个修复方案"

## 技术细节

### 后台数据保存
- `m_currentAIResponse`：完整保存所有 AI 响应内容
- UI 只显示处理状态和结果摘要
- 保存时从 `m_currentAIResponse` 提取 JSON，而不是从 UI 文本框

### 状态动画
```cpp
static int dotCount = 0;
dotCount = (dotCount + 1) % 4;
QString dots = QString(".").repeated(dotCount);
m_statusLabel->setText(QString("状态：AI正在生成修复方案%1").arg(dots));
```

### 智能结果解析
- 自动检测 JSON 格式
- 显示修复方案数量
- 提供友好的错误提示

## 优势

1. **界面更简洁**：不显示冗长的 AI 原始输出
2. **用户体验更好**：清晰的状态提示和进度显示
3. **功能不受影响**：后台完整保存所有数据，保存功能正常
4. **更专业**：类似现代 AI 工具的交互方式

## 测试建议

1. 测试 AI 修复流程是否正常
2. 检查状态提示是否清晰
3. 验证保存功能是否正常工作
4. 确认错误处理是否友好

## 相关文件

- `src/ui/TestCaseFixerDialog.cpp` - 主要修改文件
- `src/ui/TestCaseFixerDialog.h` - 无需修改
