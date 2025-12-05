# UTF-8编码配置脚本 (PowerShell版本)
# 用于修复Windows终端、PowerShell和CMake编译的编码问题

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "UTF-8编码配置脚本 (PowerShell)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 设置PowerShell编码为UTF-8
Write-Host "[1/6] 设置PowerShell编码为UTF-8..." -ForegroundColor Yellow
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001 | Out-Null
Write-Host "完成！当前编码: UTF-8" -ForegroundColor Green
Write-Host ""

# 2. 设置环境变量
Write-Host "[2/6] 设置环境变量..." -ForegroundColor Yellow
$env:LANG = "zh_CN.UTF-8"
$env:LC_ALL = "zh_CN.UTF-8"
Write-Host "完成！LANG=$env:LANG" -ForegroundColor Green
Write-Host ""

# 3. 清理旧的构建文件
Write-Host "[3/6] 清理旧的构建文件..." -ForegroundColor Yellow
if (Test-Path "build") {
    Write-Host "删除 build 目录..." -ForegroundColor Gray
    Remove-Item -Recurse -Force "build"
}
if (Test-Path ".cache") {
    Write-Host "删除 .cache 目录..." -ForegroundColor Gray
    Remove-Item -Recurse -Force ".cache"
}
Write-Host "完成！" -ForegroundColor Green
Write-Host ""

# 4. 重新配置CMake
Write-Host "[4/6] 配置CMake项目..." -ForegroundColor Yellow
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误：CMake配置失败！" -ForegroundColor Red
    exit 1
}
Write-Host "完成！" -ForegroundColor Green
Write-Host ""

# 5. 编译项目
Write-Host "[5/6] 编译项目..." -ForegroundColor Yellow
cmake --build build
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误：编译失败！" -ForegroundColor Red
    Write-Host "请检查编译错误信息。" -ForegroundColor Red
    exit 1
}
Write-Host "完成！" -ForegroundColor Green
Write-Host ""

# 6. 显示结果
Write-Host "[6/6] 编译成功！" -ForegroundColor Green
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "可执行文件位置: build\CodePracticeSystem.exe" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "提示：如果要永久设置UTF-8编码，请运行：" -ForegroundColor Yellow
Write-Host "  Set-ItemProperty -Path 'HKCU:\Console' -Name 'CodePage' -Value 65001" -ForegroundColor Gray
Write-Host ""
