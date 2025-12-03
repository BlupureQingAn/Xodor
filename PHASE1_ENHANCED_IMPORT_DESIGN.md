# 阶段一：通用题库导入增强 - 详细设计

## 📋 设计目标

实现一个**通用的**题库导入系统，支持任何编程题库格式，包括：
- 原始题库只读备份
- 智能格式识别（不限于 CCF）
- AI 扩充测试数据
- 题库规律分析

## 🗂️ 目录结构设计

```
项目根目录/data/
├── original_banks/          # 原始题库（只读备份）
│   ├── leetcode/
│   ├── ccf/
│   └── custom_bank_1/
├── processed_banks/         # 基础题库（解析后）
│   ├── leetcode/
│   │   ├── array/
│   │   │   ├── two_sum.md
│   │   │   └── ...
│   │   └── bank_analysis.json
│   └── ccf/
│       ├── exam_1/
│       └── bank_analysis.json
├── mock_banks/              # 模拟题库（AI 生成）
│   └── leetcode/
│       ├── mock_exam_1/
│       └── mock_exam_2/
└── user_data/
    ├── code_backup/
    └── answer_records/
```

## 🔧 核心类设计

### 1. QuestionBankType（题库类型枚举）

```cpp
enum class QuestionBankType {
    Original,    // 原始题库
    Processed,   // 基础题库
    Mock         // 模拟题库
};
```

### 2. UniversalQuestionParser（通用题目解析器）

**职责**：智能识别和解析各种格式的题目

**关键方法**：
```cpp
class UniversalQuestionParser {
public:
    // 分析文件格式，自动识别题目结构
    ParsePattern analyzeFormat(const QString &content);
    
    // 按识别的模式解析题目
    QVector<Question> parseContent(const QString &content, const ParsePattern &pattern);
    
    // 智能拆分单文件内的多道题目
    QStringList splitMultipleQuestions(const QString &content);
    
    // 提取测试数据（智能配对输入输出）
    QVector<TestCase> extractTestCases(const QString &content);
    
    // 提取题目元信息（难度、标签、限制等）
    QuestionMetadata extractMetadata(const QString &content);
};
```

**支持的格式特征**：
- 题目标识：`# 题目`、`## 题目描述`、`第X题`、`Problem X`
- 测试数据：`输入：`、`Input:`、`示例 1`、`Example 1`
- 难度：`简单`、`中等`、`困难`、`Easy`、`Medium`、`Hard`
- 限制：`时间限制`、`内存限制`、`Time Limit`、`Memory Limit`

### 3. QuestionBankAnalyzer（题库分析器）

**职责**：分析题库特征，生成规律报告

**关键方法**：
```cpp
class QuestionBankAnalyzer {
public:
    // 分析题库，生成统计报告
    BankAnalysis analyzeBank(const QString &bankPath);
    
    // 保存分析结果
    bool saveAnalysis(const QString &bankPath, const BankAnalysis &analysis);
    
    // 加载分析结果
    BankAnalysis loadAnalysis(const QString &bankPath);
};
```

**分析内容**：
```json
{
  "bankName": "leetcode",
  "totalQuestions": 150,
  "difficultyDistribution": {
    "简单": 60,
    "中等": 70,
    "困难": 20
  },
  "tagDistribution": {
    "数组": 45,
    "字符串": 30,
    "动态规划": 25,
    "其他": 50
  },
  "avgTestCases": 5.2,
  "commonPatterns": [
    "输入输出格式统一",
    "包含边界测试"
  ]
}
```

### 4. EnhancedQuestionBankManager（增强的题库管理器）

**职责**：管理三类题库，支持导入、切换、删除

**新增方法**：
```cpp
class EnhancedQuestionBankManager {
public:
    // 导入题库（自动处理三层结构）
    QString importQuestionBank(
        const QString &sourcePath,
        const QString &bankName,
        QuestionBankType targetType = QuestionBankType::Processed
    );
    
    // 获取指定类型的题库列表
    QVector<QuestionBankInfo> getBanksByType(QuestionBankType type);
    
    // 获取题库路径
    QString getBankPath(const QString &bankId, QuestionBankType type);
    
    // 从原始题库重新处理
    bool reprocessFromOriginal(const QString &bankId);
};
```

## 📝 实现步骤

### Step 1: 创建 UniversalQuestionParser
- 实现智能格式识别
- 支持多种题目分隔符
- 智能配对输入输出

### Step 2: 创建 QuestionBankAnalyzer
- 统计题库特征
- 生成 JSON 报告

### Step 3: 增强 SmartQuestionImporter
- 第一步：拷贝到 original_banks（只读）
- 第二步：解析并保存到 processed_banks
- 第三步：AI 扩充测试数据
- 第四步：生成分析报告

### Step 4: 修改 QuestionBankManager
- 支持三类题库管理
- 提供类型筛选

### Step 5: 更新 UI
- 导入对话框显示三层结构
- 题库管理器显示题库类型

## 🎯 验收标准

- [ ] 能导入任意格式的编程题库
- [ ] 自动识别题目结构（不需要手动配置）
- [ ] 原始题库只读保护
- [ ] 生成题库分析报告
- [ ] AI 成功扩充测试数据到 5+ 组
- [ ] 三类题库独立管理

## 📊 测试用例

1. **LeetCode 格式**：标准的 Markdown 格式
2. **CCF 格式**：中文题目，特殊分隔符
3. **自定义格式**：混合格式，测试通用性

---

开始实现！
