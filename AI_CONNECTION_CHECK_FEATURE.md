# AI连接自动检查功能

## 📋 功能概述

软件启动后会自动检查本地AI模型（Ollama）和云端API的连接状态，并给出详细的诊断信息和解决方案。

## ✨ 主要特性

### 1. **启动时自动检查**
- 程序启动后延迟500ms自动检查AI连接
- 不阻塞主界面显示
- 在状态栏显示检查进度

### 2. **智能诊断**
检查以下内容：
- ✅ Ollama服务是否运行
- ✅ 配置的模型是否已下载
- ✅ 云端API Key是否有效（如果配置）
- ✅ 网络连接是否正常

### 3. **友好的错误提示**

#### 连接成功
```
✅ AI服务连接成功
模型：qwen:latest

您可以使用AI分析功能了！
```

#### 连接失败
```
⚠️ AI服务连接失败

【本地Ollama】
连接被拒绝

Ollama服务未运行
请在终端执行：ollama serve

━━━━━━━━━━━━━━━━━━━━
💡 解决方案：

方案1：使用本地Ollama（推荐）
1. 确保已安装Ollama
2. 在终端执行：ollama serve
3. 下载模型：ollama pull qwen
4. 重启本程序

方案2：配置云端API
1. 打开 工具 → 设置
2. 在AI标签页配置API Key
3. 保存设置

━━━━━━━━━━━━━━━━━━━━

注意：没有AI服务不影响刷题和编译功能，
只是无法使用AI代码分析功能。
```

### 4. **详细的错误分类**

| 错误类型 | 提示信息 | 解决方案 |
|---------|---------|---------|
| 连接被拒绝 | Ollama服务未运行 | 执行 `ollama serve` |
| 找不到服务器 | 服务地址配置错误 | 检查设置中的URL |
| 连接超时 | 网络不稳定或服务响应慢 | 检查网络和服务状态 |
| 404错误 | Ollama版本过旧 | 更新到最新版本 |
| 模型未找到 | 模型未安装 | 执行 `ollama pull <模型名>` |

### 5. **快捷操作**
- 点击"打开设置"按钮直接跳转到设置页面
- 点击"暂时忽略"继续使用程序（不影响刷题功能）

## 🔧 技术实现

### 新增文件

#### `src/utils/AIConnectionChecker.h/cpp`
AI连接检查器，负责：
- 异步检查Ollama服务状态
- 验证模型是否已下载
- 检查云端API连接（可选）
- 提供详细的诊断信息

### 核心功能

```cpp
// 检查Ollama连接
void checkOllamaConnection(const QString &baseUrl, const QString &model);

// 检查云端API连接
void checkCloudApiConnection(const QString &apiKey, const QString &apiUrl);

// 信号：所有检查完成
void allChecksCompleted(const AIConnectionStatus &status);
```

### 检查流程

```
启动程序
    ↓
延迟500ms（让界面先显示）
    ↓
读取配置（Ollama URL、模型名、API Key）
    ↓
并行检查
    ├─→ 检查Ollama服务（GET /api/tags）
    │   ├─→ 服务是否在线？
    │   └─→ 模型是否已下载？
    │
    └─→ 检查云端API（如果配置）
        └─→ API Key是否有效？
    ↓
汇总结果
    ↓
显示状态对话框
```

## 📊 检查结果

### AIConnectionStatus 结构
```cpp
struct AIConnectionStatus {
    bool ollamaAvailable;      // Ollama是否可用
    bool cloudApiAvailable;    // 云端API是否可用
    QString ollamaError;       // Ollama错误信息
    QString cloudApiError;     // 云端API错误信息
    QString ollamaModel;       // 检测到的模型名
    QString ollamaVersion;     // Ollama版本
};
```

## 🎯 用户体验优化

### 1. **非阻塞式检查**
- 使用异步网络请求
- 不影响主界面响应
- 5-10秒超时保护

### 2. **智能提示策略**
- ✅ 连接成功：仅在状态栏显示，不弹窗
- ⚠️ 连接失败：显示详细诊断对话框
- 💡 提供具体的解决步骤

### 3. **优雅降级**
- AI服务不可用时，程序仍可正常使用
- 刷题、编译、测试功能不受影响
- 只有AI分析功能会提示需要配置

## 🔍 诊断示例

### 场景1：Ollama服务未启动
```
状态栏：AI服务不可用（不影响刷题功能）

对话框：
⚠️ AI服务连接失败

【本地Ollama】
连接被拒绝

Ollama服务未运行
请在终端执行：ollama serve

[打开设置] [暂时忽略]
```

### 场景2：模型未下载
```
【本地Ollama】
模型 'qwen' 未安装

可用模型：
llama2:latest
codellama:latest

请下载所需模型：
ollama pull qwen
```

### 场景3：连接成功
```
状态栏：AI服务已就绪

（不弹窗，用户可直接使用）
```

## 📝 配置说明

### Ollama配置
在 `工具 → 设置 → AI` 中配置：
- **服务地址**：默认 `http://localhost:11434`
- **模型名称**：默认 `qwen`

### 云端API配置（可选）
- **API Key**：输入您的OpenAI或其他兼容API的密钥
- 作为Ollama的备用方案

## 🚀 使用建议

### 推荐配置（本地Ollama）
```bash
# 1. 安装Ollama
# 访问 https://ollama.ai 下载安装

# 2. 启动服务
ollama serve

# 3. 下载推荐模型
ollama pull qwen

# 4. 启动程序
# 程序会自动检测并连接
```

### 备用方案（云端API）
如果无法使用本地Ollama：
1. 打开 `工具 → 设置`
2. 切换到 `AI` 标签页
3. 输入云端API Key
4. 保存设置

## 🎨 界面样式

对话框采用与主题一致的深色风格：
- 背景色：`#242424`
- 文字色：`#e8e8e8`
- 按钮色：`#660000`（悬停：`#880000`）
- 详细信息：等宽字体，便于阅读命令

## ⚡ 性能优化

- **超时控制**：5-10秒超时，避免长时间等待
- **并行检查**：Ollama和云端API同时检查
- **延迟启动**：延迟500ms，优先显示主界面
- **异步处理**：不阻塞UI线程

## 🔄 后续改进方向

1. **定期重检**：在使用过程中定期检查连接状态
2. **自动重连**：检测到服务恢复时自动重连
3. **状态指示器**：在界面上显示AI服务状态图标
4. **一键修复**：提供自动启动Ollama服务的功能
5. **模型管理**：集成模型下载和管理功能

## 📦 相关文件

- `src/utils/AIConnectionChecker.h` - 连接检查器头文件
- `src/utils/AIConnectionChecker.cpp` - 连接检查器实现
- `src/ui/MainWindow.h` - 添加检查方法声明
- `src/ui/MainWindow.cpp` - 集成检查功能
- `CMakeLists.txt` - 添加新文件到构建系统

## ✅ 测试场景

- [x] Ollama服务正常运行
- [x] Ollama服务未启动
- [x] 模型未下载
- [x] 网络连接失败
- [x] API端点不存在（版本过旧）
- [x] 云端API Key无效
- [x] 云端API连接成功
- [x] 无任何AI服务配置

## 🎉 总结

这个功能让用户在启动程序时就能清楚地了解AI服务的状态，避免在使用AI功能时才发现问题。通过详细的诊断信息和解决方案，大大降低了配置门槛，提升了用户体验。

即使AI服务不可用，程序的核心功能（刷题、编译、测试）仍然可以正常使用，实现了优雅降级。
