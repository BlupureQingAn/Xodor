#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

class StyleManager
{
public:
    static QString getApplicationStyle();
    static QString getMainWindowStyle();
    static QString getCodeEditorStyle();
    static QString getQuestionPanelStyle();
    static QString getAIPanelStyle();
    static QString getQuestionBankPanelStyle();
    static QString getToolBarStyle();
    static QString getMenuBarStyle();
    static QString getButtonStyle();
    static QString getScrollBarStyle();
    
private:
    // 颜色定义 - 深灰黑与深红主题
    static constexpr const char* COLOR_BG_PRIMARY = "#242424";      // 主背景 - 深灰黑
    static constexpr const char* COLOR_BG_SECONDARY = "#2d2d2d";    // 次背景 - 中灰
    static constexpr const char* COLOR_BG_TERTIARY = "#363636";     // 三级背景 - 浅灰
    static constexpr const char* COLOR_ACCENT_RED = "#660000";      // 主题深红色
    static constexpr const char* COLOR_ACCENT_RED_HOVER = "#880000"; // 深红悬停
    static constexpr const char* COLOR_ACCENT_RED_DARK = "#440000";  // 更深红
    static constexpr const char* COLOR_TEXT_PRIMARY = "#e8e8e8";    // 主文本
    static constexpr const char* COLOR_TEXT_SECONDARY = "#b0b0b0";  // 次文本
    static constexpr const char* COLOR_TEXT_DISABLED = "#707070";   // 禁用文本
    static constexpr const char* COLOR_BORDER = "#3a3a3a";          // 边框
    static constexpr const char* COLOR_BORDER_LIGHT = "#4a4a4a";    // 浅边框
    static constexpr const char* COLOR_SUCCESS = "#5a9a5a";         // 成功绿色
    static constexpr const char* COLOR_WARNING = "#d4a04e";         // 警告橙色
    static constexpr const char* COLOR_ERROR = "#880000";           // 错误红色
};

#endif // STYLEMANAGER_H
