# IDE CMake配置修复

## 问题
Kiro IDE显示错误：
```
Bad CMake executable: "". Check to make sure it is installed or the value of the "cmake.cmakePath" setting contains the correct path
```

## 原因
IDE找不到CMake可执行文件的路径配置。

## 解决方案

### 1. 创建VSCode配置文件
已创建 `.vscode/settings.json` 文件，内容如下：

```json
{
    "cmake.cmakePath": "cmake",
    "cmake.configureOnOpen": false,
    "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

### 2. 配置说明
- `cmake.cmakePath`: 指定CMake可执行文件路径，使用 "cmake" 表示使用系统PATH中的cmake
- `cmake.configureOnOpen`: 设为false，避免每次打开项目自动配置
- `cmake.buildDirectory`: 指定构建目录为 `build`

### 3. 如果仍然报错
如果IDE仍然找不到CMake，可以手动指定完整路径：

1. 找到CMake安装路径：
   ```cmd
   where cmake
   ```

2. 修改 `.vscode/settings.json`，将 `cmake.cmakePath` 改为完整路径：
   ```json
   {
       "cmake.cmakePath": "C:/Program Files/CMake/bin/cmake.exe"
   }
   ```

### 4. 重启IDE
配置文件创建后，建议重启Kiro IDE使配置生效。

## 验证
CMake已正确安装并可用：
```
cmake version 3.30.5
```

项目可以正常编译：
```cmd
cmake --build build --config Release
```

## 注意
这个错误不影响命令行编译，只是IDE的智能提示功能需要CMake配置。
