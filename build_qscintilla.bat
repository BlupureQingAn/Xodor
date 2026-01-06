@echo off
chcp 65001 >nul
echo ========================================
echo 编译 QScintilla for Qt 6.9.2 MinGW
echo ========================================

REM 设置 Qt 环境
set QTDIR=F:\Qt\6.9.2\mingw_64
set PATH=%QTDIR%\bin;%PATH%
set PATH=F:\Qt\Tools\mingw1310_64\bin;%PATH%

echo.
echo 检查 qmake...
qmake --version
if errorlevel 1 (
    echo 错误: 找不到 qmake
    echo 请检查 Qt 安装路径
    pause
    exit /b 1
)

echo.
echo 进入 QScintilla 源码目录...
cd QScintilla_src-2.14.1\src

echo.
echo 生成 Makefile...
qmake qscintilla.pro

echo.
echo 开始编译（这可能需要几分钟）...
mingw32-make -j4

echo.
echo 安装到 Qt 目录...
mingw32-make install

echo.
echo ========================================
echo 编译完成！
echo ========================================
echo.
echo 检查安装结果：
dir %QTDIR%\lib\*qscintilla*
echo.
dir %QTDIR%\include\Qsci\qsciscintilla.h

pause
