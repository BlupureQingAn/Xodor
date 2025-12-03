# CCF 刷题系统完整实现计划

## 📋 需求分析

根据《项目操作全流程详细指导.txt》，需要实现一个完整的 CCF 刷题系统，包含 5 个核心阶段：

1. **初始化与本地题库导入** - 文件拷贝、格式解析、AI 扩充测试数据
2. **刷本地题库** - AI 辅助、代码自动保存、测试验证
3. **生成模拟题库** - 基于规则生成标准化套题
4. **刷模拟题** - 套题计时、答题报告
5. **关闭软件与数据恢复** - 进度保存、状态恢复

## 🔍 现有功能对比

### ✅ 已实现的功能
- 题库导入（SmartQuestionImporter）
- AI 智能解析（OllamaClient）
- 题库管理（QuestionBankManager）
- 刷题界面（PracticeWidget）
- 代码编辑器（CodeEditor）
- 编译测试（CompilerRunner）
- 进度跟踪（ProgressManager）
- 自动保存（AutoSaver）

### ❌ 缺失的功能
1. **CCF 专属格式解析规则**
2. **原始题库只读备份机制**
3. **基础题库与模拟题库分离**
4. **出题模式规律分析**
5. **AI 模拟题生成**
6. **套题计时功能**
7. **答题报告生成**
8. **代码版本回溯**
9. **AI 实时问答辅助**

## 📅 分阶段实现计划

---

## 🎯 阶段一：CCF 题库导入增强（3-4天）

### 目标
实现 CCF 专属的题库导入流程，包括原始备份、格式解析、AI 扩充、规则生成。

### 核心任务

#### 1.1 目录结构重构
```
项目根目录/
├── 原始题库/              # 只读备份
│   └── ccf/
│       ├── 考试一.md
│       └── 考试二.md
├── 基础题库/              # 解析后的标准化题库
│   └── ccf/
│       ├── 考试一/
│       │   ├── 第1题.md
│       │   ├── 第2题.md
│       │   └── ...
│       └── 出题模式规律.md
├── 人工模拟题库/          # AI 生成的模拟题
│   └── ccf/
│       ├── 模拟题1/
│       └── 模拟题2/
├── 用户数据/
│   ├── 代码备份/
│   └── 答题记录/
└── config/
    └── ccf_parse_rule.json
```

#### 1.2 新增类：CCFQuestionParser
**文件**: `src/ai/CCFQuestionParser.h/cpp`

**功能**:
- 识别 CCF 题目格式特征（题目描述、输入输出格式、测试数据）
- 支持单文件多题拆分
- 提取时间/内存限制
- 生成解析规则 JSON

**关键方法**:
```cpp
class CCFQuestionParser {
    // 分析文件格式，生成解析规则
    ParseRule analyzeFormat(const QStringList &sampleFiles);
    
    // 按规则解析单个文件
    QVector<Question> parseFile(const QString &content, const ParseRule &rule);
    
    // 拆分单文件内的多道题目
    QStringList splitQuestions(const QString &content);
    
    // 提取测试数据（输入输出配对）
    QVector<TestCase> extractTestCases(const QString &content);
};
```

#### 1.3 增强 SmartQuestionImporter
**新增功能**:
- 第一步：拷贝到「原始题库」（只读）
- 第二步：抽取 3 份样本，生成 CCF 解析规则
- 第三步：按规则批量解析，AI 扩充测试数据
- 第四步：保存到「基础题库」，生成「出题模式规律.md」

#### 1.4 新增类：QuestionBankRuleAnalyzer
**文件**: `src/ai/QuestionBankRuleAnalyzer.h/cpp`

**功能**:
- 分析题库特征（套题数量、难度分布、知识点占比）
- 生成「出题模式规律.md」
- 为模拟题生成提供规则

**输出示例**:
```markdown
# CCF 出题模式规律

## 套题结构
- 题目数量：4-5 题/套
- 难度分布：简单 40%、中等 40%、困难 20%

## 知识点分布
- 数组/字符串：30%
- 动态规划：25%
- 图论：20%
- 其他：25%

## 代码限制
- 时间限制：1-2 秒
- 内存限制：256MB
```

### 实现文件清单
- [ ] `src/ai/CCFQuestionParser.h`
- [ ] `src/ai/CCFQuestionParser.cpp`
- [ ] `src/ai/QuestionBankRuleAnalyzer.h`
- [ ] `src/ai/QuestionBankRuleAnalyzer.cpp`
- [ ] 修改 `src/ai/SmartQuestionImporter.cpp`（增强导入流程）
- [ ] 修改 `src/core/QuestionBankManager.cpp`（支持三类题库）

---

## 🎯 阶段二：刷题功能增强（3-4天）

### 目标
完善刷题体验，增加 AI 实时辅助、代码版本回溯、测试结果优化。

### 核心任务

#### 2.1 新增类：AIAssistant（AI 实时问答）
**文件**: `src/ai/AIAssistant.h/cpp`

**功能**:
- 用户输入问题，AI 实时回复
- 一键获取思路提示（不含代码）
- 知识点讲解
- 错误诊断

**UI 组件**: 在刷题页面添加 AI 辅助面板

#### 2.2 增强 AutoSaver（代码版本管理）
**新增功能**:
- 保存时添加时间戳版本
- 提供历史版本列表
- 支持恢复到指定版本

**存储格式**:
```
用户数据/代码备份/本地题库/ccf/考试一/
├── 第1题_user_20241202_143022.cpp
├── 第1题_user_20241202_143156.cpp
└── 第1题_user_20241202_143401.cpp
```

#### 2.3 优化测试结果展示
**增强 CompilerRunner**:
- 显示失败原因（答案错误/运行超时/内存超限）
- AI 基于失败样例给出修改建议
- 标注测试数据来源（原始/AI 补充）

#### 2.4 答题记录增强
**新增字段**:
```json
{
  "questionId": "ccf_exam1_q1",
  "status": "已通过",
  "passRate": "7/7",
  "timeSpent": 1245,  // 秒
  "submitTime": "2024-12-02 14:35:00",
  "codeVersion": "第1题_user_20241202_143401.cpp"
}
```

### 实现文件清单
- [ ] `src/ai/AIAssistant.h`
- [ ] `src/ai/AIAssistant.cpp`
- [ ] 修改 `src/core/AutoSaver.cpp`（版本管理）
- [ ] 修改 `src/core/CompilerRunner.cpp`（结果优化）
- [ ] 修改 `src/ui/PracticeWidget.cpp`（添加 AI 辅助面板）
- [ ] 新增 `src/ui/CodeVersionDialog.h/cpp`（版本选择对话框）

---

## 🎯 阶段三：模拟题库生成（4-5天）

### 目标
基于「出题模式规律」，AI 生成标准化的 CCF 模拟套题。

### 核心任务

#### 3.1 新增类：MockExamGenerator
**文件**: `src/ai/MockExamGenerator.h/cpp`

**功能**:
- 读取「出题模式规律.md」
- 调用 AI 生成符合规则的题目
- 生成 5-8 组测试数据（覆盖基础/边界/异常）
- 保存为标准化套题

**关键方法**:
```cpp
class MockExamGenerator {
    // 生成指定数量的模拟套题
    bool generateMockExams(const QString &bankId, int count);
    
    // 生成单道题目
    Question generateQuestion(const QuestionRule &rule);
    
    // 生成测试数据
    QVector<TestCase> generateTestCases(const Question &question);
    
    // 保存套题
    bool saveMockExam(const QString &examName, const QVector<Question> &questions);
};
```

#### 3.2 AI 提示词设计
**生成题目提示词**:
```
你是 CCF 认证考试出题专家。请根据以下规则生成一道题目：

【规则】
- 难度：中等
- 知识点：动态规划
- 题目风格：CCF 认证考试
- 时间限制：1 秒
- 内存限制：256MB

【要求】
1. 题干：包含完整的问题描述、输入输出格式说明
2. 测试数据：生成 6 组样例（基础 2 组、边界 2 组、异常 2 组）
3. 输入输出：每组数据必须一一对应
4. 格式：严格按照 CCF 题目格式

【输出格式】
使用 JSON 格式输出...
```

#### 3.3 UI：模拟题生成对话框
**文件**: `src/ui/MockExamGeneratorDialog.h/cpp`

**功能**:
- 选择题库分类（ccf）
- 输入生成套题数量
- 显示生成进度
- 完成后跳转到模拟题库

### 实现文件清单
- [ ] `src/ai/MockExamGenerator.h`
- [ ] `src/ai/MockExamGenerator.cpp`
- [ ] `src/ui/MockExamGeneratorDialog.h`
- [ ] `src/ui/MockExamGeneratorDialog.cpp`
- [ ] 修改 `src/core/QuestionBankManager.cpp`（支持模拟题库）

---

## 🎯 阶段四：套题计时与答题报告（2-3天）

### 目标
实现模拟考试的计时功能和答题报告生成。

### 核心任务

#### 4.1 新增类：ExamTimer
**文件**: `src/core/ExamTimer.h/cpp`

**功能**:
- 套题计时（如 180 分钟）
- 支持暂停（限制 1 次，最长 30 分钟）
- 防作弊（基于系统时间戳）
- 时间到自动提交

**关键方法**:
```cpp
class ExamTimer : public QObject {
    void startTimer(int totalMinutes);
    void pauseTimer();
    void resumeTimer();
    int getRemainingSeconds() const;
    bool canPause() const;  // 是否还能暂停
};
```

#### 4.2 新增类：ExamReportGenerator
**文件**: `src/core/ExamReportGenerator.h/cpp`

**功能**:
- 生成套题答题报告
- 统计每题通过率、总得分、用时分布
- 分析知识点薄弱项
- 导出为 PDF/HTML

**报告示例**:
```markdown
# CCF 模拟题 1 答题报告

## 总体情况
- 总题数：4 题
- 完成题数：4 题
- 通过题数：3 题
- 总得分：75 分
- 总用时：145 分钟

## 各题详情
| 题号 | 难度 | 知识点 | 通过率 | 用时 | 得分 |
|------|------|--------|--------|------|------|
| 1    | 简单 | 数组   | 100%   | 15分 | 25分 |
| 2    | 中等 | DP     | 100%   | 45分 | 25分 |
| 3    | 中等 | 图论   | 100%   | 60分 | 25分 |
| 4    | 困难 | 贪心   | 0%     | 25分 | 0分  |

## 薄弱知识点
- 贪心算法：建议加强练习
```

#### 4.3 UI：套题刷题界面
**修改 PracticeWidget**:
- 添加计时器显示
- 添加暂停按钮
- 限制题目跳转（按顺序刷题）
- 时间到自动锁定

### 实现文件清单
- [ ] `src/core/ExamTimer.h`
- [ ] `src/core/ExamTimer.cpp`
- [ ] `src/core/ExamReportGenerator.h`
- [ ] `src/core/ExamReportGenerator.cpp`
- [ ] 修改 `src/ui/PracticeWidget.cpp`（添加计时功能）
- [ ] 新增 `src/ui/ExamReportDialog.h/cpp`（报告查看）

---

## 🎯 阶段五：数据恢复与优化（1-2天）

### 目标
完善软件关闭时的数据保存和启动时的状态恢复。

### 核心任务

#### 5.1 增强 SessionManager
**新增功能**:
- 保存当前刷题位置（题库、套题、题号）
- 保存代码编辑状态
- 保存 UI 配置（语言选择、AI 开关）

**存储格式**:
```json
{
  "lastSession": {
    "bankType": "本地题库",
    "bankId": "ccf",
    "examFolder": "考试一",
    "questionId": "第2题",
    "codeFile": "第2题_user_20241202_143401.cpp",
    "language": "C++",
    "aiAssistEnabled": true
  }
}
```

#### 5.2 启动时自动恢复
**修改 MainWindow**:
- 启动时读取 SessionManager
- 自动加载上次的题库和题目
- 恢复代码编辑器内容
- 恢复 UI 状态

### 实现文件清单
- [ ] 修改 `src/utils/SessionManager.cpp`（增强会话管理）
- [ ] 修改 `src/ui/MainWindow.cpp`（启动恢复）

---

## 📊 总体时间估算

| 阶段 | 任务 | 预计时间 | 优先级 |
|------|------|----------|--------|
| 阶段一 | CCF 题库导入增强 | 3-4 天 | 🔴 高 |
| 阶段二 | 刷题功能增强 | 3-4 天 | 🔴 高 |
| 阶段三 | 模拟题库生成 | 4-5 天 | 🟡 中 |
| 阶段四 | 套题计时与报告 | 2-3 天 | 🟡 中 |
| 阶段五 | 数据恢复与优化 | 1-2 天 | 🟢 低 |
| **总计** | | **13-18 天** | |

---

## 🎯 实施建议

### 推荐顺序
1. **先做阶段一** - 建立 CCF 题库的基础架构
2. **再做阶段二** - 完善刷题体验（用户最常用）
3. **然后阶段五** - 保证数据安全（防止丢失）
4. **最后阶段三、四** - 高级功能（模拟题、计时）

### 每个阶段的验收标准
- **阶段一**: 能成功导入 CCF 题库，生成解析规则和出题规律
- **阶段二**: AI 辅助正常工作，代码版本可回溯
- **阶段三**: 能生成符合规则的模拟套题
- **阶段四**: 计时器正常工作，能生成答题报告
- **阶段五**: 关闭重启后能恢复到上次状态

---

## 📝 下一步行动

请确认：
1. 是否按照这个计划实施？
2. 是否需要调整优先级？
3. 是否需要先看某个阶段的详细设计？

我可以立即开始实现任何一个阶段！
