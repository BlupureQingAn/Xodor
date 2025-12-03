// 这是showAIConnectionStatus函数的简化版本
// 请手动复制这个函数替换MainWindow.cpp中的showAIConnectionStatus函数

void MainWindow::showAIConnectionStatus(const AIConnectionStatus &status)
{
    // 总是显示AI配置对话框
    QTimer::singleShot(100, this, &MainWindow::checkAndSelectModel);
    
    // 简单的状态栏提示
    if (status.ollamaAvailable) {
        statusBar()->showMessage(QString("✓ Ollama已连接 - %1").arg(status.ollamaModel), 5000);
    } else if (status.cloudApiAvailable) {
        statusBar()->showMessage("✓ 云端API已连接", 5000);
    } else {
        statusBar()->showMessage("⚠ AI服务未配置（不影响刷题功能）", 0);
    }
}
