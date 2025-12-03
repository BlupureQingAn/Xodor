@echo off
chcp 65001 >nul
echo ========================================
echo 构建项目
echo ========================================

set PATH=F:\Qt\qt\Tools\CMake_64\bin;F:\Qt\qt\Tools\Ninja;F:\Qt\qt\Tools\mingw1310_64\bin;%PATH%

cd build
cmake --build .

if errorlevel 1 (
    echo.
    echo 构建失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 构建成功！
echo ========================================
echo.
echo 可执行文件位置: build\CodePracticeSystem.exe
pause
