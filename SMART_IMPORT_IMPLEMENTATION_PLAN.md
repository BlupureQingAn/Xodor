# 智能题库导入 - 实施计划

## 🎯 需求总结

根据您的要求，需要实现以下核心功能：

### 1. AI导入题库流程
- ✅ 拷贝题库文件夹到项目目录
- ✅ 智能拆分大文件（不切割题目）
- ✅ 逐文件AI解析
- ✅ 生成测试数据集
- ✅ 显示详细进度

### 2. 测试运行改进
- ✅ 类似LeetCode的测试反馈
- ✅ 显示每个用例的通过/失败
- ✅ 全部通过才算Accept
- ✅ 详细的错误信息

### 3. AI功能区分
- **导入时AI**：解析题目、生成测试数据
- **编辑时AI**：分析用户代码（已有）

## 📋 实施方案

由于这是一个较大的功能改进，我建议分阶段实施：

### 阶段1：最小可行产品（MVP）⭐
**目标**：快速实现核心功能，让系统可用

**包含功能**：
1. 文件拷贝到项目目录
2. 基础文件拆分（按大小）
3. AI解析题目信息
4. 基础测试数据生成（3-5个用例）
5. 简单进度显示
6. 改进的测试结果显示

**预计工作量**：2-3小时
**优先级**：🔥 高

### 阶段2：功能完善
**目标**：优化用户体验

**包含功能**：
1. 智能文件拆分（按题目边界）
2. 更多测试用例（边界、特殊情况）
3. 详细进度条和日志
4. LeetCode风格的测试界面
5. Accept/Wrong Answer状态

**预计工作量**：3-4小时
**优先级**：🟡 中

### 阶段3：高级特性
**目标**：提升专业性

**包含功能**：
1. 性能统计（时间、内存）
2. 测试用例编辑
3. 批量导入优化
4. 题目难度智能判断
5. 标签自动分类

**预计工作量**：2-3小时
**优先级**：🟢 低

## 🚀 立即可实施的改进

考虑到时间和复杂度，我建议先实施以下**快速改进**：

### 改进1：增强AI提示词（10分钟）
修改现有的AI提示词，要求生成更多测试用例：

```cpp
// 在 AIImportDialog.cpp 中修改 buildAIPrompt()
QString prompt = R"(
你是专业的编程题目解析助手。

任务：
1. 解析题目信息（标题、难度、描述、标签）
2. 生成完整测试数据集（至少5组，包含边界情况）

测试用例要求：
- 基本功能测试（2-3个）
- 边界条件（空输入、最小值、最大值）
- 特殊情况（负数、零、重复等）

输出JSON格式...
)";
```

### 改进2：改进测试结果显示（30分钟）
修改 `MainWindow::showTestResults()` 显示更详细的信息：

```cpp
// 显示每个测试用例的详细信息
for (int i = 0; i < results.size(); ++i) {
    const TestResult &result = results[i];
    
    resultText += QString("<div class='test-case %1'>")
        .arg(result.passed ? "pass" : "fail");
    
    resultText += QString("<div class='test-title'>测试用例 %1/%2: %3</div>")
        .arg(i + 1)
        .arg(total)
        .arg(result.passed ? "✓ 通过" : "✗ 失败");
    
    // 显示输入
    resultText += QString("<div class='test-detail'><b>输入：</b>%1</div>")
        .arg(result.input);
    
    // 显示期望输出
    resultText += QString("<div class='test-detail'><b>期望输出：</b>%1</div>")
        .arg(result.expectedOutput);
    
    if (!result.passed) {
        // 显示实际输出（失败时）
        resultText += QString("<div class='test-detail error'><b>实际输出：</b>%1</div>")
            .arg(result.actualOutput);
    }
    
    resultText += "</div>";
}

// 显示Accept或Wrong Answer
if (passed == total && total > 0) {
    resultText += "<div class='accept'>🎉 Accepted!</div>";
} else {
    resultText += "<div class='wrong-answer'>❌ Wrong Answer</div>";
}
```

### 改进3：添加文件拷贝功能（20分钟）
在导入时先拷贝文件夹：

```cpp
// 在 MainWindow::onImportQuestionBank() 中添加
QString targetPath = QString("data/question_banks/%1").arg(bankName);
QDir().mkpath(targetPath);

// 拷贝文件
QDir sourceDir(path);
QDir targetDir(targetPath);

for (const QFileInfo &fileInfo : sourceDir.entryInfoList(QDir::Files)) {
    QString sourcePath = fileInfo.absoluteFilePath();
    QString targetPath = targetDir.filePath(fileInfo.fileName());
    QFile::copy(sourcePath, targetPath);
}
```

## 💡 我的建议

考虑到您的需求和实现复杂度，我建议：

### 方案A：快速改进（推荐）⭐
**时间**：1小时
**内容**：
1. 增强AI提示词（生成更多测试用例）
2. 改进测试结果显示（详细反馈）
3. 添加Accept/Wrong Answer状态

**优点**：
- 快速见效
- 风险低
- 立即可用

### 方案B：完整实现
**时间**：6-8小时
**内容**：
1. 完整的智能导入系统
2. 文件拆分算法
3. 测试数据生成器
4. LeetCode风格界面

**优点**：
- 功能完整
- 体验最佳
- 长期价值高

**缺点**：
- 时间较长
- 需要大量测试
- 可能有bug

## 🎯 我的行动建议

我建议现在先实施**方案A（快速改进）**：

1. **立即改进**（30分钟）
   - 修改AI提示词
   - 改进测试结果显示
   - 添加Accept状态

2. **后续完善**（根据需要）
   - 文件拷贝功能
   - 智能拆分
   - 详细进度条

这样可以：
- ✅ 快速看到效果
- ✅ 降低风险
- ✅ 逐步完善

## 📝 您的选择

请告诉我您希望：

**选项1**：实施快速改进（1小时，立即可用）
**选项2**：完整实现（6-8小时，功能完整）
**选项3**：其他建议

我会根据您的选择开始实施！
