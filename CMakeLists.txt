cmake_minimum_required(VERSION 3.16)
project(CodePracticeSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 支持中文字符编译
if(MSVC)
    # MSVC 编译器
    add_compile_options(/utf-8)
    add_compile_options(/source-charset:utf-8)
    add_compile_options(/execution-charset:utf-8)
elseif(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
    # MinGW/GCC 编译器
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-fexec-charset=UTF-8)
endif()

# Qt 自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC_MOC_OPTIONS "-DMINGW_HAS_SECURE_API=1")
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

# 设置 Qt 路径
set(CMAKE_PREFIX_PATH "F:/Qt/qt/6.10.0/mingw_64")

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network PrintSupport)

# QScintilla 配置
set(QSCINTILLA_INCLUDE_DIR "F:/Qt/qt/6.10.0/mingw_64/include")
set(QSCINTILLA_LIBRARY "F:/Qt/qt/6.10.0/mingw_64/lib/libqscintilla2_qt6.a")

# 源文件
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/QuestionPanel.cpp
    src/ui/CodeEditor.cpp
    src/ui/AIAnalysisPanel.cpp
    src/ui/ImportDialog.cpp
    src/ui/HistoryWidget.cpp
    src/ui/QuestionListWidget.cpp
    src/ui/StyleManager.cpp
    src/ui/ModernTheme.cpp
    src/ui/WrongQuestionWidget.cpp
    src/ui/SettingsDialog.cpp
    src/ui/OriginalQuestionDialog.cpp
    src/ui/PracticeWidget.cpp
    src/ui/AIImportDialog.cpp
    src/ui/QuestionBankManagerDialog.cpp
    src/ui/SmartImportDialog.cpp
    src/ui/ExamGeneratorDialog.cpp
    src/ui/CodeVersionDialog.cpp
    src/ui/AIAssistantPanel.cpp
    src/ui/ChatBubbleDelegate.cpp
    src/ui/ChatHistoryDialog.cpp
    src/ui/ErrorListWidget.cpp
    src/ui/MockExamManagerDialog.cpp
    src/ui/ExamReportDialog.cpp
    src/core/QuestionBank.cpp
    src/core/QuestionBankManager.cpp
    src/core/WrongQuestionBook.cpp
    src/core/Question.cpp
    src/core/QuestionProgress.cpp
    src/core/ProgressManager.cpp
    src/core/AutoSaver.cpp
    src/core/CompilerRunner.cpp
    src/core/CodeVersionManager.cpp
    src/core/ExamSession.cpp
    src/core/ExamReportGenerator.cpp
    src/ai/AIService.cpp
    src/ai/OllamaClient.cpp
    src/ai/CloudAIClient.cpp
    src/ai/QuestionParser.cpp
    src/ai/FineTuneManager.cpp
    src/ai/SmartQuestionImporter.cpp
    src/ai/UniversalQuestionParser.cpp
    src/ai/QuestionBankAnalyzer.cpp
    src/ai/AIAssistant.cpp
    src/ai/MockExamGenerator.cpp
    src/ai/TestDataGenerator.cpp
    src/utils/FileManager.cpp
    src/utils/ConfigManager.cpp
    src/utils/CompilerDetector.cpp
    src/utils/SessionManager.cpp
    src/utils/CodeTemplateManager.cpp
    src/utils/ErrorHandler.cpp
    src/utils/CrashHandler.cpp
    src/utils/AIConnectionChecker.cpp
    src/utils/AutoSaveManager.cpp
    src/utils/SyntaxChecker.cpp
)

# 头文件
set(HEADERS
    src/ui/MainWindow.h
    src/ui/QuestionPanel.h
    src/ui/CodeEditor.h
    src/ui/AIAnalysisPanel.h
    src/ui/ImportDialog.h
    src/ui/HistoryWidget.h
    src/ui/QuestionListWidget.h
    src/ui/StyleManager.h
    src/ui/WrongQuestionWidget.h
    src/ui/SettingsDialog.h
    src/ui/OriginalQuestionDialog.h
    src/ui/PracticeWidget.h
    src/ui/AIImportDialog.h
    src/ui/QuestionBankManagerDialog.h
    src/ui/SmartImportDialog.h
    src/ui/ExamGeneratorDialog.h
    src/ui/CodeVersionDialog.h
    src/ui/AIAssistantPanel.h
    src/ui/ChatBubbleDelegate.h
    src/ui/ChatHistoryDialog.h
    src/ui/ErrorListWidget.h
    src/ui/MockExamManagerDialog.h
    src/ui/ExamReportDialog.h
    src/core/QuestionBank.h
    src/core/QuestionBankManager.h
    src/core/WrongQuestionBook.h
    src/core/Question.h
    src/core/QuestionProgress.h
    src/core/ProgressManager.h
    src/core/AutoSaver.h
    src/core/CompilerRunner.h
    src/core/CodeVersionManager.h
    src/core/ExamSession.h
    src/core/ExamReportGenerator.h
    src/ai/AIService.h
    src/ai/OllamaClient.h
    src/ai/CloudAIClient.h
    src/ai/QuestionParser.h
    src/ai/FineTuneManager.h
    src/ai/SmartQuestionImporter.h
    src/ai/UniversalQuestionParser.h
    src/ai/QuestionBankAnalyzer.h
    src/ai/AIAssistant.h
    src/ai/MockExamGenerator.h
    src/ai/TestDataGenerator.h
    src/utils/FileManager.h
    src/utils/ConfigManager.h
    src/utils/CompilerDetector.h
    src/utils/SessionManager.h
    src/utils/CodeTemplateManager.h
    src/utils/ErrorHandler.h
    src/utils/CrashHandler.h
    src/utils/AIConnectionChecker.h
    src/utils/AutoSaveManager.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${QSCINTILLA_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
    ${QSCINTILLA_LIBRARY}
)

# Windows 特定设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON  # 隐藏控制台窗口（正式发布版本）
    )
    
    # 确保 Windows 下正确处理 UTF-8
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        UNICODE
        _UNICODE
    )
endif()

# 输出信息
message(STATUS "Qt6 found: ${Qt6_DIR}")
message(STATUS "QScintilla library: ${QSCINTILLA_LIBRARY}")
message(STATUS "QScintilla include: ${QSCINTILLA_INCLUDE_DIR}")
