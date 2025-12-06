# AI导入MD拆分 - 当前实施策略

## 问题分析

SmartQuestionImporter.cpp文件过大（1073行），完全重写风险较高。

## 修改策略

采用**渐进式修改**策略：

### 方案1：保留现有流程，添加MD保存选项（推荐）
1. 保留现有的AI解析JSON流程
2. 在保存时增加选项：同时保存为MD格式
3. 使用MarkdownQuestionParser将Question对象转换为MD
4. 优点：风险小，可以逐步过渡
5. 缺点：仍然会生成JSON

### 方案2：完全重写为MD拆分流程
1. 重写startImport方法
2. AI只负责拆分和提取元数据
3. 不生成测试用例
4. 优点：符合最终目标
5. 缺点：改动大，风险高

## 决定：采用方案1

### 实施步骤

1. **修改parseAIResponseAndGenerateTests方法**
   - 在保存JSON后，同时保存MD文件
   - 使用Question::saveAsMarkdown()

2. **修改saveParseRulesAndQuestionBank方法**
   - 题目已在AI解析时实时保存为MD

3. **测试**
   - 导入一个小题库
   - 验证MD文件生成正确
   - 验证Front Matter格式正确

4. **后续优化**（可选）
   - 逐步移除JSON生成
   - 简化AI prompt，只要求拆分和元数据

## 当前任务

修改`parseAIResponseAndGenerateTests`方法，在保存JSON的同时保存MD文件。
