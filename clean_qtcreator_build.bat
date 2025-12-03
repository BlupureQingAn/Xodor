@echo off
chcp 65001 >nul
echo ========================================
echo 清理 Qt Creator 构建缓存
echo ========================================

echo.
echo 正在删除构建目录...
if exist build rmdir /s /q build
if exist build-* rmdir /s /q build-*

echo.
echo 正在删除 CMake 缓存...
if exist CMakeCache.txt del /f /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist .cmake rmdir /s /q .cmake

echo.
echo 正在删除 Qt Creator 生成的文件...
if exist .qtc_clangd rmdir /s /q .qtc_clangd
if exist compile_commands.json del /f /q compile_commands.json

echo.
echo ========================================
echo 清理完成！
echo ========================================
echo.
echo 请在 Qt Creator 中：
echo 1. 关闭项目
echo 2. 重新打开 CMakeLists.txt
echo 3. 配置项目（选择 Qt 6.10.0 MinGW 64-bit）
echo 4. 构建项目
echo.
pause
