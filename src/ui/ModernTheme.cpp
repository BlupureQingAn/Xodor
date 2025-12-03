#include "ModernTheme.h"

QString ModernTheme::getCompleteStyle()
{
    return QString(R"(
        /* ========== 全局样式 ========== */
        * {
            font-family: "Segoe UI", "Microsoft YaHei UI", "PingFang SC", sans-serif;
            font-size: 10pt;
            outline: none;
        }
        
        QWidget {
            background-color: %1;
            color: %2;
        }
        
        /* ========== 主窗口 ========== */
        QMainWindow {
            background-color: %1;
        }
        
        QMainWindow::separator {
            background-color: %3;
            width: 2px;
            height: 2px;
        }
        
        QMainWindow::separator:hover {
            background-color: %4;
        }
        
        /* ========== 菜单栏 ========== */
        QMenuBar {
            background-color: %5;
            color: %2;
            border-bottom: 1px solid %3;
            padding: 6px;
        }
        
        QMenuBar::item {
            background-color: transparent;
            padding: 10px 18px;
            border-radius: 10px;
            margin: 0 2px;
        }
        
        QMenuBar::item:selected {
            background-color: %4;
            color: white;
        }
        
        QMenuBar::item:pressed {
            background-color: %6;
        }
        
        /* ========== 菜单 ========== */
        QMenu {
            background-color: %7;
            border: 1px solid %8;
            border-radius: 12px;
            padding: 8px;
        }
        
        QMenu::item {
            padding: 12px 32px 12px 20px;
            border-radius: 10px;
            margin: 2px 4px;
        }
        
        QMenu::item:selected {
            background-color: %4;
            color: white;
        }
        
        QMenu::separator {
            height: 1px;
            background-color: %3;
            margin: 8px 12px;
        }
        
        QMenu::icon {
            padding-left: 8px;
        }
        
        /* ========== 工具栏 ========== */
        QToolBar {
            background-color: %5;
            border-bottom: 1px solid %3;
            spacing: 6px;
            padding: 8px;
        }
        
        QToolButton {
            background-color: transparent;
            color: %2;
            border: 2px solid transparent;
            border-radius: 12px;
            padding: 10px 20px;
            font-weight: 500;
        }
        
        QToolButton:hover {
            background-color: %4;
            border-color: %4;
            color: white;
        }
        
        QToolButton:pressed {
            background-color: %6;
        }
        
        QToolButton:checked {
            background-color: %4;
            color: white;
        }
        
        /* ========== 按钮 ========== */
        QPushButton {
            background-color: %4;
            color: white;
            border: none;
            border-radius: 14px;
            padding: 14px 28px;
            font-weight: 600;
            font-size: 10pt;
        }
        
        QPushButton:hover {
            background-color: %9;
        }
        
        QPushButton:pressed {
            background-color: %6;
        }
        
        QPushButton:disabled {
            background-color: %10;
            color: %11;
        }
        
        QPushButton:flat {
            background-color: transparent;
            color: %2;
        }
        
        QPushButton:flat:hover {
            background-color: %10;
        }
        
        /* ========== 输入框 ========== */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            padding: 12px 16px;
            selection-background-color: %4;
        }
        
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: %4;
        }
        
        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
            background-color: %10;
            color: %11;
        }
        
        /* ========== 下拉框 ========== */
        QComboBox {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            padding: 12px 16px;
            min-height: 20px;
        }
        
        QComboBox:hover {
            border-color: %8;
        }
        
        QComboBox:focus {
            border-color: %4;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 30px;
        }
        
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 7px solid %2;
            margin-right: 10px;
        }
        
        QComboBox QAbstractItemView {
            background-color: %7;
            border: 1px solid %8;
            border-radius: 12px;
            padding: 6px;
            selection-background-color: %4;
            outline: none;
        }
        
        QComboBox QAbstractItemView::item {
            padding: 10px 16px;
            border-radius: 8px;
            margin: 2px;
        }
        
        QComboBox QAbstractItemView::item:hover {
            background-color: %10;
        }
        
        QComboBox QAbstractItemView::item:selected {
            background-color: %4;
            color: white;
        }
        
        /* ========== 数字输入框 ========== */
        QSpinBox, QDoubleSpinBox {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            padding: 12px 16px;
        }
        
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: %4;
        }
        
        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background-color: transparent;
            border: none;
            width: 20px;
        }
        
        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: %10;
            border-radius: 6px;
        }
        
        /* ========== 复选框 ========== */
        QCheckBox {
            color: %2;
            spacing: 10px;
        }
        
        QCheckBox::indicator {
            width: 22px;
            height: 22px;
            border: 2px solid %3;
            border-radius: 6px;
            background-color: %5;
        }
        
        QCheckBox::indicator:hover {
            border-color: %8;
        }
        
        QCheckBox::indicator:checked {
            background-color: %4;
            border-color: %4;
            image: url(:/icons/check.png);
        }
        
        QCheckBox::indicator:checked:hover {
            background-color: %9;
        }
        
        /* ========== 单选框 ========== */
        QRadioButton {
            color: %2;
            spacing: 10px;
        }
        
        QRadioButton::indicator {
            width: 22px;
            height: 22px;
            border: 2px solid %3;
            border-radius: 11px;
            background-color: %5;
        }
        
        QRadioButton::indicator:hover {
            border-color: %8;
        }
        
        QRadioButton::indicator:checked {
            background-color: %4;
            border-color: %4;
        }
        
        QRadioButton::indicator:checked::after {
            content: "";
            width: 10px;
            height: 10px;
            border-radius: 5px;
            background-color: white;
        }
        
        /* ========== 滚动条 ========== */
        QScrollBar:vertical {
            background-color: transparent;
            width: 12px;
            margin: 0;
        }
        
        QScrollBar::handle:vertical {
            background-color: %8;
            border-radius: 6px;
            min-height: 40px;
            margin: 2px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: %4;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: transparent;
            height: 12px;
            margin: 0;
        }
        
        QScrollBar::handle:horizontal {
            background-color: %8;
            border-radius: 6px;
            min-width: 40px;
            margin: 2px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: %4;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        
        /* ========== 进度条 ========== */
        QProgressBar {
            background-color: %5;
            border: 2px solid %3;
            border-radius: 14px;
            text-align: center;
            color: %2;
            height: 28px;
            font-weight: 600;
        }
        
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %4, stop:1 %9);
            border-radius: 12px;
        }
        
        /* ========== 标签页 ========== */
        QTabWidget::pane {
            border: 2px solid %3;
            border-radius: 12px;
            background-color: %5;
            top: -1px;
        }
        
        QTabBar::tab {
            background-color: %10;
            color: %12;
            padding: 12px 24px;
            border: 2px solid %3;
            border-bottom: none;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            margin-right: 4px;
            font-weight: 500;
        }
        
        QTabBar::tab:selected {
            background-color: %4;
            color: white;
            border-color: %4;
        }
        
        QTabBar::tab:hover:!selected {
            background-color: %7;
        }
        
        /* ========== 分组框 ========== */
        QGroupBox {
            color: %2;
            border: 2px solid %3;
            border-radius: 14px;
            margin-top: 16px;
            padding-top: 16px;
            font-weight: 600;
            font-size: 11pt;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 12px;
            background-color: %1;
        }
        
        /* ========== 列表 ========== */
        QListWidget, QListView {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        
        QListWidget::item, QListView::item {
            padding: 14px 18px;
            border-radius: 10px;
            margin: 3px 2px;
        }
        
        QListWidget::item:hover, QListView::item:hover {
            background-color: %10;
        }
        
        QListWidget::item:selected, QListView::item:selected {
            background-color: %4;
            color: white;
            font-weight: 500;
        }
        
        /* ========== 树形视图 ========== */
        QTreeWidget, QTreeView {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        
        QTreeWidget::item, QTreeView::item {
            padding: 10px;
            border-radius: 8px;
        }
        
        QTreeWidget::item:hover, QTreeView::item:hover {
            background-color: %10;
        }
        
        QTreeWidget::item:selected, QTreeView::item:selected {
            background-color: %4;
            color: white;
        }
        
        /* ========== 表格 ========== */
        QTableWidget, QTableView {
            background-color: %5;
            color: %2;
            border: 2px solid %3;
            border-radius: 12px;
            gridline-color: %3;
            outline: none;
        }
        
        QTableWidget::item, QTableView::item {
            padding: 10px;
        }
        
        QTableWidget::item:hover, QTableView::item:hover {
            background-color: %10;
        }
        
        QTableWidget::item:selected, QTableView::item:selected {
            background-color: %4;
            color: white;
        }
        
        QHeaderView::section {
            background-color: %7;
            color: %2;
            padding: 12px;
            border: none;
            border-right: 1px solid %3;
            border-bottom: 2px solid %3;
            font-weight: 600;
        }
        
        QHeaderView::section:first {
            border-top-left-radius: 12px;
        }
        
        QHeaderView::section:last {
            border-top-right-radius: 12px;
            border-right: none;
        }
        
        /* ========== 状态栏 ========== */
        QStatusBar {
            background-color: %5;
            color: %2;
            border-top: 1px solid %3;
            padding: 6px 12px;
        }
        
        /* ========== 停靠窗口 ========== */
        QDockWidget {
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
        }
        
        QDockWidget::title {
            background-color: %7;
            color: %2;
            padding: 12px;
            border-bottom: 1px solid %3;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            font-weight: 600;
            font-size: 11pt;
        }
        
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: transparent;
            border: none;
            padding: 6px;
            border-radius: 8px;
        }
        
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background-color: %4;
        }
        
        /* ========== 工具提示 ========== */
        QToolTip {
            background-color: %7;
            color: %2;
            border: 1px solid %8;
            border-radius: 10px;
            padding: 10px 14px;
            font-size: 9pt;
        }
        
        /* ========== 对话框 ========== */
        QDialog {
            background-color: %1;
        }
        
        /* ========== 消息框 ========== */
        QMessageBox {
            background-color: %1;
        }
        
        QMessageBox QLabel {
            color: %2;
            font-size: 10pt;
        }
        
        QMessageBox QPushButton {
            min-width: 100px;
        }
    )")
    .arg(Colors::BG_PRIMARY)           // %1
    .arg(Colors::TEXT_PRIMARY)         // %2
    .arg(Colors::BORDER)               // %3
    .arg(Colors::PRIMARY)              // %4
    .arg(Colors::BG_SECONDARY)         // %5
    .arg(Colors::PRIMARY_PRESSED)      // %6
    .arg(Colors::BG_ELEVATED)          // %7
    .arg(Colors::BORDER_LIGHT)         // %8
    .arg(Colors::PRIMARY_HOVER)        // %9
    .arg(Colors::BG_TERTIARY)          // %10
    .arg(Colors::TEXT_DISABLED)        // %11
    .arg(Colors::TEXT_SECONDARY);      // %12
}

QString ModernTheme::getDialogStyle()
{
    return QString(R"(
        QDialog {
            background-color: %1;
            border-radius: 16px;
        }
    )").arg(Colors::BG_PRIMARY);
}

QString ModernTheme::getCardStyle()
{
    return QString(R"(
        .card {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 16px;
            padding: 20px;
        }
        
        .card:hover {
            border-color: %3;
        }
    )")
    .arg(Colors::BG_SECONDARY)
    .arg(Colors::BORDER)
    .arg(Colors::BORDER_LIGHT);
}

QString ModernTheme::getModernButtonStyle(const QString &color)
{
    QString bgColor = Colors::PRIMARY;
    QString hoverColor = Colors::PRIMARY_HOVER;
    QString pressedColor = Colors::PRIMARY_PRESSED;
    
    if (color == "secondary") {
        bgColor = Colors::SECONDARY;
    } else if (color == "success") {
        bgColor = Colors::SUCCESS;
    } else if (color == "warning") {
        bgColor = Colors::WARNING;
    } else if (color == "error") {
        bgColor = Colors::ERROR;
    }
    
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 14px;
            padding: 14px 28px;
            font-weight: 600;
            font-size: 10pt;
        }
        
        QPushButton:hover {
            background-color: %2;
        }
        
        QPushButton:pressed {
            background-color: %3;
        }
    )")
    .arg(bgColor)
    .arg(hoverColor)
    .arg(pressedColor);
}

QString ModernTheme::getFloatingButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 28px;
            padding: 16px;
            min-width: 56px;
            min-height: 56px;
            font-size: 18pt;
        }
        
        QPushButton:hover {
            background-color: %2;
        }
        
        QPushButton:pressed {
            background-color: %3;
        }
    )")
    .arg(Colors::PRIMARY)
    .arg(Colors::PRIMARY_HOVER)
    .arg(Colors::PRIMARY_PRESSED);
}

QString ModernTheme::getProgressBarStyle()
{
    return QString(R"(
        QProgressBar {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 14px;
            text-align: center;
            color: %3;
            height: 28px;
            font-weight: 600;
        }
        
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %4, stop:1 %5);
            border-radius: 12px;
        }
    )")
    .arg(Colors::BG_SECONDARY)
    .arg(Colors::BORDER)
    .arg(Colors::TEXT_PRIMARY)
    .arg(Colors::PRIMARY)
    .arg(Colors::PRIMARY_HOVER);
}

QString ModernTheme::getTabWidgetStyle()
{
    return QString(R"(
        QTabWidget::pane {
            border: 2px solid %1;
            border-radius: 12px;
            background-color: %2;
            top: -1px;
        }
        
        QTabBar::tab {
            background-color: %3;
            color: %4;
            padding: 12px 24px;
            border: 2px solid %1;
            border-bottom: none;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            margin-right: 4px;
            font-weight: 500;
        }
        
        QTabBar::tab:selected {
            background-color: %5;
            color: white;
            border-color: %5;
        }
        
        QTabBar::tab:hover:!selected {
            background-color: %6;
        }
    )")
    .arg(Colors::BORDER)
    .arg(Colors::BG_SECONDARY)
    .arg(Colors::BG_TERTIARY)
    .arg(Colors::TEXT_SECONDARY)
    .arg(Colors::PRIMARY)
    .arg(Colors::BG_ELEVATED);
}

QString ModernTheme::getGroupBoxStyle()
{
    return QString(R"(
        QGroupBox {
            color: %1;
            border: 2px solid %2;
            border-radius: 14px;
            margin-top: 16px;
            padding-top: 16px;
            font-weight: 600;
            font-size: 11pt;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 12px;
            background-color: %3;
        }
    )")
    .arg(Colors::TEXT_PRIMARY)
    .arg(Colors::BORDER)
    .arg(Colors::BG_PRIMARY);
}

QString ModernTheme::getTooltipStyle()
{
    return QString(R"(
        QToolTip {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 10px;
            padding: 10px 14px;
            font-size: 9pt;
        }
    )")
    .arg(Colors::BG_ELEVATED)
    .arg(Colors::TEXT_PRIMARY)
    .arg(Colors::BORDER_LIGHT);
}
