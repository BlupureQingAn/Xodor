@echo off
chcp 65001 >nul
echo ========================================
echo   创建安装程序（需要 NSIS）
echo ========================================
echo.

REM 检查是否已经打包
if not exist "deploy\CodePracticeSystem.exe" (
    echo 错误: 请先运行 deploy_windows.bat 进行打包
    pause
    exit /b 1
)

REM 检查 NSIS 是否安装
where makensis >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到 NSIS 安装程序制作工具
    echo.
    echo 请先安装 NSIS:
    echo 1. 访问 https://nsis.sourceforge.io/Download
    echo 2. 下载并安装 NSIS
    echo 3. 将 NSIS 安装目录添加到系统 PATH
    echo.
    pause
    exit /b 1
)

echo 正在创建安装程序...
makensis installer.nsi

if errorlevel 1 (
    echo 错误: 创建安装程序失败
    pause
    exit /b 1
)

echo.
echo ========================================
echo   安装程序创建完成！
echo ========================================
echo.
echo 安装程序: CodePracticeSystem-Setup.exe
echo.
pause
