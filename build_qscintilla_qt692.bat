@echo off
chcp 65001 >nul
echo ========================================
echo 为Qt 6.9.2 MinGW编译QScintilla
echo ========================================

REM 设置Qt环境
set QTDIR=F:\Qt\6.9.2\mingw_64
set PATH=%QTDIR%\bin;F:\Qt\Tools\mingw1310_64\bin;%PATH%

echo [1/5] 检查QScintilla源码...
if not exist "QScintilla_src-2.14.1" (
    echo 错误: 找不到QScintilla源码目录
    echo 请确保QScintilla_src-2.14.1目录存在
    pause
    exit /b 1
)

echo [2/5] 进入源码目录...
cd QScintilla_src-2.14.1\src

echo [3/5] 清理旧的编译文件...
if exist Makefile (
    mingw32-make clean 2>nul
    del Makefile 2>nul
)

echo [4/5] 生成Makefile...
qmake qscintilla.pro

if errorlevel 1 (
    echo 错误: qmake失败
    cd ..\..
    pause
    exit /b 1
)

echo [5/5] 开始编译（这可能需要几分钟）...
mingw32-make -j4

if errorlevel 1 (
    echo 错误: 编译失败
    cd ..\..
    pause
    exit /b 1
)

echo [6/5] 安装到Qt目录...
mingw32-make install

if errorlevel 1 (
    echo 警告: 安装失败，但编译成功
)

cd ..\..

echo.
echo ========================================
echo QScintilla编译完成！
echo ========================================
echo.
echo 检查编译结果：
dir "%QTDIR%\lib\*qscintilla*"
echo.
dir "%QTDIR%\bin\*qscintilla*"
echo.
echo 现在可以重新编译项目了
pause