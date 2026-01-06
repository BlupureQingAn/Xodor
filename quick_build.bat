@echo off
chcp 65001 >nul
echo ========================================
echo 快速编译项目
echo ========================================

REM 设置Qt工具链路径
set PATH=F:\Qt\qt\Tools\CMake_64\bin;F:\Qt\qt\Tools\Ninja;F:\Qt\qt\Tools\mingw1310_64\bin;%PATH%

REM 配置CMake（如果需要）
if not exist build\CMakeCache.txt (
    echo 首次配置CMake...
    cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=F:/Qt/qt/6.10.0/mingw_64
    if errorlevel 1 (
        echo CMake配置失败！
        exit /b 1
    )
)

REM 编译
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
echo.
echo 可执行文件: build\CodePracticeSystem.exe
