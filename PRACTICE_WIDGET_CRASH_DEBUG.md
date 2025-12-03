# 题库列表崩溃调试增强

## 问题
题库列表模式点击后崩溃，日志显示：
```
Switching to practice mode...
QuestionBank count: 41
PracticeWidget valid: true
The command terminated abnormally.
```

## 调试策略

### 添加详细日志
**文件**: `src/ui/PracticeWidget.cpp` - `loadQuestions()`

在关键位置添加try-catch和调试日志：

1. **表格清空**
```cpp
try {
    m_questionTable->setRowCount(0);
    qDebug() << "[PracticeWidget] Table cleared";
} catch (...) {
    qCritical() << "[PracticeWidget] Failed to clear table";
    return;
}
```

2. **获取题目数量**
```cpp
int count = 0;
try {
    count = m_questionBank->count();
    qDebug() << "[PracticeWidget] QuestionBank count:" << count;
} catch (...) {
    qCritical() << "[PracticeWidget] Exception getting count";
    return;
}
```

3. **获取题目列表**
```cpp
try {
    allQuestions = m_questionBank->allQuestions();
    qDebug() << "[PracticeWidget] Got questions, size:" << allQuestions.size();
} catch (...) {
    qCritical() << "[PracticeWidget] Exception when getting questions";
    return;
}
```

4. **收集标签**
```cpp
try {
    qDebug() << "[PracticeWidget] Collecting tags...";
    for (const auto &q : allQuestions) {
        for (const auto &tag : q.tags()) {
            allTags.insert(tag);
        }
    }
    qDebug() << "[PracticeWidget] Tags collected:" << allTags.size();
} catch (...) {
    qCritical() << "[PracticeWidget] Exception collecting tags";
    return;
}
```

5. **更新筛选器**
```cpp
try {
    qDebug() << "[PracticeWidget] Updating tag filter...";
    if (!m_tagFilter) {
        qCritical() << "[PracticeWidget] m_tagFilter is null!";
        return;
    }
    // ... 更新逻辑
    qDebug() << "[PracticeWidget] Tag filter updated";
} catch (...) {
    qCritical() << "[PracticeWidget] Exception updating tag filter";
    return;
}
```

6. **获取筛选条件**
```cpp
try {
    if (m_difficultyFilter) {
        difficultyIndex = m_difficultyFilter->currentData().toInt();
    }
    if (m_tagFilter) {
        selectedTag = m_tagFilter->currentText();
    }
    if (m_statusFilter) {
        statusFilter = m_statusFilter->currentData().toInt();
    }
    qDebug() << "[PracticeWidget] Filters:" << difficultyIndex << selectedTag << statusFilter;
} catch (...) {
    qCritical() << "[PracticeWidget] Exception getting filter values";
    return;
}
```

7. **加载题目循环**
```cpp
qDebug() << "[PracticeWidget] Loading questions into table...";
int loadedCount = 0;

try {
    for (const auto &question : allQuestions) {
        loadedCount++;
        // ... 加载逻辑
    }
    
    qDebug() << "[PracticeWidget] loadQuestions() completed. Processed:" << loadedCount << "Displayed:" << m_questionTable->rowCount();
    
} catch (const std::exception &e) {
    qCritical() << "[PracticeWidget] Exception in loop:" << e.what();
} catch (...) {
    qCritical() << "[PracticeWidget] Unknown exception in loop";
}
```

## 预期日志输出

### 正常情况
```
[PracticeWidget] loadQuestions() started
[PracticeWidget] Table cleared
[PracticeWidget] QuestionBank count: 41
[PracticeWidget] Got questions, size: 41
[PracticeWidget] Collecting tags...
[PracticeWidget] Tags collected: 5
[PracticeWidget] Updating tag filter...
[PracticeWidget] Tag filter updated
[PracticeWidget] Filters: -1 全部题型 -1
[PracticeWidget] Loading questions into table...
[PracticeWidget] loadQuestions() completed. Processed: 41 Displayed: 41
```

### 崩溃情况
日志会在崩溃前停止，最后一行日志指示崩溃位置：
- 如果停在"Table cleared"之前 → 表格对象问题
- 如果停在"Got questions"之前 → QuestionBank问题
- 如果停在"Tags collected"之前 → Question对象或tags()方法问题
- 如果停在"Tag filter updated"之前 → m_tagFilter对象问题
- 如果停在"Loading questions"之后 → 循环中的某个操作问题

## 可能的崩溃原因

### 1. 空指针访问
- m_questionTable
- m_tagFilter
- m_difficultyFilter
- m_statusFilter

### 2. Question对象问题
- tags()返回无效引用
- title()返回无效字符串
- difficulty()返回无效值

### 3. ProgressManager问题
- getProgress()崩溃
- 返回无效数据

### 4. QTableWidget操作问题
- insertRow()失败
- setItem()失败
- 内存不足

## 下一步

1. **运行程序**：查看详细日志输出
2. **定位崩溃点**：找到最后一条日志
3. **针对性修复**：根据崩溃位置修复具体问题

## 文件修改
- ✅ `src/ui/PracticeWidget.cpp` - 添加详细调试日志和异常处理

## 编译状态
✅ 编译成功，无错误，无警告

## 使用方法
1. 编译并运行程序
2. 点击"题库列表"按钮
3. 查看控制台输出
4. 找到崩溃前的最后一条日志
5. 根据日志定位问题
