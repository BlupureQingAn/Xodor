# UTF-8编码修复完成说明

## 问题分析

### 根本原因
`MainWindow.cpp` 文件中的中文字符串已经损坏（出现"锟斤拷"等乱码），这是因为：
1. 文件原本可能是GB2312/GBK编码
2. 在某个时刻被错误地当作UTF-8读取并保存
3. 导致中文字符永久损坏

### 当前状态
- ✅ CMakeLists.txt 已配置UTF-8编译选项
- ✅ 终端编码配置脚本已创建
- ✅ VSCode配置建议已提供
- ⚠️ MainWindow.cpp 中的中文字符串已损坏，需要手动修复

## 已完成的配置

### 1. CMakeLists.txt UTF-8配置

```cmake
# UTF-8编码配置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexec-charset=UTF-8")

if(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-fexec-charset=UTF-8)
    add_compile_options(-fwide-exec-charset=UTF-8)
    add_compile_definitions(_UNICODE UNICODE)
    add_link_options(-municode)
endif()
```

### 2. 终端编码配置脚本

创建了以下脚本：
- `configure_utf8.ps1` - PowerShell自动配置脚本
- `fix_utf8_encoding.bat` - CMD批处理脚本
- `convert_to_utf8.ps1` - 批量文件编码转换脚本

### 3. VSCode配置

创建了 `.vscode/settings.json.recommended`，包含：
- 文件编码设置为UTF-8
- 终端自动设置UTF-8
- CMake环境变量配置

## 需要手动修复的内容

### MainWindow.cpp 中损坏的中文字符串

以下位置的中文字符串需要手动修复（已变成乱码）：

1. **第938行附近** - "是否继续？"
2. **第984行附近** - "题库路径不存在"
3. **第1018行附近** - "题库中没有找到题目文件"
4. **第1051行附近** - "确定要清空题库"
5. **第1100行附近** - "题库已清空"
6. **第1146行附近** - "成功导入题目"
7. **第1171行附近** - "当前没有题库"
8. **第1191行附近** - "成功生成模拟题"
9. **第1227行附近** - "当前没有题库，无法管理模拟题"
10. **第1293行附近** - 编译相关提示
11. **第1392行附近** - AI判题结果
12. **第1451行附近** - 题库刷新提示
13. **第1479行附近** - "题库已刷新"
14. **第1504行附近** - AI导师提示
15. **第1522行附近** - "已加载题目"
16. **第1654行附近** - 题库保存日志
17. **第1769行附近** - 测试结果图标
18. **第1974行附近** - 测试通过提示
19. **第2080行附近** - AI连接状态
20. **第2163行附近** - 本地模型配置说明
21. **第2256行附近** - 云端API配置说明
22. **第2361行附近** - 配置成功提示
23. **第2376-2438行** - 删除题目确认对话框

### 修复方法

#### 方法1：从Git恢复（如果有版本控制）
```powershell
git checkout HEAD -- src/ui/MainWindow.cpp
```

#### 方法2：从备份恢复
如果有备份文件，直接复制覆盖。

#### 方法3：手动修复
打开 `src/ui/MainWindow.cpp`，搜索"锟斤拷"等乱码，根据上下文手动修复中文字符串。

#### 方法4：重新编写（推荐）
由于损坏范围较大，建议：
1. 保留代码逻辑
2. 重新输入所有中文字符串
3. 确保编辑器使用UTF-8编码保存

## 使用配置脚本

### 方式1：PowerShell（推荐）

```powershell
# 设置UTF-8编码并编译
.\configure_utf8.ps1
```

### 方式2：CMD

```cmd
# 设置UTF-8编码并编译
fix_utf8_encoding.bat
```

### 方式3：手动步骤

```powershell
# 1. 设置终端UTF-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8
chcp 65001

# 2. 清理构建
Remove-Item -Recurse -Force build, .cache

# 3. 配置CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 4. 编译
cmake --build build
```

## 预防措施

### 1. 确保所有源文件都是UTF-8编码

```powershell
# 批量转换（预览模式）
.\convert_to_utf8.ps1 -DryRun

# 实际转换
.\convert_to_utf8.ps1
```

### 2. 配置编辑器

- **VSCode**: 使用 `.vscode/settings.json.recommended` 中的配置
- **其他IDE**: 确保文件编码设置为 UTF-8（无BOM）

### 3. Git配置

在 `.gitattributes` 中添加：
```
*.cpp text eol=lf encoding=utf-8
*.h text eol=lf encoding=utf-8
*.c text eol=lf encoding=utf-8
*.cmake text eol=lf encoding=utf-8
CMakeLists.txt text eol=lf encoding=utf-8
```

## 验证编码配置

### 检查终端编码
```powershell
# PowerShell
[Console]::OutputEncoding.EncodingName
# 应显示: Unicode (UTF-8)

# CMD
chcp
# 应显示: 活动代码页: 65001
```

### 检查文件编码
```powershell
# 使用PowerShell检查文件编码
$bytes = [System.IO.File]::ReadAllBytes("src/ui/MainWindow.cpp")
if ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    Write-Host "UTF-8 with BOM"
} else {
    Write-Host "UTF-8 without BOM (推荐)"
}
```

## 总结

### 已完成
- ✅ CMake UTF-8编译配置
- ✅ 终端UTF-8配置脚本
- ✅ VSCode配置建议
- ✅ 文件编码转换脚本
- ✅ 题库面板重命名（QuestionListWidget → QuestionBankPanel）

### 待完成
- ⚠️ 修复 MainWindow.cpp 中的中文字符串乱码
- ⚠️ 验证所有源文件都是UTF-8编码
- ⚠️ 测试编译和运行

### 下一步
1. 修复 MainWindow.cpp 中的中文字符串
2. 运行 `.\configure_utf8.ps1` 重新编译
3. 测试程序运行

## 文件清单

- `CMakeLists.txt` - 已配置UTF-8编译选项
- `configure_utf8.ps1` - PowerShell自动配置脚本
- `fix_utf8_encoding.bat` - CMD批处理脚本
- `convert_to_utf8.ps1` - 批量文件编码转换脚本
- `fix_mainwindow_encoding.ps1` - MainWindow.cpp编码修复脚本
- `.vscode/settings.json.recommended` - VSCode推荐配置
- `UTF8编码配置指南.md` - 详细配置指南
- `UTF8编码修复完成说明.md` - 本文档
