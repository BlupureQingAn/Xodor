# AI导入简化与实时进度更新

## 改进内容

### 1. 简化导入方式
**移除**: ImportDialog、AIImportDialog等多种导入方式
**保留**: 只支持AI智能导入（SmartImportDialog）

**修改文件**: `src/ui/MainWindow.cpp`
- 移除了ImportDialog和AIImportDialog的引用
- 使用QFileDialog直接选择文件夹
- 简化了导入流程，用户体验更直接

### 2. 实时进度更新
**问题**: 之前AI导入时进度条不动，用户不知道处理进度
**解决**: 实现基于流式响应的实时进度更新

#### 2.1 AIService添加流式进度信号
**文件**: `src/ai/AIService.h`
```cpp
signals:
    void streamProgress(const QString &context, int currentLength, const QString &partialContent);
```

#### 2.2 OllamaClient发射进度信号
**文件**: `src/ai/OllamaClient.cpp`
- 在流式响应处理中，每收到一个数据块就发射进度信号
- 实时报告已接收的字符数和部分内容

#### 2.3 SmartQuestionImporter监听进度
**文件**: `src/ai/SmartQuestionImporter.h`, `src/ai/SmartQuestionImporter.cpp`
- 添加`onStreamProgress`槽函数
- 根据接收的字符数估算进度
- 每1000字符输出一次日志
- 实时更新进度条和状态信息

## 工作流程

### 导入流程
1. 用户点击"导入题库"
2. 选择题库文件夹（QFileDialog）
3. 输入题库分类名称
4. 打开SmartImportDialog
5. AI开始解析，实时显示进度

### 进度更新机制

```
用户选择文件夹
    ↓
SmartImportDialog启动
    ↓
SmartQuestionImporter.startImport()
    ↓
扫描文件 → 分块
    ↓
逐块发送给AI (sendCustomPrompt)
    ↓
OllamaClient流式接收响应
    ↓
每收到数据块 → emit streamProgress()
    ↓
SmartQuestionImporter.onStreamProgress()
    ↓
更新进度条和日志 → emit progressUpdated()
    ↓
SmartImportDialog更新UI
```

### 进度计算
- **字符数统计**: 实时统计接收的字符数
- **进度估算**: 假设平均每个题目2000字符
- **日志输出**: 每1000字符输出一次
- **状态更新**: 显示"AI正在解析... (已接收 X 字符)"

## 关键改进

### ✅ 简化用户操作
- 移除了复杂的导入模式选择
- 直接使用系统文件选择对话框
- 一键启动AI智能导入

### ✅ 实时进度反馈
- 进度条根据AI流式响应实时更新
- 日志区域显示接收进度
- 用户可以看到AI正在工作

### ✅ 更好的用户体验
- 不再是"黑盒"等待
- 清晰的进度指示
- 实时的状态反馈

## 文件修改清单
- ✅ `src/ui/MainWindow.cpp` - 简化导入逻辑
- ✅ `src/ai/AIService.h` - 添加流式进度信号
- ✅ `src/ai/OllamaClient.cpp` - 发射进度信号
- ✅ `src/ai/SmartQuestionImporter.h` - 添加进度处理方法
- ✅ `src/ai/SmartQuestionImporter.cpp` - 实现实时进度更新

## 编译状态
✅ 编译成功，无错误，无警告

## 测试要点
1. 导入题库时进度条应该实时更新
2. 日志区域应该显示"接收中... X 字符"
3. 状态标签应该显示当前接收的字符数
4. 整个过程应该流畅，无卡顿
