# AI判题崩溃修复完成

## 问题描述

AI判题功能再次崩溃，用户无法使用AI判题功能。

## 问题诊断

通过代码审查发现以下问题：

### 1. 未初始化的指针 ⚠️
**文件**：`src/ui/MainWindow.cpp`

```cpp
// 问题：m_aiJudgeProgressDialog 未在构造函数初始化列表中初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentQuestionIndex(-1)
{
    // ...
}
```

**后果**：
- 指针包含随机值（野指针）
- 在 `onAIJudgeRequested()` 中检查 `if (!m_aiJudgeProgressDialog)` 可能失败
- 访问未初始化的指针导致崩溃

### 2. 缺少空指针检查
**文件**：`src/ai/AIJudge.cpp`

在信号断开时没有检查 `m_aiClient` 是否为空：

```cpp
void AIJudge::onAIResponse(const QString &response)
{
    // 问题：如果 m_aiClient 为空，disconnect 会崩溃
    disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
    disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    // ...
}
```

### 3. 缺少异常处理
解析AI响应时没有异常处理，如果解析失败会导致崩溃。

### 4. 缺少调试日志
没有足够的日志输出，难以诊断问题。

## 解决方案

### 1. 初始化指针 ✅
**文件**：`src/ui/MainWindow.cpp`

```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentQuestionIndex(-1)
    , m_aiJudgeProgressDialog(nullptr)  // ✅ 初始化为 nullptr
{
    setupUI();
    setupMenuBar();
    setupConnections();
    loadConfiguration();
    // ...
}
```

### 2. 加强空指针检查 ✅
**文件**：`src/ai/AIJudge.cpp`

#### judgeCode 方法
```cpp
void AIJudge::judgeCode(const Question &question, const QString &code)
{
    if (!m_aiClient) {
        qCritical() << "[AIJudge] ERROR: AI客户端未初始化";
        emit error("AI客户端未初始化");
        return;
    }
    
    qDebug() << "[AIJudge] Starting judge for question:" << question.id();
    
    // 先断开旧的连接，避免重复连接
    disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
    disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    
    // 连接信号
    connect(m_aiClient, &OllamaClient::codeAnalysisReady, 
            this, &AIJudge::onAIResponse, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::error, 
            this, &AIJudge::onAIError, Qt::UniqueConnection);
    
    qDebug() << "[AIJudge] Sending prompt to AI client...";
    m_aiClient->sendCustomPrompt(prompt, "custom");
}
```

#### onAIResponse 方法
```cpp
void AIJudge::onAIResponse(const QString &response)
{
    qDebug() << "[AIJudge] Received AI response, length:" << response.length();
    
    // ✅ 断开信号前检查指针
    if (m_aiClient) {
        disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
        disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    }
    
    m_currentResponse = response;
    
    // ✅ 添加异常处理
    try {
        parseJudgeResult(response);
    } catch (const std::exception &e) {
        qCritical() << "[AIJudge] Exception in parseJudgeResult:" << e.what();
        emit error(QString("解析AI响应时发生错误：%1").arg(e.what()));
    } catch (...) {
        qCritical() << "[AIJudge] Unknown exception in parseJudgeResult";
        emit error("解析AI响应时发生未知错误");
    }
}
```

#### onAIError 方法
```cpp
void AIJudge::onAIError(const QString &error)
{
    qWarning() << "[AIJudge] AI error:" << error;
    
    // ✅ 断开信号前检查指针
    if (m_aiClient) {
        disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
        disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    }
    
    emit this->error(QString("AI判题失败：%1").arg(error));
}
```

### 3. 增强JSON解析 ✅
**文件**：`src/ai/AIJudge.cpp`

```cpp
void AIJudge::parseJudgeResult(const QString &response)
{
    qDebug() << "[AIJudge] Parsing judge result...";
    
    // ✅ 检查空响应
    if (response.isEmpty()) {
        qWarning() << "[AIJudge] Empty response";
        emit error("AI返回了空响应");
        return;
    }
    
    // 提取JSON（支持两种格式）
    QRegularExpression jsonRegex(R"(```json\s*(\{[\s\S]*?\})\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(response);
    
    if (!match.hasMatch()) {
        qDebug() << "[AIJudge] No JSON block found, trying to find raw JSON...";
        // 尝试直接查找JSON对象
        int jsonStart = response.indexOf('{');
        if (jsonStart >= 0) {
            int jsonEnd = response.lastIndexOf('}');
            if (jsonEnd > jsonStart) {
                QString jsonStr = response.mid(jsonStart, jsonEnd - jsonStart + 1);
                
                // ✅ 使用 QJsonParseError 检查解析错误
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
                
                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "[AIJudge] JSON parse error:" << parseError.errorString();
                    emit error(QString("JSON解析失败：%1").arg(parseError.errorString()));
                    return;
                }
                
                if (doc.isObject()) {
                    QJsonObject result = doc.object();
                    bool passed = result["passed"].toBool();
                    QString comment = result.value("comment").toString();
                    
                    // ✅ 提供默认值
                    if (comment.isEmpty()) {
                        comment = "AI未提供评论";
                    }
                    
                    QVector<int> failedTestCases;
                    if (result.contains("failedTestCases")) {
                        QJsonArray failedArray = result["failedTestCases"].toArray();
                        for (const QJsonValue &val : failedArray) {
                            if (val.isDouble()) {  // ✅ 检查类型
                                failedTestCases.append(val.toInt());
                            }
                        }
                    }
                    
                    qDebug() << "[AIJudge] Parse success - Passed:" << passed;
                    emit judgeCompleted(passed, comment, failedTestCases);
                    return;
                }
            }
        }
        
        qWarning() << "[AIJudge] No valid JSON found in response";
        emit error("AI响应格式错误：未找到有效的JSON数据");
        return;
    }
    
    // 处理标准JSON块格式...
    // （完整的错误检查和类型验证）
}
```

### 4. 添加调试日志 ✅

在关键位置添加了详细的调试日志：

```cpp
qDebug() << "[AIJudge] Starting judge for question:" << question.id();
qDebug() << "[AIJudge] Prompt length:" << prompt.length();
qDebug() << "[AIJudge] Sending prompt to AI client...";
qDebug() << "[AIJudge] Received AI response, length:" << response.length();
qDebug() << "[AIJudge] Parsing judge result...";
qDebug() << "[AIJudge] Found JSON block, length:" << jsonStr.length();
qDebug() << "[AIJudge] Parse success - Passed:" << passed;
```

## 修复内容总结

| 问题 | 修复方法 | 状态 |
|------|---------|------|
| 未初始化的指针 | 在构造函数初始化列表中初始化为 nullptr | ✅ |
| 缺少空指针检查 | 在断开信号前检查 m_aiClient | ✅ |
| 缺少异常处理 | 添加 try-catch 捕获异常 | ✅ |
| JSON解析不健壮 | 使用 QJsonParseError 检查错误 | ✅ |
| 缺少类型检查 | 检查 JSON 值类型再转换 | ✅ |
| 缺少默认值 | 为可选字段提供默认值 | ✅ |
| 缺少调试日志 | 添加详细的调试日志 | ✅ |

## 修改文件清单

1. ✅ `src/ui/MainWindow.cpp` - 初始化 m_aiJudgeProgressDialog
2. ✅ `src/ai/AIJudge.cpp` - 加强空指针检查、异常处理、JSON解析
3. ✅ 编译成功，无错误

## 测试验证

### 测试步骤
1. 启动程序，加载题库
2. 选择一道题目
3. 编写代码
4. 点击"AI判题"按钮
5. 观察调试日志和结果

### 预期结果
- ✅ 不会崩溃
- ✅ 显示进度对话框
- ✅ AI分析代码
- ✅ 显示判题结果
- ✅ 如果出错，显示友好的错误信息

### 调试日志示例
```
[AIJudge] Starting judge for question: question_001 两数之和
[AIJudge] Prompt length: 1234
[AIJudge] Sending prompt to AI client...
[AIJudge] Received AI response, length: 567
[AIJudge] Parsing judge result...
[AIJudge] Found JSON block, length: 234
[AIJudge] Parse success - Passed: true Failed cases: 0
```

## 防崩溃机制

### 1. 指针初始化
所有指针成员变量在构造函数初始化列表中初始化为 nullptr。

### 2. 空指针检查
在使用指针前检查是否为空：
```cpp
if (m_aiClient) {
    // 安全使用
}
```

### 3. 异常捕获
使用 try-catch 捕获异常，防止程序崩溃：
```cpp
try {
    parseJudgeResult(response);
} catch (const std::exception &e) {
    emit error(QString("错误：%1").arg(e.what()));
} catch (...) {
    emit error("未知错误");
}
```

### 4. 类型检查
在转换 JSON 值前检查类型：
```cpp
if (val.isDouble()) {
    int value = val.toInt();
}
```

### 5. 默认值
为可选字段提供默认值：
```cpp
QString comment = result.value("comment").toString();
if (comment.isEmpty()) {
    comment = "AI未提供评论";
}
```

### 6. 详细日志
添加详细的调试日志，便于诊断问题。

## 总结

AI判题功能崩溃问题已修复：
- ✅ 修复未初始化的指针
- ✅ 加强空指针检查
- ✅ 添加异常处理
- ✅ 增强JSON解析健壮性
- ✅ 添加详细调试日志
- ✅ 编译成功

现在AI判题功能应该能够稳定运行，即使遇到错误也会显示友好的错误信息而不是崩溃！
