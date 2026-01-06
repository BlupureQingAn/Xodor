# 智能数据文件复制脚本
# 只复制不存在的文件，保护用户数据不被覆盖
# 使用方式: cmake -DSOURCE_DIR=... -DBUILD_DIR=... -P copy_data_if_needed.cmake

# 需要保护的目录（用户数据，不覆盖）
set(PROTECTED_DIRS
    "user_answers"
    "question_banks"
)

# 需要保护的文件（用户配置和会话，不覆盖）
set(PROTECTED_FILES
    "last_session.json"
)

# 总是更新的目录（模板数据，每次都复制）
set(TEMPLATE_DIRS
    "sample_questions"
)

message(STATUS "Syncing data files...")
message(STATUS "  Source: ${SOURCE_DIR}")
message(STATUS "  Target: ${BUILD_DIR}")

# 确保目标目录存在
file(MAKE_DIRECTORY "${BUILD_DIR}")

# 1. 复制模板目录（总是更新）
foreach(dir ${TEMPLATE_DIRS})
    if(EXISTS "${SOURCE_DIR}/${dir}")
        message(STATUS "  [UPDATE] ${dir}")
        file(REMOVE_RECURSE "${BUILD_DIR}/${dir}")
        file(COPY "${SOURCE_DIR}/${dir}" 
             DESTINATION "${BUILD_DIR}")
    endif()
endforeach()

# 2. 复制原始题库和基础题库（只在不存在时复制）
foreach(dir "原始题库" "基础题库")
    if(EXISTS "${SOURCE_DIR}/${dir}")
        if(NOT EXISTS "${BUILD_DIR}/${dir}")
            message(STATUS "  [NEW] ${dir}")
            file(COPY "${SOURCE_DIR}/${dir}" 
                 DESTINATION "${BUILD_DIR}")
        else()
            message(STATUS "  [SKIP] ${dir} (already exists)")
        endif()
    endif()
endforeach()

# 3. 复制config目录（只在不存在时复制）
if(EXISTS "${SOURCE_DIR}/config")
    if(NOT EXISTS "${BUILD_DIR}/config")
        message(STATUS "  [NEW] config/")
        file(COPY "${SOURCE_DIR}/config" 
             DESTINATION "${BUILD_DIR}")
    else()
        message(STATUS "  [SKIP] config/ (already exists)")
    endif()
endif()

# 4. 创建用户数据目录（如果不存在）
foreach(dir ${PROTECTED_DIRS})
    if(NOT EXISTS "${BUILD_DIR}/${dir}")
        message(STATUS "  [CREATE] ${dir}/")
        file(MAKE_DIRECTORY "${BUILD_DIR}/${dir}")
    endif()
endforeach()

# 5. 复制config.json（只在不存在时复制）
if(EXISTS "${SOURCE_DIR}/config.json")
    if(NOT EXISTS "${BUILD_DIR}/config.json")
        message(STATUS "  [NEW] config.json")
        file(COPY "${SOURCE_DIR}/config.json" 
             DESTINATION "${BUILD_DIR}")
    else()
        message(STATUS "  [SKIP] config.json (already exists)")
    endif()
endif()

# 6. 保护的文件永远不复制
foreach(file ${PROTECTED_FILES})
    if(EXISTS "${BUILD_DIR}/${file}")
        message(STATUS "  [PROTECTED] ${file}")
    endif()
endforeach()

message(STATUS "Data sync completed.")
