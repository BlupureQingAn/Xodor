@echo off
chcp 65001 >nul
echo ========================================
echo 编译项目并验证图标
echo ========================================
echo.

echo [步骤 1/4] 检查图标文件...
if not exist "resources\icon.ico" (
    echo ✗ 错误: resources\icon.ico 不存在
    echo.
    echo 正在生成图标...
    py create_icon_pil.py
    if errorlevel 1 (
        echo ✗ 图标生成失败
        pause
        exit /b 1
    )
)
echo ✓ 图标文件存在

echo.
echo [步骤 2/4] 清理并重新编译项目...
cmake --build build\Qt_6_9_2_MinGW_64_bit-Debug --target CodePracticeSystem --clean-first
if errorlevel 1 (
    echo ✗ 编译失败
    pause
    exit /b 1
)
echo ✓ 编译成功

echo.
echo [步骤 3/4] 验证 EXE 文件...
set EXE_PATH=build\Qt_6_9_2_MinGW_64_bit-Debug\CodePracticeSystem.exe
if not exist "%EXE_PATH%" (
    echo ✗ 错误: 找不到 EXE 文件
    pause
    exit /b 1
)
echo ✓ EXE 文件存在: %EXE_PATH%

echo.
echo [步骤 4/4] 打开文件所在目录...
explorer /select,"%EXE_PATH%"

echo.
echo ========================================
echo ✓ 编译完成！
echo ========================================
echo.
echo 请在文件资源管理器中查看 CodePracticeSystem.exe 的图标
echo 应该显示酒红色的 X 与 C 组合图标
echo.
echo 提示：
echo   - 如果图标未显示，请清除 Windows 图标缓存
echo   - 运行程序查看任务栏图标
echo   - 图标已嵌入 EXE，无需单独复制图标文件
echo.
pause
