# Xodor - 源代码运行说明

## 📋 项目信息

- **项目名称**: Xodor - 智能刷题系统
- **版本**: v1.0.0
- **开发语言**: C++17
- **UI框架**: Qt 6.9.2

---

## 🖥️ 运行环境

### 必需环境

- **操作系统**: Windows 10/11 (64-bit)
- **Qt版本**: Qt 6.9.2 MinGW 64-bit
- **编译器**: MinGW 11.2.0 (64-bit)
- **CMake**: 3.16 或更高版本
- **QScintilla**: 2.14.1（已包含在项目中）

### 可选环境（AI功能）

- **本地AI**: Ollama + qwen2.5-coder 模型
- **云端AI**: DeepSeek API / OpenAI API（已提供测试Key）

---

## 📦 依赖库及安装

### 1. 安装 Qt 6.9.2

**下载地址**: https://www.qt.io/download-qt-installer

**安装步骤**:
1. 运行 Qt 在线安装器
2. 选择 Qt 6.9.2 版本
3. 勾选以下组件：
   - MinGW 11.2.0 64-bit
   - Qt 6.9.2 MinGW 64-bit
   - CMake
   - Ninja

**安装路径示例**: `F:/Qt/6.9.2/mingw_64`

### 2. 安装 CMake

**下载地址**: https://cmake.org/download/

或使用 Qt 自带的 CMake（推荐）：
- 路径: `F:/Qt/Tools/CMake_64/bin/cmake.exe`

### 3. QScintilla（已包含）

项目已包含编译好的 QScintilla 2.14.1 库，无需额外安装。

---

## 🚀 详细运行步骤

### 方式一：使用 Qt Creator（推荐）

#### 步骤 1: 打开项目

1. 启动 Qt Creator
2. 点击 **文件 → 打开文件或项目**
3. 选择项目根目录下的 `CMakeLists.txt`

#### 步骤 2: 配置 Kit

1. Qt Creator 会自动检测 Kit
2. 选择 **Desktop Qt 6.9.2 MinGW 64-bit**
3. 点击 **Configure Project**

#### 步骤 3: 构建项目

1. 点击左下角的 **构建** 按钮（锤子图标）
2. 或使用快捷键 `Ctrl+B`
3. 等待编译完成（首次编译约需 3-5 分钟）

#### 步骤 4: 运行程序

1. 点击左下角的 **运行** 按钮（绿色三角形）
2. 或使用快捷键 `Ctrl+R`
3. 程序启动后会自动加载 CCF 题库

---

### 方式二：使用命令行编译

#### 步骤 1: 设置环境变量

打开 PowerShell 或 CMD，设置 Qt 路径：

```powershell
# 设置 Qt 路径（替换为你的实际路径）
$env:PATH = "F:\Qt\6.9.2\mingw_64\bin;F:\Qt\Tools\mingw1120_64\bin;$env:PATH"
```

#### 步骤 2: 配置 CMake

```powershell
# 进入项目目录
cd F:\Xodor

# 配置 CMake（替换为你的 Qt 路径）
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=F:/Qt/6.9.2/mingw_64 -DCMAKE_BUILD_TYPE=Release
```

#### 步骤 3: 编译项目

```powershell
# 编译
cmake --build build --config Release --target CodePracticeSystem
```

#### 步骤 4: 部署依赖

```powershell
# 使用 windeployqt 部署 Qt 依赖
F:\Qt\6.9.2\mingw_64\bin\windeployqt.exe build\CodePracticeSystem.exe

# 复制 QScintilla DLL
copy QScintilla_src-2.14.1\build-qt692\release\qscintilla2_qt6.dll build\
```

#### 步骤 5: 运行程序

```powershell
# 运行
.\build\CodePracticeSystem.exe
```

---

## ⚙️ 首次运行配置

### 1. 配置 AI 服务

程序首次启动后，需要配置 AI 服务才能使用 AI 判题和 AI 导师功能。

#### 推荐配置：使用云端模式（已提供测试 API Key）

1. 点击菜单栏 **设置 → AI 配置**
2. 选择 **云端模式**
3. 填入以下信息：
   - **API URL**: `https://api.deepseek.com`
   - **API Key**: `sk-1a8bae2865f1443c99b924ffd14c4252`
   - **模型**: `deepseek-chat`
4. 点击 **测试连接** 验证配置
5. 点击 **保存** 完成配置

**注意**: 此 API Key 仅供评审测试使用。

#### 可选配置：使用本地模式

如果您已安装 Ollama：

1. 下载并安装 Ollama: https://ollama.ai/
2. 下载模型: `ollama pull qwen2.5-coder`
3. 在程序中选择 **本地模式**
4. 填入：
   - **Ollama URL**: `http://localhost:11434`
   - **模型**: `qwen2.5-coder:latest`

### 2. 验证题库加载

程序已预置 CCF 题库（45道题），首次启动会自动加载。

- 在左侧题库列表中应该能看到 **CCF** 题库
- 展开后可以看到第 31-39 次认证的题目
- 如果没有显示，点击 **文件 → 刷新题库**

---

## 🎯 快速测试指南

### 测试 AI 判题功能

1. 在左侧题库列表选择任意题目（例如：CCF → 第31次 → 问题1）
2. 在代码编辑器中编写一段简单代码：
   ```cpp
   #include <iostream>
   using namespace std;
   
   int main() {
       int n;
       cin >> n;
       cout << n * 2 << endl;
       return 0;
   }
   ```
3. 点击 **AI 判题** 按钮
4. 观察实时流式输出的判题结果

### 测试 AI 导师功能

1. 点击右侧 **AI 导师** 面板
2. 输入问题，例如："这道题应该用什么算法？"
3. 观察 AI 导师的回答
4. 可以继续追问或讨论代码

### 测试统计功能

1. 切换到 **题库面板**
2. 查看刷题统计卡片
3. 观察热力图和难度分布

---

## 📂 项目结构说明

```
Xodor/
├── src/                          # 源代码目录
│   ├── ui/                       # UI 组件
│   │   ├── MainWindow.cpp/h      # 主窗口
│   │   ├── CodeEditor.cpp/h      # 代码编辑器
│   │   ├── AIAssistantPanel.cpp/h # AI 导师面板
│   │   └── ...
│   ├── core/                     # 核心逻辑
│   │   ├── QuestionBank.cpp/h    # 题库管理
│   │   ├── ProgressManager.cpp/h # 进度管理
│   │   └── ...
│   ├── ai/                       # AI 功能
│   │   ├── OllamaClient.cpp/h    # Ollama 客户端
│   │   ├── AIJudge.cpp/h         # AI 判题
│   │   └── ...
│   └── utils/                    # 工具类
│
├── resources/                    # 资源文件
│   ├── icon.svg                  # 应用图标
│   └── app_icon.rc               # Windows 资源
│
├── data/                         # 数据目录
│   ├── 基础题库/                 # 题库文件
│   │   └── CCF/                  # CCF 题库（45道题）
│   ├── config.json               # 配置文件
│   └── last_session.json         # 会话恢复
│
├── QScintilla_src-2.14.1/        # QScintilla 源码和库
│   └── build-qt692/              # 编译好的库
│
├── CMakeLists.txt                # CMake 配置
├── CMakePresets.json             # CMake 预设
└── README.md                     # 项目说明
```

---

## 🔧 常见问题排查

### 问题 1: 编译失败 - 找不到 Qt

**错误信息**: `Could not find a package configuration file provided by "Qt6"`

**解决方案**:
```powershell
# 确保 CMAKE_PREFIX_PATH 指向正确的 Qt 路径
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=F:/Qt/6.9.2/mingw_64
```

### 问题 2: 运行时缺少 DLL

**错误信息**: `无法启动此程序，因为计算机中丢失 Qt6Core.dll`

**解决方案**:
```powershell
# 使用 windeployqt 部署依赖
F:\Qt\6.9.2\mingw_64\bin\windeployqt.exe build\CodePracticeSystem.exe
```

### 问题 3: AI 功能无法使用

**错误信息**: `TLS initialization failed`

**解决方案**:
1. 确保已配置 AI 服务（设置 → AI 配置）
2. 测试网络连接
3. 检查 API Key 是否正确

### 问题 4: 题库不显示

**解决方案**:
1. 点击 **文件 → 刷新题库**
2. 检查 `data/基础题库/CCF/` 目录是否存在
3. 查看 `data/config.json` 配置

### 问题 5: 中文乱码

**解决方案**:
- 确保所有源文件使用 UTF-8 编码
- 在 Qt Creator 中：工具 → 选项 → 文本编辑器 → 行为 → 文件编码 → UTF-8

---

## 📞 技术支持

如遇到其他问题，请查看：

- **项目文档**: `README.md`
- **故障排查**: `TROUBLESHOOTING.md`
- **使用指南**: `USAGE.md`
- **评委测试指南**: `评委测试指南.md`

---

## 📊 性能说明

- **编译时间**: 首次编译约 3-5 分钟
- **程序启动**: 约 1-2 秒
- **内存占用**: 约 80-150 MB
- **AI 判题响应**: 2-5 秒（取决于网络和模型）

---

## ✅ 验收检查清单

运行程序后，请验证以下功能：

- [ ] 程序正常启动，无崩溃
- [ ] 左侧题库列表显示 CCF 题库（45道题）
- [ ] 可以选择题目并查看题目内容
- [ ] 代码编辑器支持语法高亮
- [ ] AI 判题功能正常（需先配置 AI 服务）
- [ ] AI 导师可以正常对话
- [ ] 统计面板显示热力图和数据
- [ ] 可以导入新题目
- [ ] 程序关闭后再次打开能恢复上次状态

---

<div align="center">

**感谢您的测试和评审！**

如有任何问题，欢迎通过 GitHub Issues 反馈

Made with ❤️ by Xodor Team

</div>
