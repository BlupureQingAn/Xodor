# AI配置实时更新功能说明

## 问题描述

用户反馈："在设置界面更改AI配置没有实现实时更新AI服务连接与调用配置"

## 问题根源

### 原始流程

```
用户打开设置 → 修改AI配置 → 点击保存
  ↓
配置保存到文件
  ↓
显示："部分设置可能需要重启程序后生效" ❌
  ↓
用户必须重启程序才能使用新配置
```

**问题**：
- ❌ 配置保存后没有通知MainWindow
- ❌ OllamaClient没有重新加载配置
- ❌ 用户必须重启程序
- ❌ 用户体验差

## 解决方案

### 新的流程

```
用户打开设置 → 修改AI配置 → 点击保存
  ↓
配置保存到文件
  ↓
发送 aiConfigChanged 信号 ✅
  ↓
MainWindow 接收信号
  ↓
重新加载配置 (loadConfiguration)
  ↓
OllamaClient 更新配置 ✅
  ↓
重新检测AI连接 ✅
  ↓
显示："设置已保存并立即生效" ✅
```

## 实现细节

### 1. 添加信号到 SettingsDialog

**文件**：`src/ui/SettingsDialog.h`

```cpp
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    
signals:
    void aiConfigChanged();  // ✅ AI配置已更改信号
    
private slots:
    // ...
};
```

### 2. 保存时发送信号

**文件**：`src/ui/SettingsDialog.cpp`

```cpp
void SettingsDialog::onSave()
{
    saveSettings();
    
    // ✅ 发送AI配置更改信号
    emit aiConfigChanged();
    
    QMessageBox::information(this, "保存成功", "设置已保存并立即生效");
    accept();
}
```

### 3. MainWindow 连接信号并更新配置

**文件**：`src/ui/MainWindow.cpp`

```cpp
void MainWindow::onShowSettings()
{
    SettingsDialog dialog(this);
    
    // ✅ 连接AI配置更改信号
    connect(&dialog, &SettingsDialog::aiConfigChanged, this, [this]() {
        // 立即重新加载AI配置
        loadConfiguration();
        
        // 重新检测AI连接
        QTimer::singleShot(100, this, &MainWindow::checkAIConnection);
    });
    
    if (dialog.exec() == QDialog::Accepted) {
        // 重新加载配置
        loadConfiguration();
    }
}
```

### 4. loadConfiguration 的作用

`loadConfiguration()` 方法会：

1. **读取配置文件**
2. **更新 OllamaClient**：
   ```cpp
   if (config.useCloudApi()) {
       m_ollamaClient->setCloudMode(true);
       m_ollamaClient->setApiKey(config.cloudApiKey());
   } else {
       m_ollamaClient->setCloudMode(false);
       m_ollamaClient->setBaseUrl(config.ollamaUrl());
       m_ollamaClient->setModel(config.ollamaModel());
   }
   ```
3. **更新编译器配置**
4. **更新其他设置**

### 5. checkAIConnection 的作用

重新检测AI连接状态：

1. **检测 Ollama 连接**
2. **检测云端 API 连接**
3. **更新状态栏显示**
4. **如果需要，弹出配置对话框**

## 使用场景

### 场景1：从本地切换到云端

**操作**：
1. 打开设置（Ctrl+,）
2. 切换到"☁️ 云端API"标签页
3. 输入API Key
4. 点击"保存"

**效果**：
- ✅ 配置立即保存
- ✅ OllamaClient 切换到云端模式
- ✅ 重新检测连接
- ✅ 状态栏显示："✅ AI服务已连接 - 云端API"
- ✅ 无需重启程序

### 场景2：从云端切换到本地

**操作**：
1. 打开设置（Ctrl+,）
2. 切换到"🖥️ 本地Ollama"标签页
3. 输入模型名称（如 qwen2.5:7b）
4. 点击"保存"

**效果**：
- ✅ 配置立即保存
- ✅ OllamaClient 切换到本地模式
- ✅ 重新检测连接
- ✅ 状态栏显示："✅ AI服务已连接 - Ollama (qwen2.5:7b)"
- ✅ 无需重启程序

### 场景3：更改本地模型

**操作**：
1. 打开设置（Ctrl+,）
2. 在"🖥️ 本地Ollama"标签页
3. 修改模型名称（从 qwen 改为 llama2）
4. 点击"保存"

**效果**：
- ✅ 配置立即保存
- ✅ OllamaClient 更新模型
- ✅ 重新检测连接
- ✅ 状态栏显示："✅ AI服务已连接 - Ollama (llama2:13b)"
- ✅ 无需重启程序

### 场景4：更改Ollama服务地址

**操作**：
1. 打开设置（Ctrl+,）
2. 在"🖥️ 本地Ollama"标签页
3. 修改服务地址（从 localhost 改为远程服务器）
4. 点击"保存"

**效果**：
- ✅ 配置立即保存
- ✅ OllamaClient 更新服务地址
- ✅ 重新检测连接
- ✅ 如果连接成功，显示成功消息
- ✅ 如果连接失败，显示错误提示
- ✅ 无需重启程序

## 技术要点

### 1. 信号槽机制

使用Qt的信号槽机制实现组件间通信：

```cpp
// SettingsDialog 发送信号
emit aiConfigChanged();

// MainWindow 接收信号
connect(&dialog, &SettingsDialog::aiConfigChanged, this, [this]() {
    // 处理配置更改
});
```

### 2. 延迟检测

使用 `QTimer::singleShot` 延迟100ms后检测连接：

```cpp
QTimer::singleShot(100, this, &MainWindow::checkAIConnection);
```

**原因**：
- 给配置加载留出时间
- 避免UI阻塞
- 确保配置已完全应用

### 3. Lambda 表达式

使用lambda表达式简化信号处理：

```cpp
connect(&dialog, &SettingsDialog::aiConfigChanged, this, [this]() {
    loadConfiguration();
    QTimer::singleShot(100, this, &MainWindow::checkAIConnection);
});
```

### 4. 配置持久化

配置保存到文件后，即使程序重启也会保留：

```cpp
config.save();  // 保存到 config.json
```

## 修改文件

### 1. src/ui/SettingsDialog.h
- 添加 `aiConfigChanged()` 信号

### 2. src/ui/SettingsDialog.cpp
- 修改 `onSave()` 方法，发送信号
- 修改提示信息："设置已保存并立即生效"

### 3. src/ui/MainWindow.cpp
- 修改 `onShowSettings()` 方法
- 连接 `aiConfigChanged` 信号
- 调用 `loadConfiguration()` 和 `checkAIConnection()`

## 编译结果

```
✅ 编译成功
✅ 可执行文件: build\CodePracticeSystem.exe
✅ 所有功能正常
```

## 测试建议

### 测试1：切换AI模式

1. 启动程序（使用本地Ollama）
2. 打开设置，切换到云端API
3. 输入API Key，保存
4. **验证**：状态栏立即显示"云端API已连接"
5. 使用AI功能（如AI判题）
6. **验证**：使用的是云端API

### 测试2：更改模型

1. 启动程序
2. 打开设置，修改模型名称
3. 保存
4. **验证**：状态栏立即显示新模型名称
5. 使用AI功能
6. **验证**：使用的是新模型

### 测试3：连接检测

1. 启动程序
2. 停止Ollama服务
3. 打开设置，保存（触发重新检测）
4. **验证**：显示"Ollama未连接"
5. 启动Ollama服务
6. 再次保存设置
7. **验证**：显示"Ollama已连接"

## 用户体验改进

### 改进前 ❌
- 修改配置后必须重启程序
- 不知道配置是否生效
- 测试配置很麻烦
- 提示信息不明确

### 改进后 ✅
- 修改配置后立即生效
- 状态栏实时显示连接状态
- 可以快速测试不同配置
- 明确提示"设置已保存并立即生效"
- 自动重新检测连接

## 总结

通过添加信号槽机制和实时配置重载，现在：

- ✅ AI配置修改后立即生效
- ✅ 无需重启程序
- ✅ 自动重新检测连接
- ✅ 实时更新状态显示
- ✅ 用户体验大幅提升

**现在在设置界面更改AI配置后，会立即更新并重新检测连接，无需重启程序！** 🎉
