@echo off
REM UTF-8编码配置脚本
REM 用于修复Windows终端、PowerShell和CMake编译的编码问题

echo ========================================
echo UTF-8编码配置脚本
echo ========================================
echo.

REM 1. 设置当前会话的代码页为UTF-8
echo [1/5] 设置终端代码页为UTF-8 (65001)...
chcp 65001 >nul
echo 完成！当前代码页: 65001 (UTF-8)
echo.

REM 2. 清理旧的构建文件
echo [2/5] 清理旧的构建文件...
if exist build (
    echo 删除 build 目录...
    rmdir /s /q build
)
if exist .cache (
    echo 删除 .cache 目录...
    rmdir /s /q .cache
)
echo 完成！
echo.

REM 3. 重新配置CMake
echo [3/5] 配置CMake项目...
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 (
    echo 错误：CMake配置失败！
    pause
    exit /b 1
)
echo 完成！
echo.

REM 4. 编译项目
echo [4/5] 编译项目...
cmake --build build
if errorlevel 1 (
    echo 错误：编译失败！
    echo 请检查编译错误信息。
    pause
    exit /b 1
)
echo 完成！
echo.

REM 5. 显示结果
echo [5/5] 编译成功！
echo.
echo ========================================
echo 可执行文件位置: build\CodePracticeSystem.exe
echo ========================================
echo.

pause
