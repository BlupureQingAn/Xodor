# Ollama检测诊断指南

## 问题

用户反馈："检测还是有问题，我确实开着，竟然还是检测不到"

## 诊断方法

现在程序已经添加了详细的诊断日志，可以帮助你找出问题所在。

### 如何查看日志

1. **启动程序**
2. **查看控制台输出**（如果从命令行启动）
3. **或者查看日志文件**（如果配置了日志输出）

### 日志内容说明

程序会输出以下详细信息：

```
=== Ollama连接检测开始 ===
检测URL: http://localhost:11434/api/tags
配置的模型: qwen

=== Ollama响应接收 ===
HTTP状态码: 200
错误代码: 0
错误信息: 

Ollama服务响应成功
响应数据长度: XXX 字节
响应内容: {"models":[...]}

=== 模型检测 ===
配置的模型: qwen
检测到的模型数量: 3

开始匹配模型...
  检查模型: qwen2.5:7b
    完整名称: qwen2.5:7b
    基础名称: qwen2.5
    配置基础: qwen
    完全匹配? false
    基础匹配? false
    前缀匹配? true
    标签匹配? false
✅ 找到匹配的模型!
  配置的模型: qwen
  找到的模型: qwen2.5:7b
  匹配方式: 前缀匹配
```

## 常见问题诊断

### 问题1：连接被拒绝

**日志显示**：
```
=== Ollama响应接收 ===
错误代码: 1
错误信息: Connection refused

Ollama连接失败，错误详情:
  URL: http://localhost:11434/api/tags
  错误代码: 1
  错误信息: Connection refused
```

**原因**：Ollama服务未运行

**解决方法**：
1. 打开终端
2. 运行：`ollama serve`
3. 重新启动程序

### 问题2：找不到服务器

**日志显示**：
```
=== Ollama响应接收 ===
错误代码: 3
错误信息: Host not found

Ollama连接失败，错误详情:
  URL: http://localhost:11434/api/tags
  错误代码: 3
  错误信息: Host not found
```

**原因**：Ollama服务地址配置错误

**解决方法**：
1. 检查Ollama是否在本地运行
2. 如果在远程服务器，检查IP地址是否正确
3. 在设置中修改Ollama URL

### 问题3：连接超时

**日志显示**：
```
=== Ollama响应接收 ===
错误代码: 4
错误信息: Operation timed out

Ollama连接失败，错误详情:
  URL: http://localhost:11434/api/tags
  错误代码: 4
  错误信息: Operation timed out
```

**原因**：
- Ollama服务响应缓慢
- 网络不稳定
- 防火墙阻止连接

**解决方法**：
1. 检查Ollama服务是否正常运行
2. 检查防火墙设置
3. 尝试重启Ollama服务

### 问题4：API端点不存在

**日志显示**：
```
=== Ollama响应接收 ===
HTTP状态码: 404
错误代码: 203
错误信息: Not Found

Ollama连接失败，错误详情:
  URL: http://localhost:11434/api/tags
  错误代码: 203
  错误信息: Not Found
```

**原因**：Ollama版本过旧，不支持`/api/tags`端点

**解决方法**：
1. 更新Ollama到最新版本
2. 运行：`ollama --version` 查看版本
3. 访问 https://ollama.ai 下载最新版本

### 问题5：JSON解析失败

**日志显示**：
```
Ollama服务响应成功
响应数据长度: 50 字节
响应内容: <html>Error</html>

JSON解析失败！响应不是有效的JSON
```

**原因**：
- Ollama服务返回了错误页面
- 端口被其他服务占用
- Ollama配置错误

**解决方法**：
1. 检查端口11434是否被其他程序占用
2. 尝试访问 http://localhost:11434/api/tags 查看响应
3. 重启Ollama服务

### 问题6：未安装任何模型

**日志显示**：
```
=== 模型检测 ===
配置的模型: qwen
检测到的模型数量: 0

❌ 未找到匹配的模型
  配置的模型: qwen
```

**原因**：Ollama服务运行正常，但没有安装任何模型

**解决方法**：
1. 下载模型：`ollama pull qwen2.5`
2. 或者：`ollama pull llama2`
3. 查看已安装的模型：`ollama list`

### 问题7：配置的模型不存在

**日志显示**：
```
=== 模型检测 ===
配置的模型: qwen
检测到的模型数量: 2

开始匹配模型...
  检查模型: llama2:13b
    完整名称: llama2:13b
    基础名称: llama2
    配置基础: qwen
    完全匹配? false
    基础匹配? false
    前缀匹配? false
    标签匹配? false
  检查模型: deepseek-coder:6.7b
    完整名称: deepseek-coder:6.7b
    基础名称: deepseek-coder
    配置基础: qwen
    完全匹配? false
    基础匹配? false
    前缀匹配? false
    标签匹配? false

❌ 未找到匹配的模型
  配置的模型: qwen
```

**原因**：配置的模型名称与实际安装的模型不匹配

**解决方法**：
1. 查看已安装的模型：`ollama list`
2. 在程序设置中选择已安装的模型
3. 或者下载配置的模型：`ollama pull qwen2.5`

## 手动测试Ollama连接

### 方法1：使用curl

```bash
curl http://localhost:11434/api/tags
```

**预期输出**：
```json
{
  "models": [
    {
      "name": "qwen2.5:7b",
      "modified_at": "2024-01-01T00:00:00Z",
      "size": 4661211648
    }
  ]
}
```

### 方法2：使用浏览器

访问：http://localhost:11434/api/tags

应该看到JSON格式的模型列表

### 方法3：使用Ollama命令

```bash
# 查看Ollama版本
ollama --version

# 查看已安装的模型
ollama list

# 测试模型
ollama run qwen2.5 "你好"
```

## 收集诊断信息

如果问题仍然存在，请收集以下信息：

### 1. Ollama状态
```bash
ollama --version
ollama list
```

### 2. 网络测试
```bash
curl http://localhost:11434/api/tags
```

### 3. 程序日志
启动程序后，复制控制台输出的所有日志信息，特别是：
- `=== Ollama连接检测开始 ===` 部分
- `=== Ollama响应接收 ===` 部分
- `=== 模型检测 ===` 部分

### 4. 配置信息
- Ollama URL（默认：http://localhost:11434）
- 配置的模型名称
- 是否使用云端API模式

## 常见配置问题

### 配置文件位置

程序的配置文件通常在：
- Windows: `%APPDATA%/CodePracticeSystem/config.json`
- Linux: `~/.config/CodePracticeSystem/config.json`
- macOS: `~/Library/Application Support/CodePracticeSystem/config.json`

### 检查配置

打开配置文件，查看：
```json
{
  "ollamaUrl": "http://localhost:11434",
  "ollamaModel": "qwen",
  "useCloudApi": false
}
```

确保：
1. `ollamaUrl` 正确（默认是 http://localhost:11434）
2. `ollamaModel` 是已安装的模型名称
3. `useCloudApi` 为 false（如果要使用本地Ollama）

## 下一步

1. **启动程序**
2. **查看日志输出**
3. **根据日志信息找到对应的问题**
4. **按照解决方法操作**
5. **如果还是不行，收集诊断信息并反馈**

现在程序会输出非常详细的日志，可以精确定位问题所在！
