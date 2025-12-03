# AI模型检测修复

## 问题描述
AI配置弹窗无法检测到本地Ollama模型

## 问题原因
1. 程序启动时，`loadConfiguration()`会根据配置文件设置AI模式
2. 如果配置文件中有云端API Key，会调用`setCloudMode(true)`
3. `setCloudMode(true)`会将`m_baseUrl`改为云端API地址（`https://api.deepseek.com`）
4. 后续调用`getAvailableModels()`时，使用的是云端URL而非本地URL
5. 导致无法检测到本地Ollama模型

## 解决方案

### 方案1: 固定本地URL检测
**文件**: `src/ai/OllamaClient.cpp`

在`getAvailableModels()`方法中，始终使用固定的本地Ollama URL：
```cpp
QString ollamaUrl = "http://localhost:11434";
QNetworkRequest request(QUrl(ollamaUrl + "/api/tags"));
```

**优点**: 简单直接，不受当前模式影响
**缺点**: 如果用户的Ollama运行在其他端口，无法检测

### 方案2: 临时切换模式（已采用）
**文件**: `src/ui/MainWindow.cpp`

在`checkAndSelectModel()`中：
1. 保存当前模式状态
2. 临时切换到本地模式
3. 检测可用模型
4. 恢复之前的模式

```cpp
// 临时保存当前模式
bool wasCloudMode = m_ollamaClient->isCloudMode();

// 临时切换到本地模式以检测可用模型
if (wasCloudMode) {
    m_ollamaClient->setCloudMode(false);
    m_ollamaClient->setBaseUrl(config.ollamaUrl());
}

// 获取可用模型列表
QStringList availableModels = m_ollamaClient->getAvailableModels();

// 恢复之前的模式
if (wasCloudMode) {
    m_ollamaClient->setCloudMode(true);
    m_ollamaClient->setApiKey(config.cloudApiKey());
}
```

**优点**: 
- 尊重用户配置的Ollama URL
- 不影响当前的AI模式
- 检测完成后恢复原状态

### 新增方法
**文件**: `src/ai/OllamaClient.h`
```cpp
bool isCloudMode() const { return m_cloudMode; }
```

用于查询当前是否处于云端模式。

## 修改文件清单
- ✅ `src/ai/OllamaClient.h` - 添加isCloudMode()方法
- ✅ `src/ai/OllamaClient.cpp` - 固定本地URL检测
- ✅ `src/ui/MainWindow.cpp` - 临时切换模式检测

## 工作流程

### 启动流程
```
程序启动
    ↓
loadConfiguration()
    ↓
检查配置文件
    ↓
如果有cloudApiKey → setCloudMode(true)
如果有ollamaModel → setCloudMode(false)
    ↓
checkAndSelectModel()
    ↓
保存当前模式 (wasCloudMode)
    ↓
临时切换到本地模式
    ↓
getAvailableModels() → 使用本地URL检测
    ↓
恢复之前的模式
    ↓
显示配置对话框（显示检测到的模型）
```

## 测试场景

### 场景1: 首次启动（无配置）
- ✅ 应该能检测到本地模型
- ✅ 显示模型列表供用户选择

### 场景2: 已配置云端API
- ✅ 启动时使用云端模式
- ✅ 打开配置对话框时仍能检测本地模型
- ✅ 用户可以切换回本地模式

### 场景3: 已配置本地模型
- ✅ 启动时使用本地模式
- ✅ 打开配置对话框时能检测本地模型
- ✅ 显示当前配置的模型

### 场景4: Ollama未运行
- ✅ 检测超时（5秒）
- ✅ 显示"未检测到Ollama模型"提示
- ✅ 引导用户安装或启动Ollama

## 编译状态
✅ 编译成功，无错误，无警告

## 关键改进
1. **双重保障**: 既固定了检测URL，又实现了模式切换
2. **状态保护**: 检测完成后恢复原状态，不影响当前配置
3. **用户友好**: 无论当前什么模式，都能检测本地模型
