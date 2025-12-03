# Git 项目管理指南

## 📦 项目状态

✅ **Git仓库已初始化**  
✅ **v2.0.0 版本已标记**  
✅ **241个文件已提交**

## 🏷️ 当前版本

- **版本**: v2.0.0
- **提交**: 0a0647c
- **分支**: master
- **标签**: v2.0.0

## 📋 常用Git命令

### 查看状态
```bash
# 查看当前状态
git status

# 查看提交历史
git log --oneline

# 查看所有标签
git tag

# 查看特定标签信息
git show v2.0.0
```

### 日常开发
```bash
# 添加修改的文件
git add .

# 提交更改
git commit -m "描述你的更改"

# 查看差异
git diff

# 查看已暂存的差异
git diff --staged
```

### 分支管理
```bash
# 创建新分支
git branch feature/new-feature

# 切换分支
git checkout feature/new-feature

# 创建并切换分支
git checkout -b feature/new-feature

# 查看所有分支
git branch -a

# 合并分支
git merge feature/new-feature

# 删除分支
git branch -d feature/new-feature
```

### 版本标签
```bash
# 创建轻量标签
git tag v2.0.1

# 创建附注标签
git tag -a v2.0.1 -m "版本说明"

# 查看标签
git tag

# 删除标签
git tag -d v2.0.1

# 推送标签到远程
git push origin v2.0.1

# 推送所有标签
git push origin --tags
```

### 远程仓库
```bash
# 添加远程仓库
git remote add origin https://github.com/username/CodePracticeSystem.git

# 查看远程仓库
git remote -v

# 推送到远程
git push -u origin master

# 推送标签
git push origin --tags

# 拉取更新
git pull origin master

# 克隆仓库
git clone https://github.com/username/CodePracticeSystem.git
```

## 🔄 推荐工作流程

### 1. 功能开发流程
```bash
# 1. 创建功能分支
git checkout -b feature/new-feature

# 2. 开发并提交
git add .
git commit -m "feat: 添加新功能"

# 3. 切回主分支
git checkout master

# 4. 合并功能分支
git merge feature/new-feature

# 5. 删除功能分支
git branch -d feature/new-feature
```

### 2. Bug修复流程
```bash
# 1. 创建修复分支
git checkout -b bugfix/fix-issue

# 2. 修复并提交
git add .
git commit -m "fix: 修复某个问题"

# 3. 合并回主分支
git checkout master
git merge bugfix/fix-issue

# 4. 删除修复分支
git branch -d bugfix/fix-issue
```

### 3. 版本发布流程
```bash
# 1. 确保所有更改已提交
git status

# 2. 更新版本号（在CMakeLists.txt等文件中）
# 编辑文件...

# 3. 提交版本更新
git add .
git commit -m "chore: bump version to v2.1.0"

# 4. 创建版本标签
git tag -a v2.1.0 -m "Release v2.1.0 - 新功能描述"

# 5. 推送到远程
git push origin master
git push origin v2.1.0
```

## 📝 提交信息规范

使用语义化提交信息：

- `feat:` 新功能
- `fix:` Bug修复
- `docs:` 文档更新
- `style:` 代码格式调整
- `refactor:` 代码重构
- `perf:` 性能优化
- `test:` 测试相关
- `chore:` 构建/工具相关

### 示例
```bash
git commit -m "feat: 添加VSCode风格错误面板"
git commit -m "fix: 修复语法检查内存泄漏"
git commit -m "docs: 更新README文档"
git commit -m "perf: 优化语法检查性能"
```

## 🌿 分支策略

### 主要分支
- `master` - 主分支，稳定版本
- `develop` - 开发分支，最新开发代码

### 辅助分支
- `feature/*` - 功能分支
- `bugfix/*` - Bug修复分支
- `hotfix/*` - 紧急修复分支
- `release/*` - 发布准备分支

## 🔧 .gitignore 配置

已配置忽略以下内容：
- 构建目录 (build/, build_release/)
- 编译产物 (*.exe, *.dll, *.o)
- IDE配置 (.vscode/, .qtcreator/)
- 用户数据 (data/user_answers/)
- 临时文件 (*.tmp, *.log)

## 📤 推送到GitHub/GitLab

### 首次推送
```bash
# 1. 在GitHub/GitLab创建仓库

# 2. 添加远程仓库
git remote add origin https://github.com/username/CodePracticeSystem.git

# 3. 推送代码和标签
git push -u origin master
git push origin --tags
```

### 后续推送
```bash
# 推送代码
git push

# 推送标签
git push origin v2.0.0
```

## 🔍 查看项目信息

```bash
# 查看提交统计
git log --stat

# 查看文件修改历史
git log --follow src/ui/MainWindow.cpp

# 查看某个版本的文件
git show v2.0.0:src/ui/MainWindow.cpp

# 查看两个版本之间的差异
git diff v1.0.0 v2.0.0
```

## 🚀 下一步建议

1. **设置远程仓库**
   - 在GitHub/GitLab创建仓库
   - 推送代码到远程

2. **创建开发分支**
   ```bash
   git checkout -b develop
   git push -u origin develop
   ```

3. **配置CI/CD**
   - 设置自动构建
   - 自动运行测试
   - 自动发布Release

4. **编写贡献指南**
   - 创建CONTRIBUTING.md
   - 说明开发流程
   - 代码规范要求

## 📚 学习资源

- [Git官方文档](https://git-scm.com/doc)
- [GitHub指南](https://guides.github.com/)
- [Git分支管理](https://nvie.com/posts/a-successful-git-branching-model/)
- [语义化版本](https://semver.org/lang/zh-CN/)

---

**当前项目状态**: ✅ Git仓库已就绪，v2.0.0已发布！
