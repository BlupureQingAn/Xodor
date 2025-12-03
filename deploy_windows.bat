@echo off
chcp 65001 >nul
echo ========================================
echo   代码刷题系统 - Windows 打包脚本
echo ========================================
echo.

REM 设置Qt路径
set QT_PATH=F:\Qt\qt\6.10.0\mingw_64
set QT_BIN=%QT_PATH%\bin
set MINGW_BIN=F:\Qt\qt\Tools\mingw1310_64\bin

REM 设置项目路径
set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set DEPLOY_DIR=%PROJECT_DIR%deploy
set APP_NAME=CodePracticeSystem

echo [1/6] 检查构建文件...
if not exist "%BUILD_DIR%\%APP_NAME%.exe" (
    echo 错误: 找不到可执行文件 %BUILD_DIR%\%APP_NAME%.exe
    echo 请先运行 build_project.bat 构建项目
    pause
    exit /b 1
)

echo [2/6] 创建部署目录...
if exist "%DEPLOY_DIR%" (
    echo 清理旧的部署目录...
    rmdir /s /q "%DEPLOY_DIR%"
)
mkdir "%DEPLOY_DIR%"

echo [3/6] 复制可执行文件...
copy "%BUILD_DIR%\%APP_NAME%.exe" "%DEPLOY_DIR%\" >nul
if errorlevel 1 (
    echo 错误: 复制可执行文件失败
    pause
    exit /b 1
)

echo [4/6] 使用 windeployqt 部署 Qt 依赖...
cd /d "%DEPLOY_DIR%"
"%QT_BIN%\windeployqt.exe" --release --no-translations "%APP_NAME%.exe"
if errorlevel 1 (
    echo 警告: windeployqt 执行失败，尝试手动复制依赖...
)

echo [5/6] 复制额外依赖库...
REM 复制 MinGW 运行时库
copy "%MINGW_BIN%\libgcc_s_seh-1.dll" "%DEPLOY_DIR%\" >nul 2>&1
copy "%MINGW_BIN%\libstdc++-6.dll" "%DEPLOY_DIR%\" >nul 2>&1
copy "%MINGW_BIN%\libwinpthread-1.dll" "%DEPLOY_DIR%\" >nul 2>&1

REM 复制 QScintilla 库
copy "%QT_PATH%\bin\qscintilla2_qt6.dll" "%DEPLOY_DIR%\" >nul 2>&1

echo [6/6] 复制数据文件...
REM 创建数据目录结构
mkdir "%DEPLOY_DIR%\data" 2>nul
mkdir "%DEPLOY_DIR%\data\questions" 2>nul
mkdir "%DEPLOY_DIR%\data\question_banks" 2>nul
mkdir "%DEPLOY_DIR%\data\user_answers" 2>nul
mkdir "%DEPLOY_DIR%\data\history" 2>nul
mkdir "%DEPLOY_DIR%\data\wrong_questions" 2>nul
mkdir "%DEPLOY_DIR%\data\sessions" 2>nul
mkdir "%DEPLOY_DIR%\data\code_versions" 2>nul

REM 复制配置文件
if exist "%PROJECT_DIR%\data\config.json" (
    copy "%PROJECT_DIR%\data\config.json" "%DEPLOY_DIR%\data\" >nul
) else (
    echo 创建默认配置文件...
    echo {"compilerPath":"g++","ollamaUrl":"http://localhost:11434","ollamaModel":"qwen","cloudApiKey":"","useCloudApi":false} > "%DEPLOY_DIR%\data\config.json"
)

REM 复制示例题库（如果存在）
if exist "%PROJECT_DIR%\data\sample_questions" (
    echo 复制示例题库...
    xcopy /E /I /Y "%PROJECT_DIR%\data\sample_questions" "%DEPLOY_DIR%\data\sample_questions" >nul
)

REM 复制文档
echo 复制文档文件...
copy "%PROJECT_DIR%\README.md" "%DEPLOY_DIR%\" >nul 2>&1
copy "%PROJECT_DIR%\QUICK_START.md" "%DEPLOY_DIR%\" >nul 2>&1
copy "%PROJECT_DIR%\LICENSE" "%DEPLOY_DIR%\" >nul 2>&1

cd /d "%PROJECT_DIR%"

echo.
echo ========================================
echo   打包完成！
echo ========================================
echo.
echo 部署目录: %DEPLOY_DIR%
echo 可执行文件: %DEPLOY_DIR%\%APP_NAME%.exe
echo.
echo 提示:
echo 1. 可以直接运行 %APP_NAME%.exe
echo 2. 整个 deploy 文件夹可以打包分发
echo 3. 用户需要安装 C++ 编译器（如 MinGW）才能编译代码
echo 4. 如需使用 AI 功能，需要安装 Ollama 或配置云端 API
echo.
pause
