# AI 连接检测逻辑修复

## 问题描述

启动时检测 AI 配置连接状态的逻辑有问题：
- 明明配置的是云端 API 且可以正常使用
- 但启动时却显示"AI服务暂时无法连接"
- 原因是检测逻辑没有区分用户配置的模式（本地/云端）

## 问题根源

### 原有逻辑问题

在 `MainWindow::showAIConnectionStatus()` 中：

```cpp
// 错误的逻辑：不管用户配置的是什么模式，都检查两个状态
if (status.ollamaAvailable) {
    statusBar()->showMessage("✓ Ollama已连接");
} else if (status.cloudApiAvailable) {
    statusBar()->showMessage("✓ 云端API已连接");
} else {
    // 即使用户只配置了云端API，但因为本地Ollama不可用
    // 也会显示"暂时无法连接"
    statusBar()->showMessage("⚠ AI服务已配置但暂时无法连接");
}
```

### 问题场景

1. **用户配置了云端 API**（`useCloudApi = true`）
2. **本地没有安装 Ollama**
3. 启动时检测：
   - `status.ollamaAvailable = false`（本地没有 Ollama）
   - `status.cloudApiAvailable = true`（云端 API 正常）
4. 但因为先判断 `ollamaAvailable`，所以走到 `else` 分支
5. 显示"暂时无法连接"❌

## 修复方案

### 1. 修复 `checkAIConnection()` - 只检查配置的模式

```cpp
void MainWindow::checkAIConnection()
{
    ConfigManager &config = ConfigManager::instance();
    AIConnectionChecker *checker = new AIConnectionChecker(this);
    
    connect(checker, &AIConnectionChecker::allChecksCompleted, this, 
            &MainWindow::showAIConnectionStatus);
    
    statusBar()->showMessage("正在检查AI服务连接...", 0);
    
    // 根据用户配置的模式，只检查对应的服务
    bool useCloudApi = config.useCloudApi();
    
    if (useCloudApi) {
        // 用户配置的是云端API，只检查云端
        QString cloudApiKey = config.cloudApiKey();
        QString cloudApiUrl = "https://api.openai.com/v1/chat/completions";
        
        if (!cloudApiKey.isEmpty()) {
            checker->checkCloudApiConnection(cloudApiKey, cloudApiUrl);
        }
    } else {
        // 用户配置的是本地Ollama，只检查本地
        QString ollamaUrl = config.ollamaUrl();
        QString ollamaModel = config.ollamaModel();
        
        if (ollamaUrl.isEmpty()) {
            ollamaUrl = "http://localhost:11434";
        }
        if (ollamaModel.isEmpty()) {
            ollamaModel = "qwen";
        }
        
        checker->checkOllamaConnection(ollamaUrl, ollamaModel);
    }
}
```

### 2. 修复 `showAIConnectionStatus()` - 根据配置模式判断

```cpp
void MainWindow::showAIConnectionStatus(const AIConnectionStatus &status)
{
    ConfigManager &config = ConfigManager::instance();
    
    // 检查是否已配置过（只看配置，不管连接状态）
    bool hasOllamaConfig = !config.ollamaUrl().isEmpty() && !config.ollamaModel().isEmpty();
    bool hasCloudConfig = !config.cloudApiKey().isEmpty();
    
    // 只在完全未配置时弹窗
    if (!hasOllamaConfig && !hasCloudConfig) {
        QTimer::singleShot(100, this, &MainWindow::checkAndSelectModel);
        statusBar()->showMessage("⚠ AI服务未配置（不影响刷题功能）", 0);
        return;
    }
    
    // 根据用户配置的模式来判断连接状态
    bool useCloudApi = config.useCloudApi();
    
    if (useCloudApi) {
        // 用户配置的是云端API模式
        if (status.cloudApiAvailable) {
            statusBar()->showMessage("✓ 云端API已连接", 5000);
        } else {
            // 云端API连接失败
            statusBar()->showMessage("⚠ 云端API暂时无法连接（可在设置中检查）", 5000);
        }
    } else {
        // 用户配置的是本地Ollama模式
        if (status.ollamaAvailable) {
            statusBar()->showMessage(QString("✓ Ollama已连接 - %1").arg(status.ollamaModel), 5000);
        } else {
            // 本地Ollama连接失败
            statusBar()->showMessage("⚠ Ollama暂时无法连接（可在设置中检查）", 5000);
        }
    }
}
```

## 修复效果

### 修复前
- ❌ 配置云端 API → 显示"暂时无法连接"
- ❌ 配置本地 Ollama → 可能显示云端 API 状态

### 修复后
- ✅ 配置云端 API → 只检查云端，显示"✓ 云端API已连接"
- ✅ 配置本地 Ollama → 只检查本地，显示"✓ Ollama已连接 - qwen"
- ✅ 连接失败时，明确提示是哪个服务无法连接

## 关键改进

1. **按需检测**：只检测用户配置的服务，不做无关检测
2. **精准提示**：根据配置模式显示对应的连接状态
3. **避免误报**：不会因为未配置的服务不可用而显示错误

## 测试建议

1. **测试云端 API 模式**：
   - 在设置中配置云端 API Key
   - 重启程序
   - 应显示"✓ 云端API已连接"

2. **测试本地 Ollama 模式**：
   - 在设置中配置本地 Ollama
   - 确保 Ollama 服务运行中
   - 重启程序
   - 应显示"✓ Ollama已连接 - [模型名]"

3. **测试连接失败**：
   - 配置云端 API 但使用无效 Key
   - 应显示"⚠ 云端API暂时无法连接"
   - 配置本地 Ollama 但服务未运行
   - 应显示"⚠ Ollama暂时无法连接"

## 文件修改

- `src/ui/MainWindow.cpp`
  - `checkAIConnection()` - 根据配置模式选择性检测
  - `showAIConnectionStatus()` - 根据配置模式显示状态
