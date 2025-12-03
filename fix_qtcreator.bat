@echo off
chcp 65001 >nul
echo ========================================
echo 修复 Qt Creator 构建问题
echo ========================================

echo.
echo 步骤 1: 清理所有构建文件...
if exist build rmdir /s /q build
if exist build-* rmdir /s /q build-*
if exist .qtc_clangd rmdir /s /q .qtc_clangd
if exist CMakeCache.txt del /f /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist compile_commands.json del /f /q compile_commands.json
if exist test_moc.cpp del /f /q test_moc.cpp

echo.
echo 步骤 2: 重新配置 CMake...
set Qt6_DIR=F:\Qt\qt\6.10.0\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=F:\Qt\qt\6.10.0\mingw_64
set PATH=F:\Qt\qt\6.10.0\mingw_64\bin;F:\Qt\qt\Tools\mingw1310_64\bin;F:\Qt\qt\Tools\CMake_64\bin;F:\Qt\qt\Tools\Ninja;%PATH%

mkdir build
cd build

cmake -G "Ninja" ^
    -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DQt6_DIR=%Qt6_DIR% ^
    ..

if errorlevel 1 (
    echo.
    echo CMake 配置失败！
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo 修复完成！
echo ========================================
echo.
echo 现在请在 Qt Creator 中：
echo 1. 文件 ^> 关闭所有项目和编辑器
echo 2. 文件 ^> 打开文件或项目
echo 3. 选择 CMakeLists.txt
echo 4. 在配置页面，选择 "Desktop Qt 6.10.0 MinGW 64-bit"
echo 5. 在 "Initial Configuration" 中，确保：
echo    - CMAKE_PREFIX_PATH = F:/Qt/qt/6.10.0/mingw_64
echo    - CMAKE_BUILD_TYPE = Debug
echo 6. 点击 "Configure Project"
echo 7. 构建 ^> 重新构建项目
echo.
pause
