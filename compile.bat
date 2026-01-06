@echo off
chcp 65001 >nul

echo ========================================
echo 编译项目 (UTF-8)
echo ========================================

REM 设置Qt工具链 - 使用Qt 6.9.2
set PATH=F:\Qt\Tools\CMake_64\bin;F:\Qt\Tools\Ninja;F:\Qt\Tools\mingw1310_64\bin;F:\Qt\6.9.2\mingw_64\bin;%PATH%
set CMAKE_PREFIX_PATH=F:\Qt\6.9.2\mingw_64

REM 设置编译器环境变量解决Qt 6.9.2兼容性问题
set CXXFLAGS=-DQT_NO_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x060000
set CMAKE_CXX_FLAGS=-DQT_NO_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x060000

REM 清理旧的build（如果可以）
echo 清理旧的构建...
if exist build\CMakeCache.txt del /f /q build\CMakeCache.txt 2>nul

REM 配置CMake - 添加Qt 6.9.2兼容性标志
echo 配置CMake...
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 -DQT_NO_DEPRECATED_WARNINGS=ON

if errorlevel 1 (
    echo CMake配置失败！
    exit /b 1
)

REM 编译
echo.
echo 开始编译...
cmake --build build --config Release

if errorlevel 1 (
    echo.
    echo ========================================
    echo 编译失败！
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo ========================================
echo 可执行文件: build\CodePracticeSystem.exe
