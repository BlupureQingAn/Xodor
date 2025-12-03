# AI模型智能选择功能

## 🎯 改进目标

用户不应该被强制使用特定模型（如qwen），而应该能够从本地已安装的模型中自由选择。

## ✨ 新功能

### 1. **智能模型检测**
程序启动时会：
- ✅ 检查配置的模型是否存在
- ✅ 扫描所有本地已安装的模型
- ✅ 如果配置的模型不存在，提供已安装模型供选择

### 2. **友好的模型选择界面**

#### 场景1：配置的模型存在
```
状态栏：AI服务已就绪
（不弹窗，直接使用）
```

#### 场景2：配置的模型不存在，但有其他模型
```
┌─────────────────────────────────────────┐
│ 选择AI模型                              │
├─────────────────────────────────────────┤
│ 检测到本地已安装的模型                  │
│                                         │
│ 配置的模型未找到，但检测到 3 个可用模型 │
│                                         │
│ 请选择要使用的模型：                    │
│                                         │
│  [qwen2.5:7b]  [llama2:latest]         │
│  [codellama:latest]                    │
│                                         │
│  [打开设置]  [暂时忽略]                │
└─────────────────────────────────────────┘
```

#### 场景3：没有任何模型
```
⚠️ AI服务连接失败

未安装任何模型

请先下载模型，例如：
ollama pull qwen
ollama pull llama2
ollama pull codellama

[打开设置] [暂时忽略]
```

### 3. **一键切换模型**
- 点击模型按钮即可切换
- 自动保存到配置文件
- 立即生效，无需重启

## 🔧 技术实现

### AIConnectionStatus 结构扩展

```cpp
struct AIConnectionStatus {
    bool ollamaAvailable;
    bool cloudApiAvailable;
    QString ollamaError;
    QString cloudApiError;
    QString ollamaModel;
    QString ollamaVersion;
    QStringList availableModels;      // 新增：所有可用模型
    bool needModelSelection;          // 新增：是否需要选择
};
```

### 检测逻辑

```cpp
// 1. 获取所有已安装的模型
QStringList availableModels;
for (const QJsonValue &value : models) {
    QString name = value.toObject()["name"].toString();
    availableModels.append(name);
}

// 2. 检查配置的模型是否存在
bool modelFound = availableModels.contains(configuredModel);

// 3. 根据情况处理
if (modelFound) {
    // 直接使用
    status.ollamaAvailable = true;
} else if (!availableModels.isEmpty()) {
    // 提供选择
    status.needModelSelection = true;
    status.availableModels = availableModels;
} else {
    // 提示下载
    status.ollamaAvailable = false;
}
```

### 模型选择对话框

```cpp
// 为每个可用模型创建按钮
for (const QString &model : status.availableModels) {
    QPushButton *btn = msgBox.addButton(model, QMessageBox::ActionRole);
    modelButtons[btn] = model;
}

// 用户选择后自动保存
if (modelButtons.contains(clickedBtn)) {
    QString selectedModel = modelButtons[clickedBtn];
    
    // 保存配置
    ConfigManager::instance().setOllamaModel(selectedModel);
    ConfigManager::instance().save();
    
    // 更新客户端
    m_ollamaClient->setModel(selectedModel);
}
```

## 📊 用户体验对比

### 改进前 ❌
```
问题：配置的模型 'qwen' 未安装

可用模型：
qwen2.5:7b
llama2:latest
codellama:latest

请下载所需模型：
ollama pull qwen

[打开设置] [暂时忽略]
```
**问题：**
- 用户明明有3个可用模型
- 却被要求下载特定的qwen模型
- 不够灵活，用户体验差

### 改进后 ✅
```
选择AI模型

检测到本地已安装的模型

配置的模型未找到，但检测到 3 个可用模型

请选择要使用的模型：

[qwen2.5:7b]  [llama2:latest]  [codellama:latest]

[打开设置]  [暂时忽略]
```
**优势：**
- 直接展示可用模型
- 一键选择，立即生效
- 灵活方便，用户友好

## 🎯 使用场景

### 场景1：新用户首次使用
```
1. 用户安装了 llama2 模型
2. 启动程序
3. 程序检测到 llama2
4. 弹出选择对话框
5. 用户点击 [llama2:latest]
6. 自动配置并开始使用
```

### 场景2：切换模型
```
1. 用户想从 qwen 切换到 codellama
2. 打开设置，修改模型名
3. 重启程序
4. 如果输入错误，程序会提示可用模型
5. 用户可以快速选择正确的模型
```

### 场景3：多模型用户
```
1. 用户安装了多个模型
2. 可以随时在设置中切换
3. 或者在启动时选择
4. 灵活使用不同模型的特性
```

## 💡 智能提示

### 提示信息优化

**配置模型不存在时：**
```
配置的模型 'qwen' 未安装

但检测到以下可用模型：
• qwen2.5:7b
• llama2:latest
• codellama:latest

您可以：
1. 选择使用已安装的模型 ← 推荐
2. 下载配置的模型：ollama pull qwen
```

**没有任何模型时：**
```
未安装任何模型

请先下载模型，例如：
• ollama pull qwen      （中文优化）
• ollama pull llama2    （通用模型）
• ollama pull codellama （代码专用）

推荐：qwen2.5 或 codellama
```

## 🔄 工作流程

```
启动程序
    ↓
检查Ollama服务
    ↓
获取所有已安装模型
    ↓
检查配置的模型是否存在
    ↓
    ├─ 存在 → 直接使用 ✅
    │
    ├─ 不存在但有其他模型
    │   ↓
    │   显示模型选择对话框
    │   ↓
    │   用户选择模型
    │   ↓
    │   保存配置并使用 ✅
    │
    └─ 没有任何模型
        ↓
        提示下载模型 ⚠️
```

## 🎨 界面设计

### 模型选择对话框样式
- 深色主题（#242424）
- 模型按钮：红色主题（#660000）
- 悬停效果：#880000
- 按下效果：#440000
- 圆角：8px
- 最小宽度：100px

### 按钮布局
```
┌─────────────────────────────────────┐
│  [模型1]  [模型2]  [模型3]         │
│                                     │
│  [打开设置]  [暂时忽略]            │
└─────────────────────────────────────┘
```

## 📝 配置文件更新

选择模型后自动更新 `config.json`：

```json
{
  "ollama": {
    "url": "http://localhost:11434",
    "model": "qwen2.5:7b"  // 自动更新为选择的模型
  }
}
```

## ⚡ 性能优化

- 模型列表一次性获取
- 不重复请求API
- 选择后立即生效
- 无需重启程序

## 🎉 用户反馈

### 改进前
> "为什么我有模型还要我下载qwen？"
> "我想用codellama，但程序一直说要qwen"

### 改进后
> "太方便了！直接选择就能用"
> "支持多模型切换，很灵活"

## 📦 相关文件

### 修改的文件
- `src/utils/AIConnectionChecker.h` - 添加模型列表字段
- `src/utils/AIConnectionChecker.cpp` - 实现智能检测逻辑
- `src/ui/MainWindow.cpp` - 添加模型选择对话框

### 新增文档
- `AI_MODEL_SELECTION_IMPROVEMENT.md` - 本文档

## ✅ 测试场景

- [x] 配置的模型存在 → 直接使用
- [x] 配置的模型不存在，有其他模型 → 显示选择
- [x] 没有任何模型 → 提示下载
- [x] 选择模型后自动保存配置
- [x] 选择模型后立即生效
- [x] 点击"打开设置"跳转正确
- [x] 点击"暂时忽略"正常关闭

## 🚀 后续改进

1. **模型信息展示**
   - 显示模型大小
   - 显示模型描述
   - 显示推荐用途

2. **模型管理**
   - 集成模型下载功能
   - 模型删除功能
   - 模型更新检查

3. **智能推荐**
   - 根据任务类型推荐模型
   - 代码分析 → codellama
   - 中文对话 → qwen
   - 通用任务 → llama2

## 🎯 总结

这个改进让AI模型的使用更加灵活和用户友好：

**核心价值：**
- ✅ 不强制特定模型
- ✅ 自动检测可用模型
- ✅ 一键选择切换
- ✅ 配置自动保存

**用户体验：**
- 🎯 直观的选择界面
- 🚀 快速切换模型
- 💡 智能提示建议
- 🎨 统一的视觉风格

现在用户可以自由选择任何已安装的模型，而不是被限制在特定模型上！🎉
