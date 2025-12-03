# AI驱动的智能导入系统 - 设计文档

## 🎯 系统目标

实现完全基于AI的智能题库导入系统，自动化处理整个导入流程，无需用户手动配置。

## ✅ 核心功能

### 1. AI驱动的题库导入器 (AIQuestionBankImporter)

**文件：**
- `src/ai/AIQuestionBankImporter.h`
- `src/ai/AIQuestionBankImporter.cpp`

**导入流程（8个阶段）：**

#### 阶段1：文件复制 (CopyingFiles)
- 自动将用户选择的文件夹复制到"原始题库"目录
- 保持原始文件结构
- 设置为只读，确保数据安全

#### 阶段2：AI分析格式 (AnalyzingFormat)
- 抽取3个样本文件
- AI自动识别题目格式特征：
  - 题目标题模式
  - 题目描述模式
  - 输入输出格式模式
  - 测试用例模式
  - 代码限制模式
  - 题目分割模式

#### 阶段3：生成解析规则 (GeneratingRules)
- 根据AI分析结果生成解析规则
- 保存到 `config/{category}_parse_rule.json`
- 规则包含所有识别的模式

#### 阶段4：解析题目 (ParsingQuestions)
- 按文件顺序依次处理
- AI根据解析规则提取：
  - 题目标题
  - 完整描述
  - 原始测试数据
  - 代码限制
  - 难度（AI推断）
  - 知识点标签（AI识别）

#### 阶段5：生成测试数据 (GeneratingTestData)
- AI为每道题生成3-5组补充测试数据
- 覆盖场景：
  - 边界条件（最小值、最大值）
  - 异常情况（空输入、特殊字符）
  - 等价类测试
- 标注"AI补充"

#### 阶段6：组织题目 (OrganizingQuestions)
- 按标准格式保存到"基础题库"
- 每道题保存为独立的Markdown文件
- 格式：`第X题.md`
- 包含原始数据和AI补充数据

#### 阶段7：分析出题规律 (AnalyzingPattern)
- AI分析整个题库
- 生成出题模式规律：
  - 套题数量
  - 难度分布
  - 知识点占比
  - 题号规则
  - 代码限制规则
- 保存到 `出题模式规律.json`

#### 阶段8：完成 (Complete)
- 显示导入结果
- 题目数量统计
- 刷新题库列表

## 📊 数据流转

```
用户选择文件夹
  ↓
【阶段1】复制到原始题库
  ↓
【阶段2】AI分析格式（抽取3个样本）
  ↓
【阶段3】生成解析规则
  ↓
【阶段4】AI解析所有题目
  ├─ 文件1 → AI解析 → 题目1, 题目2...
  ├─ 文件2 → AI解析 → 题目3, 题目4...
  └─ 文件N → AI解析 → 题目X...
  ↓
【阶段5】AI生成测试数据
  ├─ 题目1 → AI生成 → 3-5组补充数据
  ├─ 题目2 → AI生成 → 3-5组补充数据
  └─ 题目N → AI生成 → 3-5组补充数据
  ↓
【阶段6】组织保存到基础题库
  ↓
【阶段7】AI分析出题规律
  ↓
【阶段8】完成，显示结果
```

## 🎨 用户界面

### 导入对话框

```
┌─────────────────────────────────────────────────┐
│ 🤖 AI智能导入题库                              │
├─────────────────────────────────────────────────┤
│                                                 │
│ 当前阶段：📋 AI正在分析题目格式...             │
│                                                 │
│ [████████████████░░░░░░░░] 65%                 │
│                                                 │
│ 📋 导入日志：                                   │
│ ┌─────────────────────────────────────────────┐ │
│ │ ✅ 已复制 15 个文件                         │ │
│ │ 🔍 已抽取 3 个样本文件，正在分析...         │ │
│ │ ✅ 格式分析完成                             │ │
│ │ ✅ 解析规则已生成                           │ │
│ │ 📝 正在解析文件 5/15: 考试一.md             │ │
│ │ ✅ 已解析 12 道题目                         │ │
│ │ 🤖 正在为题目 8/12 生成测试数据...          │ │
│ └─────────────────────────────────────────────┘ │
│                                                 │
│                              [取消]             │
└─────────────────────────────────────────────────┘
```

### 完成提示

```
┌─────────────────────────────────────────────────┐
│ 🎉 导入完成                                     │
├─────────────────────────────────────────────────┤
│                                                 │
│ 【ccf】题库导入成功！                           │
│                                                 │
│ • 已生成基础题库（含 AI 扩充测试数据）          │
│ • 总题数：45 道                                 │
│ • 原始测试数据：135 组                          │
│ • AI补充数据：180 组                            │
│ • 出题规律已分析                                │
│                                                 │
│ 现在可以直接刷题或生成模拟题！                  │
│                                                 │
│                              [确定]             │
└─────────────────────────────────────────────────┘
```

## 💡 AI提示词设计

### 1. 格式分析提示词

```
你是一个专业的题目格式分析助手。请分析以下Markdown格式的题目文件，识别题目的结构特征。

【样本文件】
...

【分析任务】
请识别以下内容的模式：
1. 题目标题的标识
2. 题目描述的标识
3. 输入格式的标识
4. 输出格式的标识
5. 测试用例的标识
6. 代码限制的标识
7. 题目分割的标识

【输出格式】
返回JSON格式...
```

### 2. 题目解析提示词

```
你是一个专业的题目解析助手。请根据以下解析规则，从Markdown文件中提取题目信息。

【解析规则】
...

【文件内容】
...

【解析任务】
1. 识别文件中的所有题目
2. 提取每道题的完整信息
3. 推断难度和知识点

【输出格式】
返回JSON格式...
```

### 3. 测试数据生成提示词

```
你是一个专业的测试数据生成助手。请为以下题目生成补充测试数据。

【题目信息】
...

【现有测试数据】
...

【生成要求】
1. 生成3-5组新的测试数据
2. 覆盖边界、异常、等价类场景
3. 标注为AI生成

【输出格式】
返回JSON格式...
```

### 4. 出题规律分析提示词

```
你是一个专业的出题规律分析助手。请分析以下题库，总结出题模式。

【题库信息】
...

【分析任务】
1. 统计难度分布
2. 统计知识点分布
3. 识别套题数量
4. 识别题号规则
5. 识别代码限制规则

【输出格式】
返回JSON格式...
```

## 🔧 技术实现

### 解析规则结构

```cpp
struct ParseRule {
    QString category;
    QStringList titlePatterns;
    QStringList descriptionPatterns;
    QStringList inputPatterns;
    QStringList outputPatterns;
    QStringList testCasePatterns;
    QStringList constraintPatterns;
    QStringList splitPatterns;
};
```

### 导入阶段枚举

```cpp
enum class ImportStage {
    Idle,
    CopyingFiles,
    AnalyzingFormat,
    GeneratingRules,
    ParsingQuestions,
    GeneratingTestData,
    OrganizingQuestions,
    AnalyzingPattern,
    Complete
};
```

### 信号机制

```cpp
signals:
    void stageChanged(ImportStage stage, const QString &message);
    void progressUpdated(int percentage, const QString &message);
    void importComplete(const QString &categoryName, int questionCount);
    void importFailed(const QString &error);
```

## 📁 文件结构

### 导入后的目录结构

```
原始题库/
└── ccf/
    ├── 考试一.md
    ├── 考试二.md
    └── ...

基础题库/
└── ccf/
    ├── all/
    │   ├── 第1题.md
    │   ├── 第2题.md
    │   └── ...
    └── 出题模式规律.json

config/
└── ccf_parse_rule.json
```

### 题目文件格式

```markdown
# 数组去重

## 题目描述
给定一个整数数组，去除重复元素...

## 输入格式
第一行...

## 输出格式
输出...

## 测试用例

### 测试 1：基本测试
输入：
```
5
1 2 3 2 1
```

输出：
```
3
1 2 3
```

### 测试 2：边界条件：最小值 [AI补充]
输入：
```
1
1
```

输出：
```
1
1
```
```

## ⚠️ 注意事项

### 1. AI服务要求
- 需要配置Ollama服务
- 推荐使用qwen2.5-coder:7b或更强模型
- 确保AI服务稳定运行

### 2. 文件格式要求
- 支持Markdown格式（.md）
- 文件编码：UTF-8
- 建议文件大小：<1MB

### 3. 导入时间
- 取决于文件数量和题目数量
- 平均：每道题约10-30秒
- 15道题约需5-10分钟

### 4. 质量保证
- AI生成的数据建议人工审核
- 检查测试数据的正确性
- 验证难度和标签的准确性

## 🚀 使用方式

### 1. 用户操作

```
1. 点击"导入题库"按钮
2. 选择题库文件夹（如ccf）
3. 输入分类名称（如"ccf"）
4. 点击"开始导入"
5. 等待AI自动处理
6. 查看导入结果
```

### 2. 代码集成

```cpp
// 创建导入器
AIQuestionBankImporter *importer = new AIQuestionBankImporter(aiClient);

// 连接信号
connect(importer, &AIQuestionBankImporter::stageChanged,
        this, &Dialog::onStageChanged);
connect(importer, &AIQuestionBankImporter::progressUpdated,
        this, &Dialog::onProgressUpdated);
connect(importer, &AIQuestionBankImporter::importComplete,
        this, &Dialog::onImportComplete);

// 开始导入
importer->startImport("/path/to/ccf", "ccf");
```

## 🎯 优势

### 1. 完全自动化
- 无需手动配置
- AI自动识别格式
- 自动生成测试数据
- 自动分析规律

### 2. 智能化
- AI理解题目内容
- 智能推断难度
- 自动识别知识点
- 生成高质量测试数据

### 3. 标准化
- 统一的文件格式
- 规范的目录结构
- 完整的元数据
- 清晰的标注

### 4. 可扩展
- 支持任意格式的题库
- AI自适应学习
- 规则可复用
- 易于维护

---

**AI驱动的智能导入系统设计完成！** ✨

这个系统将彻底改变题库导入的方式，让用户只需选择文件夹，剩下的全部交给AI自动处理！🤖
