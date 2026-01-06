@echo off
chcp 65001 >nul
echo ========================================
echo 提交 Xodor 项目到 GitHub
echo ========================================
echo.

echo [步骤 1/6] 检查 Git 是否已安装...
git --version >nul 2>&1
if errorlevel 1 (
    echo ✗ 错误: 未安装 Git
    echo 请从 https://git-scm.com/downloads 下载并安装 Git
    pause
    exit /b 1
)
echo ✓ Git 已安装

echo.
echo [步骤 2/6] 初始化 Git 仓库...
if not exist ".git" (
    git init
    echo ✓ Git 仓库已初始化
) else (
    echo ✓ Git 仓库已存在
)

echo.
echo [步骤 3/6] 添加所有文件...
git add .
if errorlevel 1 (
    echo ✗ 添加文件失败
    pause
    exit /b 1
)
echo ✓ 文件已添加

echo.
echo [步骤 4/6] 提交更改...
git commit -m "feat: 提交挑战赛完整作品，包含核心功能与文档"
if errorlevel 1 (
    echo ✗ 提交失败
    echo 可能原因：没有更改或需要配置 Git 用户信息
    echo.
    echo 配置 Git 用户信息：
    echo   git config --global user.name "你的名字"
    echo   git config --global user.email "你的邮箱"
    pause
    exit /b 1
)
echo ✓ 更改已提交

echo.
echo [步骤 5/6] 设置远程仓库...
git remote remove origin >nul 2>&1
git remote add origin https://github.com/BlupureQingAn/Xodor.git
if errorlevel 1 (
    echo ✗ 设置远程仓库失败
    pause
    exit /b 1
)
echo ✓ 远程仓库已设置: https://github.com/BlupureQingAn/Xodor.git

echo.
echo [步骤 6/6] 推送到 GitHub...
git branch -M main
echo.
echo 正在推送到 GitHub...
echo 如果是首次推送，可能需要输入 GitHub 用户名和密码（或 Personal Access Token）
echo.
git push -u origin main
if errorlevel 1 (
    echo.
    echo ✗ 推送失败
    echo.
    echo 可能原因：
    echo   1. 需要 GitHub 身份验证
    echo   2. 远程仓库不存在或无权限
    echo   3. 网络连接问题
    echo.
    echo 解决方法：
    echo   1. 确保已在 GitHub 创建仓库: https://github.com/BlupureQingAn/Xodor
    echo   2. 使用 Personal Access Token 代替密码
    echo   3. 检查网络连接
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✓ 成功推送到 GitHub！
echo ========================================
echo.
echo 仓库地址: https://github.com/BlupureQingAn/Xodor
echo.
echo 下一步：
echo   1. 访问 GitHub 仓库确认文件已上传
echo   2. 创建 Release 版本（v1.0）
echo   3. 准备提交邮件
echo.
pause
