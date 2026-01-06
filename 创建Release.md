# 创建 GitHub Release 指南

## 步骤说明

### 1. 访问 GitHub 仓库

打开浏览器，访问：
```
https://github.com/BlupureQingAn/Xodor
```

### 2. 进入 Releases 页面

- 点击右侧边栏的 **"Releases"** 链接
- 或直接访问：`https://github.com/BlupureQingAn/Xodor/releases`

### 3. 创建新 Release

点击 **"Create a new release"** 或 **"Draft a new release"** 按钮

### 4. 填写 Release 信息

#### Tag version（标签版本）
```
v1.0.0
```

#### Release title（发布标题）
```
Xodor v1.0.0 - 首次正式发布
```

#### Description（发布说明）

复制以下内容：

```markdown
# Xodor v1.0.0 - AI 驱动的代码练习系统

## 🎉 首次正式发布

这是 Xodor 的首个正式版本，提交参加【码上AI】技术挑战赛。

## ✨ 核心功能

### AI 智能判题系统
- ✅ 支持本地 Ollama 和云端 API 双模式
- ✅ 实时流式输出判题结果
- ✅ 详细的代码分析和优化建议
- ✅ 支持中断和重试

### 智能题库管理
- ✅ AI 驱动的题目导入
- ✅ 自动识别多种格式
- ✅ 智能拆分多题目文档
- ✅ 树形结构管理

### AI 学习导师
- ✅ 实时对话式辅导
- ✅ 代码分析和讲解
- ✅ 算法思路指导

### 专业代码编辑器
- ✅ 基于 QScintilla
- ✅ 语法高亮和代码补全
- ✅ 实时语法检查

### 可视化统计
- ✅ Codeforces 风格热力图
- ✅ 刷题统计和进度追踪

## 📦 下载

### Windows 版本

**系统要求**：
- Windows 10/11 (64-bit)
- 无需安装 Qt 运行时

**下载链接**：
- [CodePracticeSystem-v1.0.0-Windows-x64.zip](附件)

**使用方法**：
1. 下载并解压
2. 运行 `CodePracticeSystem.exe`
3. 首次使用请配置 AI 服务（设置 → AI 配置）

## 📚 文档

- [README.md](https://github.com/BlupureQingAn/Xodor/blob/main/README.md) - 完整项目说明
- [CHANGELOG.md](https://github.com/BlupureQingAn/Xodor/blob/main/CHANGELOG.md) - 更新日志

## 🔧 从源码编译

```bash
git clone https://github.com/BlupureQingAn/Xodor.git
cd Xodor
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build build
```

详见 [README.md](https://github.com/BlupureQingAn/Xodor/blob/main/README.md#快速开始)

## 🐛 已知问题

暂无

## 🙏 致谢

感谢所有支持和使用 Xodor 的用户！

---

**完整源代码**: https://github.com/BlupureQingAn/Xodor
**问题反馈**: https://github.com/BlupureQingAn/Xodor/issues
```

### 5. 上传附件（可选）

如果有编译好的可执行文件：

1. 点击 **"Attach binaries by dropping them here or selecting them"**
2. 上传 `CodePracticeSystem-v1.0.0-Windows-x64.zip`（包含 deploy 目录的所有文件）

### 6. 发布 Release

- 确认所有信息无误
- 点击 **"Publish release"** 按钮

## Release 地址

发布后，Release 地址为：
```
https://github.com/BlupureQingAn/Xodor/releases/tag/v1.0.0
```

## 提交邮件模板

### 邮件信息

**收件人**：`zgsm@sangfor.com.cn`

**主题**：`【码上AI】技术挑战赛-Xodor Team-[你的学校名称]`

**正文**：

```
尊敬的评审老师：

您好！

我们是 Xodor Team，现提交【码上AI】技术挑战赛作品。

【团队信息】
团队名称：Xodor Team
团队成员：[成员1姓名]、[成员2姓名]（如有）
所在学校：[学校名称]

【项目信息】
项目名称：Xodor - AI 驱动的代码练习系统
GitHub 仓库：https://github.com/BlupureQingAn/Xodor
Release 地址：https://github.com/BlupureQingAn/Xodor/releases/tag/v1.0.0

【项目简介】
Xodor 是一个集成 AI 辅助功能的现代化代码练习平台，提供智能判题、AI 学习导师、可视化统计等核心功能。

【核心特性】
1. AI 智能判题系统（支持本地 Ollama 和云端 API）
2. AI 驱动的题库智能导入
3. 实时对话式 AI 学习导师
4. 专业代码编辑器（基于 QScintilla）
5. Codeforces 风格的可视化统计

【技术栈】
- 框架：Qt 6.9.2
- 语言：C++ 17
- 构建：CMake
- AI 集成：Ollama / OpenAI-compatible API

【仓库状态】
✅ 仓库已公开
✅ 代码已完整提交
✅ Release v1.0.0 已发布
✅ 文档完整（README、CHANGELOG、LICENSE）

感谢评审老师的审阅！

此致
敬礼

Xodor Team
[日期]
```

## 检查清单

提交前请确认：

- [ ] GitHub 仓库已创建并公开
- [ ] 所有代码已推送到 main 分支
- [ ] README.md 完整且格式正确
- [ ] LICENSE 文件已添加
- [ ] CHANGELOG.md 已更新
- [ ] .gitignore 已配置
- [ ] Release v1.0.0 已创建
- [ ] Release 说明完整
- [ ] 邮件已准备好
- [ ] 邮件主题格式正确
- [ ] 仓库地址和 Release 地址正确

## 注意事项

1. **仓库必须公开**：评审老师需要能够访问
2. **代码在 main 分支**：确保稳定可运行的代码在 main 分支
3. **Release 标签**：使用 `v1.0.0` 或 `submission-final`
4. **邮件截止日期**：注意比赛截止时间
5. **文档完整性**：README.md 是评审的主要依据

## 常见问题

### Q: 如何创建 Personal Access Token？

A: 
1. GitHub 设置 → Developer settings → Personal access tokens
2. Generate new token (classic)
3. 选择 `repo` 权限
4. 生成并保存 token
5. 推送时使用 token 代替密码

### Q: 推送失败怎么办？

A:
1. 检查网络连接
2. 确认仓库已创建
3. 使用 HTTPS 而非 SSH
4. 使用 Personal Access Token

### Q: 如何打包 Release 附件？

A:
```bash
# 压缩 deploy 目录
cd deploy
zip -r ../CodePracticeSystem-v1.0.0-Windows-x64.zip *
```

或使用 Windows 资源管理器右键压缩。

---

**祝提交顺利！** 🎉
