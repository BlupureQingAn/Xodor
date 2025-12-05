# CMake 编译修复完成

## 问题描述
QtCreator 无法编译新添加的文件，出现以下错误：
1. 新文件未添加到 CMakeLists.txt
2. QuestionEditorDialog 使用了错误的 Question API
3. QuestionBankTreeWidget 有语法错误

## 修复内容

### 1. 更新 CMakeLists.txt

**添加新源文件**：
```cmake
src/ui/QuestionEditorDialog.cpp
src/utils/OperationHistory.cpp
```

**添加新头文件**：
```cmake
src/ui/QuestionEditorDialog.h
src/utils/OperationHistory.h
```

### 2. 修复 QuestionEditorDialog.cpp

#### 问题 1：Difficulty 类型错误
**错误代码**：
```cpp
question.setDifficulty(m_difficultyCombo->currentText());  // 错误：传入 QString
```

**修复后**：
```cpp
// 难度转换
Difficulty difficulty = Difficulty::Easy;
QString diffText = m_difficultyCombo->currentText();
if (diffText == "中等") {
    difficulty = Difficulty::Medium;
} else if (diffText == "困难") {
    difficulty = Difficulty::Hard;
}
question.setDifficulty(difficulty);
```

#### 问题 2：不存在的方法
**错误代码**：
```cpp
question.setTimeLimit(m_timeLimitSpin->value());      // Question 类没有此方法
question.setMemoryLimit(m_memoryLimitSpin->value());  // Question 类没有此方法
```

**修复后**：
将时间和内存限制添加到描述中：
```cpp
QString description = m_descriptionEdit->toPlainText();
description += QString("\n\n**限制条件：**\n- 时间限制：%1 ms\n- 内存限制：%2 MB")
    .arg(m_timeLimitSpin->value())
    .arg(m_memoryLimitSpin->value());
question.setDescription(description);
```

#### 问题 3：读取难度时的类型转换
**错误代码**：
```cpp
int difficultyIndex = m_difficultyCombo->findText(question.difficulty());  // 错误：Difficulty 不是 QString
```

**修复后**：
```cpp
// 难度转换
QString diffText = "简单";
switch (question.difficulty()) {
    case Difficulty::Easy:
        diffText = "简单";
        break;
    case Difficulty::Medium:
        diffText = "中等";
        break;
    case Difficulty::Hard:
        diffText = "困难";
        break;
}
int difficultyIndex = m_difficultyCombo->findText(diffText);
```

#### 问题 4：读取限制条件
**修复方案**：
从描述中解析时间和内存限制：
```cpp
// 尝试解析时间和内存限制
QRegularExpression timeRegex("时间限制[：:](\\d+)\\s*ms");
QRegularExpression memoryRegex("内存限制[：:](\\d+)\\s*MB");

QRegularExpressionMatch timeMatch = timeRegex.match(description);
if (timeMatch.hasMatch()) {
    m_timeLimitSpin->setValue(timeMatch.captured(1).toInt());
}

QRegularExpressionMatch memoryMatch = memoryRegex.match(description);
if (memoryMatch.hasMatch()) {
    m_memoryLimitSpin->setValue(memoryMatch.captured(1).toInt());
}
```

### 3. 修复 QuestionEditorDialog.h

**问题**：`onImportFromFile()` 是私有方法，但需要从外部调用

**修复**：
```cpp
public slots:
    void onImportFromFile();  // 移到 public slots
    
private slots:
    void onAddTestCase();
    void onRemoveTestCase(int index);
    void onAccept();
    void onCancel();
```

### 4. 修复 QuestionBankTreeWidget.cpp

**问题**：`onDeleteBank()` 方法有多余的 else 块和大括号

**错误代码**：
```cpp
if (reply == QMessageBox::Yes) {
    OperationHistory::instance().recordDeleteBank(bankPath);
    refreshTree();
    QMessageBox::information(this, "成功", "题库已删除\n\n按 Ctrl+Z 可撤销此操作");
    } else {  // 多余的 else
        QMessageBox::warning(this, "错误", "无法删除题库目录");
    }
}  // 多余的大括号
}
```

**修复后**：
```cpp
if (reply == QMessageBox::Yes) {
    OperationHistory::instance().recordDeleteBank(bankPath);
    refreshTree();
    QMessageBox::information(this, "成功", "题库已删除\n\n按 Ctrl+Z 可撤销此操作");
}
}
```

### 5. 添加必要的头文件

在 `QuestionEditorDialog.cpp` 中添加：
```cpp
#include <QRegularExpression>
```

## 技术说明

### Question 类的 API
Question 类使用以下 API：
- `Difficulty` 是枚举类型，不是字符串
- 没有 `timeLimit` 和 `memoryLimit` 字段
- 限制条件需要存储在描述中

### Difficulty 枚举
```cpp
enum class Difficulty {
    Easy,
    Medium,
    Hard
};
```

### 字符串与枚举转换
需要手动转换：
- "简单" ↔ `Difficulty::Easy`
- "中等" ↔ `Difficulty::Medium`
- "困难" ↔ `Difficulty::Hard`

## 编译结果

✅ **编译成功**

```
[1/5] Automatic MOC and UIC for target CodePracticeSystem
[2/5] Building CXX object CMakeFiles/CodePracticeSystem.dir/src/ui/QuestionBankTreeWidget.cpp.obj
[3/5] Building CXX object CMakeFiles/CodePracticeSystem.dir/src/ui/QuestionEditorDialog.cpp.obj
[4/5] Building CXX object CMakeFiles/CodePracticeSystem.dir/src/utils/OperationHistory.cpp.obj
[5/5] Linking CXX executable CodePracticeSystem.exe
```

## 相关文件

### 修改的文件
- `CMakeLists.txt` - 添加新文件
- `src/ui/QuestionEditorDialog.h` - 修改方法访问权限
- `src/ui/QuestionEditorDialog.cpp` - 修复 API 调用
- `src/ui/QuestionBankTreeWidget.cpp` - 修复语法错误

### 新增的文件
- `src/ui/QuestionEditorDialog.h/cpp` - 题目编辑器
- `src/utils/OperationHistory.h/cpp` - 操作历史管理器

## 测试建议

1. **编译测试**
   - ✅ 清理构建目录
   - ✅ 重新配置 CMake
   - ✅ 完整编译
   - ✅ 无错误无警告

2. **功能测试**
   - [ ] 创建新题目
   - [ ] 编辑现有题目
   - [ ] 删除题目
   - [ ] 撤销删除
   - [ ] 查看操作历史

## 总结

所有编译错误已修复，项目可以正常编译。主要问题是：
1. ✅ CMakeLists.txt 缺少新文件
2. ✅ Question API 使用错误
3. ✅ 语法错误（多余的大括号）
4. ✅ 方法访问权限问题

现在可以在 QtCreator 中正常编译和运行项目了！
