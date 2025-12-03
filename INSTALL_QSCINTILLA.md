# QScintilla 安装指南（Windows + Qt6）

## 方法1：从源码编译（推荐）

### 1. 下载 QScintilla
访问：https://www.riverbankcomputing.com/software/qscintilla/download
下载最新版本（如 QScintilla-2.14.1.tar.gz）

### 2. 解压到临时目录
例如：`C:\Temp\QScintilla-2.14.1`

### 3. 打开 Qt 命令提示符
- 开始菜单搜索 "Qt 6.10.0 (MinGW 64-bit)"
- 或者找到：`C:\Qt\6.10.0\mingw_64\bin\qtenv2.bat`
- 运行后会打开配置好环境的命令行

### 4. 编译 QScintilla
```cmd
cd C:\Temp\QScintilla-2.14.1\src
qmake qscintilla.pro
mingw32-make
mingw32-make install
```

### 5. 验证安装
检查以下文件是否存在：
- `C:\Qt\6.10.0\mingw_64\lib\libqscintilla2_qt6.a`
- `C:\Qt\6.10.0\mingw_64\include\Qsci\qsciscintilla.h`

---

## 方法2：使用预编译版本（如果有）

某些 Qt 发行版可能包含 QScintilla，检查：
```
C:\Qt\6.10.0\mingw_64\lib\qscintilla*
```

---

## 方法3：暂时不安装（先开发其他功能）

项目已配置为可选依赖，如果找不到 QScintilla：
- 会使用 QPlainTextEdit 作为代码编辑器
- 功能略简化，但不影响核心开发
- 后续可随时升级

---

## 安装后的配置

### 更新 CMakeLists.txt
如果 QScintilla 安装在非标准位置，修改：

```cmake
find_library(QSCINTILLA_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6
    PATHS "你的Qt路径/lib"
)
find_path(QSCINTILLA_INCLUDE_DIR Qsci/qsciscintilla.h 
    PATHS "你的Qt路径/include"
)
```

### 恢复 CodeEditor 使用 QScintilla

1. 修改 `src/ui/CodeEditor.h`：
```cpp
#include <Qsci/qsciscintilla.h>
// ...
QsciScintilla *m_editor;
```

2. 修改 `src/ui/CodeEditor.cpp`：
```cpp
#include <Qsci/qscilexercpp.h>

void CodeEditor::setupEditor()
{
    m_editor = new QsciScintilla(this);
    QsciLexerCPP *lexer = new QsciLexerCPP(m_editor);
    m_editor->setLexer(lexer);
    // ... 其他配置
}
```

---

## 常见问题

### Q: 找不到 qmake
**A**: 需要在 Qt 命令提示符中运行，或手动添加到 PATH：
```
set PATH=C:\Qt\6.10.0\mingw_64\bin;%PATH%
```

### Q: 编译错误
**A**: 确保：
1. Qt 版本匹配（Qt6 需要 QScintilla 2.13+）
2. 编译器匹配（MinGW 64-bit）
3. 使用 Qt 命令提示符

### Q: 链接错误
**A**: 检查库文件名，可能是：
- `qscintilla2_qt6`
- `qscintilla2-qt6`
- `qscintilla2qt6`

---

## 快速开始（不安装 QScintilla）

当前项目已配置为可选，可以直接：
1. 打开 Qt Creator
2. 打开 `CMakeLists.txt`
3. 构建运行

代码编辑器会使用 QPlainTextEdit，功能包括：
- ✅ 代码编辑
- ✅ 自动保存
- ✅ 等宽字体
- ❌ 语法高亮（需要 QScintilla）
- ❌ 代码补全（需要 QScintilla）
