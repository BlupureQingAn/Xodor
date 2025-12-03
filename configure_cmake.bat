@echo off
chcp 65001 >nul
echo ========================================
echo 配置 CMake 项目
echo ========================================

REM 设置环境变量
set Qt6_DIR=F:\Qt\qt\6.10.0\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=F:\Qt\qt\6.10.0\mingw_64
set PATH=F:\Qt\qt\6.10.0\mingw_64\bin;F:\Qt\qt\Tools\mingw1310_64\bin;F:\Qt\qt\Tools\CMake_64\bin;F:\Qt\qt\Tools\Ninja;%PATH%

echo.
echo 清理旧构建...
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo 运行 CMake 配置...
cmake -G "Ninja" ^
    -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DQt6_DIR=%Qt6_DIR% ^
    ..

if errorlevel 1 (
    echo.
    echo ========================================
    echo CMake 配置失败！
    echo ========================================
    pause
    exit /b 1
)

echo.
echo ========================================
echo CMake 配置成功！
echo ========================================
echo.
echo 现在可以：
echo 1. 在 Qt Creator 中打开 CMakeLists.txt
echo 2. 或者运行: cmake --build .
echo.
pause
