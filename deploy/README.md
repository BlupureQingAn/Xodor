# 代码刷题系统

基于 Qt6 的智能代码刷题系统，集成本地 AI 模型（Ollama）和云端 API，支持题库导入、代码编辑、自动编译测试和 AI 分析。

![Version](https://img.shields.io/badge/version-1.5.0-blue)
![Qt](https://img.shields.io/badge/Qt-6.10.0-green)
![C++](https://img.shields.io/badge/C++-17-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## ✨ 功能特性

### 核心功能
- 📚 **题库管理**：支持 Markdown 格式题库导入，自动解析题目结构
- ✏️ **代码编辑**：基于 QScintilla，支持语法高亮、自动补全、行号显示
- 💾 **自动保存**：实时保存代码（300ms 防抖），防止数据丢失
- 🔧 **编译测试**：本地 C++ 编译运行，自动执行测试用例，详细结果展示
- 🤖 **AI 分析**：使用 Ollama 本地模型或云端 API 分析代码，提供优化建议

### 🎯 刷题系统 (v1.5.0 新增)
- 📊 **进度跟踪**：自动记录每道题的完成状态（未开始/进行中/已完成/已掌握）
- 🔍 **多维筛选**：按难度、题型、状态筛选，支持搜索
- 📈 **统计分析**：实时显示总题数、完成数、正确率、进度百分比
- 📋 **详细信息**：显示每题的尝试次数、正确率、最后提交时间
- 🎯 **智能判定**：根据测试结果自动更新状态，连续正确自动标记为"已掌握"
- 💾 **数据持久化**：进度数据自动保存，关闭程序不丢失

### 辅助功能
- 📖 **查看原题**：完整显示题目描述、测试用例、参考答案
- 📝 **错题本**：自动记录失败题目，支持标记已解决
- 📊 **历史记录**：查看所有提交历史，追踪学习进度
- ⚙️ **设置中心**：图形化配置编译器、AI服务、编辑器

## 🚀 快速开始

### 环境要求

- **Qt 6.10.0** 或更高版本
- **QScintilla 2.14.1**
- **C++ 编译器** (g++ 或 clang++)
- **CMake 3.16+** 和 **Ninja**
- **Ollama** (可选，用于本地 AI)

### 构建项目

#### Windows

```bash
# 1. 配置 CMake
.\configure_cmake.bat

# 2. 构建项目
.\build_project.bat

# 3. 运行程序
.\build\CodePracticeSystem.exe
```

#### 手动构建

```bash
mkdir build && cd build
cmake -G "Ninja" -DCMAKE_PREFIX_PATH=F:/Qt/qt/6.10.0/mingw_64 ..
cmake --build .
```

### 配置系统

编辑 `data/config.json`：

```json
{
  "compilerPath": "g++",                      // C++ 编译器路径
  "ollamaUrl": "http://localhost:11434",     // Ollama 服务地址
  "ollamaModel": "qwen",                     // AI 模型名称
  "cloudApiKey": ""                          // 云端 API Key（可选）
}
```

## 📖 使用说明

### 1. 导入题库

1. 启动程序
2. 点击 **文件 → 导入题库** (或按 `Ctrl+I`)
3. 选择题库文件夹（例如：`data/sample_questions`）
4. 选择导入模式，点击确定

### 2. 开始刷题

1. 浏览题目（使用上一题/下一题按钮）
2. 在代码编辑器中编写 C++ 代码
3. 点击 **运行测试** 按钮
4. 查看测试结果和通过率

### 3. AI 代码分析

1. 编写完代码后，点击 **AI分析代码** 按钮
2. 等待 AI 分析完成
3. 查看代码思路、优化建议和知识点

### 4. 查看历史

- 点击 **历史 → 查看做题记录** (或按 `Ctrl+H`)
- 查看所有做过的题目和保存的代码

## 📁 项目结构

```
CodePracticeSystem/
├── src/                          # 源代码
│   ├── ui/                       # 界面层
│   │   ├── MainWindow.*          # 主窗口
│   │   ├── QuestionPanel.*       # 题目面板
│   │   ├── CodeEditor.*          # 代码编辑器
│   │   ├── AIAnalysisPanel.*     # AI 分析面板
│   │   ├── ImportDialog.*        # 导入对话框
│   │   └── HistoryWidget.*       # 历史记录
│   ├── core/                     # 核心逻辑
│   │   ├── QuestionBank.*        # 题库管理
│   │   ├── Question.*            # 题目模型
│   │   ├── AutoSaver.*           # 自动保存
│   │   └── CompilerRunner.*      # 编译运行
│   ├── ai/                       # AI 服务
│   │   ├── AIService.*           # AI 服务接口
│   │   ├── OllamaClient.*        # Ollama 客户端
│   │   ├── CloudAIClient.*       # 云端 API 客户端
│   │   ├── QuestionParser.*      # 题目解析器
│   │   └── FineTuneManager.*     # 微调管理
│   ├── utils/                    # 工具类
│   │   ├── FileManager.*         # 文件管理
│   │   └── ConfigManager.*       # 配置管理
│   └── main.cpp                  # 程序入口
├── data/                         # 数据目录
│   ├── config.json               # 配置文件
│   ├── sample_questions/         # 示例题库
│   ├── questions/                # 导入的题库
│   └── user_answers/             # 用户代码
├── build/                        # 构建目录
├── CMakeLists.txt                # CMake 配置
├── README.md                     # 项目说明
├── FEATURES.md                   # 功能文档
├── USAGE.md                      # 使用说明
└── PROJECT_SUMMARY.md            # 项目总结
```

## 🎯 题库格式

### Markdown 格式示例

```markdown
# 题目标题

难度：简单

## 题目描述

这里是题目的详细描述...

## 示例

### 示例 1
输入: 测试输入
输出: 期望输出

### 示例 2
输入: 另一个输入
输出: 另一个输出

## 提示

- 提示 1
- 提示 2
```

详细格式说明请参考 [USAGE.md](USAGE.md)

## 🔧 技术栈

- **Qt 6.10.0** - GUI 框架
- **QScintilla 2.14.1** - 代码编辑器组件
- **C++17** - 编程语言标准
- **CMake + Ninja** - 构建系统
- **Ollama** - 本地 AI 模型运行环境
- **MinGW 13.1.0** - Windows 编译器

## 📊 功能完成度

- ✅ 核心功能：100%
- ✅ UI 功能：85%
- ✅ AI 功能：70%
- 🔄 扩展功能：30%

详细功能列表请参考 [FEATURES.md](FEATURES.md)

## 🐛 已知问题

1. **测试用例识别**：部分 Markdown 格式可能无法识别，请使用标准格式
2. **编译器路径**：默认使用 `g++`，需要在 PATH 中或配置完整路径
3. **AI 服务**：需要先启动 Ollama 服务 (`ollama serve`)

## 📝 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+I` | 导入题库 |
| `Ctrl+H` | 查看历史记录 |
| `Ctrl+Q` | 退出程序 |

## 🗺️ 开发计划

### 短期
- [ ] 题目列表侧边栏
- [ ] AI 分析优化（Markdown 渲染）
- [ ] 图形化设置界面

### 中期
- [ ] 错题本功能
- [ ] 题库搜索和筛选
- [ ] 进度统计图表

### 长期
- [ ] AI 题目生成
- [ ] 多语言支持（Python、Java）
- [ ] 在线题库同步

## 📚 文档

- [功能文档](FEATURES.md) - 详细功能说明和实现状态
- [使用说明](USAGE.md) - 完整的使用指南
- [项目总结](PROJECT_SUMMARY.md) - 项目概览和技术细节

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 🙏 致谢

感谢以下开源项目：
- [Qt](https://www.qt.io/) - 强大的跨平台 GUI 框架
- [QScintilla](https://www.riverbankcomputing.com/software/qscintilla/) - 专业的代码编辑器组件
- [Ollama](https://ollama.ai/) - 本地 AI 模型运行环境
- [CMake](https://cmake.org/) - 跨平台构建工具

---

**版本**: v1.0.0  
**最后更新**: 2024-11-29  
**状态**: ✅ 核心功能完成，可正常使用
