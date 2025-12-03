QT += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# QScintilla库
LIBS += -L$$[QT_INSTALL_LIBS] -lqscintilla2_qt6
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/Qsci
INCLUDEPATH += F:/Qt/qt/6.10.0/mingw_64/include/Qsci

# 源文件
SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/QuestionPanel.cpp \
    src/ui/CodeEditor.cpp \
    src/ui/AIAnalysisPanel.cpp \
    src/ui/ImportDialog.cpp \
    src/ui/HistoryWidget.cpp \
    src/core/QuestionBank.cpp \
    src/core/Question.cpp \
    src/core/AutoSaver.cpp \
    src/core/CompilerRunner.cpp \
    src/ai/AIService.cpp \
    src/ai/OllamaClient.cpp \
    src/ai/CloudAIClient.cpp \
    src/ai/QuestionParser.cpp \
    src/ai/FineTuneManager.cpp \
    src/utils/FileManager.cpp \
    src/utils/ConfigManager.cpp

# 头文件
HEADERS += \
    src/ui/MainWindow.h \
    src/ui/QuestionPanel.h \
    src/ui/CodeEditor.h \
    src/ui/AIAnalysisPanel.h \
    src/ui/ImportDialog.h \
    src/ui/HistoryWidget.h \
    src/core/QuestionBank.h \
    src/core/Question.h \
    src/core/AutoSaver.h \
    src/core/CompilerRunner.h \
    src/ai/AIService.h \
    src/ai/OllamaClient.h \
    src/ai/CloudAIClient.h \
    src/ai/QuestionParser.h \
    src/ai/FineTuneManager.h \
    src/utils/FileManager.h \
    src/utils/ConfigManager.h

# 包含路径
INCLUDEPATH += src

# 默认部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui
