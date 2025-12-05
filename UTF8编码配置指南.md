# UTF-8编码配置指南

## 问题描述

Windows系统默认使用GB2312/GBK编码，导致：
1. 终端显示中文乱码
2. CMake编译时中文字符串出错
3. 源代码中的中文注释和字符串无法正确处理

## 解决方案

### 方案一：使用自动配置脚本（推荐）

#### PowerShell版本
```powershell
# 运行配置脚本
.\configure_utf8.ps1
```

#### CMD版本
```cmd
# 运行配置脚本
fix_utf8_encoding.bat
```

这些脚本会自动：
1. 设置终端编码为UTF-8
2. 清理旧的构建文件
3. 重新配置CMake
4. 编译项目

### 方案二：手动配置

#### 1. 配置终端编码

**PowerShell：**
```powershell
# 临时设置（当前会话）
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001
```

**CMD：**
```cmd
chcp 65001
```

#### 2. 永久设置Windows终端UTF-8

**方法1：修改注册表（推荐）**
```powershell
# 以管理员身份运行PowerShell
Set-ItemProperty -Path 'HKCU:\Console' -Name 'CodePage' -Value 65001
```

**方法2：Windows设置**
1. 打开"设置" → "时间和语言" → "语言和区域"
2. 点击"管理语言设置"
3. 点击"更改系统区域设置"
4. 勾选"Beta版：使用Unicode UTF-8提供全球语言支持"
5. 重启电脑

#### 3. 配置VSCode/IDE

将 `.vscode/settings.json.recommended` 的内容复制到 `.vscode/settings.json`

关键配置：
```json
{
    "files.encoding": "utf8",
    "terminal.integrated.profiles.windows": {
        "PowerShell": {
            "args": ["-NoExit", "-Command", 
                "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; chcp 65001 | Out-Null"]
        }
    }
}
```

#### 4. 清理并重新编译

```powershell
# 清理旧文件
Remove-Item -Recurse -Force build, .cache

# 重新配置
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 编译
cmake --build build
```

## CMakeLists.txt配置说明

项目已配置以下编译选项：

### MinGW/GCC
```cmake
add_compile_options(-finput-charset=UTF-8)      # 源文件编码
add_compile_options(-fexec-charset=UTF-8)       # 执行字符集
add_compile_options(-fwide-exec-charset=UTF-8)  # 宽字符集
add_link_options(-municode)                     # Unicode链接
```

### MSVC
```cmake
add_compile_options(/utf-8)              # UTF-8编码
add_compile_options(/Zc:__cplusplus)     # C++版本
```

## 验证配置

### 1. 检查终端编码
```powershell
# PowerShell
[Console]::OutputEncoding.EncodingName
# 应该显示: Unicode (UTF-8)

# CMD
chcp
# 应该显示: 活动代码页: 65001
```

### 2. 测试编译
```powershell
# 编译项目
cmake --build build

# 如果成功，应该看到：
# [65/65] Linking CXX executable CodePracticeSystem.exe
```

### 3. 测试运行
```powershell
# 运行程序
.\build\CodePracticeSystem.exe

# 检查界面中文显示是否正常
```

## 常见问题

### Q1: 终端仍然显示乱码
**A:** 确保：
1. 运行了 `chcp 65001`
2. 终端字体支持中文（如：Microsoft YaHei Mono, Consolas）
3. 重启终端/IDE

### Q2: 编译时出现中文字符错误
**A:** 
1. 删除 `build` 和 `.cache` 目录
2. 确认所有源文件都是UTF-8编码
3. 重新运行 `configure_utf8.ps1`

### Q3: VSCode终端编码不对
**A:** 
1. 使用 `.vscode/settings.json.recommended` 中的配置
2. 重启VSCode
3. 在终端中运行 `chcp` 确认是65001

### Q4: 程序运行时中文显示乱码
**A:** 
1. 确保Windows系统区域设置支持UTF-8
2. 检查Qt应用程序是否正确设置了编码：
   ```cpp
   QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
   ```

## 文件清单

- `configure_utf8.ps1` - PowerShell自动配置脚本
- `fix_utf8_encoding.bat` - CMD自动配置脚本
- `.vscode/settings.json.recommended` - VSCode推荐配置
- `CMakeLists.txt` - 已配置UTF-8编译选项

## 快速开始

```powershell
# 1. 运行配置脚本
.\configure_utf8.ps1

# 2. 如果成功，可执行文件在：
.\build\CodePracticeSystem.exe

# 3. 如果失败，检查错误信息并参考本文档
```

## 注意事项

1. **首次配置后需要重启终端/IDE**
2. **所有源文件必须保存为UTF-8编码**
3. **建议使用PowerShell而不是CMD**
4. **编译前确保终端显示 `活动代码页: 65001`**

## 技术支持

如果遇到问题：
1. 检查终端编码：`chcp`
2. 检查CMake配置：`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`
3. 查看编译日志：`cmake --build build > build.log 2>&1`
4. 确认所有文件都是UTF-8编码
