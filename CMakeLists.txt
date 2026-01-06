cmake_minimum_required(VERSION 3.16)
project(CodePracticeSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译器特定的标志来解决Qt 6.9.2兼容性问题
if(MINGW)
    # MinGW特定设置 - 解决Qt 6.9.2编译问题
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-unused-parameter)
    # 解决Qt 6.9.2的编译器检测问题
    add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)
endif()

# ============================================
# UTF-8编码配置 - 支持中文字符编译
# ============================================
# 确保源文件、编译器输出、运行时都使用UTF-8编码

# 设置CMake使用UTF-8
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexec-charset=UTF-8")

if(MSVC)
    # MSVC 编译器
    add_compile_options(/utf-8)  # 源文件和执行字符集都使用UTF-8
    add_compile_options(/Zc:__cplusplus)  # 正确报告C++版本
    add_compile_options(/permissive-)  # 启用严格的C++标准符合性
    add_compile_definitions(_UNICODE UNICODE)
elseif(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
    # MinGW/GCC 编译器
    add_compile_options(-finput-charset=UTF-8)  # 源文件编码
    add_compile_options(-fexec-charset=UTF-8)   # 执行字符集
    add_compile_options(-fwide-exec-charset=UTF-8)  # 宽字符集
    
    # 确保字符串字面量使用UTF-8
    add_compile_definitions(_UNICODE UNICODE)
    
    # 链接器选项 - 支持UTF-8
    add_link_options(-municode)
    
    # 解决Qt 6.9.2的特定问题
    add_compile_definitions(QT_NO_DEPRECATED_WARNINGS)
    add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)
endif()

# Qt 自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC_MOC_OPTIONS "-DMINGW_HAS_SECURE_API=1")
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

# 设置 Qt 路径 - 强制使用Qt 6.9.2
set(CMAKE_PREFIX_PATH "F:/Qt/6.9.2/mingw_64" CACHE PATH "Qt installation path" FORCE)
set(Qt6_DIR "F:/Qt/6.9.2/mingw_64/lib/cmake/Qt6" CACHE PATH "Qt6 CMake directory" FORCE)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network PrintSupport)

# QScintilla 配置
set(QSCINTILLA_INCLUDE_DIR "F:/Qt/6.9.2/mingw_64/include")
set(QSCINTILLA_LIBRARY "F:/Qt/6.9.2/mingw_64/lib/libqscintilla2_qt6.a")

# Windows 资源文件（图标）
if(WIN32)
    set(APP_ICON_RC "${CMAKE_SOURCE_DIR}/resources/app_icon.rc")
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/QuestionPanel.cpp
    src/ui/CodeEditor.cpp
    src/ui/AIAnalysisPanel.cpp
    src/ui/ImportDialog.cpp
    src/ui/HistoryWidget.cpp
    src/ui/QuestionBankPanel.cpp
    src/ui/QuestionBankTreeWidget.cpp
    src/ui/StyleManager.cpp
    src/ui/ModernTheme.cpp
    src/ui/WrongQuestionWidget.cpp
    src/ui/SettingsDialog.cpp
    src/ui/OriginalQuestionDialog.cpp
    src/ui/PracticeWidget.cpp
    src/ui/PracticeStatsPanel.cpp
    src/ui/AIImportDialog.cpp
    src/ui/QuestionBankManagerDialog.cpp
    src/ui/SmartImportDialog.cpp
    src/ui/ExamGeneratorDialog.cpp
    src/ui/CodeVersionDialog.cpp
    src/ui/AIAssistantPanel.cpp
    src/ui/AIJudgeProgressDialog.cpp
    src/ui/ChatBubbleDelegate.cpp
    src/ui/ChatBubbleWidget.cpp
    src/ui/ChatHistoryDialog.cpp
    src/ui/ErrorListWidget.cpp
    src/ui/MockExamManagerDialog.cpp
    src/ui/ExamReportDialog.cpp
    src/core/QuestionBank.cpp
    src/core/QuestionBankManager.cpp
    src/core/WrongQuestionBook.cpp
    src/core/Question.cpp
    src/core/MarkdownQuestionParser.cpp
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
    src/ai/TestCaseFixer.cpp
    src/ai/AIJudge.cpp
    src/ui/TestCaseFixerDialog.cpp
    src/ui/BatchTestCaseFixerDialog.cpp
    src/ui/QuestionEditorDialog.cpp
    src/utils/FileManager.cpp
    src/utils/OperationHistory.cpp
    src/utils/ConfigManager.cpp
    src/utils/CompilerDetector.cpp
    src/utils/SessionManager.cpp
    src/utils/CodeTemplateManager.cpp
    src/utils/ErrorHandler.cpp
    src/utils/CrashHandler.cpp
    src/utils/AIConnectionChecker.cpp
    src/utils/AutoSaveManager.cpp
    src/utils/SyntaxChecker.cpp
    src/utils/FileUtils.cpp
    src/utils/MarkdownRenderer.cpp
    src/utils/ImportRuleManager.cpp
)

# 头文件
set(HEADERS
    src/ui/MainWindow.h
    src/ui/QuestionPanel.h
    src/ui/CodeEditor.h
    src/ui/AIAnalysisPanel.h
    src/ui/ImportDialog.h
    src/ui/HistoryWidget.h
    src/ui/QuestionBankPanel.h
    src/ui/QuestionBankTreeWidget.h
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
    src/ui/AIJudgeProgressDialog.h
    src/ui/ChatBubbleDelegate.h
    src/ui/ChatHistoryDialog.h
    src/ui/ErrorListWidget.h
    src/ui/TestCaseFixerDialog.h
    src/ui/BatchTestCaseFixerDialog.h
    src/ui/QuestionEditorDialog.h
    src/ai/TestCaseFixer.h
    src/utils/OperationHistory.h
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
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${APP_ICON_RC})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

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

# 智能复制数据文件到构建目录（不覆盖用户数据）
# 使用自定义脚本，只复制模板文件，保护用户数据
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
        -DSOURCE_DIR="${CMAKE_SOURCE_DIR}/data"
        -DBUILD_DIR="$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
        -P "${CMAKE_SOURCE_DIR}/cmake/copy_data_if_needed.cmake"
    COMMENT "Syncing data files (preserving user data)..."
)

# 设置运行时环境变量 - 修复Qt插件路径冲突
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "QT_PLUGIN_PATH=${CMAKE_PREFIX_PATH}/plugins;QT_QPA_PLATFORM_PLUGIN_PATH=${CMAKE_PREFIX_PATH}/plugins/platforms"
)

# 输出信息
message(STATUS "Qt6 found: ${Qt6_DIR}")
message(STATUS "QScintilla library: ${QSCINTILLA_LIBRARY}")
message(STATUS "QScintilla include: ${QSCINTILLA_INCLUDE_DIR}")
message(STATUS "Data files will be copied from: ${CMAKE_SOURCE_DIR}/data")
message(STATUS "Data files will be copied to: build/data")
message(STATUS "Qt plugins path: ${CMAKE_PREFIX_PATH}/plugins")
