# 智能题库导入系统 - 设计文档

## 🎯 需求概述

### 核心需求
1. **文件管理**：拷贝题库到项目目录
2. **智能拆分**：大文件按题目边界拆分
3. **AI解析**：逐文件AI解析题目
4. **测试生成**：自动生成完整测试数据集
5. **进度显示**：详细的进度条和日志
6. **测试运行**：类似LeetCode的测试反馈

### 功能区分
- **导入时AI**：解析题目结构、生成测试数据
- **编辑时AI**：分析用户代码（已有功能）

## 📋 详细设计

### 1. 导入流程

```
用户选择题库文件夹
    ↓
第一步：拷贝到项目目录
    ├─ 源路径：用户选择的文件夹
    └─ 目标路径：data/question_banks/<题库名>/
    ↓
第二步：扫描文件
    ├─ 查找所有 .md 文件
    ├─ 统计文件大小和行数
    └─ 记录文件列表
    ↓
第三步：智能拆分
    ├─ 小文件（<8000字符）：不拆分
    ├─ 大文件（>=8000字符）：按题目边界拆分
    │   ├─ 识别题目分隔符（# 标题）
    │   ├─ 确保不切割题目
    │   └─ 生成多个文件块
    └─ 生成文件块列表
    ↓
第四步：AI解析（逐块处理）
    ├─ 构建AI提示词
    ├─ 发送给Ollama
    ├─ 等待响应
    ├─ 解析JSON结果
    └─ 提取题目信息
    ↓
第五步：生成测试数据
    ├─ 基于题目描述
    ├─ 生成多组测试用例
    ├─ 包含边界情况
    └─ 保存到题目对象
    ↓
第六步：保存题库
    ├─ 保存为JSON格式
    ├─ 更新题库索引
    └─ 完成导入
```

### 2. 文件拆分算法

```cpp
QVector<FileChunk> splitLargeFile(const QString &content) {
    QVector<FileChunk> chunks;
    
    if (content.length() < MAX_CHUNK_SIZE) {
        // 小文件，不拆分
        chunks.append({content, 0, 1});
        return chunks;
    }
    
    // 大文件，按题目边界拆分
    QStringList lines = content.split('\n');
    QString currentChunk;
    int chunkIndex = 0;
    
    for (const QString &line : lines) {
        // 检查是否是题目边界
        if (isQuestionBoundary(line) && 
            currentChunk.length() > MIN_CHUNK_SIZE) {
            // 保存当前块
            chunks.append({currentChunk, chunkIndex++, -1});
            currentChunk = line + "\n";
        } else {
            currentChunk += line + "\n";
        }
        
        // 如果当前块太大，强制分割
        if (currentChunk.length() > MAX_CHUNK_SIZE) {
            chunks.append({currentChunk, chunkIndex++, -1});
            currentChunk.clear();
        }
    }
    
    // 保存最后一块
    if (!currentChunk.isEmpty()) {
        chunks.append({currentChunk, chunkIndex, -1});
    }
    
    // 更新总块数
    for (auto &chunk : chunks) {
        chunk.totalChunks = chunks.size();
    }
    
    return chunks;
}

bool isQuestionBoundary(const QString &line) {
    // 识别题目分隔符
    // 1. 一级标题：# 题目名
    // 2. 题号：1. 题目名 或 第1题
    // 3. 分隔线：--- 或 ===
    
    QString trimmed = line.trimmed();
    
    // 一级标题
    if (trimmed.startsWith("# ") && !trimmed.startsWith("## ")) {
        return true;
    }
    
    // 题号格式
    if (QRegularExpression(R"(^\d+[\.\)、]\s+)").match(trimmed).hasMatch()) {
        return true;
    }
    
    if (QRegularExpression(R"(^第\d+题)").match(trimmed).hasMatch()) {
        return true;
    }
    
    // 分隔线（至少3个字符）
    if (trimmed.length() >= 3 && 
        (trimmed == QString(trimmed.length(), '-') ||
         trimmed == QString(trimmed.length(), '='))) {
        return true;
    }
    
    return false;
}
```

### 3. AI提示词设计

```
你是一个专业的编程题目解析和测试用例生成助手。

任务：
1. 解析题目信息（标题、难度、描述、标签）
2. 生成完整的测试数据集（至少5组）

要求：
- 测试用例要覆盖：
  * 基本功能测试
  * 边界条件（空输入、最小值、最大值）
  * 特殊情况（负数、零、重复等）
  * 性能测试（大数据量）
- 难度判断标准：
  * 简单：基础语法、简单逻辑
  * 中等：数据结构、算法应用
  * 困难：复杂算法、优化要求
- 标签要准确：数组、字符串、动态规划、贪心等

输出格式（JSON）：
{
  "questions": [
    {
      "title": "两数之和",
      "difficulty": "简单",
      "description": "给定一个整数数组...",
      "tags": ["数组", "哈希表"],
      "testCases": [
        {
          "input": "[2,7,11,15]\n9",
          "output": "[0,1]",
          "description": "基本测试"
        },
        {
          "input": "[3,2,4]\n6",
          "output": "[1,2]",
          "description": "不同位置"
        },
        {
          "input": "[3,3]\n6",
          "output": "[0,1]",
          "description": "重复元素"
        },
        {
          "input": "[1,2,3,4,5]\n10",
          "output": "[]",
          "description": "无解情况"
        },
        {
          "input": "[-1,-2,-3,-4,-5]\n-8",
          "output": "[2,4]",
          "description": "负数测试"
        }
      ]
    }
  ]
}

文件内容：
---
<文件块内容>
---

请开始解析。
```

### 4. 进度显示设计

```
┌─────────────────────────────────────────────────┐
│ 🤖 AI智能导入题库                              │
├─────────────────────────────────────────────────┤
│                                                 │
│ 当前状态：正在处理文件 3/10                     │
│ 文件名：leetcode_array.md (块 2/3)             │
│                                                 │
│ [████████████████░░░░░░░░░░] 65%               │
│                                                 │
│ 📊 统计信息：                                   │
│ • 已处理文件：3/10                              │
│ • 已处理块：8/15                                │
│ • 已导入题目：25                                │
│                                                 │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│                                                 │
│ 📋 处理日志：                                   │
│ ┌─────────────────────────────────────────────┐ │
│ │ ✓ 拷贝题库到项目目录                        │ │
│ │ ✓ 扫描到 10 个文件                          │ │
│ │ ✓ 文件拆分完成，共 15 个块                  │ │
│ │ ⏳ 正在处理：leetcode_array.md (块 2/3)    │ │
│ │   - 发送AI请求...                           │ │
│ │   - AI响应接收完成                          │ │
│ │   - 解析到 3 道题目                         │ │
│ │   - 生成测试用例...                         │ │
│ │   ✓ 两数之和 [简单] - 5个测试用例           │ │
│ │   ✓ 三数之和 [中等] - 6个测试用例           │ │
│ │   ✓ 四数之和 [中等] - 5个测试用例           │ │
│ └─────────────────────────────────────────────┘ │
│                                                 │
│              [取消]              [完成]         │
└─────────────────────────────────────────────────┘
```

### 5. 测试运行改进（类似LeetCode）

```
运行测试
    ↓
编译代码
    ↓
逐个运行测试用例
    ├─ 测试用例 1/5
    │   ├─ 输入：[2,7,11,15], 9
    │   ├─ 期望输出：[0,1]
    │   ├─ 实际输出：[0,1]
    │   └─ 结果：✓ 通过
    ├─ 测试用例 2/5
    │   ├─ 输入：[3,2,4], 6
    │   ├─ 期望输出：[1,2]
    │   ├─ 实际输出：[1,2]
    │   └─ 结果：✓ 通过
    ├─ 测试用例 3/5
    │   ├─ 输入：[3,3], 6
    │   ├─ 期望输出：[0,1]
    │   ├─ 实际输出：[0,1]
    │   └─ 结果：✓ 通过
    ├─ 测试用例 4/5
    │   ├─ 输入：[1,2,3,4,5], 10
    │   ├─ 期望输出：[]
    │   ├─ 实际输出：[2,3]
    │   └─ 结果：✗ 失败
    └─ 测试用例 5/5
        ├─ 输入：[-1,-2,-3,-4,-5], -8
        ├─ 期望输出：[2,4]
        ├─ 实际输出：[2,4]
        └─ 结果：✓ 通过
    ↓
显示结果
    ├─ 通过：4/5 (80%)
    ├─ 失败：1/5
    └─ 状态：Wrong Answer
```

### 6. 测试结果显示

```
┌─────────────────────────────────────────────────┐
│ 📊 测试结果                                     │
├─────────────────────────────────────────────────┤
│                                                 │
│ ❌ Wrong Answer                                 │
│                                                 │
│ 通过率：4/5 (80%)                               │
│                                                 │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│                                                 │
│ ✓ 测试用例 1：基本测试                         │
│   输入：[2,7,11,15], 9                          │
│   输出：[0,1] ✓                                 │
│                                                 │
│ ✓ 测试用例 2：不同位置                         │
│   输入：[3,2,4], 6                              │
│   输出：[1,2] ✓                                 │
│                                                 │
│ ✓ 测试用例 3：重复元素                         │
│   输入：[3,3], 6                                │
│   输出：[0,1] ✓                                 │
│                                                 │
│ ✗ 测试用例 4：无解情况                         │
│   输入：[1,2,3,4,5], 10                         │
│   期望输出：[]                                  │
│   实际输出：[2,3]                               │
│   ❌ 输出不匹配                                 │
│                                                 │
│ ✓ 测试用例 5：负数测试                         │
│   输入：[-1,-2,-3,-4,-5], -8                    │
│   输出：[2,4] ✓                                 │
│                                                 │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│                                                 │
│ 💡 提示：                                       │
│ • 检查无解情况的处理逻辑                        │
│ • 确保返回空数组而不是错误结果                  │
│                                                 │
│                          [查看AI分析]  [关闭]  │
└─────────────────────────────────────────────────┘
```

### 7. Accept状态

```
┌─────────────────────────────────────────────────┐
│ 🎉 Accepted!                                    │
├─────────────────────────────────────────────────┤
│                                                 │
│ ✅ 所有测试用例通过！                           │
│                                                 │
│ 通过率：5/5 (100%)                              │
│                                                 │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│                                                 │
│ 📊 统计信息：                                   │
│ • 执行时间：15ms                                │
│ • 内存使用：2.3MB                               │
│ • 代码行数：25行                                │
│                                                 │
│ 🏆 恭喜！你已经完成了这道题！                   │
│                                                 │
│                    [下一题]  [查看题解]        │
└─────────────────────────────────────────────────┘
```

## 🔧 技术实现

### 核心类

#### 1. SmartQuestionImporter
```cpp
class SmartQuestionImporter : public QObject {
    // 智能导入器
    // 负责整个导入流程的协调
    
    void startImport(source, target);
    void processNextChunk();
    void parseAIResponse();
    void generateTestCases();
};
```

#### 2. TestCaseGenerator
```cpp
class TestCaseGenerator {
    // 测试用例生成器
    // 基于题目描述生成测试数据
    
    QVector<TestCase> generate(const Question &q);
    TestCase generateBasicCase();
    TestCase generateBoundaryCase();
    TestCase generateEdgeCase();
};
```

#### 3. EnhancedTestRunner
```cpp
class EnhancedTestRunner : public QObject {
    // 增强的测试运行器
    // 类似LeetCode的测试反馈
    
    void runAllTests(code, testCases);
    TestResult runSingleTest(code, testCase);
    void showDetailedResults();
};
```

### 数据结构

#### TestCase扩展
```cpp
struct TestCase {
    QString input;
    QString expectedOutput;
    QString description;      // 新增：测试用例描述
    QString category;         // 新增：类别（基本/边界/特殊）
    int timeLimit;           // 新增：时间限制（ms）
    int memoryLimit;         // 新增：内存限制（MB）
};
```

#### TestResult扩展
```cpp
struct TestResult {
    bool passed;
    QString input;
    QString expectedOutput;
    QString actualOutput;
    QString error;
    int executionTime;       // 新增：执行时间
    int memoryUsed;          // 新增：内存使用
    QString category;        // 新增：测试类别
};
```

## 📁 文件结构

```
src/
├── ai/
│   ├── SmartQuestionImporter.h/cpp    # 智能导入器
│   ├── TestCaseGenerator.h/cpp        # 测试用例生成器
│   └── QuestionParser.cpp             # 现有解析器（保留）
├── core/
│   ├── EnhancedTestRunner.h/cpp       # 增强测试运行器
│   ├── Question.h/cpp                 # 扩展TestCase结构
│   └── CompilerRunner.cpp             # 现有编译器（扩展）
└── ui/
    ├── SmartImportDialog.h/cpp        # 新的导入对话框
    ├── TestResultDialog.h/cpp         # 测试结果对话框
    └── MainWindow.cpp                 # 集成新功能
```

## 🎯 实现优先级

### Phase 1：核心导入流程（高优先级）
- [x] 文件拷贝功能
- [ ] 文件扫描和拆分
- [ ] AI逐块解析
- [ ] 进度显示

### Phase 2：测试数据生成（高优先级）
- [ ] 测试用例生成器
- [ ] AI辅助生成测试数据
- [ ] 测试数据验证

### Phase 3：测试运行改进（中优先级）
- [ ] 增强的测试运行器
- [ ] 详细的测试结果显示
- [ ] Accept/Wrong Answer状态

### Phase 4：UI优化（中优先级）
- [ ] 新的导入对话框
- [ ] 测试结果对话框
- [ ] 进度条和日志

### Phase 5：高级功能（低优先级）
- [ ] 性能统计（时间、内存）
- [ ] 测试用例编辑
- [ ] 批量测试

## 🚀 使用流程

### 用户视角

1. **导入题库**
   ```
   文件 → 导入题库 → AI智能导入
   → 选择文件夹
   → 输入题库名称
   → 开始导入
   → 查看进度
   → 导入完成
   ```

2. **刷题**
   ```
   选择题目
   → 编写代码
   → 运行测试
   → 查看详细结果
   → 修改代码
   → 再次测试
   → Accept！
   ```

3. **AI分析**
   ```
   编写代码
   → 点击"AI分析"
   → 查看代码分析
   → 优化建议
   → 改进代码
   ```

## 📊 预期效果

### 导入效率
- 10个文件 → 约2-3分钟
- 100道题目 → 约5-10分钟
- 自动生成500+测试用例

### 测试体验
- 类似LeetCode的反馈
- 清晰的错误提示
- 详细的测试结果
- Accept成就感

### 用户满意度
- 导入更简单
- 测试更完善
- 反馈更清晰
- 体验更流畅

## 🎉 总结

这个设计提供了一个完整的智能题库导入和测试系统，核心特点：

1. **智能化**：AI自动解析和生成测试数据
2. **可视化**：详细的进度和结果显示
3. **专业化**：类似LeetCode的测试体验
4. **用户友好**：清晰的反馈和提示

实现后将大大提升用户的刷题体验！
