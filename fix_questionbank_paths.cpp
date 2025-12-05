// 题库路径修复补丁
// 此文件包含需要应用到 QuestionBankManagerDialog.cpp 的修复

// 在 onViewBankDetails() 函数中，在 "// 计算完成度" 之后添加：
/*
    // 获取绝对路径用于显示
    QDir currentDir;
    QString absolutePath = currentDir.absoluteFilePath(info.path);
    QString absoluteOriginalPath = info.originalPath.isEmpty() ? "无" : currentDir.absoluteFilePath(info.originalPath);
*/

// 然后将 details 字符串中的最后两个 .arg() 替换为：
/*
    .arg(absolutePath)
    .arg(absoluteOriginalPath);
*/

// 具体修改位置：src/ui/QuestionBankManagerDialog.cpp 第510-550行
