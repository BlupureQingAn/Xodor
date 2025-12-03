#include "CompilerDetector.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

QStringList CompilerDetector::getSearchPaths()
{
    QStringList paths;
    
    // 常见的编译器名称
    QStringList compilerNames = {"g++", "clang++", "gcc", "clang"};
    
    // 添加 PATH 中的编译器
    paths.append(compilerNames);
    
    // Windows 常见路径
#ifdef Q_OS_WIN
    QStringList commonPaths = {
        "C:/MinGW/bin/g++.exe",
        "C:/MinGW/bin/gcc.exe",
        "C:/Program Files/mingw-w64/bin/g++.exe",
        "C:/Program Files/LLVM/bin/clang++.exe",
        "C:/msys64/mingw64/bin/g++.exe",
        "C:/msys64/ucrt64/bin/g++.exe",
        "F:/Qt/qt/Tools/mingw1310_64/bin/g++.exe",
        "F:/Qt/qt/Tools/mingw1310_64/bin/gcc.exe"
    };
    paths.append(commonPaths);
#endif
    
    // Linux/Mac 常见路径
#ifdef Q_OS_UNIX
    QStringList unixPaths = {
        "/usr/bin/g++",
        "/usr/bin/clang++",
        "/usr/local/bin/g++",
        "/usr/local/bin/clang++"
    };
    paths.append(unixPaths);
#endif
    
    return paths;
}

CompilerInfo CompilerDetector::testCompiler(const QString &path)
{
    CompilerInfo info;
    info.path = path;
    info.isValid = false;
    
    QProcess process;
    process.start(path, {"--version"});
    
    if (!process.waitForFinished(3000)) {
        return info;
    }
    
    if (process.exitCode() != 0) {
        return info;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    if (lines.isEmpty()) {
        return info;
    }
    
    // 解析编译器名称和版本
    QString firstLine = lines.first().toLower();
    
    if (firstLine.contains("gcc") || firstLine.contains("g++")) {
        info.name = "GCC";
    } else if (firstLine.contains("clang")) {
        info.name = "Clang";
    } else if (firstLine.contains("msvc")) {
        info.name = "MSVC";
    } else {
        info.name = "Unknown";
    }
    
    // 提取版本号
    QRegularExpression versionRegex(R"((\d+\.\d+\.\d+))");
    QRegularExpressionMatch match = versionRegex.match(lines.first());
    if (match.hasMatch()) {
        info.version = match.captured(1);
    }
    
    info.isValid = true;
    return info;
}

QList<CompilerInfo> CompilerDetector::detectCompilers()
{
    QList<CompilerInfo> compilers;
    QStringList searchPaths = getSearchPaths();
    QSet<QString> foundPaths; // 避免重复
    
    for (const QString &path : searchPaths) {
        // 检查文件是否存在
        QFileInfo fileInfo(path);
        if (!fileInfo.exists() && !path.contains('/') && !path.contains('\\')) {
            // 可能是 PATH 中的命令，尝试执行
            CompilerInfo info = testCompiler(path);
            if (info.isValid && !foundPaths.contains(info.path)) {
                compilers.append(info);
                foundPaths.insert(info.path);
            }
        } else if (fileInfo.exists() && fileInfo.isExecutable()) {
            QString absolutePath = fileInfo.absoluteFilePath();
            if (!foundPaths.contains(absolutePath)) {
                CompilerInfo info = testCompiler(absolutePath);
                if (info.isValid) {
                    compilers.append(info);
                    foundPaths.insert(absolutePath);
                }
            }
        }
    }
    
    return compilers;
}

CompilerInfo CompilerDetector::detectBestCompiler()
{
    QList<CompilerInfo> compilers = detectCompilers();
    
    if (compilers.isEmpty()) {
        return CompilerInfo();
    }
    
    // 优先选择 GCC
    for (const CompilerInfo &info : compilers) {
        if (info.name == "GCC") {
            return info;
        }
    }
    
    // 其次选择 Clang
    for (const CompilerInfo &info : compilers) {
        if (info.name == "Clang") {
            return info;
        }
    }
    
    // 返回第一个可用的
    return compilers.first();
}

bool CompilerDetector::validateCompiler(const QString &path)
{
    CompilerInfo info = testCompiler(path);
    return info.isValid;
}

QString CompilerDetector::getCompilerVersion(const QString &path)
{
    CompilerInfo info = testCompiler(path);
    return info.version;
}
