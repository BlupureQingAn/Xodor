# AI导入改为MD拆分 - 实施进度

## 已完成

### 阶段1：核心功能 ✅
- [x] 创建MarkdownQuestionParser类 (src/core/MarkdownQuestionParser.h/cpp)
- [x] 实现Front Matter解析
- [x] 实现测试用例解析  
- [x] Question类添加MD支持方法
- [x] 添加到CMakeLists.txt

## 进行中

### 阶段2：AI拆分（核心）✅
- [x] 修改SmartQuestionImporter
  - [x] 修改保存逻辑为MD格式
  - [x] 使用Question::saveAsMarkdown()

### 阶段3：适配现有功能
- [x] QuestionBank加载MD文件（优先MD，兼容JSON）
- [ ] QuestionEditorDialog保存MD
- [ ] 测试编译

## 待完成

### 阶段4：测试和优化
- [ ] 测试拆分功能
- [ ] 测试MD加载
- [ ] 性能优化

### 阶段4：测试和优化
- [ ] 测试拆分功能
- [ ] 测试MD加载
- [ ] 性能优化

## 技术决策

1. **保留现有JSON支持**：向后兼容，QuestionBank优先加载MD，如果没有则加载JSON
2. **Front Matter格式**：使用YAML格式，简单的key: value解析
3. **测试用例格式**：支持多种常见Markdown格式
4. **文件命名**：使用题目标题，特殊字符转义为下划线

## 下一步

继续实施阶段2：修改SmartQuestionImporter的AI拆分流程
