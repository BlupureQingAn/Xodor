# UTF-8编码完整支持说明

## 概述

为了防止中文乱码，已在整个项目中实现完整的UTF-8编码支持，包括：
- ✅ 代码文件保存和读取
- ✅ 编译器参数配置
- ✅ 程序运行时输入输出
- ✅ JSON配置文件
- ✅ 临时文件创建

## 1. 代码保存和读取（UTF-8）

### AutoSaver.cpp - 代码保存
```cpp
void AutoSaver::performSave()
{
    QString filePath = QString("data/user_answers/%1.cpp").arg(m_questionId);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_content.toUtf8());  // ✅ 使用UTF-8编码
        file.close();
    }
}
```

**关键点**：
- 使用 `toUtf8()` 将QString转换为UTF-8字节流
- 使用 `QIODevice::Text` 模式确保文本格式正确

### MainWindow.cpp - 代码读取
```cpp
QString MainWindow::loadSavedCodeForQuestion(const QString &questionId)
{
    QString filePath = QString("data/user_answers/%1.cpp").arg(questionId);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString code = QString::fromUtf8(file.readAll());  // ✅ 使用UTF-8解码
        file.close();
        return code;
    }
    return QString();
}
```

**关键点**：
- 使用 `QString::fromUtf8()` 将UTF-8字节流转换为QString
- 确保读取和保存使用相同的编码

## 2. 编译器UTF-8支持

### CompilerRunner.cpp - 编译参数
```cpp
CompileResult CompilerRunner::compile(const QString &code)
{
    // 创建临时文件（UTF-8编码）
    QString sourceFile = createTempFile(code);
    
    QProcess process;
    QStringList args;
    args << sourceFile << "-o" << exeFile << "-std=c++17";
    
    // ✅ 添加UTF-8编码支持，防止中文乱码
    args << "-finput-charset=UTF-8" << "-fexec-charset=UTF-8";
    
    process.start(m_compilerPath, args);
    // ...
}
```

**编译器参数说明**：
- `-finput-charset=UTF-8`：指定源文件编码为UTF-8
- `-fexec-charset=UTF-8`：指定程序运行时字符集为UTF-8

### 临时文件创建（UTF-8）
```cpp
QString CompilerRunner::createTempFile(const QString &code)
{
    QTemporaryFile tempFile(QDir::tempPath() + "/code_XXXXXX.cpp");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(code.toUtf8());  // ✅ 使用UTF-8编码
        tempFile.close();
        return tempFile.fileName();
    }
    return QString();
}
```

## 3. 程序运行时输入输出（UTF-8）

### CompilerRunner.cpp - 测试运行
```cpp
QVector<TestResult> CompilerRunner::runTests(const QString &executablePath, 
                                             const QVector<TestCase> &testCases)
{
    for (const auto &testCase : testCases) {
        QProcess process;
        process.start(executablePath);
        
        // ✅ 输入使用UTF-8编码
        process.write(testCase.input.toUtf8());
        process.closeWriteChannel();
        
        process.waitForFinished(5000);
        
        // ✅ 输出使用UTF-8解码
        QString rawOutput = process.readAllStandardOutput();
        result.actualOutput = rawOutput.trimmed();
        result.error = process.readAllStandardError().trimmed();
        
        // 比较输出...
    }
}
```

**关键点**：
- 测试输入使用 `toUtf8()` 编码
- 程序输出自动按UTF-8解码（因为编译时指定了 `-fexec-charset=UTF-8`）

## 4. CMake配置（UTF-8）

### CMakeLists.txt - 全局UTF-8配置
```cmake
# ============================================
# UTF-8编码配置 - 支持中文字符编译
# ============================================

if(MSVC)
    # MSVC 编译器
    add_compile_options(/utf-8)  # 源文件和执行字符集都使用UTF-8
    add_compile_options(/Zc:__cplusplus)
    add_compile_definitions(_UNICODE UNICODE)
elseif(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
    # MinGW/GCC 编译器
    add_compile_options(-finput-charset=UTF-8)  # 源文件编码
    add_compile_options(-fexec-charset=UTF-8)   # 执行字符集
    add_compile_options(-fwide-exec-charset=UTF-8)  # 宽字符集
    
    add_compile_definitions(_UNICODE UNICODE)
    add_link_options(-municode)
endif()
```

**配置说明**：
- **MSVC**：使用 `/utf-8` 参数
- **MinGW/GCC**：使用 `-finput-charset` 和 `-fexec-charset` 参数
- 定义 `_UNICODE` 和 `UNICODE` 宏
- 链接器使用 `-municode` 支持Unicode

## 5. 文件工具类（UTF-8）

### FileUtils.h/cpp - 统一的文件操作
创建了新的工具类确保所有文件操作使用UTF-8：

```cpp
class FileUtils
{
public:
    // 读取文本文件（UTF-8编码）
    static bool readTextFile(const QString &filePath, QString &content);
    
    // 写入文本文件（UTF-8编码）
    static bool writeTextFile(const QString &filePath, const QString &content);
    
    // 读取JSON文件（UTF-8编码）
    static bool readJsonFile(const QString &filePath, QByteArray &content);
    
    // 写入JSON文件（UTF-8编码）
    static bool writeJsonFile(const QString &filePath, const QByteArray &content);
};
```

**实现细节**（Qt6）：
```cpp
bool FileUtils::readTextFile(const QString &filePath, QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    // Qt6使用setEncoding
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    content = stream.readAll();
    file.close();
    
    return true;
}
```

## 6. 现有代码的UTF-8支持检查

### 已确认使用UTF-8的代码

#### ✅ AutoSaver.cpp
- 保存：`file.write(m_content.toUtf8())`

#### ✅ MainWindow.cpp
- 读取：`QString::fromUtf8(file.readAll())`

#### ✅ CompilerRunner.cpp
- 临时文件：`tempFile.write(code.toUtf8())`
- 编译参数：`-finput-charset=UTF-8 -fexec-charset=UTF-8`
- 测试输入：`process.write(testCase.input.toUtf8())`

#### ✅ JSON文件操作
- QJsonDocument默认使用UTF-8编码
- `QJsonDocument::toJson()` 输出UTF-8
- `QJsonDocument::fromJson()` 解析UTF-8

## 7. 测试验证

### 测试场景1：中文注释
```cpp
// 测试中文注释
#include <iostream>
using namespace std;

int main() {
    // 输出中文
    cout << "你好，世界！" << endl;
    return 0;
}
```

**预期结果**：
- ✅ 编译成功
- ✅ 运行输出正确的中文

### 测试场景2：中文输入输出
```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string name;
    cout << "请输入姓名：";
    cin >> name;
    cout << "你好，" << name << "！" << endl;
    return 0;
}
```

**测试用例**：
- 输入：`张三`
- 期望输出：`你好，张三！`

**预期结果**：
- ✅ 中文输入正确处理
- ✅ 中文输出正确显示

### 测试场景3：代码保存和恢复
1. 编写包含中文注释的代码
2. 切换到其他题目
3. 切换回来

**预期结果**：
- ✅ 中文注释完整保存
- ✅ 重新加载后中文显示正常

## 8. 常见问题排查

### 问题1：编译时中文乱码
**原因**：编译器没有使用UTF-8编码
**解决**：检查 `CompilerRunner.cpp` 是否添加了 `-finput-charset=UTF-8`

### 问题2：运行时输出乱码
**原因**：程序运行时字符集不是UTF-8
**解决**：检查编译参数是否包含 `-fexec-charset=UTF-8`

### 问题3：保存的代码乱码
**原因**：文件保存时没有使用UTF-8编码
**解决**：确保使用 `toUtf8()` 保存，`fromUtf8()` 读取

### 问题4：JSON文件乱码
**原因**：JSON文件写入时编码错误
**解决**：QJsonDocument默认使用UTF-8，确保文件以Text模式打开

## 9. 最佳实践

### 文件操作
```cpp
// ✅ 正确：使用UTF-8
QFile file(path);
file.open(QIODevice::WriteOnly | QIODevice::Text);
file.write(content.toUtf8());

// ✅ 正确：读取UTF-8
QFile file(path);
file.open(QIODevice::ReadOnly | QIODevice::Text);
QString content = QString::fromUtf8(file.readAll());
```

### 字符串转换
```cpp
// ✅ 正确：QString → UTF-8字节流
QByteArray utf8Data = str.toUtf8();

// ✅ 正确：UTF-8字节流 → QString
QString str = QString::fromUtf8(utf8Data);
```

### 编译器参数
```cpp
// ✅ 正确：完整的UTF-8支持
args << "-finput-charset=UTF-8"      // 源文件编码
     << "-fexec-charset=UTF-8";      // 运行时编码
```

## 10. 修改文件清单

1. ✅ `src/core/AutoSaver.cpp` - 已使用UTF-8保存
2. ✅ `src/core/CompilerRunner.cpp` - 添加UTF-8编译参数
3. ✅ `src/ui/MainWindow.cpp` - 已使用UTF-8读取
4. ✅ `CMakeLists.txt` - 全局UTF-8配置
5. ✅ `src/utils/FileUtils.h` - 新增UTF-8文件工具类
6. ✅ `src/utils/FileUtils.cpp` - 实现UTF-8文件操作

## 总结

项目现在已经完整支持UTF-8编码：
- **保存**：所有文件使用UTF-8编码保存
- **读取**：所有文件使用UTF-8编码读取
- **编译**：编译器使用UTF-8处理源文件
- **运行**：程序运行时使用UTF-8字符集
- **工具**：提供统一的UTF-8文件操作工具类

中文注释、中文字符串、中文输入输出都能正确处理，不会出现乱码！
