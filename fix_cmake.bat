@echo off
chcp 65001 >nul
echo 修复 CMake 配置...

REM 设置环境变量
set Qt6_DIR=F:\Qt\qt\6.10.0\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=F:\Qt\qt\6.10.0\mingw_64
set PATH=F:\Qt\qt\6.10.0\mingw_64\bin;F:\Qt\qt\Tools\mingw1310_64\bin;F:\Qt\qt\Tools\CMake_64\bin;%PATH%

REM 删除旧构建
if exist build rmdir /s /q build
mkdir build
cd build

REM 使用 Ninja 生成器
cmake -G "Ninja" -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% -DCMAKE_BUILD_TYPE=Debug ..

if errorlevel 1 (
    echo CMake 配置失败
    pause
    exit /b 1
)

echo.
echo CMake 配置成功！现在可以在 Qt Creator 中打开项目了。
pause
