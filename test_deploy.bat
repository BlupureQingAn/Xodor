@echo off
chcp 65001 >nul
echo ========================================
echo   测试打包程序
echo ========================================
echo.

cd /d "%~dp0deploy"

echo 检查必需文件...
echo.

set MISSING=0

if not exist "CodePracticeSystem.exe" (
    echo ❌ 缺少: CodePracticeSystem.exe
    set MISSING=1
) else (
    echo ✅ CodePracticeSystem.exe
)

if not exist "qscintilla2_qt6.dll" (
    echo ❌ 缺少: qscintilla2_qt6.dll
    set MISSING=1
) else (
    echo ✅ qscintilla2_qt6.dll
)

if not exist "Qt6Core.dll" (
    echo ❌ 缺少: Qt6Core.dll
    set MISSING=1
) else (
    echo ✅ Qt6Core.dll
)

if not exist "Qt6Widgets.dll" (
    echo ❌ 缺少: Qt6Widgets.dll
    set MISSING=1
) else (
    echo ✅ Qt6Widgets.dll
)

if not exist "platforms\qwindows.dll" (
    echo ❌ 缺少: platforms\qwindows.dll
    set MISSING=1
) else (
    echo ✅ platforms\qwindows.dll
)

if not exist "data\config.json" (
    echo ❌ 缺少: data\config.json
    set MISSING=1
) else (
    echo ✅ data\config.json
)

echo.
if %MISSING%==0 (
    echo ========================================
    echo   ✅ 所有必需文件都存在！
    echo ========================================
    echo.
    echo 正在启动程序...
    start "" "CodePracticeSystem.exe"
) else (
    echo ========================================
    echo   ❌ 缺少必需文件，请重新打包
    echo ========================================
)

echo.
pause
