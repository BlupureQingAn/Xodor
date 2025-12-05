# AI判题进度对话框样式优化

## 问题
AI判题进度对话框的进度条需要：
1. 居中显示
2. 使用红色 (#660000) 作为进度条颜色

## 解决方案

在 `src/ui/MainWindow.cpp` 的 `onAIJudgeRequested()` 方法中，添加样式表设置：

```cpp
// 设置样式：红色进度条，居中对齐
m_aiJudgeProgressDialog->setStyleSheet(
    "QProgressDialog { "
    "   background-color: #2b2b2b; "
    "} "
    "QProgressBar { "
    "   border: 2px solid #660000; "
    "   border-radius: 5px; "
    "   text-align: center; "
    "   background-color: #1e1e1e; "
    "   color: white; "
    "} "
    "QProgressBar::chunk { "
    "   background-color: #660000; "
    "   width: 20px; "
    "}"
);
```

## 样式说明

- **QProgressDialog**: 对话框背景色设为深灰色 (#2b2b2b)
- **QProgressBar**: 
  - 边框：2px 红色 (#660000)
  - 圆角：5px
  - 文本居中对齐
  - 背景色：深色 (#1e1e1e)
  - 文字颜色：白色
- **QProgressBar::chunk**: 
  - 进度条填充色：红色 (#660000)
  - 每个块宽度：20px

## 编译问题

由于系统资源不足，编译器持续崩溃。建议：
1. 重启系统
2. 关闭其他占用内存的程序
3. 然后重新编译

## 已完成的功能

1. ✅ AI判题点击后立即显示进度对话框
2. ✅ 判题完成后关闭进度对话框并显示结果
3. ✅ 统一AI判题通过和运行测试通过的状态图标（都显示绿色对钩✅）
4. ✅ 在题目列表中显示每道题的通过情况
5. ⏳ 进度条红色样式和居中（代码已修改，等待编译）
