# AI导入题库优化设计文档

## Overview

本设计文档针对AI导入题库模块的三个核心问题提供解决方案：

1. **进度条计算优化** - 重新设计进度计算逻辑，确保导入完成时达到100%
2. **配置文件路径重构** - 将导入规则文件从题库目录迁移到统一的config目录
3. **完成摘要重写** - 提供详细的导入结果统计，移除过时描述

## Architecture

### 组件关系

```
SmartImportDialog (UI层)
    ↓ 监听进度
SmartQuestionImporter (业务逻辑层)
    ↓ 发出进度信号
    ├─ 扫描阶段 (0-10%)
    ├─ AI解析阶段 (10-95%)
    └─ 保存完成阶段 (95-100%)
    
配置文件管理：
data/config/
    └─ {bankName}_parse_rule.json  (导入规则)
    
data/基础题库/
    └─ {bankName}/
        ├─ {sourceFile1}/
        │   ├─ 题目1.md
        │   └─ 题目2.md
        └─ 出题模式规律.md
```

## Components and Interfaces

### 1. ImportProgress 结构体增强

```cpp
struct ImportProgress {
    // 阶段标识
    enum Stage {
        Scanning,      // 扫描文件阶段
        Parsing,       // AI解析阶段
        Saving,        // 保存完成阶段
        Complete       // 全部完成
    };
    
    Stage currentStage = Scanning;
    
    // 文件统计
    int totalFiles = 0;
    int processedFiles = 0;      // 扫描完成的文件数
    int currentFileIndex = 0;     // 当前处理的文件索引
    
    // 题目统计
    int totalQuestions = 0;       // 已识别的题目总数
    int estimatedTotalQuestions = 0;  // 预估总题目数
    
    // 当前状态
    QString currentFile;
    QString currentStatus;
    
    // 计算进度百分比
    int calculatePercentage() const;
};
```

### 2. SmartQuestionImporter 进度管理

```cpp
class SmartQuestionImporter {
private:
    // 进度计算方法
    void updateProgress();
    int calculateScanningProgress() const;
    int calculateParsingProgress() const;
    int calculateSavingProgress() const;
    
    // 阶段转换
    void enterScanningStage();
    void enterParsingStage();
    void enterSavingStage();
    void enterCompleteStage();
};
```

### 3. 配置文件管理器

```cpp
class ImportRuleManager {
public:
    // 保存导入规则到config目录
    static bool saveImportRule(const QString &bankName, const QJsonObject &rule);
    
    // 从config目录读取导入规则
    static QJsonObject loadImportRule(const QString &bankName);
    
    // 检查规则文件是否存在
    static bool hasImportRule(const QString &bankName);
    
    // 删除规则文件
    static bool deleteImportRule(const QString &bankName);
    
    // 获取规则文件路径
    static QString getRulePath(const QString &bankName);
    
private:
    static const QString CONFIG_DIR;  // "data/config"
};
```

### 4. QuestionBankManager 过滤增强

```cpp
class QuestionBankManager {
private:
    // 过滤配置文件
    bool isConfigFile(const QString &fileName) const;
    
    // 扫描题库时跳过配置文件
    void scanQuestionBankDirectory(const QString &path);
};
```

## Data Models

### 导入规则文件格式

```json
{
  "bankName": "CCF",
  "createdTime": "2024-12-08T10:30:00",
  "parseMode": "AI智能解析",
  "modulePatterns": [
    {
      "题干标识": ["【题目描述】", "问题：", "题目："],
      "输入标识": ["【输入】", "输入格式：", "Input:"],
      "输出标识": ["【输出】", "输出格式：", "Output:"],
      "测试数据分隔": ["空行", "测试用例", "样例"],
      "代码限制": ["【时间限制】", "【内存限制】", "支持语言："]
    }
  ],
  "statistics": {
    "totalQuestions": 50,
    "difficultyDistribution": {
      "简单": 15,
      "中等": 25,
      "困难": 10
    },
    "avgTestCases": 5.2
  }
}
```

### 导入结果统计

```cpp
struct ImportResult {
    bool success;
    int totalQuestions;
    
    // 按源文件分类
    QMap<QString, int> questionsByFile;
    
    // 按难度分类
    QMap<QString, int> questionsByDifficulty;
    
    // 保存路径
    QString basePath;
    
    // 错误信息
    QString errorMessage;
    QStringList warnings;
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 进度单调递增

*For any* 导入过程中的两个连续进度更新事件，后一个进度百分比应该大于或等于前一个进度百分比
**Validates: Requirements 1.4**

### Property 2: 完成时进度为100%

*For any* 成功完成的导入任务，当importCompleted(true)信号发出时，最后一次进度更新的百分比应该等于100
**Validates: Requirements 1.1, 1.5**

### Property 3: 配置文件路径一致性

*For any* 题库名称，保存导入规则和读取导入规则使用的文件路径应该相同
**Validates: Requirements 2.2, 2.1.1**

### Property 4: 题库列表不包含配置文件

*For any* 题库目录扫描结果，返回的文件列表中不应包含任何.json配置文件
**Validates: Requirements 2.1, 2.4**

### Property 5: 导入结果统计完整性

*For any* 成功导入的题目集合，导入结果统计中的题目总数应该等于实际保存的题目数量
**Validates: Requirements 3.1**

## Error Handling

### 进度计算错误

- **场景**: 计算出的进度百分比超过100%或小于0
- **处理**: 限制在[0, 100]范围内，记录警告日志
- **恢复**: 使用上一次有效的进度值

### 配置文件访问错误

- **场景**: 无法读写config目录中的规则文件
- **处理**: 记录错误日志，继续导入流程（规则文件非必需）
- **用户提示**: 在日志中提示规则文件保存失败，但不影响题目导入

### 统计数据不一致

- **场景**: 导入结果统计与实际文件数量不匹配
- **处理**: 重新扫描保存目录，以实际文件为准
- **用户提示**: 在完成弹窗中显示实际统计结果

## Testing Strategy

### Unit Tests

1. **进度计算测试**
   - 测试各阶段进度计算的正确性
   - 测试边界条件（0个文件、1个文件、大量文件）
   - 测试进度值的范围限制

2. **配置文件管理测试**
   - 测试保存和读取规则文件
   - 测试文件路径生成
   - 测试文件不存在的情况

3. **统计数据测试**
   - 测试按文件分类统计
   - 测试按难度分类统计
   - 测试空题库的统计

### Integration Tests

1. **完整导入流程测试**
   - 测试从扫描到完成的完整流程
   - 验证进度从0%到100%
   - 验证配置文件正确保存到config目录

2. **题库管理器集成测试**
   - 验证扫描题库时过滤配置文件
   - 验证删除题库时清理配置文件

### Property-Based Tests

使用Qt Test框架进行属性测试：

1. **进度单调性测试**
   - 生成随机的文件数量和题目数量
   - 验证所有进度更新都满足单调递增

2. **路径一致性测试**
   - 生成随机的题库名称
   - 验证保存和读取使用相同路径

## Implementation Notes

### 进度计算公式

```cpp
int ImportProgress::calculatePercentage() const {
    switch (currentStage) {
        case Scanning:
            // 扫描阶段: 0% → 10%
            return (processedFiles * 10) / qMax(1, totalFiles);
            
        case Parsing:
            // AI解析阶段: 10% → 95%
            if (totalFiles == 0) return 10;
            
            // 基础进度：已完成文件的进度
            int baseProgress = 10 + (currentFileIndex * 85) / totalFiles;
            
            // 当前文件内的进度：基于已识别题目数
            // 使用对数函数平滑增长
            double factor = totalQuestions / 5.0;
            int currentFileBonus = (85 / totalFiles) * (1 - exp(-factor));
            
            return qMin(95, baseProgress + currentFileBonus);
            
        case Saving:
            // 保存阶段: 95% → 100%
            return 95 + (5 * saveProgress) / 100;
            
        case Complete:
            return 100;
    }
}
```

### 配置文件命名规范

- 格式: `{bankName}_parse_rule.json`
- 示例: `CCF_parse_rule.json`, `LeetCode_parse_rule.json`
- 位置: `data/config/`

### 完成弹窗内容模板

```
✅ 导入完成！

📊 导入统计：
• 成功导入: {totalQuestions} 道题目
• 保存位置: data/基础题库/{bankName}/

📁 按源文件分类：
• {sourceFile1}: {count1} 道题目
• {sourceFile2}: {count2} 道题目
...

📈 难度分布：
• 🟢 简单: {easyCount} 道 ({easyPercent}%)
• 🟡 中等: {mediumCount} 道 ({mediumPercent}%)
• 🔴 困难: {hardCount} 道 ({hardPercent}%)

💡 提示：
• 题目已按源文件组织在不同文件夹中
• 可在题库面板中查看和练习这些题目
```

## AI生成模拟题库集成

### 规则文件读取流程

当用户请求为某个题库生成模拟题时，系统需要读取该题库的导入规则以保持风格一致：

```cpp
class MockExamGenerator {
public:
    // 生成模拟题库
    bool generateMockExam(const QString &sourceBankName, 
                         const QString &mockBankName,
                         int questionCount);
    
private:
    // 从config目录加载规则
    QJsonObject loadSourceBankRules(const QString &bankName);
    
    // 根据规则生成题目
    Question generateQuestionByRules(const QJsonObject &rules);
    
    // 应用格式规则
    QString applyFormatRules(const QString &content, 
                            const QJsonObject &modulePatterns);
};
```

### 生成流程

```
1. 用户选择源题库 → 检查规则文件
   ↓
2. 读取 data/config/{sourceBankName}_parse_rule.json
   ↓
3. 提取规则信息：
   • 难度分布比例
   • 知识点分布
   • 测试用例数量要求
   • Markdown格式模板
   ↓
4. 调用AI生成题目（使用规则约束）
   ↓
5. 保存到 data/基础题库/{mockBankName}/
   ↓
6. 为模拟题库创建新的规则文件
   data/config/{mockBankName}_parse_rule.json
```

### 规则应用示例

```cpp
bool MockExamGenerator::generateMockExam(const QString &sourceBankName, 
                                        const QString &mockBankName,
                                        int questionCount) {
    // 1. 加载源题库规则
    QJsonObject sourceRules = ImportRuleManager::loadImportRule(sourceBankName);
    
    if (sourceRules.isEmpty()) {
        // 规则文件不存在，提示用户
        emit error(QString("题库 '%1' 缺少导入规则文件，无法生成模拟题。\n"
                          "请先导入该题库以生成规则文件。").arg(sourceBankName));
        return false;
    }
    
    // 2. 提取统计信息
    QJsonObject stats = sourceRules["statistics"].toObject();
    QJsonObject diffDist = stats["difficultyDistribution"].toObject();
    double avgTestCases = stats["avgTestCases"].toDouble();
    
    // 3. 计算各难度题目数量
    int totalSource = stats["totalQuestions"].toInt();
    int easyCount = qRound(questionCount * diffDist["简单"].toInt() / (double)totalSource);
    int mediumCount = qRound(questionCount * diffDist["中等"].toInt() / (double)totalSource);
    int hardCount = questionCount - easyCount - mediumCount;
    
    // 4. 提取格式模板
    QJsonArray modulePatterns = sourceRules["modulePatterns"].toArray();
    
    // 5. 构建AI生成提示词
    QString prompt = buildGenerationPrompt(sourceBankName, 
                                          easyCount, mediumCount, hardCount,
                                          avgTestCases, modulePatterns);
    
    // 6. 调用AI生成
    // ... AI生成逻辑 ...
    
    // 7. 保存模拟题库的规则文件
    QJsonObject mockRules = sourceRules;  // 复制源规则
    mockRules["bankName"] = mockBankName;
    mockRules["sourceBank"] = sourceBankName;
    mockRules["isMockExam"] = true;
    mockRules["createdTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    ImportRuleManager::saveImportRule(mockBankName, mockRules);
    
    return true;
}
```

### AI生成提示词模板

```cpp
QString MockExamGenerator::buildGenerationPrompt(
    const QString &sourceBankName,
    int easyCount, int mediumCount, int hardCount,
    double avgTestCases,
    const QJsonArray &modulePatterns) {
    
    QString prompt = QString(R"(
你是编程题目生成专家。请为 %1 题库生成 %2 道模拟题目。

【题目要求】
1. 难度分布：
   - 简单题: %3 道
   - 中等题: %4 道
   - 困难题: %5 道

2. 测试用例：
   - 每道题至少 %6 组测试用例
   - 包含基本功能、边界条件、特殊情况

3. 格式要求（必须严格遵守）：
%7

【输出格式】
使用Markdown格式，每道题包含：
- 题目标题
- 题目描述
- 输入格式
- 输出格式
- 测试用例

请开始生成...
)").arg(sourceBankName)
   .arg(easyCount + mediumCount + hardCount)
   .arg(easyCount)
   .arg(mediumCount)
   .arg(hardCount)
   .arg(qRound(avgTestCases))
   .arg(formatModulePatterns(modulePatterns));
    
    return prompt;
}
```

### 错误处理

**场景1：规则文件不存在**
```cpp
if (!ImportRuleManager::hasImportRule(sourceBankName)) {
    QMessageBox::warning(parent, "缺少导入规则",
        QString("题库 '%1' 缺少导入规则文件。\n\n"
                "导入规则用于保持生成题目的格式一致性。\n"
                "请先通过AI导入功能导入该题库，系统会自动生成规则文件。")
        .arg(sourceBankName));
    return false;
}
```

**场景2：规则文件损坏**
```cpp
QJsonObject rules = ImportRuleManager::loadImportRule(sourceBankName);
if (!rules.contains("statistics") || !rules.contains("modulePatterns")) {
    QMessageBox::warning(parent, "规则文件损坏",
        QString("题库 '%1' 的导入规则文件不完整或已损坏。\n\n"
                "建议重新导入该题库以生成新的规则文件。")
        .arg(sourceBankName));
    return false;
}
```

### 用户界面提示

在模拟题库生成对话框中添加提示：

```cpp
// 检查规则文件状态
if (ImportRuleManager::hasImportRule(selectedBank)) {
    m_statusLabel->setText("✅ 已找到导入规则，可以生成模拟题");
    m_generateBtn->setEnabled(true);
} else {
    m_statusLabel->setText("⚠️ 缺少导入规则，请先导入该题库");
    m_generateBtn->setEnabled(false);
    
    // 提供导入按钮
    m_importBtn->setVisible(true);
    m_importBtn->setText("导入题库以生成规则");
}
```
