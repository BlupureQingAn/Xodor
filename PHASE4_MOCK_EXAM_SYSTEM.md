# 阶段四：模拟题库生成系统 - 实施完成

## 🎯 实施目标

实现完整的模拟题库生成系统，包括：
1. **基于规则的AI题目生成** - 分析题库规律，生成符合风格的新题
2. **自动测试数据生成** - 为题目生成完整的测试用例
3. **模拟题管理** - 管理生成的模拟套题

## ✅ 已完成功能

### 1. 模拟题生成器 (MockExamGenerator)

**核心功能：**
- ✅ 题库分析 - 自动分析难度分布、知识点分布
- ✅ 出题规则生成 - 提取题库特征，生成出题模式
- ✅ AI题目生成 - 基于规则生成完整套题
- ✅ 批量生成 - 支持一次生成多套模拟题

**文件：**
- `src/ai/MockExamGenerator.h` - 模拟题生成器头文件
- `src/ai/MockExamGenerator.cpp` - 模拟题生成器实现

**关键特性：**
```cpp
// 出题规则结构
struct ExamPattern {
    QString categoryName;                    // 分类名称
    int questionsPerExam;                    // 每套题目数量
    int timeLimit;                           // 时间限制
    QMap<Difficulty, double> difficultyRatio; // 难度分布
    QMap<QString, double> topicRatio;        // 知识点分布
    int timeLimitPerQuestion;                // 单题时间限制
    int memoryLimit;                         // 内存限制
    QStringList supportedLanguages;          // 支持的语言
};

// 核心方法
- analyzeQuestionBank() - 分析题库生成规则
- generateMockExam() - 生成模拟题
- savePattern() / loadPattern() - 保存/加载出题规则
```

### 2. 测试数据生成器 (TestDataGenerator)

**核心功能：**
- ✅ 单题测试数据生成 - 为单个题目生成补充测试用例
- ✅ 批量测试数据生成 - 为多个题目批量生成测试数据
- ✅ 智能场景覆盖 - 自动生成边界、异常、等价类测试

**文件：**
- `src/ai/TestDataGenerator.h` - 测试数据生成器头文件
- `src/ai/TestDataGenerator.cpp` - 测试数据生成器实现

**测试场景覆盖：**
- 基本功能测试
- 边界条件（最小值、最大值）
- 异常情况（空输入、特殊字符）
- 等价类测试
- 性能测试（大数据量）

### 3. 模拟题管理对话框 (MockExamManagerDialog)

**核心功能：**
- ✅ 题库分析界面 - 可视化显示题库分析结果
- ✅ 模拟题生成界面 - 配置生成参数，启动生成
- ✅ 模拟题列表 - 显示已生成的模拟题
- ✅ 模拟题操作 - 查看、删除、导出模拟题

**文件：**
- `src/ui/MockExamManagerDialog.h` - 模拟题管理对话框头文件
- `src/ui/MockExamManagerDialog.cpp` - 模拟题管理对话框实现

**界面功能：**
```
┌─────────────────────────────────────────────────┐
│ 📚 模拟题库管理                                │
├─────────────────────────────────────────────────┤
│ 【生成配置】                                    │
│ • 题库分类选择                                  │
│ • 分析题库按钮                                  │
│ • 出题规律显示                                  │
│ • 生成数量设置                                  │
│ • 开始生成按钮                                  │
│                                                 │
│ 【已有模拟题】                                  │
│ • 模拟题列表                                    │
│ • 查看/删除/导出按钮                            │
│                                                 │
│ 【进度显示】                                    │
│ • 进度条                                        │
│ • 操作日志                                      │
└─────────────────────────────────────────────────┘
```

## 📋 使用流程

### 流程1：分析题库并生成模拟题

```
1. 打开模拟题库管理
   菜单: 题目 → 模拟题库管理 (Ctrl+Shift+M)

2. 选择题库分类
   - CCF
   - LeetCode
   - 自定义

3. 分析题库
   点击"分析题库"按钮
   系统自动分析：
   - 题目总数
   - 难度分布
   - 知识点分布
   - 出题规律

4. 配置生成参数
   - 设置生成套数（1-10套）
   - 查看出题规律

5. 开始生成
   点击"开始生成"按钮
   系统自动：
   - 构建AI提示词
   - 调用AI生成题目
   - 解析生成结果
   - 保存模拟题文件

6. 查看结果
   - 在模拟题列表中查看
   - 可以查看、删除、导出
```

### 流程2：为现有题目生成测试数据

```cpp
// 使用TestDataGenerator
TestDataGenerator *generator = new TestDataGenerator(aiClient);

// 为单个题目生成测试数据
generator->generateTestData(question, 5);  // 生成5组补充数据

// 批量生成
generator->generateBatchTestData(questions, 5);

// 接收生成结果
connect(generator, &TestDataGenerator::testDataGenerated,
        [](const QString &questionId, const QVector<TestCase> &testCases) {
    // 处理生成的测试数据
});
```

## 🔧 技术实现

### 1. AI提示词构建

**模拟题生成提示词结构：**
```
【出题规则】
- 分类、题目数量、时间限制
- 难度分布、知识点分布
- 代码限制

【生成要求】
- 题目风格要求
- 测试数据要求
- 题目编号规则

【输出格式】
- JSON格式规范
- 字段定义
```

**测试数据生成提示词结构：**
```
【题目信息】
- 标题、描述
- 现有测试数据

【生成要求】
- 场景覆盖要求
- 数据格式要求

【输出格式】
- JSON格式
```

### 2. 题库分析算法

```cpp
ExamPattern analyzeQuestionBank(questions) {
    // 1. 统计难度分布
    for (question in questions) {
        diffCount[question.difficulty]++;
    }
    
    // 2. 统计知识点分布
    for (question in questions) {
        for (tag in question.tags) {
            topicCount[tag]++;
        }
    }
    
    // 3. 计算比例
    pattern.difficultyRatio = diffCount / total;
    pattern.topicRatio = topicCount / total;
    
    // 4. 提取其他规则
    pattern.questionsPerExam = detectQuestionCount();
    pattern.timeLimit = detectTimeLimit();
    
    return pattern;
}
```

### 3. 模拟题保存结构

```
人工模拟题库/
├── ccf/
│   ├── 模拟题1/
│   │   ├── 第1题.md
│   │   ├── 第2题.md
│   │   ├── 第3题.md
│   │   ├── 第4题.md
│   │   └── 答题说明.md
│   ├── 模拟题2/
│   │   └── ...
│   └── 出题模式规律.json
├── leetcode/
│   └── ...
└── custom/
    └── ...
```

## 📊 数据流转

### 生成模拟题流程

```
用户操作
  ↓
选择分类 → 分析题库
  ↓
题库分析器
  ↓
生成出题规则 → 保存规则文件
  ↓
配置生成参数
  ↓
模拟题生成器
  ↓
构建AI提示词
  ↓
调用AI服务
  ↓
解析AI响应
  ↓
生成Question对象
  ↓
保存为Markdown文件
  ↓
更新模拟题列表
  ↓
完成
```

### 测试数据生成流程

```
题目信息
  ↓
测试数据生成器
  ↓
分析现有测试数据
  ↓
构建AI提示词
  ↓
调用AI服务
  ↓
解析测试用例
  ↓
标记为AI生成
  ↓
返回TestCase列表
  ↓
合并到题目
```

## 🎨 UI集成

### MainWindow集成

**新增菜单项：**
```cpp
题目菜单
├── 导入题库
├── 刷新题库
├── ...
├── 生成模拟题 (Ctrl+G)
└── 模拟题库管理 (Ctrl+Shift+M)  ← 新增
```

**新增功能：**
- `onManageMockExams()` - 打开模拟题管理对话框

## 📦 文件清单

### 新增文件

**AI模块：**
- `src/ai/MockExamGenerator.h` - 模拟题生成器头文件
- `src/ai/MockExamGenerator.cpp` - 模拟题生成器实现
- `src/ai/TestDataGenerator.h` - 测试数据生成器头文件
- `src/ai/TestDataGenerator.cpp` - 测试数据生成器实现

**UI模块：**
- `src/ui/MockExamManagerDialog.h` - 模拟题管理对话框头文件
- `src/ui/MockExamManagerDialog.cpp` - 模拟题管理对话框实现

### 修改文件

- `CMakeLists.txt` - 添加新文件到构建系统
- `src/ui/MainWindow.h` - 添加新槽函数声明
- `src/ui/MainWindow.cpp` - 添加菜单项和实现

## 🚀 使用示例

### 示例1：生成CCF模拟题

```
1. 打开模拟题库管理
2. 选择"CCF"分类
3. 点击"分析题库"
   结果：
   - 每套4道题
   - 时间180分钟
   - 难度：简单30%，中等50%，困难20%
   - 知识点：数组、字符串、动态规划...

4. 设置生成2套
5. 点击"开始生成"
6. 等待生成完成（约2-3分钟）
7. 查看生成的模拟题
```

### 示例2：为题目生成测试数据

```cpp
// 创建生成器
TestDataGenerator *generator = new TestDataGenerator(m_ollamaClient);

// 连接信号
connect(generator, &TestDataGenerator::testDataGenerated,
        this, &MyClass::onTestDataReady);

// 生成测试数据
generator->generateTestData(currentQuestion, 5);

// 处理结果
void MyClass::onTestDataReady(const QString &questionId, 
                              const QVector<TestCase> &testCases) {
    // 添加到题目
    Question q = findQuestion(questionId);
    QVector<TestCase> allCases = q.testCases();
    allCases.append(testCases);
    q.setTestCases(allCases);
    
    // 保存
    saveQuestion(q);
}
```

## ⚠️ 注意事项

### 1. AI服务要求
- 需要配置Ollama服务或云端API
- 确保AI服务正常运行
- 生成时间取决于题目数量和AI性能

### 2. 生成质量
- 题库越丰富，生成质量越高
- 建议至少有10道题作为参考
- 每次生成不超过5套，保证质量

### 3. 测试数据
- AI生成的测试数据需要人工审核
- 确保输入输出格式正确
- 验证测试数据的合理性

### 4. 文件管理
- 模拟题保存在"人工模拟题库"目录
- 每套题独立文件夹
- 包含答题说明文件

## 🔍 常见问题

### Q: 生成的题目质量如何？
A: 质量取决于：
- 参考题库的质量和数量
- AI模型的能力
- 出题规则的准确性

建议：
- 使用高质量题库作为参考
- 生成后人工审核
- 根据反馈调整规则

### Q: 可以生成多少套题？
A: 支持1-10套，建议：
- 首次生成：2-3套
- 验证质量后：可增加到5-10套
- 每次生成不超过5套

### Q: 测试数据生成需要多久？
A: 时间取决于：
- 题目数量
- 每题生成的测试用例数量
- AI服务性能

通常：
- 单题：10-30秒
- 批量（10题）：2-5分钟

### Q: 如何自定义出题规则？
A: 两种方式：
1. 修改分析算法（代码层面）
2. 手动编辑"出题模式规律.json"文件

### Q: 生成失败怎么办？
A: 检查：
1. AI服务状态
2. 网络连接
3. 题库是否为空
4. 日志中的错误信息

## 📈 性能优化

### 1. 批量生成优化
- 使用异步生成
- 显示实时进度
- 支持中断和恢复

### 2. 缓存机制
- 缓存题库分析结果
- 缓存出题规则
- 减少重复分析

### 3. 并发控制
- 限制同时生成的套数
- 避免AI服务过载
- 合理分配资源

## 🎯 下一步计划

### 短期优化
- [ ] 添加模拟题预览功能
- [ ] 支持模拟题导出为PDF
- [ ] 添加模拟题评分系统
- [ ] 支持自定义出题规则

### 中期扩展
- [ ] 支持更多题库格式
- [ ] 添加题目相似度检测
- [ ] 实现智能难度调整
- [ ] 支持多语言题目生成

### 长期规划
- [ ] 建立题目质量评估体系
- [ ] 实现协同出题功能
- [ ] 支持题目版本管理
- [ ] 集成在线评测系统

## ✅ 编译验证

```bash
# 配置CMake
cmake -B build -S . --preset=default

# 编译
cmake --build build

# 运行
./build/CodePracticeSystem.exe
```

## 🎉 功能亮点

### 1. 智能分析
- 自动分析题库特征
- 提取出题规律
- 生成符合风格的题目

### 2. 灵活配置
- 可配置生成数量
- 支持多种分类
- 自定义出题规则

### 3. 完整测试
- 自动生成测试数据
- 覆盖多种场景
- 标记数据来源

### 4. 便捷管理
- 可视化管理界面
- 支持查看、删除、导出
- 实时进度显示

---

**阶段四实施完成！** ✨

模拟题库生成系统已经完整实现，用户可以：
1. 分析现有题库，提取出题规律
2. 基于规律生成新的模拟套题
3. 自动生成完整的测试数据
4. 管理和使用生成的模拟题

系统现在具备了完整的题目生成和管理能力！🚀
