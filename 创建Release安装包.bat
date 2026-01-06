@echo off
chcp 65001 >nul
echo ========================================
echo 创建 Xodor v1.0.0 Release 安装包
echo ========================================
echo.

set VERSION=v1.0.0
set PACKAGE_NAME=CodePracticeSystem-%VERSION%-Windows-x64
set DEPLOY_DIR=deploy
set OUTPUT_DIR=release

echo [步骤 1/5] 检查 deploy 目录...
if not exist "%DEPLOY_DIR%" (
    echo ✗ deploy 目录不存在！
    echo 请先运行编译和部署脚本
    pause
    exit /b 1
)
echo ✓ deploy 目录存在

echo.
echo [步骤 2/5] 创建 release 目录...
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
echo ✓ release 目录已创建

echo.
echo [步骤 3/5] 清理旧的安装包...
if exist "%OUTPUT_DIR%\%PACKAGE_NAME%.zip" (
    del "%OUTPUT_DIR%\%PACKAGE_NAME%.zip"
    echo ✓ 已删除旧的安装包
) else (
    echo ✓ 无需清理
)

echo.
echo [步骤 4/5] 打包文件...
echo 正在压缩 deploy 目录...

REM 使用 PowerShell 压缩文件
powershell -Command "Compress-Archive -Path '%DEPLOY_DIR%\*' -DestinationPath '%OUTPUT_DIR%\%PACKAGE_NAME%.zip' -Force"

if %ERRORLEVEL% EQU 0 (
    echo ✓ 打包成功
) else (
    echo ✗ 打包失败
    pause
    exit /b 1
)

echo.
echo [步骤 5/5] 验证安装包...
if exist "%OUTPUT_DIR%\%PACKAGE_NAME%.zip" (
    echo ✓ 安装包已创建
    for %%A in ("%OUTPUT_DIR%\%PACKAGE_NAME%.zip") do (
        echo   文件大小: %%~zA 字节
    )
) else (
    echo ✗ 安装包创建失败
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✓ Release 安装包创建完成！
echo ========================================
echo.
echo 安装包位置：%OUTPUT_DIR%\%PACKAGE_NAME%.zip
echo.
echo 下一步操作：
echo 1. 访问 https://github.com/BlupureQingAn/Xodor/releases
echo 2. 点击 "Create a new release"
echo 3. Tag version: %VERSION%
echo 4. 上传安装包：%PACKAGE_NAME%.zip
echo 5. 发布 Release
echo.
pause
