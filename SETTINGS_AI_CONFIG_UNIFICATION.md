# 设置界面AI配置统一

## 目标
让设置界面的AI配置与启动时的弹窗保持一致的设计和逻辑

## 改进内容

### 1. UI设计统一
**文件**: `src/ui/SettingsDialog.cpp`

#### 之前的设计
- 简单的文本输入框
- 分为两个独立的GroupBox
- 没有明确的模式选择概念

#### 现在的设计（与启动弹窗一致）
- 双标签页设计
- 🖥️ 本地Ollama 标签页
- ☁️ 云端API 标签页
- 统一的样式和布局

### 2. 本地Ollama标签页
```
💻 使用本地Ollama服务
• 完全免费，数据隐私
• 需要先安装Ollama并下载模型

服务地址: [http://localhost:11434]
模型名称: [qwen2.5:7b]

💡 提示：
1. 访问 https://ollama.ai 下载安装
2. 运行命令：ollama pull qwen2.5:7b
3. 启动服务：ollama serve
```

### 3. 云端API标签页
```
☁️ 使用云端AI服务
• 支持OpenAI、DeepSeek等API
• 需要API Key（可能需要付费）

API Key: [输入你的API Key...]

💡 提示：
• OpenAI: 使用默认地址
• DeepSeek等兼容OpenAI API的服务也可使用
• 配置云端API后将自动切换到云端模式
```

### 4. 保存逻辑优化
**文件**: `src/ui/SettingsDialog.cpp` - `saveSettings()`

#### 智能模式切换
```cpp
if (!cloudApiKey.isEmpty()) {
    // 配置了云端API → 使用云端模式
    config.setCloudApiKey(cloudApiKey);
    config.setOllamaModel("");  // 清空本地模型
    
} else if (!ollamaModel.isEmpty()) {
    // 配置了本地模型 → 使用本地模式
    config.setOllamaModel(ollamaModel);
    config.setOllamaUrl(ollamaUrl);
    config.setCloudApiKey("");  // 清空云端配置
    
} else {
    // 都没配置 → 警告用户
    config.setOllamaModel("");
    config.setCloudApiKey("");
}
```

#### 用户反馈
- 配置云端API：显示"已切换到云端API模式"
- 配置本地模型：显示"已切换到本地Ollama模式"
- 都没配置：警告"AI功能将不可用"

## 样式统一

### 标签页样式
```css
QTabWidget::pane {
    border: 2px solid #3a3a3a;
    border-radius: 8px;
    background: #1e1e1e;
}
QTabBar::tab:selected {
    background: #660000;
    color: white;
}
```

### 输入框样式
```css
QLineEdit {
    background-color: #1a1a1a;
    color: #e8e8e8;
    border: 2px solid #3a3a3a;
    border-radius: 8px;
    padding: 8px;
}
QLineEdit:focus {
    border-color: #660000;
}
```

### 信息提示样式
```css
/* 普通提示 */
color: #b0b0b0;
background: #1a1a1a;
border-radius: 5px;

/* 警告提示 */
color: #ff8800;
background: #2a1a00;
border-radius: 5px;
```

## 对比

### 启动弹窗 vs 设置界面

| 特性 | 启动弹窗 | 设置界面 |
|------|---------|---------|
| 双标签页设计 | ✅ | ✅ |
| 本地Ollama配置 | ✅ | ✅ |
| 云端API配置 | ✅ | ✅ |
| 模型列表检测 | ✅ | ❌ (不需要) |
| 样式统一 | ✅ | ✅ |
| 智能模式切换 | ✅ | ✅ |
| 用户反馈 | ✅ | ✅ |

**注意**: 设置界面不需要实时检测模型列表，用户可以手动输入模型名称

## 用户体验改进

### 1. 一致性
- 启动时和设置中看到相同的界面
- 相同的配置逻辑
- 相同的视觉风格

### 2. 清晰性
- 明确的模式选择（本地 vs 云端）
- 详细的配置说明
- 清楚的提示信息

### 3. 智能性
- 自动根据配置切换模式
- 保存时验证配置
- 给出明确的反馈

## 工作流程

### 配置本地模式
```
打开设置 → AI标签页
    ↓
选择"本地Ollama"标签
    ↓
输入服务地址和模型名称
    ↓
点击保存
    ↓
系统切换到本地模式
    ↓
显示"已切换到本地Ollama模式"
```

### 配置云端模式
```
打开设置 → AI标签页
    ↓
选择"云端API"标签
    ↓
输入API Key
    ↓
点击保存
    ↓
系统切换到云端模式
    ↓
显示"已切换到云端API模式"
```

### 切换模式
```
从本地切换到云端：
1. 打开设置
2. 切换到"云端API"标签
3. 输入API Key
4. 保存（自动清空本地配置）

从云端切换到本地：
1. 打开设置
2. 切换到"本地Ollama"标签
3. 输入模型名称
4. 保存（自动清空云端配置）
```

## 文件修改清单
- ✅ `src/ui/SettingsDialog.cpp` - 重构AI配置UI和逻辑
- ✅ `src/ui/SettingsDialog.h` - 保持不变（成员变量已足够）

## 编译状态
✅ 编译成功，无错误，无警告

## 测试要点
1. 打开设置 → AI标签页，应该看到双标签页设计
2. 配置本地模型 → 保存 → 应该显示"已切换到本地模式"
3. 配置云端API → 保存 → 应该显示"已切换到云端模式"
4. 重启程序 → 配置应该正确加载
5. 样式应该与启动弹窗一致
