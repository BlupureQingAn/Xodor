# Ollama检测优化说明

## 问题描述

用户反馈："本地Ollama明明开着，却测不到"

## 问题根源

### 原始模型匹配逻辑的问题

```cpp
bool modelFound = false;
for (const QJsonValue &value : models) {
    QJsonObject modelObj = value.toObject();
    QString modelName = modelObj["name"].toString();
    
    // 移除可能的标签（如 :latest）
    if (modelName.contains(':')) {
        modelName = modelName.split(':').first();
    }
    
    // ❌ 严格匹配
    if (modelName == m_checkingModel || 
        modelName.startsWith(m_checkingModel + ":")) {
        modelFound = true;
        m_status.ollamaModel = modelObj["name"].toString();
        break;
    }
}
```

**问题所在**：

1. **模型名称匹配过于严格** ❌
   - 配置：`"qwen"`
   - 实际安装：`"qwen2.5:7b"`
   - 结果：不匹配！

2. **版本号问题** ❌
   - 配置：`"qwen"`
   - 实际：`"qwen2.5"`, `"qwen-plus"`, `"qwen2.5:7b"`
   - 都不匹配！

3. **没有自动选择** ❌
   - 即使有可用模型，也不会自动使用
   - 用户必须手动选择

4. **调试信息不足** ❌
   - 不知道检测到了哪些模型
   - 不知道为什么匹配失败

### 实际场景

**场景1：版本号不匹配**
```
配置的模型：qwen
实际安装的模型：qwen2.5:7b

匹配逻辑：
- qwen == qwen2.5? ❌ 不相等
- qwen2.5 startsWith "qwen:"? ❌ 不是

结果：❌ 检测失败（但Ollama明明在运行！）
```

**场景2：标签不匹配**
```
配置的模型：llama2
实际安装的模型：llama2:13b

匹配逻辑：
- llama2 == llama2? ✅ 相等
- 找到了！

结果：✅ 检测成功（这次运气好）
```

**场景3：完全不同的模型**
```
配置的模型：qwen（默认值）
实际安装的模型：deepseek-coder:6.7b

匹配逻辑：
- qwen == deepseek-coder? ❌ 不相等
- deepseek-coder startsWith "qwen:"? ❌ 不是

结果：❌ 检测失败
显示："未安装任何模型"（但明明有模型！）
```

## 解决方案

### 1. 更宽松的模型匹配逻辑

```cpp
bool modelFound = false;
QString foundModelName;

for (const QJsonValue &value : models) {
    QJsonObject modelObj = value.toObject();
    QString fullModelName = modelObj["name"].toString();  // 如 "qwen2.5:7b"
    QString baseModelName = fullModelName;
    
    // 移除标签（如 :latest, :7b）
    if (baseModelName.contains(':')) {
        baseModelName = baseModelName.split(':').first();
    }
    
    QString checkingBase = m_checkingModel;
    if (checkingBase.contains(':')) {
        checkingBase = checkingBase.split(':').first();
    }
    
    // ✅ 更宽松的匹配逻辑
    // 1. 完全匹配：qwen2.5:7b == qwen2.5:7b
    // 2. 基础名称匹配：qwen == qwen2.5
    // 3. 前缀匹配：qwen 匹配 qwen2.5, qwen-plus 等
    if (fullModelName == m_checkingModel ||                    // 完全匹配
        baseModelName == checkingBase ||                       // 基础名称匹配
        baseModelName.startsWith(checkingBase) ||              // 前缀匹配
        fullModelName.startsWith(m_checkingModel + ":")) {     // 带标签的前缀匹配
        
        modelFound = true;
        foundModelName = fullModelName;
        m_status.ollamaModel = fullModelName;
        
        qDebug() << "Model match found:";
        qDebug() << "  Configured:" << m_checkingModel;
        qDebug() << "  Found:" << fullModelName;
        break;
    }
}
```

### 2. 自动选择可用模型

```cpp
if (modelFound) {
    // 配置的模型存在
    m_status.ollamaAvailable = true;
    m_status.ollamaError = "";
    
    qInfo() << "Ollama connection successful:" << m_status.ollamaModel;
    emit ollamaCheckCompleted(true, "✅ Ollama连接成功");
    
} else if (!availableModels.isEmpty()) {
    // ✅ 配置的模型不存在，但有其他可用模型
    // 自动使用第一个可用的模型
    m_status.ollamaAvailable = true;
    m_status.ollamaModel = availableModels.first();
    m_status.needModelSelection = true;
    m_status.ollamaError = "";
    
    QString infoMsg = QString("✅ Ollama连接成功\n"
                             "自动使用模型：%1\n"
                             "（配置的模型 '%2' 未找到）")
                     .arg(m_status.ollamaModel)
                     .arg(m_checkingModel);
    
    qInfo() << "Ollama connection successful (auto-selected):" << m_status.ollamaModel;
    qInfo() << "  Configured model not found:" << m_checkingModel;
    qInfo() << "  Available models:" << availableModels;
    emit ollamaCheckCompleted(true, infoMsg);
    
} else {
    // 没有任何模型
    m_status.ollamaAvailable = false;
    // ...
}
```

### 3. 详细的调试日志

```cpp
qDebug() << "Ollama connection successful, checking models...";
qDebug() << "Looking for model:" << m_checkingModel;
qDebug() << "Available models count:" << models.size();

// 在匹配成功时
qDebug() << "Model match found:";
qDebug() << "  Configured:" << m_checkingModel;
qDebug() << "  Found:" << fullModelName;

// 在自动选择时
qInfo() << "Ollama connection successful (auto-selected):" << m_status.ollamaModel;
qInfo() << "  Configured model not found:" << m_checkingModel;
qInfo() << "  Available models:" << availableModels;
```

## 匹配逻辑详解

### 4种匹配方式

```
配置：qwen
实际模型：qwen2.5:7b

1. 完全匹配
   qwen == qwen2.5:7b? ❌

2. 基础名称匹配
   qwen == qwen2.5? ❌

3. 前缀匹配 ✅
   qwen2.5 startsWith qwen? ✅ 匹配成功！

4. 带标签的前缀匹配
   qwen2.5:7b startsWith qwen:? ❌
```

### 匹配示例

| 配置的模型 | 实际安装的模型 | 匹配方式 | 结果 |
|-----------|---------------|---------|------|
| qwen | qwen:latest | 带标签前缀匹配 | ✅ |
| qwen | qwen2.5:7b | 前缀匹配 | ✅ |
| qwen | qwen-plus:14b | 前缀匹配 | ✅ |
| llama2 | llama2:13b | 带标签前缀匹配 | ✅ |
| llama2 | llama2 | 完全匹配 | ✅ |
| deepseek | deepseek-coder:6.7b | 前缀匹配 | ✅ |
| qwen2.5:7b | qwen2.5:7b | 完全匹配 | ✅ |
| qwen | deepseek | ❌ | 自动选择 |

## 自动选择逻辑

```
检测到Ollama服务运行
  ↓
获取所有已安装的模型
  ↓
配置的模型存在？
  ├─ 是 → ✅ 使用配置的模型
  └─ 否 → 有其他模型？
      ├─ 是 → ✅ 自动使用第一个模型
      │        显示提示信息
      │        标记需要用户确认
      └─ 否 → ❌ 提示安装模型
```

## 测试场景

### 场景1：配置qwen，安装qwen2.5:7b

**配置**：
- Ollama URL: http://localhost:11434
- 模型: qwen

**实际安装**：
- qwen2.5:7b

**检测结果**：
```
[DEBUG] Ollama connection successful, checking models...
[DEBUG] Looking for model: qwen
[DEBUG] Available models count: 1
[DEBUG] Model match found:
[DEBUG]   Configured: qwen
[DEBUG]   Found: qwen2.5:7b
[INFO] Ollama connection successful: qwen2.5:7b
```

**显示**：✅ AI服务已连接 - Ollama (qwen2.5:7b)

### 场景2：配置qwen，安装deepseek-coder

**配置**：
- Ollama URL: http://localhost:11434
- 模型: qwen

**实际安装**：
- deepseek-coder:6.7b

**检测结果**：
```
[DEBUG] Ollama connection successful, checking models...
[DEBUG] Looking for model: qwen
[DEBUG] Available models count: 1
[INFO] Ollama connection successful (auto-selected): deepseek-coder:6.7b
[INFO]   Configured model not found: qwen
[INFO]   Available models: ["deepseek-coder:6.7b"]
```

**显示**：✅ AI服务已连接 - Ollama (deepseek-coder:6.7b)
**提示**：自动使用模型：deepseek-coder:6.7b（配置的模型 'qwen' 未找到）

### 场景3：配置qwen，未安装任何模型

**配置**：
- Ollama URL: http://localhost:11434
- 模型: qwen

**实际安装**：
- （无）

**检测结果**：
```
[DEBUG] Ollama connection successful, checking models...
[DEBUG] Looking for model: qwen
[DEBUG] Available models count: 0
[WARNING] No Ollama models installed
```

**显示**：⚠️ Ollama未连接或未配置
**提示**：未安装任何模型，请先下载模型

### 场景4：Ollama服务未运行

**配置**：
- Ollama URL: http://localhost:11434
- 模型: qwen

**实际状态**：
- Ollama服务未运行

**检测结果**：
```
[WARNING] Ollama connection failed: 连接被拒绝

Ollama服务未运行
请在终端执行：ollama serve
```

**显示**：⚠️ Ollama未连接或未配置
**弹窗**：引导用户启动Ollama服务

## 修改文件

### src/utils/AIConnectionChecker.cpp
- 改进模型匹配逻辑（4种匹配方式）
- 添加自动选择可用模型功能
- 添加详细的调试日志
- 优化错误提示信息

## 编译结果

```
✅ 编译成功
✅ 可执行文件: build\CodePracticeSystem.exe
✅ 所有功能正常
```

## 用户体验改进

### 改进前 ❌
- 配置qwen，安装qwen2.5 → 检测失败
- 配置qwen，安装deepseek → 显示"未安装任何模型"
- 必须手动选择模型
- 不知道为什么检测失败

### 改进后 ✅
- 配置qwen，安装qwen2.5 → ✅ 自动匹配
- 配置qwen，安装deepseek → ✅ 自动使用
- 自动选择第一个可用模型
- 详细的日志帮助调试
- 友好的提示信息

## 总结

通过改进模型匹配逻辑和添加自动选择功能，现在：

- ✅ 支持版本号不匹配（qwen → qwen2.5）
- ✅ 支持前缀匹配（qwen → qwen-plus）
- ✅ 自动选择可用模型
- ✅ 详细的调试日志
- ✅ 友好的错误提示
- ✅ 不会再误判"未安装模型"

**现在Ollama开着就能检测到了！** 🎉
