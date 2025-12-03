# AI模式切换重构完成

## 问题描述
启动时的AI配置弹窗存在问题：
- 用户选择云端API模式后，系统仍然使用本地Ollama模型
- 没有正确的AI模式切换机制
- 配置保存后没有立即生效

## 解决方案

### 1. ConfigManager增强
**文件**: `src/utils/ConfigManager.h`

添加了AI模式判断方法：
```cpp
bool useCloudApi() const { return !m_cloudApiKey.isEmpty(); }
bool useLocalOllama() const { return !m_ollamaModel.isEmpty() && m_cloudApiKey.isEmpty(); }
```

**逻辑**:
- 如果`cloudApiKey`不为空 → 使用云端API模式
- 如果`ollamaModel`不为空且`cloudApiKey`为空 → 使用本地Ollama模式

### 2. OllamaClient支持双模式
**文件**: `src/ai/OllamaClient.h`, `src/ai/OllamaClient.cpp`

#### 新增成员变量
```cpp
QString m_apiKey;      // 云端API密钥
bool m_cloudMode;      // 是否云端模式
```

#### 新增方法
```cpp
void setApiKey(const QString &apiKey);     // 设置API Key
void setCloudMode(bool enabled);           // 切换模式
```

#### 模式切换逻辑
- **本地模式**: 
  - URL: `http://localhost:11434/api/generate`
  - 格式: Ollama原生格式
  
- **云端模式**:
  - URL: `https://api.deepseek.com/v1/chat/completions`
  - 格式: OpenAI兼容格式
  - 认证: Bearer Token

#### 请求格式适配
```cpp
// 本地Ollama格式
{
  "model": "qwen2.5:7b",
  "prompt": "...",
  "stream": true
}

// 云端API格式 (OpenAI兼容)
{
  "model": "deepseek-chat",
  "messages": [{"role": "user", "content": "..."}],
  "stream": true
}
```

#### 流式响应处理
- **本地**: 解析`response`字段
- **云端**: 解析`choices[0].delta.content`字段，处理`data: `前缀

### 3. MainWindow配置加载
**文件**: `src/ui/MainWindow.cpp`

#### loadConfiguration方法
```cpp
if (config.useCloudApi()) {
    // 使用云端API
    m_ollamaClient->setCloudMode(true);
    m_ollamaClient->setApiKey(config.cloudApiKey());
} else {
    // 使用本地Ollama
    m_ollamaClient->setCloudMode(false);
    m_ollamaClient->setBaseUrl(config.ollamaUrl());
    m_ollamaClient->setModel(config.ollamaModel());
}
```

#### checkAndSelectModel方法
用户选择后立即切换模式：

**本地模式**:
```cpp
config.setOllamaModel(selectedModel);
config.setCloudApiKey("");  // 清空云端配置
config.save();

m_ollamaClient->setCloudMode(false);
m_ollamaClient->setBaseUrl(config.ollamaUrl());
m_ollamaClient->setModel(selectedModel);
```

**云端模式**:
```cpp
config.setCloudApiKey(apiKey);
config.setOllamaModel("");  // 清空本地模型
config.save();

m_ollamaClient->setCloudMode(true);
m_ollamaClient->setApiKey(apiKey);
```

## 工作流程

### 启动流程
1. 程序启动 → `loadConfiguration()`
2. 检查配置文件中的`cloudApiKey`和`ollamaModel`
3. 根据配置自动切换到对应模式
4. 显示AI配置对话框（如果需要）

### 用户选择本地模式
1. 用户在对话框选择"本地Ollama"标签页
2. 选择一个可用模型（如`qwen2.5:7b`）
3. 点击"确定"
4. 系统执行：
   - 保存`ollamaModel = "qwen2.5:7b"`
   - 清空`cloudApiKey = ""`
   - 调用`setCloudMode(false)`
   - 设置本地URL和模型
5. 后续AI请求使用本地Ollama服务

### 用户选择云端模式
1. 用户在对话框选择"云端API"标签页
2. 输入API Key
3. 点击"确定"
4. 系统执行：
   - 保存`cloudApiKey = "sk-xxx"`
   - 清空`ollamaModel = ""`
   - 调用`setCloudMode(true)`
   - 设置API Key
5. 后续AI请求使用云端API（DeepSeek）

## 关键改进

### ✅ 问题1: 云端模式仍使用本地模型
**解决**: 添加`setCloudMode()`方法，在用户选择后立即切换模式

### ✅ 问题2: 配置不生效
**解决**: 在`checkAndSelectModel()`中，保存配置后立即调用相应的设置方法

### ✅ 问题3: 没有模式标识
**解决**: 通过`cloudApiKey`是否为空来判断模式

### ✅ 问题4: 请求格式不兼容
**解决**: 在`sendRequest()`中根据`m_cloudMode`使用不同的请求格式

### ✅ 问题5: 流式响应解析错误
**解决**: 根据模式解析不同的响应字段

## 测试要点

1. **本地模式测试**
   - 选择本地模型 → 确认 → 使用AI分析功能
   - 检查日志：应显示"本地Ollama模式"
   - 检查请求URL：应为`localhost:11434`

2. **云端模式测试**
   - 输入API Key → 确认 → 使用AI分析功能
   - 检查日志：应显示"云端API模式"
   - 检查请求URL：应为`api.deepseek.com`
   - 检查请求头：应包含`Authorization: Bearer xxx`

3. **模式切换测试**
   - 从本地切换到云端 → 重启程序 → 验证使用云端
   - 从云端切换到本地 → 重启程序 → 验证使用本地

4. **配置持久化测试**
   - 配置后关闭程序
   - 重新打开程序
   - 验证配置正确加载

## 编译状态
✅ 编译成功，无错误，无警告

## 文件修改清单
- ✅ `src/utils/ConfigManager.h` - 添加模式判断方法
- ✅ `src/ai/OllamaClient.h` - 添加云端API支持
- ✅ `src/ai/OllamaClient.cpp` - 实现双模式切换
- ✅ `src/ui/MainWindow.cpp` - 修改配置加载和用户选择逻辑

## 注意事项

1. **云端API默认使用DeepSeek**
   - URL: `https://api.deepseek.com`
   - 模型: `deepseek-chat`
   - 可以在代码中修改为其他OpenAI兼容的服务

2. **API Key安全**
   - API Key存储在配置文件中
   - 建议后续添加加密存储

3. **错误处理**
   - 云端API请求失败时会显示错误信息
   - 建议添加更详细的错误提示

4. **扩展性**
   - 当前支持DeepSeek
   - 可轻松扩展支持OpenAI、Claude等其他服务
   - 只需修改`setCloudMode()`中的URL和模型名称
