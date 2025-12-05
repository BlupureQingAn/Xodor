# AI判题功能设计文档

## 功能概述

在练习界面的"运行与测试"按钮旁边添加"AI判题"按钮，让AI分析代码逻辑判断是否符合题目要求，并自动更新题目的Accept状态。

## 需求详情

### 1. 触发方式
- 用户点击"AI判题"按钮（位于运行与测试按钮旁边）

### 2. AI判定逻辑
- AI纯粹通过分析代码逻辑判断是否符合题目要求
- 给出"通过/不通过"的结论
- 如果AI判定通过但实际运行测试失败，以AI判定为准
- AI会尝试修复失败的测试用例（更新JSON文件）
- 如果测试用例IO过长，放弃修复并标明"过长"

### 3. Accept状态存储
- 使用现有的 `ProgressManager` 管理题目状态
- 现有状态枚举 `QuestionStatus`：
  - `NotStarted` - 未开始
  - `InProgress` - 进行中
  - `Completed` - 已完成
  - `Mastered` - 已掌握
- 新增状态或复用 `Completed` 表示AI判定通过

### 4. UI反馈
- AI判定通过后弹窗提示："✅ AI判定通过，已更新题目状态"
- 题库列表中显示Accept状态（图标+颜色）

---

## 修改文件清单

### 1. `src/core/QuestionProgress.h`
**修改内容**：添加AI判定相关字段

```cpp
// 在 QuestionStatus 枚举中添加（可选，或复用Completed）
enum class QuestionStatus {
    NotStarted,     // 未开始
    InProgress,     // 进行中
    Completed,      // 已完成（运行测试通过）
    Mastered,       // 已掌握（多次正确）
    AIAccepted      // AI判定通过（新增）
};

// 在 QuestionProgressRecord 结构体中添加
struct QuestionProgressRecord {
    // ... 现有字段 ...
    bool aiJudgePassed;         // AI判定是否通过
    QDateTime aiJudgeTime;      // AI判定时间
    QString aiJudgeComment;     // AI判定评语
};
```

### 2. `src/core/ProgressManager.h` / `.cpp`
**修改内容**：添加AI判定相关API

```cpp
// 头文件添加
void recordAIJudge(const QString &questionId, bool passed, const QString &comment = QString());
bool isAIJudgePassed(const QString &questionId) const;

// 实现文件添加
void ProgressManager::recordAIJudge(const QString &questionId, bool passed, const QString &comment)
{
    QuestionProgressRecord record = getProgress(questionId);
    record.aiJudgePassed = passed;
    record.aiJudgeTime = QDateTime::currentDateTime();
    record.aiJudgeComment = comment;
    
    if (passed) {
        record.status = QuestionStatus::Completed;  // 或 AIAccepted
    }
    
    m_progressMap[questionId] = record;
    emit progressUpdated(questionId);
    save();
}
```

### 3. `src/ui/MainWindow.h` / `.cpp`
**修改内容**：添加AI判题按钮和槽函数

```cpp
// 头文件添加
private slots:
    void onAIJudgeClicked();
    void onAIJudgeResponse(const QString &response);

private:
    QPushButton *m_aiJudgeBtn;  // AI判题按钮
```

```cpp
// 实现文件 - 在setupUI中添加按钮
m_aiJudgeBtn = new QPushButton("🤖 AI判题", this);
m_aiJudgeBtn->setToolTip("让AI分析代码逻辑，判断是否符合题目要求");
// 添加到运行测试按钮旁边的布局中

connect(m_aiJudgeBtn, &QPushButton::clicked, this, &MainWindow::onAIJudgeClicked);
```

### 4. `src/ai/AIJudge.h` / `.cpp` (新建)
**新建文件**：AI判题核心逻辑

```cpp
// AIJudge.h
#ifndef AIJUDGE_H
#define AIJUDGE_H

#include <QObject>
#include "../core/Question.h"

class OllamaClient;

class AIJudge : public QObject
{
    Q_OBJECT
public:
    explicit AIJudge(OllamaClient *aiClient, QObject *parent = nullptr);
    
    void judgeCode(const Question &question, const QString &code);
    
signals:
    void judgeStarted();
    void judgeProgress(const QString &status);
    void judgeCompleted(bool passed, const QString &comment, const QVector<int> &failedTestCases);
    void testCaseFixed(int index, const QString &newInput, const QString &newOutput);
    void error(const QString &errorMsg);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    QString buildJudgePrompt(const Question &question, const QString &code);
    void parseJudgeResult(const QString &response);
    void fixFailedTestCases(const Question &question, const QString &code, const QVector<int> &failedIndices);
    
    OllamaClient *m_aiClient;
    Question m_currentQuestion;
    QString m_currentCode;
    QString m_currentResponse;
};

#endif // AIJUDGE_H
```

```cpp
// AIJudge.cpp
#include "AIJudge.h"
#include "OllamaClient.h"

AIJudge::AIJudge(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
{
}

QString AIJudge::buildJudgePrompt(const Question &question, const QString &code)
{
    QString prompt = QString(R"(
你是一个专业的代码评判专家。请分析以下C++代码是否正确实现了题目要求。

【题目信息】
标题：%1
描述：%2

【测试用例】
%3

【学生代码】
```cpp
%4
```

【评判要求】
1. 分析代码逻辑是否正确实现了题目要求
2. 检查边界条件处理是否完善
3. 判断代码能否通过所有测试用例
4. 如果代码正确但测试用例有问题，指出测试用例的问题

【输出格式】
请以JSON格式输出评判结果：
```json
{
    "passed": true/false,
    "comment": "评判说明",
    "codeAnalysis": "代码逻辑分析",
    "failedTestCases": [1, 3],  // 可能失败的测试用例索引（1-based），如果代码正确则为空
    "testCaseIssues": [         // 测试用例本身的问题（如果有）
        {
            "index": 1,
            "issue": "问题描述",
            "suggestedInput": "建议的输入",
            "suggestedOutput": "建议的输出"
        }
    ]
}
```

请开始评判：
)");

    // 构建测试用例文本
    QString testCasesText;
    QVector<TestCase> testCases = question.testCases();
    for (int i = 0; i < testCases.size(); ++i) {
        const TestCase &tc = testCases[i];
        testCasesText += QString("测试用例 %1：\n").arg(i + 1);
        testCasesText += QString("输入：\n%1\n").arg(tc.input.left(500));
        testCasesText += QString("期望输出：\n%1\n\n").arg(tc.expectedOutput.left(500));
    }

    return prompt.arg(question.title(), question.description(), testCasesText, code);
}

void AIJudge::judgeCode(const Question &question, const QString &code)
{
    if (!m_aiClient) {
        emit error("AI客户端未初始化");
        return;
    }
    
    m_currentQuestion = question;
    m_currentCode = code;
    m_currentResponse.clear();
    
    emit judgeStarted();
    emit judgeProgress("正在分析代码...");
    
    QString prompt = buildJudgePrompt(question, code);
    
    // 连接信号
    connect(m_aiClient, &OllamaClient::codeAnalysisReady, 
            this, &AIJudge::onAIResponse, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::error, 
            this, &AIJudge::onAIError, Qt::UniqueConnection);
    
    m_aiClient->sendCustomPrompt(prompt, "ai_judge");
}

void AIJudge::onAIResponse(const QString &response)
{
    // 断开信号
    disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
    disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    
    m_currentResponse = response;
    parseJudgeResult(response);
}

void AIJudge::parseJudgeResult(const QString &response)
{
    // 提取JSON
    QRegularExpression jsonRegex(R"(```json\s*(\{[\s\S]*?\})\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(response);
    
    if (!match.hasMatch()) {
        emit error("AI响应格式错误");
        return;
    }
    
    QString jsonStr = match.captured(1);
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isObject()) {
        emit error("JSON解析失败");
        return;
    }
    
    QJsonObject result = doc.object();
    bool passed = result["passed"].toBool();
    QString comment = result["comment"].toString();
    
    // 获取失败的测试用例
    QVector<int> failedTestCases;
    QJsonArray failedArray = result["failedTestCases"].toArray();
    for (const QJsonValue &val : failedArray) {
        failedTestCases.append(val.toInt());
    }
    
    // 处理测试用例问题
    QJsonArray issuesArray = result["testCaseIssues"].toArray();
    for (const QJsonValue &val : issuesArray) {
        QJsonObject issue = val.toObject();
        int index = issue["index"].toInt();
        QString suggestedInput = issue["suggestedInput"].toString();
        QString suggestedOutput = issue["suggestedOutput"].toString();
        
        // 检查是否过长
        if (suggestedInput.length() > 5000 || suggestedOutput.length() > 5000) {
            // 过长，放弃修复
            comment += QString("\n⚠️ 测试用例 %1 的IO数据过长，无法自动修复").arg(index);
        } else if (!suggestedInput.isEmpty() && !suggestedOutput.isEmpty()) {
            emit testCaseFixed(index, suggestedInput, suggestedOutput);
        }
    }
    
    emit judgeCompleted(passed, comment, failedTestCases);
}

void AIJudge::onAIError(const QString &error)
{
    disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
    disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    
    emit this->error(QString("AI判题失败：%1").arg(error));
}
```

### 5. `src/ui/QuestionListWidget.cpp`
**修改内容**：在题目列表中显示Accept状态

```cpp
// 在 filterQuestions() 函数中修改题目显示
void QuestionListWidget::filterQuestions()
{
    m_questionList->clear();
    
    // ... 现有筛选逻辑 ...
    
    for (int i = 0; i < m_allQuestions.size(); ++i) {
        const Question &q = m_allQuestions[i];
        
        // ... 现有筛选逻辑 ...
        
        // 获取题目状态
        QuestionProgressRecord progress = ProgressManager::instance().getProgress(q.id());
        QString statusIcon;
        QString statusColor;
        
        switch (progress.status) {
            case QuestionStatus::NotStarted:
                statusIcon = "⚪";  // 未开始
                statusColor = "#888888";
                break;
            case QuestionStatus::InProgress:
                statusIcon = "🔵";  // 进行中
                statusColor = "#2196F3";
                break;
            case QuestionStatus::Completed:
                statusIcon = "✅";  // 已完成/Accept
                statusColor = "#4CAF50";
                break;
            case QuestionStatus::Mastered:
                statusIcon = "⭐";  // 已掌握
                statusColor = "#FFD700";
                break;
        }
        
        // AI判定通过额外标记
        if (progress.aiJudgePassed) {
            statusIcon = "🤖✅";  // AI判定通过
        }
        
        QString itemText = QString("%1 %2. %3 [%4]")
            .arg(statusIcon)
            .arg(i + 1)
            .arg(q.title())
            .arg(difficultyText);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, i);
        item->setForeground(QColor(statusColor));
        m_questionList->addItem(item);
    }
}
```

### 6. `CMakeLists.txt`
**修改内容**：添加新文件

```cmake
# 在 AI 模块中添加
src/ai/AIJudge.cpp
src/ai/AIJudge.h
```

---

## 工作流程

```
用户点击"AI判题"按钮
        ↓
MainWindow::onAIJudgeClicked()
        ↓
AIJudge::judgeCode(question, code)
        ↓
构建AI提示词，发送给AI
        ↓
AI分析代码逻辑
        ↓
解析AI响应
        ↓
    ┌───────────────────┐
    │  AI判定通过？      │
    └───────────────────┘
        ↓是              ↓否
ProgressManager::         显示失败原因
recordAIJudge(true)       和改进建议
        ↓
更新题库列表显示
        ↓
弹窗提示"✅ AI判定通过"
        ↓
如果有测试用例问题，
自动修复JSON文件
```

---

## UI设计

### 按钮位置
```
┌─────────────────────────────────────────┐
│  [▶ 运行] [🧪 测试] [🤖 AI判题]         │
└─────────────────────────────────────────┘
```

### 题库列表状态显示
```
⚪ 1. 两数之和 [简单]           // 未开始
🔵 2. 反转链表 [中等]           // 进行中
✅ 3. 二分查找 [简单]           // 已完成
🤖✅ 4. 快速排序 [中等]         // AI判定通过
⭐ 5. 动态规划 [困难]           // 已掌握
```

### AI判题结果弹窗
```
┌─────────────────────────────────────────┐
│  ✅ AI判题结果                          │
├─────────────────────────────────────────┤
│                                         │
│  判定结果：通过                          │
│                                         │
│  评语：                                  │
│  代码逻辑正确，正确实现了两数之和的功能。  │
│  使用哈希表优化了时间复杂度到O(n)。       │
│                                         │
│  已自动更新题目状态为"已完成"            │
│                                         │
│              [确定]                      │
└─────────────────────────────────────────┘
```

---

## 实现优先级

1. **Phase 1**：基础功能
   - 添加AI判题按钮
   - 实现AIJudge类
   - 基本的通过/不通过判定

2. **Phase 2**：状态管理
   - 更新ProgressManager
   - 题库列表状态显示

3. **Phase 3**：测试用例修复
   - 解析AI建议的测试用例修复
   - 自动更新JSON文件
   - 过长数据的处理

---

## 注意事项

1. AI判题使用独立的OllamaClient实例，避免影响主对话框
2. 测试用例修复时需要验证数据完整性（不能有省略号）
3. 过长的IO数据（>5000字符）放弃修复，只标注
4. AI判定结果需要持久化保存
