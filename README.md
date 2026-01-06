# Xodor - AI 驱动的代码练习系统

<div align="center">

![Xodor Logo](resources/icon.png)

**一个集成 AI 辅助功能的现代化代码练习平台**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.9.2-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)

[功能特性](#功能特性) • [快速开始](#快速开始) • [使用指南](#使用指南) • [技术架构](#技术架构) • [开发文档](#开发文档)

</div>

---

## 📖 项目简介

Xodor 是一个专为编程学习者设计的智能代码练习系统，深度集成 AI 技术，提供从题目导入、代码编写、智能判题到学习分析的完整学习闭环。

### 核心亮点

- 🤖 **AI 智能判题** - 支持本地 Ollama 和云端 API，提供详细的代码分析和改进建议
- 📚 **智能题库管理** - AI 驱动的题目导入，自动识别和解析多种格式
- 💡 **AI 学习导师** - 实时对话式辅导，帮助理解算法和调试代码
- 📊 **可视化统计** - Codeforces 风格的热力图和进度追踪
- 🎯 **模拟考试** - AI 生成个性化练习题，智能难度调节
- 🔧 **专业代码编辑器** - 基于 QScintilla，支持语法高亮、代码补全、实时错误检查

---

## ✨ 功能特性

### 1. AI 智能判题系统

- **多模式支持**：本地 Ollama 模型 / 云端 API（DeepSeek、OpenAI 等）
- **深度分析**：代码质量评估、算法复杂度分析、优化建议
- **实时反馈**：流式输出判题结果，支持中断和重试
- **历史记录**：保存所有判题记录，支持对比和回顾

### 2. 智能题库管理

- **AI 驱动导入**：
  - 自动识别题目格式（Markdown、纯文本、混合格式）
  - 智能拆分多题目文档
  - 递归处理嵌套内容
  - 自动提取测试用例

- **题库组织**：
  - 树形结构管理
  - 难度分类和标签系统
  - 完成度统计和进度追踪
  - 支持题库导入/导出

### 3. AI 学习导师

- **对话式辅导**：
  - 实时代码分析
  - 算法思路讲解
  - 调试建议
  - 知识点扩展

- **智能交互**：
  - 上下文记忆
  - 代码片段引用
  - Markdown 格式化输出
  - 代码高亮显示

### 4. 专业代码编辑器

- **语法支持**：C/C++、Python、Java 等多语言
- **智能功能**：
  - 代码补全（关键字、变量、函数）
  - 实时语法检查
  - 错误提示和定位
  - 代码折叠
  - 括号匹配

- **编辑增强**：
  - 多主题支持
  - 自动缩进
  - 行号显示
  - UTF-8 编码支持

### 5. 可视化统计分析

- **刷题统计**：
  - 总完成题数
  - 连续刷题天数
  - 今日完成数
  - 正确率统计

- **热力图**：
  - GitHub 风格的活跃度展示
  - 12 周历史数据
  - 颜色渐变表示强度

- **难度分布**：
  - 简单/中等/困难题目统计
  - 完成度百分比
  - 进度条可视化

### 6. 模拟考试系统

- **AI 生成题目**：
  - 基于已有题库智能生成
  - 难度自适应调节
  - 保留原题特征

- **考试管理**：
  - 时间限制
  - 自动评分
  - 成绩报告
  - 错题分析

---

## 🚀 快速开始

### 系统要求

- **操作系统**：Windows 10/11 (64-bit)
- **Qt 版本**：Qt 6.9.2 或更高
- **编译器**：MinGW 64-bit
- **CMake**：3.16 或更高
- **AI 支持**（可选）：
  - 本地：Ollama（推荐 qwen2.5-coder 模型）
  - 云端：DeepSeek API / OpenAI API

### 安装步骤

#### 方式一：使用预编译版本（推荐）

1. 下载最新 Release 版本
2. 解压到任意目录
3. 运行 `CodePracticeSystem.exe`

#### 方式二：从源码编译

```bash
# 1. 克隆仓库
git clone https://github.com/BlupureQingAn/Xodor.git
cd Xodor

# 2. 配置 CMake（替换为你的 Qt 路径）
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=F:/Qt/6.9.2/mingw_64

# 3. 编译项目
cmake --build build --target CodePracticeSystem

# 4. 运行程序
build/CodePracticeSystem.exe
```

### 首次配置

1. **配置 AI 服务**（设置 → AI 配置）：
   - **本地模式**：安装 Ollama，下载模型
   - **云端模式**：配置 API Key 和端点

2. **导入题库**：
   - 文件 → 导入题库
   - 支持 Markdown、JSON 格式
   - 可使用 AI 智能导入

3. **开始练习**：
   - 选择题目
   - 编写代码
   - AI 判题或运行测试

---

## 📚 使用指南

### 基础操作

#### 1. 题目练习

```
1. 在左侧题库列表选择题目
2. 在代码编辑器中编写解答
3. 点击"AI 判题"获取智能反馈
4. 查看判题结果和改进建议
```

#### 2. AI 导师对话

```
1. 点击右侧"AI 导师"面板
2. 输入问题或请求代码分析
3. AI 导师提供实时辅导
4. 支持代码片段引用和讨论
```

#### 3. 题库管理

```
1. 文件 → 题库管理
2. 查看所有题库统计
3. 导入新题库或删除题库
4. 刷新题库更新题目数量
```

### 高级功能

#### AI 智能导入

```
1. 文件 → AI 智能导入
2. 选择包含题目的文档
3. AI 自动识别和拆分题目
4. 预览并确认导入
```

#### 模拟考试

```
1. 工具 → 模拟考试
2. 选择题库和题目数量
3. AI 生成个性化题目
4. 限时完成并查看报告
```

#### 统计分析

```
1. 切换到"题库面板"
2. 查看刷题统计卡片
3. 分析热力图和难度分布
4. 导出进度报告
```

---

## 🏗️ 技术架构

### 技术栈

- **UI 框架**：Qt 6.9.2 (Widgets)
- **编程语言**：C++ 17
- **代码编辑器**：QScintilla 2.14.1
- **构建系统**：CMake 3.16+
- **网络通信**：Qt Network (HTTPS/TLS)
- **数据存储**：JSON 文件
- **AI 集成**：
  - Ollama API (本地)
  - OpenAI-compatible API (云端)

### 项目结构

```
Xodor/
├── src/                    # 源代码
│   ├── ui/                # UI 组件
│   │   ├── MainWindow.*   # 主窗口
│   │   ├── CodeEditor.*   # 代码编辑器
│   │   ├── AIAssistantPanel.*  # AI 导师面板
│   │   └── ...
│   ├── core/              # 核心逻辑
│   │   ├── QuestionBank.* # 题库管理
│   │   ├── ProgressManager.*  # 进度管理
│   │   └── ...
│   ├── ai/                # AI 功能
│   │   ├── OllamaClient.* # Ollama 客户端
│   │   ├── AIJudge.*      # AI 判题
│   │   ├── SmartQuestionImporter.*  # 智能导入
│   │   └── ...
│   └── utils/             # 工具类
├── resources/             # 资源文件
│   ├── icon.svg          # 应用图标
│   └── app_icon.rc       # Windows 资源
├── data/                  # 数据目录
│   ├── 基础题库/         # 题库文件
│   └── config/           # 配置文件
├── CMakeLists.txt        # CMake 配置
├── README.md             # 项目说明
└── LICENSE               # 许可证
```

### 核心模块

#### 1. AI 判题引擎 (`AIJudge`)

```cpp
// 支持流式输出的 AI 判题
class AIJudge : public QObject {
    Q_OBJECT
public:
    void judgeCode(const QString &questionId, 
                   const QString &code);
    void stopJudging();
    
signals:
    void judgeStarted();
    void judgeProgress(const QString &text);
    void judgeCompleted(const QString &result);
    void judgeError(const QString &error);
};
```

#### 2. 智能题目导入 (`SmartQuestionImporter`)

```cpp
// AI 驱动的题目识别和导入
class SmartQuestionImporter : public QObject {
    Q_OBJECT
public:
    void importFromText(const QString &content);
    void importFromFile(const QString &filePath);
    
signals:
    void importProgress(int current, int total);
    void questionParsed(const Question &question);
    void importCompleted(int count);
};
```

#### 3. 进度管理 (`ProgressManager`)

```cpp
// 统计和进度追踪
class ProgressManager : public QObject {
    Q_OBJECT
public:
    void updateProgress(const QString &questionId, 
                       QuestionStatus status);
    QMap<QDate, int> getActivityByDate(int days);
    double getOverallAccuracy();
    
signals:
    void progressUpdated(const QString &questionId);
    void statisticsChanged();
};
```

---

## 🎨 界面展示

### 主界面

- 左侧：题库树形列表
- 中间：代码编辑器 + 题目面板
- 右侧：AI 导师对话面板
- 底部：错误列表和输出

### 题库面板

- 题库选择器
- 刷题统计卡片
- 热力图
- 难度分布图
- 题目列表表格

### AI 判题

- 实时流式输出
- 代码分析结果
- 优化建议
- 测试用例验证

---

## 🔧 开发文档

### 编译要求

- Qt 6.9.2 MinGW 64-bit
- CMake 3.16+
- QScintilla 2.14.1
- C++17 编译器

### 编译步骤

```bash
# 配置
cmake -B build -G "Ninja" \
  -DCMAKE_PREFIX_PATH=/path/to/qt \
  -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --config Release

# 部署
cmake --install build --prefix deploy
```

### 开发环境

推荐使用 Qt Creator：

1. 打开 `CMakeLists.txt`
2. 配置 Qt Kit (Qt 6.9.2 MinGW)
3. 构建项目
4. 运行调试

### 代码规范

- 使用 C++17 标准
- 遵循 Qt 命名约定
- 使用 UTF-8 编码
- 添加必要的注释

---

## 📝 配置说明

### AI 配置文件

位置：`C:/Users/用户名/AppData/Roaming/CodePractice/CodePracticeSystem/config.json`

```json
{
  "ai": {
    "mode": "cloud",
    "ollama": {
      "url": "http://localhost:11434",
      "model": "qwen2.5-coder:latest"
    },
    "cloud": {
      "url": "https://api.deepseek.com",
      "apiKey": "your-api-key",
      "model": "deepseek-chat"
    }
  }
}
```

### 题库配置

位置：`C:/Users/用户名/AppData/Roaming/CodePractice/CodePracticeSystem/question_banks.json`

```json
{
  "banks": [
    {
      "id": "ccf-basic",
      "name": "CCF 基础题库",
      "path": "data/基础题库/CCF",
      "questionCount": 45
    }
  ]
}
```

---

## 🤝 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 如何贡献

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 报告问题

请使用 GitHub Issues 报告问题，并提供：

- 问题描述
- 复现步骤
- 预期行为
- 实际行为
- 系统环境
- 错误日志

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 👥 团队信息

- **团队名称**：Xodor Team
- **项目负责人**：BlupureQingAn
- **联系方式**：通过 GitHub Issues

---

## 🙏 致谢

- Qt Framework - 强大的跨平台 UI 框架
- QScintilla - 专业的代码编辑器组件
- Ollama - 本地 AI 模型运行环境
- DeepSeek - 优秀的云端 AI 服务

---

## 📊 项目统计

- **代码行数**：~15,000 行 C++
- **文件数量**：~150 个源文件
- **开发周期**：3 个月
- **支持语言**：C++、Python、Java
- **测试题库**：500+ 道题目

---

<div align="center">

**如果这个项目对你有帮助，请给一个 ⭐ Star！**

Made with ❤️ by Xodor Team

</div>
