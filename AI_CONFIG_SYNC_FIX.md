# AI配置同步修复

## 问题描述
启动界面和设置界面的AI配置不同步：
- 启动界面的云端API Key没有保存之前设置的值
- 两个界面修改配置时会互相清空对方的配置
- 用户无法同时保存本地和云端两种配置

## 根本原因
之前的实现使用"互斥"逻辑：
- 选择本地模式时，清空云端API Key
- 选择云端模式时，清空本地模型
- 通过"哪个配置不为空"来判断使用哪种模式

这导致：
1. 用户无法同时保存两种配置
2. 切换模式时会丢失另一种配置
3. 两个界面的行为不一致

## 解决方案

### 1. 添加明确的模式标志
在 `ConfigManager` 中添加 `m_useCloudMode` 标志：
```cpp
bool m_useCloudMode = false;  // false=本地，true=云端
```

### 2. 修改判断逻辑
```cpp
// 旧逻辑（通过配置是否为空判断）
bool useCloudApi() const { return !m_cloudApiKey.isEmpty(); }
bool useLocalOllama() const { return !m_ollamaModel.isEmpty() && m_cloudApiKey.isEmpty(); }

// 新逻辑（通过明确的标志判断）
bool useCloudApi() const { return m_useCloudMode; }
bool useLocalOllama() const { return !m_useCloudMode; }
```

### 3. 修改保存逻辑
**启动界面 (MainWindow.cpp)**：
```cpp
// 本地模式
config.setOllamaModel(selectedModel);
config.setUseCloudMode(false);  // 不清空云端配置
config.save();

// 云端模式
config.setCloudApiKey(apiKey);
config.setUseCloudMode(true);  // 不清空本地配置
config.save();
```

**设置界面 (SettingsDialog.cpp)**：
```cpp
// 根据当前选中的标签页决定模式
int currentTab = m_aiTabWidget->currentIndex();

if (currentTab == 0) {
    // 本地模式
    config.setOllamaModel(ollamaModel);
    config.setUseCloudMode(false);
} else {
    // 云端模式
    config.setCloudApiKey(cloudApiKey);
    config.setUseCloudMode(true);
}
```

### 4. 标签页自动选择
两个界面都根据当前模式自动选择标签页：
```cpp
if (config.useCloudMode()) {
    tabWidget->setCurrentIndex(1);  // 云端标签页
} else {
    tabWidget->setCurrentIndex(0);  // 本地标签页
}
```

## 修改的文件
1. `src/utils/ConfigManager.h` - 添加 `m_useCloudMode` 成员和相关方法
2. `src/utils/ConfigManager.cpp` - 保存和加载 `useCloudMode` 配置
3. `src/ui/MainWindow.cpp` - 修改启动界面的保存逻辑和标签页选择
4. `src/ui/SettingsDialog.h` - 添加 `m_aiTabWidget` 成员变量
5. `src/ui/SettingsDialog.cpp` - 修改设置界面的保存逻辑和标签页选择

## 效果
✅ 用户可以同时保存本地和云端两种配置
✅ 切换模式时不会丢失另一种配置
✅ 启动界面和设置界面完全同步
✅ 两个界面都能正确显示和保存配置
✅ 标签页自动选择当前使用的模式

## 测试建议
1. 在启动界面配置云端API Key，保存
2. 重启程序，检查启动界面是否显示之前保存的API Key
3. 在设置界面切换到本地模式，配置本地模型
4. 再次打开设置界面，切换到云端标签页，检查API Key是否还在
5. 重启程序，检查启动界面是否选中本地标签页（因为最后使用的是本地模式）
