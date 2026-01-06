@echo off
chcp 65001 >nul
echo ========================================
echo 创建 Xodor Release 压缩包
echo ========================================
echo.

set VERSION=v1.0.0
set RELEASE_NAME=Xodor-%VERSION%-Windows-x64
set RELEASE_DIR=release_temp

echo [步骤 1/5] 清理旧的临时文件...
if exist %RELEASE_DIR% rmdir /s /q %RELEASE_DIR%
if exist %RELEASE_NAME%.zip del /q %RELEASE_NAME%.zip
echo ✓ 清理完成

echo.
echo [步骤 2/5] 创建临时目录...
mkdir %RELEASE_DIR%
echo ✓ 目录创建完成

echo.
echo [步骤 3/5] 复制程序文件...
xcopy /E /I /Y deploy %RELEASE_DIR%\Xodor >nul
echo ✓ 程序文件复制完成

echo.
echo [步骤 4/5] 复制文档文件...
copy /Y README.md %RELEASE_DIR%\ >nul
copy /Y CHANGELOG.md %RELEASE_DIR%\ >nul
copy /Y QUICK_START.md %RELEASE_DIR%\ >nul
copy /Y USAGE.md %RELEASE_DIR%\ >nul
copy /Y TROUBLESHOOTING.md %RELEASE_DIR%\ >nul
copy /Y DEPLOYMENT_GUIDE.md %RELEASE_DIR%\ >nul
copy /Y 评委测试指南.md %RELEASE_DIR%\ >nul
echo ✓ 文档文件复制完成

echo.
echo [步骤 5/5] 创建压缩包...
powershell -Command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%RELEASE_NAME%.zip' -Force"
if %errorlevel% equ 0 (
    echo ✓ 压缩包创建成功
) else (
    echo ✗ 压缩包创建失败
    pause
    exit /b 1
)

echo.
echo [清理] 删除临时目录...
rmdir /s /q %RELEASE_DIR%
echo ✓ 清理完成

echo.
echo ========================================
echo ✓ Release 创建完成！
echo ========================================
echo.
echo 文件名: %RELEASE_NAME%.zip
echo 位置: %CD%\%RELEASE_NAME%.zip
echo.

REM 显示文件大小
for %%A in (%RELEASE_NAME%.zip) do (
    set size=%%~zA
    set /a sizeMB=!size! / 1048576
    echo 大小: !sizeMB! MB
)

echo.
echo 接下来的步骤：
echo 1. 访问 https://github.com/BlupureQingAn/Xodor/releases
echo 2. 点击 "Create a new release"
echo 3. 上传 %RELEASE_NAME%.zip
echo 4. 填写 Release 信息
echo.
pause
