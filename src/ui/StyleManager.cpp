#include "StyleManager.h"

QString StyleManager::getApplicationStyle()
{
    return QString(R"(
        * {
            font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
            font-size: 10pt;
        }
        
        QWidget {
            background-color: %1;
            color: %2;
        }
        
        QMainWindow {
            background-color: %1;
        }
        
        QMainWindow::separator {
            background-color: %3;
            width: 1px;
            height: 1px;
        }
        
        QMainWindow::separator:hover {
            background-color: %4;
        }
    )")
    .arg(COLOR_BG_PRIMARY)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_ACCENT_RED);
}

QString StyleManager::getMenuBarStyle()
{
    return QString(R"(
        QMenuBar {
            background-color: %1;
            color: %2;
            border-bottom: 1px solid %3;
            padding: 4px;
        }
        
        QMenuBar::item {
            background-color: transparent;
            padding: 8px 16px;
            border-radius: 8px;
        }
        
        QMenuBar::item:selected {
            background-color: %4;
            color: white;
        }
        
        QMenuBar::item:pressed {
            background-color: %5;
        }
        
        QMenu {
            background-color: %6;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 4px;
        }
        
        QMenu::item {
            padding: 10px 28px 10px 16px;
            border-radius: 8px;
            margin: 3px;
        }
        
        QMenu::item:selected {
            background-color: %4;
            color: white;
        }
        
        QMenu::separator {
            height: 1px;
            background-color: %3;
            margin: 4px 8px;
        }
    )")
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_ACCENT_RED)
    .arg(COLOR_ACCENT_RED_HOVER)
    .arg(COLOR_BG_TERTIARY);
}

QString StyleManager::getToolBarStyle()
{
    return QString(R"(
        QToolBar {
            background-color: %1;
            border-bottom: 1px solid %2;
            spacing: 8px;
            padding: 6px;
        }
        
        QToolButton {
            background-color: transparent;
            color: %3;
            border: 1px solid transparent;
            border-radius: 10px;
            padding: 10px 20px;
            font-weight: 500;
        }
        
        QToolButton:hover {
            background-color: %4;
            border-color: %5;
            color: white;
        }
        
        QToolButton:pressed {
            background-color: %5;
        }
        
        QToolBar::separator {
            background-color: %2;
            width: 1px;
            margin: 4px 8px;
        }
    )")
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_ACCENT_RED)
    .arg(COLOR_ACCENT_RED_HOVER);
}

QString StyleManager::getButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 12px;
            padding: 12px 24px;
            font-weight: 500;
            font-size: 10pt;
        }
        
        QPushButton:hover {
            background-color: %2;
        }
        
        QPushButton:pressed {
            background-color: %3;
        }
        
        QPushButton:disabled {
            background-color: %4;
            color: %5;
        }
    )")
    .arg(COLOR_ACCENT_RED)
    .arg(COLOR_ACCENT_RED_HOVER)
    .arg(COLOR_ACCENT_RED_DARK)
    .arg(COLOR_BG_TERTIARY)
    .arg(COLOR_TEXT_DISABLED);
}

QString StyleManager::getScrollBarStyle()
{
    return QString(R"(
        QScrollBar:vertical {
            background-color: %1;
            width: 14px;
            border-radius: 7px;
            margin: 2px;
        }
        
        QScrollBar::handle:vertical {
            background-color: %2;
            border-radius: 7px;
            min-height: 40px;
            margin: 2px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: %3;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: %1;
            height: 14px;
            border-radius: 7px;
            margin: 2px;
        }
        
        QScrollBar::handle:horizontal {
            background-color: %2;
            border-radius: 7px;
            min-width: 40px;
            margin: 2px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: %3;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )")
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_BORDER_LIGHT)
    .arg(COLOR_ACCENT_RED);
}

QString StyleManager::getQuestionPanelStyle()
{
    return QString(R"(
        QTextBrowser {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 12px;
            padding: 20px;
            selection-background-color: %4;
        }
        
        %5
    )")
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_ACCENT_RED)
    .arg(getScrollBarStyle());
}

QString StyleManager::getCodeEditorStyle()
{
    return QString(R"(
        QsciScintilla {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 12px;
            selection-background-color: %4;
        }
    )")
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_ACCENT_RED);
}

QString StyleManager::getAIPanelStyle()
{
    return getQuestionPanelStyle();
}

QString StyleManager::getQuestionBankPanelStyle()
{
    return QString(R"(
        QWidget {
            background-color: %1;
            border-right: 1px solid %2;
        }
        
        QLabel {
            color: %3;
            font-weight: 600;
            font-size: 9pt;
            padding: 8px 12px 4px 12px;
        }
        
        QLineEdit {
            background-color: %4;
            color: %3;
            border: 1px solid %2;
            border-radius: 10px;
            padding: 10px 16px;
            margin: 4px 8px;
        }
        
        QLineEdit:focus {
            border-color: %5;
        }
        
        QComboBox {
            background-color: %4;
            color: %3;
            border: 1px solid %2;
            border-radius: 10px;
            padding: 10px 16px;
            margin: 4px 8px;
        }
        
        QComboBox:hover {
            border-color: %5;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid %3;
            margin-right: 8px;
        }
        
        QListWidget {
            background-color: %4;
            color: %3;
            border: 1px solid %2;
            border-radius: 12px;
            padding: 6px;
            margin: 4px 8px;
            outline: none;
        }
        
        QListWidget::item {
            padding: 12px 16px;
            border-radius: 8px;
            margin: 3px 2px;
        }
        
        QListWidget::item:hover {
            background-color: %6;
        }
        
        QListWidget::item:selected {
            background-color: %5;
            color: white;
            font-weight: 500;
        }
        
        %7
    )")
    .arg(COLOR_BG_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_TEXT_PRIMARY)
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_ACCENT_RED)
    .arg(COLOR_BG_TERTIARY)
    .arg(getScrollBarStyle());
}

QString StyleManager::getMainWindowStyle()
{
    return QString(R"(
        QMainWindow {
            background-color: %1;
        }
        
        QSplitter::handle {
            background-color: %2;
            width: 1px;
            height: 1px;
        }
        
        QSplitter::handle:hover {
            background-color: %3;
        }
        
        QStatusBar {
            background-color: %4;
            color: %5;
            border-top: 1px solid %2;
            padding: 4px 8px;
        }
        
        QDockWidget {
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
        }
        
        QDockWidget::title {
            background-color: %4;
            color: %5;
            padding: 8px;
            border-bottom: 1px solid %2;
            font-weight: 600;
        }
        
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: transparent;
            border: none;
            padding: 4px;
        }
        
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background-color: %3;
            border-radius: 4px;
        }
    )")
    .arg(COLOR_BG_PRIMARY)
    .arg(COLOR_BORDER)
    .arg(COLOR_ACCENT_RED)
    .arg(COLOR_BG_SECONDARY)
    .arg(COLOR_TEXT_PRIMARY);
}
