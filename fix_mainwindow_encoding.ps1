# 修复MainWindow.cpp的编码问题
Write-Host "修复 MainWindow.cpp 编码..." -ForegroundColor Yellow

$file = "src/ui/MainWindow.cpp"

# 尝试用GB2312读取
try {
    $gb2312 = [System.Text.Encoding]::GetEncoding("GB2312")
    $content = [System.IO.File]::ReadAllText($file, $gb2312)
    
    # 保存为UTF-8（无BOM）
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($file, $content, $utf8NoBom)
    
    Write-Host "成功！文件已转换为UTF-8编码" -ForegroundColor Green
} catch {
    Write-Host "错误: $_" -ForegroundColor Red
}
