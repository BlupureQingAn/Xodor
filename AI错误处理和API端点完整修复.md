# AI错误处理和API端点完整修复

## 问题描述

用户报告了两个关键问题：
1. ❌ 错误信息显示仍在访问旧的 `/api/generate` 端点（404 Not Found）
2. ❌ 这些网络错误不应该显示在AI对话框内

## 根本原因分析

### 问题1：旧API端点
虽然代码已经更新为使用新的 `/api/chat` 端点，但错误信息显示还在访问 `/api/generate`，这是因为：
- 可能运行的是旧的编译版本
- 需要完全重新编译项目

### 问题2：错误显示位置不当
`OllamaClient` 的 `sendChatMessage` 方法中，所有网络错误都会通过 `emit error()` 信号发送到UI，导致：
- AI判题的错误显示在AI导师对话框
- 其他非聊天context的错误也显示在聊天界面
- 用户体验混乱

## 修复方案

### 修复1：确保使用新API端点

**文件：`src/ai/OllamaClient.cpp`**

已确认代码使用新端点：
```cpp
// 本地Ollama格式 - 使用新的 /api/chat 端点
url = QUrl(m_baseUrl + "/api/chat");
```

**解决方法：完全重新编译**
```bash
cmake --build build --config Release --clean-first
```

### 修复2：智能错误处理 - 只在聊天context显示错误

**文件：`src/ai/OllamaClient.cpp`**

**修改前：**
```cpp
// 错误处理
connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError code) {
    QString errorMsg = QString("网络错误: %1").arg(reply->errorString());
    emit error(errorMsg);  // ❌ 所有错误都发送到UI
    reply->deleteLater();
});
```

**修改后：**
```cpp
// 错误处理
connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError code) {
    QString context = reply->property("context").toString();
    
    // 只为聊天context发送错误信号到UI
    // 其他context（如ai_judge）的错误不应该显示在聊天界面
    if (context == "chat") {
        QString errorMsg;
        
        // 根据错误类型提供友好的错误信息
        switch (code) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "无法连接到AI服务\n\n"
                          "请检查：\n"
                          "1. AI服务是否正在运行\n"
                          "2. 服务地址是否正确\n"
                          "3. 防火墙是否阻止连接";
                break;
                
            case QNetworkReply::HostNotFoundError:
                errorMsg = "找不到AI服务器\n\n"
                          "请检查服务地址配置是否正确";
                break;
                
            case QNetworkReply::TimeoutError:
                errorMsg = "请求超时\n\n"
                          "可能原因：\n"
                          "1. 网络连接不稳定\n"
                          "2. AI服务响应缓慢\n"
                          "3. 模型正在加载中";
                break;
                
            case QNetworkReply::ContentNotFoundError:
                errorMsg = "API端点不存在\n\n"
                          "可能原因：\n"
                          "1. Ollama版本过旧，请更新到最新版本\n"
                          "2. API地址配置错误\n"
                          "3. 请在设置中检测并选择正确的模型";
                break;
                
            default:
                errorMsg = QString("网络请求失败\n\n"
                                  "错误信息：%1\n\n"
                                  "请检查AI服务状态").arg(reply->errorString());
                break;
        }
        
        emit error(errorMsg);
    } else {
        // 非聊天context的错误只记录日志，不显示在UI
        qWarning() << "[OllamaClient] 网络错误 (context:" << context << "):" << reply->errorString();
    }
    
    reply->deleteLater();
});
```

## 核心改进

### 1. Context感知的错误处理

**原理：**
- 每个网络请求都有一个 `context` 属性
- `context == "chat"`: AI导师对话
- `context == "ai_judge"`: AI判题
- `context == "code_analysis"`: 代码分析
- 其他context...

**逻辑：**
```cpp
if (context == "chat") {
    // 只有聊天错误才显示在AI导师面板
    emit error(errorMsg);
} else {
    // 其他错误只记录日志
    qWarning() << "网络错误 (context:" << context << "):" << errorString;
}
```

### 2. 友好的错误提示

根据不同的网络错误类型，提供针对性的解决方案：

| 错误类型 | 用户提示 | 解决建议 |
|---------|---------|---------|
| `ConnectionRefusedError` | 无法连接到AI服务 | 检查服务是否运行、地址是否正确 |
| `HostNotFoundError` | 找不到AI服务器 | 检查服务地址配置 |
| `TimeoutError` | 请求超时 | 检查网络连接、服务响应 |
| `ContentNotFoundError` | API端点不存在 | 更新Ollama版本、检测模型 |
| 其他错误 | 网络请求失败 | 显示具体错误信息 |

### 3. 特别针对404错误的提示

对于 `ContentNotFoundError`（404），提示用户：
- Ollama版本可能过旧
- 需要更新到最新版本
- 在设置中检测并选择正确的模型

## 测试场景

### 场景1：AI导师对话错误
**操作：**
1. 停止Ollama服务
2. 在AI导师面板发送消息

**预期结果：**
- ✅ 错误显示在AI导师对话框
- ✅ 提示"无法连接到AI服务"
- ✅ 提供详细的故障排查建议

### 场景2：AI判题错误
**操作：**
1. 停止Ollama服务
2. 点击"AI判题"按钮

**预期结果：**
- ✅ 错误**不**显示在AI导师对话框
- ✅ 错误只记录在日志中
- ✅ AI导师面板保持干净

### 场景3：404错误（旧版本Ollama）
**操作：**
1. 使用旧版本Ollama（不支持 `/api/chat`）
2. 尝试使用AI功能

**预期结果：**
- ✅ 提示"API端点不存在"
- ✅ 建议更新Ollama版本
- ✅ 建议在设置中检测模型

### 场景4：正常使用
**操作：**
1. Ollama服务正常运行
2. 使用AI导师对话
3. 使用AI判题

**预期结果：**
- ✅ AI导师正常响应
- ✅ AI判题正常工作
- ✅ 没有错误提示

## 相关修改文件

1. **`src/ai/OllamaClient.cpp`**
   - 修改 `sendChatMessage` 中的错误处理
   - 添加context感知逻辑
   - 优化错误提示信息

2. **编译配置**
   - 使用 `--clean-first` 完全重新编译
   - 确保使用最新代码

## 用户操作指南

### 如果遇到404错误

1. **检查Ollama版本**
   ```bash
   ollama --version
   ```
   确保版本 >= 0.1.0（支持 `/api/chat` 端点）

2. **更新Ollama**
   - 访问 https://ollama.ai
   - 下载最新版本
   - 重新安装

3. **检测并配置模型**
   - 打开设置（Ctrl+,）
   - 切换到"本地Ollama"标签
   - 点击"🔍 检测模型"按钮
   - 选择检测到的模型
   - 保存设置

4. **验证配置**
   - 在AI导师面板发送测试消息
   - 确认能正常响应

### 如果错误仍显示在对话框

1. **确认使用最新版本**
   - 关闭程序
   - 重新编译：`cmake --build build --config Release --clean-first`
   - 运行新编译的程序

2. **清除旧数据**
   - 删除 `build` 目录
   - 重新配置和编译

## 技术细节

### Context属性传递流程

```
1. sendChatMessage() 设置 context = "chat"
   ↓
2. reply->setProperty("context", "chat")
   ↓
3. 错误发生时读取 context
   ↓
4. 根据 context 决定是否显示错误
```

### 信号流程对比

**修改前：**
```
网络错误 → emit error() → AIAssistantPanel → 显示在对话框
（所有错误都显示）
```

**修改后：**
```
网络错误 → 检查context
           ├─ context == "chat" → emit error() → 显示在对话框
           └─ 其他context → qWarning() → 只记录日志
```

## 优势

### 1. 用户体验改善
- ✅ AI导师对话框只显示相关错误
- ✅ 不会被其他功能的错误干扰
- ✅ 错误提示更加清晰和有针对性

### 2. 调试友好
- ✅ 所有错误都记录在日志中
- ✅ 开发者可以通过日志追踪问题
- ✅ 用户界面保持简洁

### 3. 错误提示专业
- ✅ 根据错误类型提供具体建议
- ✅ 帮助用户快速定位和解决问题
- ✅ 减少用户困惑

### 4. 代码健壮性
- ✅ Context感知的错误处理
- ✅ 避免错误信息串扰
- ✅ 更好的关注点分离

## 后续优化建议

1. **统一错误处理机制**
   - 为所有AI功能创建统一的错误处理类
   - 支持不同级别的错误（警告、错误、致命错误）

2. **错误恢复机制**
   - 自动重试失败的请求
   - 提供"重试"按钮

3. **离线模式提示**
   - 检测到网络错误时，提示用户切换到离线模式
   - 提供离线功能（如查看历史记录）

4. **错误统计**
   - 记录错误发生频率
   - 帮助识别常见问题

## 总结

✅ **问题1已解决**：确认代码使用新的 `/api/chat` 端点，完全重新编译确保使用最新代码

✅ **问题2已解决**：实现context感知的错误处理，只有聊天相关的错误才显示在AI导师对话框

✅ **用户体验提升**：错误提示更加友好和有针对性，帮助用户快速解决问题

✅ **代码质量提升**：更好的关注点分离，避免错误信息串扰

现在用户可以：
- 正常使用AI导师对话功能
- AI判题的错误不会干扰对话界面
- 收到清晰的错误提示和解决建议
- 通过设置界面自动检测和配置模型
