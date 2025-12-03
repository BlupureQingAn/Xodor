# 调试指南 - 代码保存和错误检测问题

## 🐛 问题诊断

已添加详细的调试日志，帮助诊断以下问题：
1. 代码没有保存
2. 错误列表没有检测到错误

## 📝 调试日志说明

### 1. 代码编辑器日志
```
[CodeEditor] Text changed, code length: XXX
[CodeEditor] Triggering syntax check...
[CodeEditor] Code is empty, clearing errors
[CodeEditor] WARNING: Syntax checker is null!
```

### 2. 自动保存日志
```
[AutoSaver] WARNING: questionId is empty, cannot save!
[AutoSaver] Saved code to: data/user_answers/XXX.json length: XXX
[AutoSaver] ERROR: Failed to open file for writing: XXX
```

### 3. 语法检查器日志
```
[SyntaxChecker] Performing syntax check with compiler: g++
[SyntaxChecker] Temp file created: XXX
[SyntaxChecker] Process finished, exit code: X status: X
[SyntaxChecker] Compiler output: XXX
[SyntaxChecker] Found X errors/warnings
[SyntaxChecker] Skipping check: code or compiler empty
[SyntaxChecker] Skipping check: code unchanged
```

## 🔍 问题排查步骤

### 问题1：代码没有保存

#### 检查点1：QuestionId是否设置
**查看日志：**
```
[AutoSaver] WARNING: questionId is empty, cannot save!
```

**解决方法：**
1. 确保已经选择了题目
2. 检查题目是否有有效的ID
3. 查看MainWindow::loadCurrentQuestion()是否被调用

#### 检查点2：文件写入权限
**查看日志：**
```
[AutoSaver] ERROR: Failed to open file for writing: XXX
```

**解决方法：**
1. 检查data/user_answers目录是否存在
2. 检查文件写入权限
3. 确保程序有创建目录的权限

#### 检查点3：保存是否触发
**查看日志：**
```
[CodeEditor] Text changed, code length: XXX
```

**解决方法：**
1. 确认文本变化事件被触发
2. 检查AutoSaver的定时器是否工作
3. 等待200ms后查看是否有保存日志

### 问题2：错误列表没有检测到错误

#### 检查点1：语法检查器是否初始化
**查看日志：**
```
[CodeEditor] WARNING: Syntax checker is null!
```

**解决方法：**
1. 检查setupSyntaxChecker()是否被调用
2. 确认SyntaxChecker对象创建成功

#### 检查点2：编译器路径是否正确
**查看日志：**
```
[SyntaxChecker] Performing syntax check with compiler: g++
```

**解决方法：**
1. 确认系统中安装了g++或MinGW
2. 检查编译器是否在PATH中
3. 在设置中配置正确的编译器路径

#### 检查点3：编译器是否执行成功
**查看日志：**
```
[SyntaxChecker] Process finished, exit code: X status: X
[SyntaxChecker] Compiler output: XXX
```

**解决方法：**
1. 检查exit code（0=成功，非0=失败）
2. 查看编译器输出是否有错误信息
3. 确认临时文件创建成功

#### 检查点4：错误解析是否正确
**查看日志：**
```
[SyntaxChecker] Found X errors/warnings
```

**解决方法：**
1. 如果找到0个错误但代码确实有错误，检查正则表达式
2. 查看编译器输出格式是否匹配
3. 确认错误信息被正确解析

## 🧪 测试步骤

### 测试1：代码保存功能
1. 启动程序
2. 选择一个题目
3. 在代码编辑器中输入代码
4. 查看控制台日志：
   ```
   [CodeEditor] Text changed, code length: XXX
   [AutoSaver] Saved code to: data/user_answers/XXX.json length: XXX
   ```
5. 检查data/user_answers目录下是否有对应的JSON文件
6. 重启程序，检查代码是否被加载

### 测试2：语法检查功能
1. 启动程序
2. 选择一个题目
3. 输入有语法错误的代码，例如：
   ```cpp
   int main() {
       return 0  // 缺少分号
   }
   ```
4. 等待500ms
5. 查看控制台日志：
   ```
   [CodeEditor] Triggering syntax check...
   [SyntaxChecker] Performing syntax check with compiler: g++
   [SyntaxChecker] Found 1 errors/warnings
   ```
6. 检查错误列表是否显示在底部
7. 检查代码中是否有红色波浪线

## 🛠️ 常见问题解决

### Q1: 看不到任何日志输出
**A:** 
- 确保在Debug模式下运行
- 检查控制台是否被隐藏
- 尝试在命令行中运行程序

### Q2: QuestionId始终为空
**A:**
- 检查题库是否正确加载
- 确认题目有有效的ID字段
- 查看loadCurrentQuestion()是否被调用

### Q3: 编译器找不到
**A:**
- 安装MinGW或GCC
- 将编译器路径添加到系统PATH
- 在设置中手动配置编译器路径

### Q4: 错误列表不显示
**A:**
- 检查错误列表是否被隐藏
- 按Ctrl+Shift+M显示错误列表
- 确认有实际的语法错误

### Q5: 代码保存后重启消失
**A:**
- 检查data/user_answers目录权限
- 确认JSON文件被正确创建
- 查看loadSavedCode()是否被调用

## 📊 性能监控

### 关键指标
- **保存延迟**: 200ms
- **检查延迟**: 500ms
- **文件大小**: 通常<10KB
- **检查时间**: 通常<1秒

### 异常情况
- 保存延迟>1秒 → 磁盘IO问题
- 检查延迟>5秒 → 编译器性能问题
- 文件大小>100KB → 代码过长
- 检查时间>10秒 → 编译器卡死

## 🎯 下一步

如果问题仍然存在：
1. 收集完整的控制台日志
2. 检查data/user_answers目录内容
3. 验证编译器是否可用
4. 查看系统事件日志

---

**提示**: 运行程序后，查看控制台输出的调试日志，可以快速定位问题所在！
