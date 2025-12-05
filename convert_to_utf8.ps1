# 批量转换源文件为UTF-8编码（无BOM）
# 用于修复中文字符编码问题

param(
    [string]$Path = "src",
    [switch]$DryRun = $false
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "批量转换源文件为UTF-8编码" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置UTF-8编码（无BOM）
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# 支持的文件扩展名
$extensions = @("*.cpp", "*.h", "*.c", "*.hpp", "*.cc", "*.cxx")

# 可能的源编码
$sourceEncodings = @(
    [System.Text.Encoding]::GetEncoding("GB2312"),
    [System.Text.Encoding]::GetEncoding("GBK"),
    [System.Text.Encoding]::Default,
    [System.Text.Encoding]::UTF8
)

$script:convertedCount = 0
$script:skippedCount = 0
$script:errorCount = 0

# 递归查找所有源文件
Get-ChildItem -Path $Path -Include $extensions -Recurse -File | ForEach-Object {
    $file = $_
    $relativePath = $file.FullName.Replace((Get-Location).Path + "\", "")
    
    try {
        # 尝试用不同编码读取文件
        $content = $null
        $detectedEncoding = $null
        
        foreach ($enc in $sourceEncodings) {
            try {
                $testContent = [System.IO.File]::ReadAllText($file.FullName, $enc)
                
                # 检查是否包含中文字符
                if ($testContent -match "[\u4e00-\u9fa5]") {
                    $content = $testContent
                    $detectedEncoding = $enc
                    break
                }
                
                # 如果没有中文，使用UTF-8
                if ($enc.EncodingName -eq "Unicode (UTF-8)") {
                    $content = $testContent
                    $detectedEncoding = $enc
                }
            } catch {
                continue
            }
        }
        
        if ($content) {
            # 检查是否需要转换
            $currentBytes = [System.IO.File]::ReadAllBytes($file.FullName)
            $utf8Bytes = $utf8NoBom.GetBytes($content)
            
            $needsConversion = $false
            if ($currentBytes.Length -ne $utf8Bytes.Length) {
                $needsConversion = $true
            } else {
                for ($i = 0; $i -lt $currentBytes.Length; $i++) {
                    if ($currentBytes[$i] -ne $utf8Bytes[$i]) {
                        $needsConversion = $true
                        break
                    }
                }
            }
            
            if ($needsConversion) {
                if ($DryRun) {
                    Write-Host "[DRY-RUN] 将转换: $relativePath" -ForegroundColor Yellow
                    Write-Host "  检测到编码: $($detectedEncoding.EncodingName)" -ForegroundColor Gray
                } else {
                    # 保存为UTF-8（无BOM）
                    [System.IO.File]::WriteAllText($file.FullName, $content, $utf8NoBom)
                    Write-Host "[转换] $relativePath" -ForegroundColor Green
                    Write-Host "  $($detectedEncoding.EncodingName) -> UTF-8 (无BOM)" -ForegroundColor Gray
                }
                $script:convertedCount++
            } else {
                Write-Host "[跳过] $relativePath (已是UTF-8)" -ForegroundColor Gray
                $script:skippedCount++
            }
        } else {
            Write-Host "[错误] 无法读取: $relativePath" -ForegroundColor Red
            $script:errorCount++
        }
    } catch {
        Write-Host "[错误] 处理失败: $relativePath" -ForegroundColor Red
        Write-Host "  $_" -ForegroundColor Red
        $script:errorCount++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "转换完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "转换文件数: $script:convertedCount" -ForegroundColor Green
Write-Host "跳过文件数: $script:skippedCount" -ForegroundColor Yellow
Write-Host "错误文件数: $script:errorCount" -ForegroundColor Red
Write-Host ""

if ($DryRun) {
    Write-Host "这是预览模式。要实际转换文件，请运行：" -ForegroundColor Yellow
    Write-Host "  .\convert_to_utf8.ps1" -ForegroundColor Gray
    Write-Host ""
}
