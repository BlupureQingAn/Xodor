@echo off
chcp 65001 >nul
echo ========================================
echo 部署带图标的应用程序
echo ========================================
echo.

set SOURCE_EXE=build\Qt_6_9_2_MinGW_64_bit-Debug\CodePracticeSystem.exe
set DEPLOY_DIR=deploy
set QT_DIR=F:\Qt\6.9.2\mingw_64

echo [步骤 1/5] 检查源文件...
if not exist "%SOURCE_EXE%" (
    echo ✗ 错误: 找不到编译后的 EXE 文件
    echo 请先编译项目: cmake --build build\Qt_6_9_2_MinGW_64_bit-Debug --target CodePracticeSystem
    pause
    exit /b 1
)
echo ✓ 源文件存在

echo.
echo [步骤 2/5] 创建部署目录...
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
echo ✓ 部署目录已创建: %DEPLOY_DIR%

echo.
echo [步骤 3/5] 复制 EXE 文件（包含图标）...
copy /Y "%SOURCE_EXE%" "%DEPLOY_DIR%\"
if errorlevel 1 (
    echo ✗ 复制失败
    pause
    exit /b 1
)
echo ✓ EXE 文件已复制

echo.
echo [步骤 4/5] 部署 Qt 依赖...
echo 正在运行 windeployqt...
"%QT_DIR%\bin\windeployqt.exe" "%DEPLOY_DIR%\CodePracticeSystem.exe" --no-translations
if errorlevel 1 (
    echo ✗ windeployqt 失败
    pause
    exit /b 1
)
echo ✓ Qt 依赖已部署

echo.
echo [步骤 5/5] 复制 TLS 插件...
if not exist "%DEPLOY_DIR%\tls" mkdir "%DEPLOY_DIR%\tls"
copy /Y "%QT_DIR%\plugins\tls\*.dll" "%DEPLOY_DIR%\tls\" >nul 2>&1
echo ✓ TLS 插件已复制

echo.
echo ========================================
echo ✓ 部署完成！
echo ========================================
echo.
echo 部署位置: %DEPLOY_DIR%\
echo 可执行文件: %DEPLOY_DIR%\CodePracticeSystem.exe
echo.
echo 图标状态:
echo   ✓ 图标已嵌入到 EXE 文件中
echo   ✓ 无需单独复制图标文件
echo   ✓ 在任何位置运行都会显示图标
echo.
echo 正在打开部署目录...
explorer "%DEPLOY_DIR%"

echo.
echo 测试步骤:
echo   1. 在部署目录中查看 CodePracticeSystem.exe 的图标
echo   2. 双击运行程序
echo   3. 查看任务栏图标
echo   4. 应该看到酒红色的 X 与 C 组合图标
echo.
pause
