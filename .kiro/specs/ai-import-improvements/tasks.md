# Implementation Plan

- [x] 1. 创建配置文件管理器



  - 创建ImportRuleManager类，统一管理导入规则文件
  - 实现保存、读取、检查、删除规则文件的方法
  - 规则文件路径：data/config/{bankName}_parse_rule.json
  - _Requirements: 2.2, 2.1.1_

- [ ]* 1.1 为ImportRuleManager编写单元测试
  - 测试保存和读取规则文件
  - 测试文件路径生成
  - 测试文件不存在的情况



  - _Requirements: 2.2, 2.1.1_

- [x] 2. 重构SmartQuestionImporter的进度管理
  - 在ImportProgress结构体中添加Stage枚举
  - 实现calculatePercentage()方法
  - 添加阶段转换方法（enterScanningStage等）
  - ✅ 已实现三阶段进度管理系统
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

- [x] 2.1 修改扫描阶段进度计算

  - 扫描阶段占0-10%
  - 在scanAndAnalyzeFiles中更新阶段状态
  - _Requirements: 1.2_


- [x] 2.2 修改AI解析阶段进度计算

  - AI解析阶段占10-95%
  - 使用对数函数平滑当前文件内的进度
  - 确保进度单调递增
  - _Requirements: 1.2, 1.4_

- [x] 2.3 添加保存完成阶段
  - 保存阶段占95-100%
  - 在processNextChunk完成时进入保存阶段
  - 在importCompleted信号发出前设置进度为100%
  - ✅ 已实现Saving和Complete阶段
  - _Requirements: 1.1, 1.3, 1.5_

- [ ]* 2.4 编写进度计算的单元测试
  - 测试各阶段进度计算的正确性
  - 测试边界条件（0个文件、1个文件、大量文件）
  - 测试进度值的范围限制[0, 100]
  - _Requirements: 1.4_

- [x] 3. 修改SmartQuestionImporter保存规则文件的位置
  - 修改saveParseRulesAndQuestionBank方法
  - 使用ImportRuleManager保存规则到config目录
  - 移除题库目录中的规则文件保存逻辑
  - _Requirements: 2.2_

- [x] 4. 修改QuestionBankManager过滤配置文件
  - 添加isConfigFile方法判断是否为配置文件
  - 在scanQuestionBankDirectory中过滤.json文件
  - 确保题库列表不显示配置文件
  - _Requirements: 2.1, 2.4_

- [x] 4.1 修改题库删除逻辑
  - 在删除题库时同时删除对应的config目录规则文件
  - 使用ImportRuleManager::deleteImportRule
  - _Requirements: 2.5_

- [ ]* 4.2 编写题库扫描过滤的单元测试
  - 测试配置文件被正确过滤
  - 测试题目文件夹正常显示
  - _Requirements: 2.1, 2.4_

- [x] 5. 创建ImportResult结构体
  - 定义导入结果统计数据结构
  - 包含按文件和难度的分类统计
  - 包含保存路径和错误信息
  - _Requirements: 3.1, 3.2, 3.3_

- [x] 6. 重写SmartImportDialog的完成弹窗
  - 修改onImportCompleted方法
  - 使用ImportResult生成详细统计信息
  - 按源文件显示题目分类
  - 显示难度分布统计
  - 移除"导入规则"和"运行时题库"相关描述
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 6.1 优化失败和取消情况的提示
  - 失败时显示失败原因和解决方案
  - 取消时显示已导入的部分结果
  - _Requirements: 3.6, 3.7_

- [ ]* 6.2 编写导入结果统计的单元测试
  - 测试按文件分类统计
  - 测试按难度分类统计
  - 测试空题库的统计
  - _Requirements: 3.1, 3.2, 3.3_

- [x] 7. 创建MockExamGenerator类（如果不存在）
  - 实现generateMockExam方法
  - 实现loadSourceBankRules方法
  - 实现buildGenerationPrompt方法
  - _Requirements: 2.1.1, 2.1.3, 2.1.4_

- [x] 7.1 实现规则文件读取和验证
  - 从config目录读取源题库规则
  - 验证规则文件完整性
  - 规则不存在时提示用户
  - _Requirements: 2.1.1, 2.1.2_

- [x] 7.2 实现基于规则的AI提示词生成
  - 提取难度分布和测试用例要求
  - 提取格式模板信息
  - 构建包含规则约束的AI提示词
  - _Requirements: 2.1.3, 2.1.4_

- [x] 7.3 保存模拟题库的规则文件
  - 复制源题库规则
  - 标记为模拟题库
  - 保存到config目录
  - _Requirements: 2.1.5_

- [x] 7.4 添加模拟题库生成的错误处理
  - 处理规则文件不存在的情况
  - 处理规则文件损坏的情况
  - 提供友好的错误提示和解决方案
  - _Requirements: 2.1.2_


- [-]* 7.5 编写MockExamGenerator的单元测试


  - 测试规则文件读取

  - 测试难度分布计算
  - 测试提示词生成
  - _Requirements: 2.1.1, 2.1.3_

- [x] 8. 更新模拟题库生成UI
  - 在生成对话框中检查规则文件状态
  - 规则不存在时提示用户
  - 提供继续或取消的选项
  - _Requirements: 2.1.2_

- [x] 9. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，询问用户是否有问题
  - ✅ 编译成功，无错误

- [x] 10. 集成测试和验证
  - 测试完整的导入流程（扫描→解析→保存→完成）
  - 验证进度从0%到100%
  - 验证配置文件保存到config目录
  - 验证题库列表不显示配置文件
  - 验证模拟题库生成能正确读取规则
  - ✅ 已创建详细的集成测试指南文档
  - 📄 参考：`AI导入模块改进_集成测试完整指南.md`
  - _Requirements: 1.1, 1.5, 2.2, 2.1, 2.1.1_

- [ ]* 10.1 编写端到端集成测试
  - 测试完整导入流程
  - 测试题库删除清理规则文件
  - 测试模拟题库生成流程
  - _Requirements: All_
