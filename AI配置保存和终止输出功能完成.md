# AI配置保存和终止输出功能完成

## 修改内容

### 1. AI配置保存修复 ✅

**问题：** 更换AI配置后云端key丢失

**原因分析：**
在 `SettingsDialog::saveSettings()` 中，根据当前选中的标签页决定保存哪种配置：
- 选择本地Ollama标签页时，只保存本地配置
- 选择云端API标签页时，只保存云端配置
- 这导致切换标签页时，另一种配置会丢失

**修改前逻辑：**
```cpp
if (currentTab == 0) {
    // 本地Ollama标签页
    config.setOllamaModel(ollamaModel);
    config.setOllamaUrl(ollamaUrl);
    config.setUseCloudMode(false);
    // ❌ 没有保存云端配置
} else {
    // 云端API标签页
    config.setCloudApiKey(cloudApiKey);
    config.setUseCloudMode(true);
    // ❌ 没有保存本地配置
}
```

**修改后逻辑：**
```cpp
// ✅ 始终保存所有配置
if (!ollamaModel.isEmpty()) {
    config.setOllamaModel(ollamaModel);
    config.setOllamaUrl(ollamaUrl.isEmpty() ? "http://localhost:11434" : ollamaUrl);
}

if (!cloudApiKey.isEmpty()) {
    config.setCloudApiKey(cloudApiKey);
}

// 根据当前标签页决定使用哪种模式
if (currentTab == 0) {
    config.setUseCloudMode(false);
    // 提示：云端API配置已保留
} else {
    config.setUseCloudMode(true);
    // 提示：本地Ollama配置已保留
}
```

**改进点：**
1. **保留所有配置**：无论选择哪个标签页，都保存所有有效的配置
2. **模式切换**：只改变 `useCloudMode` 标志，不删除配置
3. **友好提示**：明确告知用户另一种配置已保留

**用户体验：**
- ✅ 可以在本地和云端模式之间自由切换
- ✅ 配置不会丢失
- ✅ 切换模式时有明确的提示信息

### 2. AI导师终止输出功能 ✅

**问题：** AI导师对话需要终止输出按钮

#### 2.1 OllamaClient 添加终止功能

**修改文件：** `src/ai/OllamaClient.h` 和 `src/ai/OllamaClient.cpp`

**添加成员变量：**
```cpp
private:
    QNetworkReply *m_currentReply;  // 当前正在进行的请求
```

**添加公共方法：**
```cpp
public:
    void abortCurrentRequest();  // 终止当前请求
```

**实现终止功能：**
```cpp
void OllamaClient::abortCurrentRequest()
{
    if (m_currentReply) {
        qDebug() << "[OllamaClient] 终止当前请求";
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        emit streamingFinished();  // 发送完成信号
    }
}
```

**在 sendChatMessage 中记录请求：**
```cpp
void OllamaClient::sendChatMessage(...)
{
    // 如果有正在进行的请求，先终止它
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    // ... 创建新请求
    QNetworkReply *reply = m_networkManager->post(...);
    m_currentReply = reply;  // 记录当前请求
    
    // ... 连接信号
    
    // 完成后清理
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (m_currentReply == reply) {
            m_currentReply = nullptr;
        }
        reply->deleteLater();
    });
}
```

#### 2.2 AIAssistantPanel 添加终止按钮

**修改文件：** `src/ui/AIAssistantPanel.h` 和 `src/ui/AIAssistantPanel.cpp`

**添加UI组件：**
```cpp
private:
    QPushButton *m_stopButton;  // 终止输出按钮
```

**添加槽函数：**
```cpp
private slots:
    void onStopGeneration();  // 终止输出
```

**创建终止按钮：**
```cpp
// 终止按钮（初始隐藏）
m_stopButton = new QPushButton("⏹ 终止", this);
m_stopButton->setFixedWidth(60);
m_stopButton->setMinimumHeight(35);
m_stopButton->setVisible(false);  // 初始隐藏
m_stopButton->setStyleSheet(R"(
    QPushButton {
        background-color: #cc0000;
        color: white;
        border: none;
        border-radius: 8px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #ff0000;
    }
    QPushButton:pressed {
        background-color: #990000;
    }
)");
```

**按钮切换逻辑：**

开始接收消息时：
```cpp
void AIAssistantPanel::startAssistantMessage()
{
    m_isReceivingMessage = true;
    
    // 显示终止按钮，隐藏发送按钮
    m_sendButton->setVisible(false);
    m_stopButton->setVisible(true);
    
    // ... 创建气泡
}
```

完成接收消息时：
```cpp
void AIAssistantPanel::finishAssistantMessage()
{
    m_isReceivingMessage = false;
    
    // 恢复发送按钮，隐藏终止按钮
    m_stopButton->setVisible(false);
    m_sendButton->setVisible(true);
    
    // ... 保存消息
}
```

**终止功能实现：**
```cpp
void AIAssistantPanel::onStopGeneration()
{
    if (!m_aiClient) {
        return;
    }
    
    qDebug() << "[AIAssistantPanel] 用户请求终止输出";
    
    // 终止AI客户端的当前请求
    m_aiClient->abortCurrentRequest();
    
    // 如果正在接收消息，添加终止标记
    if (m_isReceivingMessage && m_currentAssistantBubble) {
        m_currentAssistantMessage += "\n\n⏹ **输出已终止**";
        m_currentAssistantBubble->setContent(m_currentAssistantMessage);
        finishAssistantMessage();
    }
    
    // 恢复按钮状态
    m_stopButton->setVisible(false);
    m_sendButton->setVisible(true);
}
```

**工作流程：**
```
用户发送消息
  ↓
显示终止按钮，隐藏发送按钮
  ↓
AI开始流式输出
  ↓
用户点击终止按钮（可选）
  ↓
调用 abortCurrentRequest()
  ↓
添加"输出已终止"标记
  ↓
恢复发送按钮，隐藏终止按钮
```

### 3. 聊天气泡高度优化 ✅

**问题：** 用户只发了一行字，气泡却有三行高

**原因分析：**
在 `adjustHeight()` 方法中：
- 只设置了 `setMinimumHeight()`，没有设置 `setMaximumHeight()`
- 导致Widget可能会被拉伸到比实际内容更高

**修改前：**
```cpp
void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    int docHeight = doc->size().toSize().height();
    int margin = doc->documentMargin();
    int height = docHeight + margin * 2 + 4;
    
    m_textBrowser->setFixedHeight(height);
    setMinimumHeight(height + 20);  // ❌ 只设置最小高度
}
```

**修改后：**
```cpp
void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    int docHeight = doc->size().toSize().height();
    int margin = doc->documentMargin();
    
    // QTextBrowser 的高度 = 文档高度 + 文档边距
    int textBrowserHeight = docHeight + margin * 2 + 4;
    m_textBrowser->setFixedHeight(textBrowserHeight);
    
    // Widget 的高度 = QTextBrowser 高度 + 布局边距（上下各10px）
    int widgetHeight = textBrowserHeight + 20;
    setMinimumHeight(widgetHeight);
    setMaximumHeight(widgetHeight);  // ✅ 设置最大高度，避免多余空间
}
```

**改进点：**
1. **精确高度**：同时设置最小和最大高度为相同值
2. **避免拉伸**：Widget不会被布局拉伸到比内容更高
3. **注释清晰**：明确说明每个高度的计算方式

**效果：**
- ✅ 一行文本的气泡只有一行高
- ✅ 多行文本的气泡根据实际行数调整
- ✅ 没有多余的空白空间

## 相关文件

### 修改的文件

1. **`src/ui/SettingsDialog.cpp`**
   - 修改 `saveSettings()` 方法
   - 始终保存所有配置
   - 优化提示信息

2. **`src/ai/OllamaClient.h`**
   - 添加 `m_currentReply` 成员变量
   - 添加 `abortCurrentRequest()` 方法

3. **`src/ai/OllamaClient.cpp`**
   - 实现 `abortCurrentRequest()` 方法
   - 在 `sendChatMessage()` 中记录当前请求
   - 请求完成后清理 `m_currentReply`

4. **`src/ui/AIAssistantPanel.h`**
   - 添加 `m_stopButton` 成员变量
   - 添加 `onStopGeneration()` 槽函数

5. **`src/ui/AIAssistantPanel.cpp`**
   - 创建终止按钮UI
   - 实现按钮切换逻辑
   - 实现 `onStopGeneration()` 方法

6. **`src/ui/ChatBubbleWidget.cpp`**
   - 修改 `adjustHeight()` 方法
   - 同时设置最小和最大高度

## 测试建议

### 测试1：配置保存
1. 打开设置（Ctrl+,）
2. 配置本地Ollama模型
3. 切换到云端API标签页
4. 输入API Key
5. 保存设置
6. 重新打开设置
7. 验证：本地Ollama配置仍然存在
8. 切换到本地Ollama标签页
9. 保存设置
10. 重新打开设置
11. 验证：云端API Key仍然存在

### 测试2：终止输出
1. 在AI导师面板发送一个问题
2. 观察：发送按钮变为终止按钮
3. AI开始输出时点击终止按钮
4. 验证：
   - 输出立即停止
   - 显示"输出已终止"标记
   - 终止按钮变回发送按钮
5. 可以继续发送新消息

### 测试3：气泡高度
1. 发送一行短消息："你好"
2. 验证：气泡高度只有一行
3. 发送多行消息
4. 验证：气泡高度根据实际行数调整
5. 调整窗口宽度
6. 验证：气泡高度随文本换行自动调整

## 技术细节

### 配置保存策略

**设计原则：**
- 保留所有有效配置
- 只切换使用模式
- 不删除未使用的配置

**配置结构：**
```json
{
  "ollamaUrl": "http://localhost:11434",
  "ollamaModel": "qwen2.5:7b",
  "cloudApiKey": "sk-xxx",
  "cloudApiUrl": "https://api.deepseek.com",
  "cloudApiModel": "deepseek-chat",
  "useCloudMode": false
}
```

**切换流程：**
```
本地模式 → 云端模式
  ↓
只改变 useCloudMode: false → true
  ↓
本地配置保留
  ↓
下次可以切换回本地模式
```

### 终止请求机制

**QNetworkReply::abort()：**
- 立即终止网络请求
- 触发 `finished` 信号
- 不会触发 `readyRead` 信号

**清理流程：**
```
用户点击终止
  ↓
调用 reply->abort()
  ↓
触发 finished 信号
  ↓
清理 m_currentReply
  ↓
发送 streamingFinished 信号
  ↓
UI恢复正常状态
```

**防止内存泄漏：**
```cpp
// 方法1：在 finished 信号中清理
connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (m_currentReply == reply) {
        m_currentReply = nullptr;
    }
    reply->deleteLater();
});

// 方法2：在 abort 时清理
void abortCurrentRequest() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}
```

### 气泡高度计算

**高度组成：**
```
Widget总高度 = QTextBrowser高度 + 布局边距(20px)
             = (文档高度 + 文档边距×2 + 4px) + 20px
```

**示例计算（单行文本）：**
```
假设：
- 字体大小：11pt
- 行高：1.5倍 = 16.5px
- 文档边距：8px

计算：
- 文档高度 = 16.5px
- QTextBrowser高度 = 16.5 + 8×2 + 4 = 36.5px
- Widget高度 = 36.5 + 20 = 56.5px ≈ 57px
```

**关键设置：**
```cpp
setMinimumHeight(widgetHeight);  // 最小高度
setMaximumHeight(widgetHeight);  // 最大高度（关键！）
```

## 优势总结

### 1. 配置管理
- ✅ 配置不会丢失
- ✅ 可以自由切换模式
- ✅ 用户体验友好

### 2. 终止功能
- ✅ 即时响应
- ✅ 清晰的视觉反馈
- ✅ 防止资源浪费

### 3. 气泡高度
- ✅ 精确匹配内容
- ✅ 没有多余空间
- ✅ 视觉效果更好

## 后续优化建议

### 1. 配置管理
- 添加配置导入/导出功能
- 支持多个配置预设
- 配置验证和测试

### 2. 终止功能
- 添加终止确认对话框（可选）
- 显示已生成的token数量
- 支持暂停/恢复（如果API支持）

### 3. 气泡显示
- 添加气泡动画效果
- 支持气泡折叠/展开
- 优化长代码块的显示

## 总结

✅ **配置保存修复**：始终保存所有配置，只切换使用模式，避免配置丢失

✅ **终止输出功能**：添加终止按钮，支持即时终止AI输出，提升用户体验

✅ **气泡高度优化**：精确计算气泡高度，避免多余空间，视觉效果更好

所有修改已编译成功，可以直接使用！
