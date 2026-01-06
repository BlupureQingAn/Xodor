# GitHub Release 创建步骤

## ✅ 准备工作已完成

- ✅ 压缩包已创建: `Xodor-v1.0.0-Windows-x64.zip` (46.13 MB)
- ✅ 位置: `F:\Xodor\Xodor-v1.0.0-Windows-x64.zip`

---

## 📋 创建 Release 的详细步骤

### 第1步：访问 GitHub Releases 页面

1. 打开浏览器，访问：https://github.com/BlupureQingAn/Xodor/releases
2. 点击右上角的 **"Create a new release"** 按钮（绿色按钮）

### 第2步：填写 Release 信息

#### 2.1 选择标签（Tag）

- **Tag version**: 输入 `v1.0.0`
- 点击 **"Create new tag: v1.0.0 on publish"**

#### 2.2 填写标题

- **Release title**: 输入 `Xodor v1.0.0 - 智能刷题系统首次发布`

#### 2.3 填写描述（复制以下内容）

```markdown
# Xodor v1.0.0 - 智能刷题系统

> 🎉 首次正式发布 - 专为【码上AI】技术挑战赛提交

## 📖 项目简介

Xodor 是一个集成 AI 辅助功能的现代化代码练习平台，深度整合 AI 技术，提供从题目导入、代码编写、智能判题到学习分析的完整学习闭环。

## ✨ 核心功能

### 🤖 AI 智能判题
- 支持本地 Ollama 和云端 API（DeepSeek、OpenAI 等）
- 深度代码分析、算法复杂度评估、优化建议
- 实时流式输出，支持中断和重试

### 📚 智能题库管理
- AI 驱动的题目导入，自动识别和解析多种格式
- 递归处理嵌套内容，智能拆分多题目文档
- 预置 CCF 题库（45道题 + 529道MD格式题目）

### 💡 AI 学习导师
- 实时对话式辅导，帮助理解算法和调试代码
- 上下文记忆，支持代码片段引用
- Markdown 格式化输出，代码高亮显示

### 📊 可视化统计分析
- Codeforces 风格的热力图和进度追踪
- 刷题统计、连续天数、正确率分析
- 难度分布可视化

### 🔧 专业代码编辑器
- 基于 QScintilla，支持 C++/Python/Java 等多语言
- 语法高亮、代码补全、实时错误检查
- 代码折叠、括号匹配、自动缩进

### 🎯 AI 模拟考试
- AI 生成个性化练习题，智能难度调节
- 时间限制、自动评分、成绩报告

## 🚀 快速开始

### 系统要求
- Windows 10/11 (64-bit)
- 无需安装 Qt 或其他依赖（已打包所有必需文件）

### 安装步骤
1. 下载 `Xodor-v1.0.0-Windows-x64.zip`
2. 解压到任意目录
3. 运行 `Xodor/CodePracticeSystem.exe`

### 首次配置（已提供测试 API Key）

**为方便评委老师测试，已提供预配置的 API Key：**

1. 启动程序后，点击 **设置 → AI 配置**
2. 选择 **云端模式**
3. 填入以下信息：
   ```
   API URL: https://api.deepseek.com
   API Key: sk-1a8bae2865f1443c99b924ffd14c4252
   模型: deepseek-chat
   ```
4. 点击 **测试连接** → **保存**
5. 开始使用！

## 📚 文档

- [README.md](https://github.com/BlupureQingAn/Xodor/blob/main/README.md) - 完整项目说明
- [评委测试指南.md](https://github.com/BlupureQingAn/Xodor/blob/main/评委测试指南.md) - 5分钟快速测试
- [QUICK_START.md](https://github.com/BlupureQingAn/Xodor/blob/main/QUICK_START.md) - 快速开始指南
- [USAGE.md](https://github.com/BlupureQingAn/Xodor/blob/main/USAGE.md) - 详细使用说明
- [TROUBLESHOOTING.md](https://github.com/BlupureQingAn/Xodor/blob/main/TROUBLESHOOTING.md) - 故障排除

## 🔧 技术栈

- **UI 框架**: Qt 6.9.2 (Widgets)
- **编程语言**: C++ 17
- **代码编辑器**: QScintilla 2.14.1
- **构建系统**: CMake 3.16+
- **AI 集成**: Ollama API / OpenAI-compatible API

## 📊 项目统计

- **代码行数**: ~15,000 行 C++
- **文件数量**: ~150 个源文件
- **预置题库**: 574 道题目（CCF 认证题库）
- **开发周期**: 3 个月

## 🎯 适用场景

- 算法竞赛练习（ACM、CCF、蓝桥杯等）
- 编程学习和训练
- 代码能力提升
- 面试准备

## 📝 更新日志

详见 [CHANGELOG.md](https://github.com/BlupureQingAn/Xodor/blob/main/CHANGELOG.md)

## 🙏 致谢

- Qt Framework - 强大的跨平台 UI 框架
- QScintilla - 专业的代码编辑器组件
- Ollama - 本地 AI 模型运行环境
- DeepSeek - 优秀的云端 AI 服务

---

## 📦 下载说明

### 文件列表

- **Xodor-v1.0.0-Windows-x64.zip** (46 MB)
  - 包含完整的可执行程序
  - 包含所有必需的 DLL 文件
  - 包含预置的 CCF 题库（574道题）
  - 包含完整文档

### 安装后目录结构

```
Xodor/
├── CodePracticeSystem.exe    # 主程序
├── data/                      # 数据目录
│   ├── 基础题库/             # 预置题库
│   │   └── CCF/              # CCF 认证题库
│   └── sample_questions/     # 示例题目
├── platforms/                 # Qt 插件
├── styles/                    # Qt 样式
├── tls/                       # TLS 支持
├── *.dll                      # 依赖库
├── README.md                  # 项目说明
├── 评委测试指南.md           # 快速测试指南
└── 其他文档...
```

## 🐛 已知问题

无重大已知问题。如遇到问题，请查看 [TROUBLESHOOTING.md](https://github.com/BlupureQingAn/Xodor/blob/main/TROUBLESHOOTING.md)

## 📞 反馈与支持

- **GitHub Issues**: https://github.com/BlupureQingAn/Xodor/issues
- **项目主页**: https://github.com/BlupureQingAn/Xodor

---

**感谢使用 Xodor！如果觉得有帮助，请给个 ⭐ Star！**

Made with ❤️ by Xodor Team
```

### 第3步：上传压缩包

1. 在页面底部找到 **"Attach binaries by dropping them here or selecting them"**
2. 点击或拖拽 `Xodor-v1.0.0-Windows-x64.zip` 文件上传
3. 等待上传完成（46 MB，可能需要几分钟）

### 第4步：发布 Release

1. 确认所有信息填写正确
2. 勾选 **"Set as the latest release"**（设为最新版本）
3. 点击绿色按钮 **"Publish release"**

---

## ✅ 发布后的检查

### 1. 验证 Release 页面

访问：https://github.com/BlupureQingAn/Xodor/releases/tag/v1.0.0

确认：
- ✅ 标题和描述显示正确
- ✅ 压缩包可以下载
- ✅ 标记为 "Latest" 版本

### 2. 测试下载

1. 点击下载链接
2. 解压文件
3. 运行 `CodePracticeSystem.exe`
4. 验证程序正常启动

### 3. 更新提交邮件

在提交邮件中添加 Release 链接：
```
Release 页面：https://github.com/BlupureQingAn/Xodor/releases/tag/v1.0.0
下载链接：https://github.com/BlupureQingAn/Xodor/releases/download/v1.0.0/Xodor-v1.0.0-Windows-x64.zip
```

---

## 📧 提交邮件模板

**收件人**: zgsm@sangfor.com.cn

**主题**: 【码上AI】技术挑战赛-[你的团队名称]-[你的学校]

**正文**:

```
尊敬的评审老师：

您好！

我们是[团队名称]，来自[学校名称]。现提交我们的参赛作品：Xodor - 智能刷题系统。

【项目信息】
- 项目名称：Xodor - 智能刷题系统
- GitHub 仓库：https://github.com/BlupureQingAn/Xodor
- Release 页面：https://github.com/BlupureQingAn/Xodor/releases/tag/v1.0.0
- 下载链接：https://github.com/BlupureQingAn/Xodor/releases/download/v1.0.0/Xodor-v1.0.0-Windows-x64.zip
- 仓库状态：公开（Public）

【快速测试】
为方便评审，我们提供了：
1. 预置的 CCF 题库（574道题）
2. 测试用 API Key（已在 README 中说明）
3. 详细的《评委测试指南.md》（5分钟快速体验）

【团队成员】
1. [成员1姓名] - [角色/职责]
2. [成员2姓名] - [角色/职责]

【项目简介】
Xodor 是一个基于 Qt 6.9.2 和 C++17 开发的智能刷题系统，集成了 AI 辅助功能，
旨在帮助学生更高效地进行算法练习和竞赛准备。

核心特性：
- AI 智能判题和学习导师
- 智能题目导入和管理
- 专业代码编辑器（基于 QScintilla）
- Codeforces 风格的统计分析
- AI 生成模拟考试

【技术亮点】
- 完整的 Qt/C++ 桌面应用架构
- 集成 Ollama 本地 AI 和云端 API
- Markdown 题目解析和渲染
- 会话恢复和数据持久化
- 现代化 UI 设计

感谢评审老师的审阅！

此致
敬礼

[团队名称]
[日期]
```

---

## 🎯 重要提示

1. **确保仓库公开**: 进入仓库 Settings → 确认显示 "Public"
2. **测试下载链接**: 发送邮件前，先测试 Release 下载链接是否有效
3. **截止日期**: 确保在比赛截止日期前完成提交
4. **保留备份**: 保存好压缩包的本地副本

---

## 📞 需要帮助？

如有问题，请查看：
- [GitHub 官方文档 - 创建 Release](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

---

**祝比赛顺利！🎉**
