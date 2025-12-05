# 数据文件管理说明

## 📁 目录结构

### 项目根目录 `data/`（源数据）
```
data/
├── sample_questions/     # 示例题目（模板，每次编译会更新）
├── user_answers/         # 用户答案模板
├── 原始题库/             # 导入的原始题目文件（只读备份）
├── 基础题库/             # AI解析后的JSON题库
├── config/              # 配置文件模板
└── config.json          # 默认配置
```

### 构建目录 `build/data/`（运行时数据）
```
build/data/
├── sample_questions/     # 示例题目
├── user_answers/         # 用户的答题记录 ⚠️ 受保护
├── question_banks/       # 分类题库 ⚠️ 受保护
├── 原始题库/             # 原始题目备份
├── 基础题库/             # 基础题库
├── config/              # 配置文件
├── config.json          # 配置文件
└── last_session.json    # 会话记录 ⚠️ 受保护
```

## 🔄 数据同步策略

### 编译时自动同步
每次编译后，CMake会智能同步数据文件：

#### ✅ 总是更新（覆盖）
- `sample_questions/` - 示例题目模板

#### 🆕 仅首次复制（不覆盖）
- `原始题库/` - 原始题库文件
- `基础题库/` - 基础题库文件
- `config/` - 配置目录
- `config.json` - 默认配置

#### 🛡️ 永不覆盖（受保护）
- `user_answers/` - 用户答题记录
- `question_banks/` - 用户题库
- `last_session.json` - 会话恢复数据

## 💡 使用场景

### 场景1：开发调试
在Qt Creator中运行时：
- 软件从 `build/data/` 加载题库
- 用户答题记录保存在 `build/data/user_answers/`
- 重新编译不会丢失答题记录 ✅

### 场景2：更新题库模板
如果你在 `data/sample_questions/` 中更新了示例题目：
1. 重新编译项目
2. 新的示例会自动同步到 `build/data/sample_questions/`
3. 用户数据不受影响 ✅

### 场景3：添加新题库
如果你在 `data/基础题库/` 中添加了新题库：
1. 如果 `build/data/基础题库/` 不存在，会自动复制
2. 如果已存在，不会覆盖（保护用户修改）
3. 需要手动复制或删除 `build/data/基础题库/` 后重新编译

### 场景4：重置数据
如果想重置所有数据到初始状态：
```bash
# 删除构建目录的数据
rmdir /s /q build\data

# 重新编译
cmake --build build
```

## 🚀 发布部署

发布软件时，`deploy_windows.bat` 会：
1. 复制 `build/data/` 到发布目录
2. 用户首次运行时使用这些初始数据
3. 之后的数据修改都在用户的安装目录中

## ⚙️ 技术实现

数据同步由 `cmake/copy_data_if_needed.cmake` 脚本实现：
- 在每次编译后自动执行（POST_BUILD）
- 智能判断文件是否存在
- 保护用户数据不被覆盖
- 输出详细的同步日志

## 📝 注意事项

1. **不要直接修改 `build/data/`**：这些文件可能被覆盖
2. **模板文件放在 `data/`**：会自动同步到构建目录
3. **用户数据自动保护**：答题记录和会话不会丢失
4. **题库更新需谨慎**：已存在的题库不会自动更新

## 🔧 自定义保护规则

如果需要修改保护规则，编辑 `cmake/copy_data_if_needed.cmake`：

```cmake
# 添加需要保护的目录
set(PROTECTED_DIRS
    "user_answers"
    "question_banks"
    "your_custom_dir"  # 添加你的目录
)

# 添加需要保护的文件
set(PROTECTED_FILES
    "last_session.json"
    "your_config.json"  # 添加你的文件
)

# 添加总是更新的目录
set(TEMPLATE_DIRS
    "sample_questions"
    "your_templates"  # 添加你的模板目录
)
```
