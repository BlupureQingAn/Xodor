@echo off
REM 设置控制台为 UTF-8 编码
chcp 65001 >nul

echo ========================================
echo 使用 UTF-8 编码构建项目
echo ========================================

REM 设置环境变量
set Qt6_DIR=F:\Qt\qt\6.10.0\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=F:\Qt\qt\6.10.0\mingw_64
set PATH=F:\Qt\qt\6.10.0\mingw_64\bin;F:\Qt\qt\Tools\mingw1310_64\bin;F:\Qt\qt\Tools\CMake_64\bin;F:\Qt\qt\Tools\Ninja;%PATH%

REM 设置编译器环境变量以支持 UTF-8
set LANG=zh_CN.UTF-8
set LC_ALL=zh_CN.UTF-8

echo.
echo 当前编码设置:
echo LANG=%LANG%
echo LC_ALL=%LC_ALL%

echo.
echo 检查构建目录...
if not exist build (
    echo 构建目录不存在，正在创建...
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
        echo CMake 配置失败！
        cd ..
        exit /b 1
    )
    cd ..
)

echo.
echo 开始构建...
cd build
cmake --build . --verbose

if errorlevel 1 (
    echo.
    echo ========================================
    echo 构建失败！
    echo ========================================
    cd ..
    exit /b 1
)

cd ..

echo.
echo ========================================
echo 构建成功！
echo ========================================
echo.
echo 可执行文件: build\CodePracticeSystem.exe
