#ifndef MODERNTHEME_H
#define MODERNTHEME_H

#include <QString>
#include <QColor>

// 现代化主题管理器
class ModernTheme
{
public:
    // 获取完整应用样式
    static QString getCompleteStyle();
    
    // 组件样式
    static QString getDialogStyle();
    static QString getCardStyle();
    static QString getInputStyle();
    static QString getProgressBarStyle();
    static QString getTabWidgetStyle();
    static QString getGroupBoxStyle();
    static QString getCheckBoxStyle();
    static QString getRadioButtonStyle();
    static QString getSpinBoxStyle();
    static QString getSliderStyle();
    static QString getTooltipStyle();
    static QString getMessageBoxStyle();
    
    // 特殊组件
    static QString getModernButtonStyle(const QString &color = "primary");
    static QString getIconButtonStyle();
    static QString getFloatingButtonStyle();
    static QString getBadgeStyle();
    static QString getChipStyle();
    
    // 动画效果
    static QString getHoverAnimation();
    static QString getClickAnimation();
    
    // 颜色方案
    struct Colors {
        // 背景色
        static constexpr const char* BG_PRIMARY = "#1e1e1e";
        static constexpr const char* BG_SECONDARY = "#252525";
        static constexpr const char* BG_TERTIARY = "#2d2d2d";
        static constexpr const char* BG_ELEVATED = "#323232";
        static constexpr const char* BG_OVERLAY = "#3a3a3a";
        
        // 主题色
        static constexpr const char* PRIMARY = "#e63946";        // 现代红
        static constexpr const char* PRIMARY_HOVER = "#f25c66";
        static constexpr const char* PRIMARY_PRESSED = "#d62839";
        static constexpr const char* PRIMARY_LIGHT = "#ff6b7a";
        static constexpr const char* PRIMARY_DARK = "#c5303d";
        
        // 辅助色
        static constexpr const char* SECONDARY = "#457b9d";      // 蓝色
        static constexpr const char* SUCCESS = "#06d6a0";        // 绿色
        static constexpr const char* WARNING = "#ffd166";        // 黄色
        static constexpr const char* ERROR = "#ef476f";          // 粉红
        static constexpr const char* INFO = "#118ab2";           // 青色
        
        // 文本色
        static constexpr const char* TEXT_PRIMARY = "#f1f1f1";
        static constexpr const char* TEXT_SECONDARY = "#b8b8b8";
        static constexpr const char* TEXT_TERTIARY = "#8a8a8a";
        static constexpr const char* TEXT_DISABLED = "#5a5a5a";
        static constexpr const char* TEXT_INVERSE = "#1e1e1e";
        
        // 边框色
        static constexpr const char* BORDER = "#3a3a3a";
        static constexpr const char* BORDER_LIGHT = "#4a4a4a";
        static constexpr const char* BORDER_FOCUS = "#e63946";
        
        // 阴影
        static constexpr const char* SHADOW_LIGHT = "rgba(0, 0, 0, 0.1)";
        static constexpr const char* SHADOW_MEDIUM = "rgba(0, 0, 0, 0.2)";
        static constexpr const char* SHADOW_HEAVY = "rgba(0, 0, 0, 0.3)";
    };
    
    // 圆角半径
    struct Radius {
        static constexpr int SMALL = 8;
        static constexpr int MEDIUM = 12;
        static constexpr int LARGE = 16;
        static constexpr int XLARGE = 20;
        static constexpr int ROUND = 999;
    };
    
    // 间距
    struct Spacing {
        static constexpr int XS = 4;
        static constexpr int SM = 8;
        static constexpr int MD = 12;
        static constexpr int LG = 16;
        static constexpr int XL = 24;
        static constexpr int XXL = 32;
    };
};

#endif // MODERNTHEME_H
