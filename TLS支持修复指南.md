# Qt TLS支持修复指南

## 问题描述

程序运行时出现以下错误：
```
qt.network.ssl: No functional TLS backend was found
qt.network.ssl: QSslSocket::connectToHostEncrypted: TLS initialization failed
```

这是因为Qt 6.9.2 MinGW版本不包含OpenSSL库，无法进行HTTPS请求（如访问DeepSeek API）。

## 解决方案

### 方法1：自动安装（推荐）

1. 下载OpenSSL安装程序：
   - 访问：https://slproweb.com/products/Win32OpenSSL.html
   - 下载 **Win64 OpenSSL v3.4.0 Light** (约5MB)
   - 直接下载链接：https://slproweb.com/download/Win64OpenSSL_Light-3_4_0.exe

2. 安装OpenSSL：
   - 双击运行安装程序
   - 安装到默认位置：`C:\Program Files\OpenSSL-Win64`
   - 完成安装

3. 运行修复脚本：
   ```batch
   fix_tls_support.bat
   ```

### 方法2：手动复制DLL文件

如果已经安装了OpenSSL，可以手动复制DLL文件：

1. 从OpenSSL安装目录复制以下文件：
   ```
   C:\Program Files\OpenSSL-Win64\bin\libssl-3-x64.dll
   C:\Program Files\OpenSSL-Win64\bin\libcrypto-3-x64.dll
   ```

2. 复制到以下目录：
   - `F:\Xodor\deploy\`
   - `F:\Xodor\build\Qt_6_9_2_MinGW_64_bit-Debug\`

### 方法3：从其他来源获取DLL

如果无法访问上述网站，可以从以下备用源下载：

1. **从Qt官方镜像**：
   - 访问：https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/tools_openssl_x64/
   - 下载最新的OpenSSL包

2. **从GitHub**：
   - 访问：https://github.com/IndySockets/OpenSSL-Binaries
   - 下载 `openssl-3.0/x64/` 目录下的DLL文件

## 验证修复

修复完成后，重新运行程序，应该看到：

```
[OllamaClient] 云端API模式 - 发送请求到: "https://api.deepseek.com/v1/chat/completions"
[OllamaClient] 请求已发送，等待响应...
```

不再出现 "TLS initialization failed" 错误。

## 需要的文件

- `libssl-3-x64.dll` (约600KB)
- `libcrypto-3-x64.dll` (约5MB)

## 注意事项

1. 必须使用 **x64 (64位)** 版本的OpenSSL
2. 必须使用 **OpenSSL 3.x** 版本（不是1.1.x）
3. DLL文件必须与程序exe文件在同一目录

## 故障排除

### 问题：仍然提示TLS错误

**解决方案**：
1. 确认DLL文件已正确复制到exe所在目录
2. 检查DLL文件大小是否正确（不是0字节）
3. 重启Qt Creator和程序

### 问题：找不到OpenSSL安装目录

**解决方案**：
1. 重新安装OpenSSL，确保安装到默认位置
2. 或者手动指定安装路径，修改 `fix_tls_support.bat` 中的路径

### 问题：程序启动时提示缺少DLL

**解决方案**：
1. 运行 `deploy_debug_manual.bat` 重新部署所有依赖
2. 确保OpenSSL DLL在程序目录中

## 相关文件

- `fix_tls_support.bat` - 自动修复脚本
- `install_openssl.ps1` - PowerShell自动下载脚本（需要网络）
- `deploy_debug_manual.bat` - 部署所有Qt依赖

## 技术说明

Qt 6.x在Windows上支持以下TLS后端：
- **OpenSSL 3.x** (推荐) - 需要手动安装
- **Schannel** - Windows原生，但Qt 6.9.2 MinGW版本未启用

MinGW编译的Qt默认使用OpenSSL后端，因此需要手动提供OpenSSL DLL文件。
