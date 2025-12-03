#include <QApplication>
#include <QMessageBox>
#include "ui/MainWindow.h"
#include "utils/ConfigManager.h"
#include "utils/CrashHandler.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("CodePracticeSystem");
    app.setApplicationVersion("1.7.2");
    app.setOrganizationName("CodePractice");
    
    // 安装崩溃处理器
    CrashHandler::install();
    
    try {
        // 初始化配置
        ConfigManager::instance().load();
        
        MainWindow window;
        window.show();
        
        return app.exec();
        
    } catch (const std::exception &e) {
        // 捕获标准异常
        QString details = QString("异常类型：std::exception\n"
                                 "错误信息：%1").arg(e.what());
        CrashHandler::showCrashDialog("程序启动失败", details);
        return 1;
        
    } catch (...) {
        // 捕获所有其他异常
        CrashHandler::showCrashDialog("程序启动失败", "未知异常");
        return 1;
    }
}
