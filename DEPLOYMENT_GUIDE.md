# 代码刷题系统 - 部署指南

## ✅ 打包完成

您的应用程序已成功打包！

### 📦 打包文件

- **部署目录**: `deploy/`
- **压缩包**: `CodePracticeSystem-v1.5.0-Windows-x64.zip` (37.14 MB)
- **可执行文件**: `deploy/CodePracticeSystem.exe`

### 📁 部署目录结构

```
deploy/
├── CodePracticeSystem.exe          # 主程序
├── Qt6Core.dll                     # Qt 核心库
├── Qt6Gui.dll                      # Qt GUI 库
├── Qt6Widgets.dll                  # Qt 控件库
├── Qt6Network.dll                  # Qt 网络库
├── Qt6PrintSupport.dll             # Qt 打印支持
├── Qt6Svg.dll                      # Qt SVG 支持
├── libgcc_s_seh-1.dll             # MinGW 运行时
├── libstdc++-6.dll                # C++ 标准库
├── libwinpthread-1.dll            # 线程库
├── opengl32sw.dll                 # OpenGL 软件渲染
├── D3Dcompiler_47.dll             # DirectX 编译器
├── platforms/                      # Qt 平台插件
│   └── qwindows.dll
├── styles/                         # Qt 样式插件
│   └── qmodernwindowsstyle.dll
├── iconengines/                    # 图标引擎
├── imageformats/                   # 图像格式支持
├── networkinformation/             # 网络信息
├── tls/                           # TLS 支持
├── data/                          # 数据目录
│   ├── config.json                # 配置文件
│   ├── sample_questions/          # 示例题库
│   ├── questions/                 # 题库目录
│   ├── user_answers/              # 用户代码
│   ├── history/                   # 历史记录
│   ├── wrong_questions/           # 错题本
│   ├── sessions/                  # 会话数据
│   └── code_versions/             # 代码版本
├── README.md                      # 项目说明
├── QUICK_START.md                 # 快速开始
├── LICENSE                        # 许可证
└── 使用说明.txt                   # 使用说明
```

## 🚀 分发方式

### 方式 1: 直接分发压缩包（推荐）

```bash
# 压缩包已生成
CodePracticeSystem-v1.5.0-Windows-x64.zip
```

用户只需：
1. 解压到任意目录
2. 双击运行 `CodePracticeSystem.exe`

### 方式 2: 分发整个 deploy 文件夹

直接将 `deploy/` 文件夹复制给用户，保持目录结构完整。

### 方式 3: 创建安装程序（可选）

如需创建专业的安装程序，可使用 NSIS：

```bash
# 1. 安装 NSIS
# 下载: https://nsis.sourceforge.io/Download

# 2. 创建安装程序
.\create_installer.bat
```

## 📋 用户环境要求

### 必需
- ✅ Windows 7/10/11 (64位)
- ✅ 约 100 MB 磁盘空间

### 可选（用于编译代码）
- 🔧 C++ 编译器
  - MinGW-w64: https://winlibs.com/
  - MSVC: Visual Studio Build Tools
  - 或系统已安装的 g++/clang++

### 可选（用于 AI 功能）
- 🤖 Ollama (本地 AI)
  - 下载: https://ollama.ai/
  - 安装后运行: `ollama pull qwen`
- 🌐 或配置云端 API Key

## 🎯 测试清单

在分发前，建议在干净的系统上测试：

- [ ] 程序能正常启动
- [ ] 界面显示正常
- [ ] 能导入示例题库
- [ ] 代码编辑器功能正常
- [ ] 配置编译器后能编译运行
- [ ] AI 功能（如已配置）正常工作
- [ ] 数据能正常保存和加载

## 📝 用户安装步骤

### 1. 解压文件
```
解压 CodePracticeSystem-v1.5.0-Windows-x64.zip
到任意目录，如: C:\CodePracticeSystem\
```

### 2. 首次运行
```
双击 CodePracticeSystem.exe
```

### 3. 配置编译器（可选）
```
1. 点击"工具 → 设置"
2. 在"编译器路径"中输入 g++ 完整路径
   例如: C:\mingw64\bin\g++.exe
3. 点击"保存"
```

### 4. 配置 AI（可选）
```
方式 A - 本地 Ollama:
1. 安装 Ollama
2. 运行: ollama serve
3. 运行: ollama pull qwen
4. 在设置中保持默认配置即可

方式 B - 云端 API:
1. 点击"工具 → 设置"
2. 勾选"使用云端 API"
3. 输入 API Key
4. 点击"保存"
```

### 5. 导入题库
```
1. 点击"文件 → AI智能导入题库"
2. 选择题库文件夹（可先用 data/sample_questions 测试）
3. 输入题库名称
4. 等待 AI 解析完成
```

### 6. 开始刷题
```
1. 在题库列表中选择题目
2. 编写代码
3. 点击"运行测试"
4. 查看结果和 AI 分析
```

## 🔧 常见问题

### Q: 程序无法启动？
A: 确保解压了完整的文件夹，包括所有 DLL 文件和子目录。

### Q: 提示缺少 DLL？
A: 重新解压压缩包，确保所有文件都在同一目录。

### Q: 编译失败？
A: 
1. 检查是否安装了 C++ 编译器
2. 在设置中配置正确的编译器路径
3. 确保编译器在系统 PATH 中或使用完整路径

### Q: AI 功能无响应？
A:
1. 检查 Ollama 是否运行: `ollama serve`
2. 检查模型是否下载: `ollama list`
3. 或切换到云端 API 模式

### Q: 题库导入失败？
A:
1. 确保题库文件为 Markdown 格式
2. 参考 sample_questions 中的示例格式
3. 检查 AI 服务是否正常

## 📊 版本信息

- **版本**: v1.5.0
- **构建日期**: 2024-12-03
- **Qt 版本**: 6.10.0
- **编译器**: MinGW 13.1.0
- **架构**: x64

## 🔄 更新说明

如需更新程序：
1. 备份 `data/` 目录（保存用户数据）
2. 删除旧版本文件
3. 解压新版本
4. 将备份的 `data/` 目录复制回来

## 📞 技术支持

如遇问题：
1. 查看 README.md 和 QUICK_START.md
2. 检查 TROUBLESHOOTING.md
3. 提交 Issue 到项目仓库

---

**打包完成时间**: 2024-12-03
**打包工具**: windeployqt + 自定义脚本
