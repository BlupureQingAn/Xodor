# AI导入改为MD拆分完整方案

## 需求概述

将AI导入功能从"解析生成JSON"改为"拆分套题MD为单题MD"：
- AI识别题目边界并生成拆分规则文档
- 拆分后每道题一个MD文件
- MD文件包含元数据（难度、标签等）在文件头部
- 保存到基础题库目录
- 所有现有JSON功能改为读取MD文件

## 一、MD文件格式设计

### 1.1 Front Matter格式（YAML）

使用标准的Markdown Front Matter格式，在文件开头用`---`包裹元数据：

```markdown
---
id: "q_12345"
title: "两数之和"
type: "code"
difficulty: "easy"
tags: ["数组", "哈希表"]
---

# 两数之和

## 题目描述

给定一个整数数组 nums 和一个整数目标值 target，请你在该数组中找出和为目标值的那两个整数，并返回它们的数组下标。

## 输入格式

第一行包含...

## 输出格式

输出两个整数...

## 测试用例

### 测试用例1：基本测试
**输入：**
```
2 7 11 15
9
```

**输出：**
```
0 1
```

### 测试用例2：边界条件
...
```

### 1.2 元数据字段说明

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| id | string | 题目唯一标识 | "q_12345" |
| title | string | 题目标题 | "两数之和" |
| type | string | 题目类型 | "code"/"choice"/"fill" |
| difficulty | string | 难度 | "easy"/"medium"/"hard" |
| tags | array | 标签列表 | ["数组", "哈希表"] |

### 1.3 测试用例格式

在MD正文中使用标准的Markdown格式：

```markdown
## 测试用例

### 测试用例1：基本测试
**输入：**
```
输入数据
```

**输出：**
```
输出数据
```

**说明：** 测试基本功能
```

## 二、AI拆分流程

### 2.1 第一步：AI生成拆分规则

让AI分析套题MD文件，生成拆分规则文档：

**AI Prompt：**
```
分析以下Markdown文件，识别题目边界规律，生成拆分规则文档。

要求：
1. 识别题目边界的特征（标题级别、分隔符、题号模式等）
2. 识别题目的组成部分（标题、描述、输入格式、输出格式、测试用例等）
3. 生成JSON格式的拆分规则

返回格式：
{
  "boundaryPatterns": [
    {"type": "heading", "level": 1, "pattern": "^# \\d+\\."},
    {"type": "separator", "pattern": "^---+$"}
  ],
  "sectionPatterns": {
    "title": "^#+\\s*(.+)",
    "description": "题目描述|问题描述",
    "input": "输入格式|输入说明|Input",
    "output": "输出格式|输出说明|Output",
    "testCase": "测试用例|样例|示例"
  },
  "questions": [
    {
      "startLine": 1,
      "endLine": 50,
      "title": "题目1标题"
    }
  ]
}
```

保存到：`data/基础题库/{bankName}/拆分规则.json`

### 2.2 第二步：根据规则拆分MD文件

根据AI生成的拆分规则，将套题MD拆分为单题MD：

1. 读取拆分规则JSON
2. 按照边界规则切分文件
3. 为每道题生成独立的MD文件

### 2.3 第三步：AI提取元数据

对每个拆分后的MD文件，让AI提取元数据：

**AI Prompt：**
```
分析以下题目，提取元数据。

要求：
1. 不要修改题目内容
2. 只提取元数据信息
3. 判断难度和标签

返回格式：
{
  "title": "题目标题",
  "difficulty": "easy/medium/hard",
  "tags": ["数组", "哈希表"],
  "type": "code"
}
```

### 2.4 第四步：生成带Front Matter的MD文件

将AI提取的元数据和原始内容组合：

```markdown
---
id: "{sourceFile}_{hash}"
title: "题目标题"
type: "code"
difficulty: "easy"
tags: ["数组", "哈希表"]
---

{原始题目内容}
```

保存到：`data/基础题库/{bankName}/{题目标题}.md`

## 三、代码修改方案

### 3.1 新增类：MarkdownQuestionParser

**文件：** `src/core/MarkdownQuestionParser.h/cpp`

**功能：**
- 解析带Front Matter的MD文件
- 提取元数据和题目内容
- 解析测试用例
- 转换为Question对象

**关键方法：**
```cpp
class MarkdownQuestionParser {
public:
    // 从MD文件解析Question对象
    static Question parseFromFile(const QString &filePath);
    
    // 从MD内容解析Question对象
    static Question parseFromContent(const QString &content);
    
    // 提取Front Matter
    static QJsonObject extractFrontMatter(const QString &content);
    
    // 解析测试用例
    static QVector<TestCase> parseTestCases(const QString &content);
    
    // 生成带Front Matter的MD内容
    static QString generateMarkdown(const Question &question);
};
```

### 3.2 修改类：SmartQuestionImporter

**文件：** `src/ai/SmartQuestionImporter.h/cpp`

**修改内容：**

1. **新增方法：**
```cpp
// 第一步：AI生成拆分规则
void generateSplitRules(const QString &content);

// 第二步：根据规则拆分文件
QVector<QString> splitByRules(const QString &content, const QJsonObject &rules);

// 第三步：AI提取元数据
QJsonObject extractMetadata(const QString &content);

// 第四步：生成MD文件
bool saveAsMarkdown(const QString &content, const QJsonObject &metadata, const QString &filePath);
```

2. **修改流程：**
```cpp
void SmartQuestionImporter::startImport() {
    // 1. 扫描文件
    scanFiles();
    
    // 2. AI生成拆分规则
    for (each file) {
        generateSplitRules(fileContent);
    }
    
    // 3. 根据规则拆分
    for (each file) {
        QVector<QString> questions = splitByRules(fileContent, rules);
        
        // 4. 为每道题提取元数据并保存
        for (each question) {
            QJsonObject metadata = extractMetadata(question);
            saveAsMarkdown(question, metadata, outputPath);
        }
    }
    
    // 5. 完成
    emit importCompleted(true, "成功拆分N道题目");
}
```

### 3.3 修改类：Question

**文件：** `src/core/Question.h/cpp`

**新增方法：**
```cpp
// 从MD文件加载
static Question fromMarkdownFile(const QString &filePath);

// 从MD内容加载
static Question fromMarkdown(const QString &content);

// 保存为MD文件
bool saveAsMarkdown(const QString &filePath) const;

// 转换为MD内容
QString toMarkdown() const;
```

### 3.4 修改类：QuestionBank

**文件：** `src/core/QuestionBank.h/cpp`

**修改加载逻辑：**
```cpp
void QuestionBank::loadQuestions(const QString &path) {
    QDir dir(path);
    
    // 优先加载MD文件
    QStringList mdFilters;
    mdFilters << "*.md";
    QFileInfoList mdFiles = dir.entryInfoList(mdFilters, QDir::Files);
    
    for (const QFileInfo &fileInfo : mdFiles) {
        // 跳过README等非题目文件
        if (fileInfo.fileName().toLower().contains("readme") ||
            fileInfo.fileName().toLower().contains("拆分规则")) {
            continue;
        }
        
        Question q = Question::fromMarkdownFile(fileInfo.absoluteFilePath());
        if (!q.id().isEmpty()) {
            m_questions.append(q);
        }
    }
    
    // 兼容：如果没有MD文件，尝试加载JSON
    if (m_questions.isEmpty()) {
        QStringList jsonFilters;
        jsonFilters << "*.json";
        QFileInfoList jsonFiles = dir.entryInfoList(jsonFilters, QDir::Files);
        
        for (const QFileInfo &fileInfo : jsonFiles) {
            // 原有JSON加载逻辑
        }
    }
}
```

### 3.5 修改类：QuestionEditorDialog

**文件：** `src/ui/QuestionEditorDialog.h/cpp`

**修改保存逻辑：**
```cpp
void QuestionEditorDialog::saveQuestion() {
    // 收集数据到Question对象
    Question q;
    q.setTitle(m_titleEdit->text());
    // ... 其他字段
    
    // 保存为MD文件
    QString filePath = QString("data/基础题库/%1/%2.md")
        .arg(m_bankName)
        .arg(q.title());
    
    if (q.saveAsMarkdown(filePath)) {
        QMessageBox::information(this, "成功", "题目已保存");
    }
}
```

## 四、AI Prompt设计

### 4.1 拆分规则生成Prompt

```cpp
QString SmartQuestionImporter::buildSplitRulesPrompt(const QString &content) {
    return R"(
你是题目拆分专家。分析以下Markdown文件，识别题目边界规律。

任务：
1. 识别题目边界特征（标题、分隔符、题号等）
2. 识别每道题的起止位置
3. 生成拆分规则

返回JSON格式：
{
  "boundaryPatterns": [
    {"type": "heading", "level": 1, "pattern": "^# \\d+\\."},
    {"type": "separator", "pattern": "^---+$"}
  ],
  "questions": [
    {
      "startLine": 1,
      "endLine": 50,
      "title": "题目1"
    }
  ]
}

文件内容：
---
)" + content + R"(
---

返回纯JSON，不要其他文字。
)";
}
```

### 4.2 元数据提取Prompt

```cpp
QString SmartQuestionImporter::buildMetadataPrompt(const QString &content) {
    return R"(
你是题目分析专家。分析以下题目，提取元数据。

⚠️ 重要：不要修改题目内容，只提取元数据！

任务：
1. 提取题目标题
2. 判断难度（easy/medium/hard）
3. 识别标签（数组、字符串、动态规划等）
4. 判断类型（code/choice/fill）

难度判断标准：
- easy：基础语法、简单逻辑
- medium：多个数据结构、算法应用
- hard：复杂算法、高级数据结构

返回JSON格式：
{
  "title": "题目标题",
  "difficulty": "easy",
  "tags": ["数组", "哈希表"],
  "type": "code"
}

题目内容：
---
)" + content + R"(
---

返回纯JSON，不要其他文字。
)";
}
```

## 五、测试用例解析

### 5.1 测试用例格式识别

在MD文件中识别测试用例的多种格式：

**格式1：标准格式**
```markdown
### 测试用例1：基本测试
**输入：**
```
输入数据
```

**输出：**
```
输出数据
```
```

**格式2：简化格式**
```markdown
**输入样例1：**
```
输入数据
```

**输出样例1：**
```
输出数据
```
```

**格式3：表格格式**
```markdown
| 输入 | 输出 | 说明 |
|------|------|------|
| 数据1 | 结果1 | 基本测试 |
```

### 5.2 解析逻辑

```cpp
QVector<TestCase> MarkdownQuestionParser::parseTestCases(const QString &content) {
    QVector<TestCase> testCases;
    
    // 正则匹配测试用例
    QRegularExpression testCasePattern(
        R"(###?\s*测试用例\s*\d*[：:]\s*(.+?)\n\*\*输入[：:]\*\*\n```\n(.+?)\n```\n\*\*输出[：:]\*\*\n```\n(.+?)\n```)",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatchIterator it = testCasePattern.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        TestCase tc;
        tc.description = match.captured(1).trimmed();
        tc.input = match.captured(2).trimmed();
        tc.expectedOutput = match.captured(3).trimmed();
        tc.isAIGenerated = false;
        testCases.append(tc);
    }
    
    return testCases;
}
```

## 六、兼容性处理

### 6.1 向后兼容

保持对现有JSON格式的支持：

```cpp
Question Question::fromFile(const QString &filePath) {
    if (filePath.endsWith(".md")) {
        return fromMarkdownFile(filePath);
    } else if (filePath.endsWith(".json")) {
        // 原有JSON加载逻辑
        QFile file(filePath);
        file.open(QIODevice::ReadOnly);
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        return Question(doc.object());
    }
    return Question();
}
```

### 6.2 迁移工具

提供JSON到MD的迁移工具：

```cpp
class QuestionMigrationTool {
public:
    // 将JSON题库迁移为MD格式
    static bool migrateJsonToMarkdown(const QString &bankPath);
    
    // 批量转换
    static int convertAllJsonToMarkdown(const QString &basePath);
};
```

## 七、文件结构

### 7.1 导入后的目录结构

```
data/
├── 基础题库/
│   ├── CCF-CSP/
│   │   ├── 拆分规则.json          # AI生成的拆分规则
│   │   ├── 两数之和.md            # 单题MD文件
│   │   ├── 三数之和.md
│   │   └── ...
│   └── LeetCode/
│       ├── 拆分规则.json
│       ├── 反转链表.md
│       └── ...
└── 原始题库/                      # 保留原始套题文件（只读）
    ├── CCF-CSP/
    │   └── 套题.md
    └── LeetCode/
        └── 套题.md
```

## 八、实施步骤

### 阶段1：核心功能（优先）
1. ✅ 创建MarkdownQuestionParser类
2. ✅ 实现Front Matter解析
3. ✅ 实现测试用例解析
4. ✅ Question类添加MD支持

### 阶段2：AI拆分（核心）
1. ✅ 修改SmartQuestionImporter
2. ✅ 实现AI拆分规则生成
3. ✅ 实现按规则拆分
4. ✅ 实现元数据提取

### 阶段3：适配现有功能
1. ✅ QuestionBank加载MD文件
2. ✅ QuestionEditorDialog保存MD
3. ✅ 其他读取题目的地方适配

### 阶段4：测试和优化
1. ✅ 测试拆分功能
2. ✅ 测试MD加载
3. ✅ 性能优化

## 九、注意事项

1. **文件编码**：所有MD文件使用UTF-8编码
2. **文件名安全**：题目标题中的特殊字符需要转义
3. **Front Matter解析**：使用标准YAML解析库
4. **测试用例格式**：支持多种常见格式
5. **错误处理**：MD解析失败时的降级处理
6. **性能**：大量MD文件的加载性能优化

## 十、优势

1. **可读性**：MD格式人类可读，便于手动编辑
2. **版本控制**：MD文件更适合Git管理
3. **灵活性**：可以包含更丰富的格式（图片、表格等）
4. **标准化**：Front Matter是业界标准
5. **简化**：不需要AI生成测试用例，保持原题完整性
