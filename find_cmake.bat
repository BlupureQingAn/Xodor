@echo off
echo ========================================
echo 查找CMake安装位置
echo ========================================
echo.

REM 常见的CMake安装位置
set "PATHS=C:\Program Files\CMake\bin;C:\Program Files (x86)\CMake\bin;F:\Qt\qt\Tools\CMake_64\bin;C:\Qt\Tools\CMake_64\bin"

echo 正在搜索常见位置...
echo.

for %%p in ("%PATHS:;=" "%") do (
    if exist %%p\cmake.exe (
        echo [找到] %%p\cmake.exe
        %%p\cmake.exe --version
        echo.
        echo 要将此路径添加到PATH，请运行：
        echo setx PATH "%%PATH%%;%%~p"
        echo.
        echo 或者临时添加（仅当前会话）：
        echo set PATH=%%PATH%%;%%~p
        echo.
        goto :found
    )
)

echo [未找到] 在常见位置未找到cmake.exe
echo.
echo 请检查以下位置：
echo 1. Qt安装目录下的Tools\CMake_64\bin
echo 2. 独立安装的CMake（通常在Program Files）
echo 3. Visual Studio附带的CMake
echo.

:found
echo ========================================
echo 提示：你可以继续使用 build_project.bat 编译
echo ========================================
pause
