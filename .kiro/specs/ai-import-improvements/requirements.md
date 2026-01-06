# AI导入题库优化需求文档

## Introduction

本需求文档旨在解决AI导入题库模块中存在的三个关键问题：进度条显示不准确、导入规则文件管理混乱、以及导入完成弹窗内容过时。这些问题影响了用户体验和系统的可维护性。

## Glossary

- **AI导入模块** (AI Import Module): 使用AI技术自动识别和解析题库文件的导入系统
- **进度条** (Progress Bar): 显示导入进度的UI组件
- **导入规则文件** (Import Rule File): 存储题库解析规则的JSON配置文件
- **基础题库** (Base Question Bank): 存储已解析题目的目录结构
- **SmartQuestionImporter**: 负责AI导入逻辑的核心类
- **SmartImportDialog**: AI导入的用户界面对话框
- **递归拆分** (Recursive Split): AI逐个识别题目的处理方式

## Requirements

### Requirement 1

**User Story:** 作为用户，我希望看到准确的导入进度，以便了解导入任务的真实完成情况。

#### Acceptance Criteria

1. WHEN AI导入完成所有题目识别 THEN 进度条 SHALL 显示100%
2. WHEN AI正在递归拆分识别题目 THEN 进度条 SHALL 根据已识别题目数量和文件进度平滑增长
3. WHEN 导入流程进入最后的保存阶段 THEN 进度条 SHALL 从95%增长到100%
4. WHEN 计算进度百分比 THEN 系统 SHALL 确保进度值单调递增且不会倒退
5. WHEN 所有文件处理完成 THEN 进度条 SHALL 在importCompleted信号发出前达到100%

### Requirement 2

**User Story:** 作为开发者，我希望导入规则文件不会出现在用户可见的题库列表中，以保持界面整洁。

#### Acceptance Criteria

1. WHEN 系统扫描基础题库目录 THEN QuestionBankManager SHALL 过滤掉所有JSON配置文件
2. WHEN 保存导入规则文件 THEN 系统 SHALL 将其存储在data/config目录而非题库目录
3. WHEN 用户查看题库列表 THEN 界面 SHALL 只显示题目文件夹，不显示配置文件
4. WHEN 删除题库 THEN 系统 SHALL 同时清理对应的config目录中的规则文件

### Requirement 2.1

**User Story:** 作为用户，我希望AI生成模拟题库功能能够正确使用导入规则，以便生成符合原题库风格的模拟题。

#### Acceptance Criteria

1. WHEN AI生成模拟题库需要读取规则文件 THEN 系统 SHALL 从data/config目录读取对应题库的规则文件
2. WHEN 规则文件不存在于config目录 THEN 系统 SHALL 提示用户该题库缺少导入规则
3. WHEN 生成模拟题库 THEN 系统 SHALL 使用规则文件中的难度分布和知识点分布信息
4. WHEN 生成模拟题库 THEN 系统 SHALL 使用规则文件中的测试用例生成规则
5. WHEN 保存生成的模拟题 THEN 系统 SHALL 使用与原题库相同的Markdown格式

### Requirement 3

**User Story:** 作为用户，我希望导入完成后看到清晰准确的结果摘要，以便了解导入的具体情况。

#### Acceptance Criteria

1. WHEN 导入成功完成 THEN 完成弹窗 SHALL 显示成功导入的题目总数
2. WHEN 导入成功完成 THEN 完成弹窗 SHALL 显示题目按源文件的分类统计
3. WHEN 导入成功完成 THEN 完成弹窗 SHALL 显示题目难度分布统计
4. WHEN 导入成功完成 THEN 完成弹窗 SHALL 说明题目已保存到基础题库的具体路径
5. WHEN 导入成功完成 THEN 完成弹窗 SHALL 移除过时的"导入规则"和"运行时题库"相关描述
6. WHEN 导入失败 THEN 完成弹窗 SHALL 显示失败原因和可能的解决方案
7. WHEN 用户取消导入 THEN 完成弹窗 SHALL 显示已导入的部分结果统计
