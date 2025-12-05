# 变量名补全功能实现计划

## 需求描述
用户希望在输入变量名时，能够自动补全当前代码中已定义的变量。

## 示例场景
```cpp
vector<pair<int,int>> point(m);
for(int i=0;i<m;i++){
    cin>>p  // 应该补全为 point
}
```

## 实现方案

### 1. 变量提取
从当前代码中提取所有已定义的变量名：
- 局部变量声明：`int n`, `vector<int> v`, `string s`
- 函数参数
- 全局变量
- 循环变量：`for(int i=0; ...)`

### 2. 触发时机
- 用户输入字母或下划线时
- 当前单词至少2个字符
- 与关键字补全合并

### 3. 补全逻辑
1. 提取当前输入的前缀（如 `p`）
2. 从代码中提取所有变量名
3. 过滤出以前缀开头的变量名
4. 与关键字补全合并显示

### 4. 变量提取正则表达式
```cpp
// 基本类型变量：int n, double x
R"(\b(?:int|long|short|char|bool|float|double|void|size_t|auto)\s+(\w+))"

// STL 容器变量：vector<int> v, map<int,int> m
R"(\b(?:vector|map|set|string|pair|queue|stack|deque|list|array)\s*<[^>]+>\s+(\w+))"

// 循环变量：for(int i=0; ...)
R"(\bfor\s*\(\s*(?:int|long|short|size_t|auto)\s+(\w+))"

// 范围for：for(auto x : v)
R"(\bfor\s*\(\s*(?:auto|const\s+auto)\s*&?\s*(\w+)\s*:)"
```

## 实现步骤

1. ✅ 创建 `extractVariableNames()` 函数
2. ✅ 在 `handleKeywordCompletion()` 中调用
3. ✅ 合并变量名和关键字
4. ✅ 去重并排序
5. ✅ 测试

## 优先级
🔴 高 - 这是一个非常实用的功能
