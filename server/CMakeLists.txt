# =============================================================================
# QK Chat Server - CMake Configuration
# =============================================================================

cmake_minimum_required(VERSION 3.16)

project(QKChatServer 
    VERSION 1.0.0 
    DESCRIPTION "QK Chat Server - 聊天服务器"
    LANGUAGES CXX
)

# =============================================================================
# 编译器设置
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra)
endif()

# =============================================================================
# Qt6依赖配置
# =============================================================================

find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Widgets
    Network
    Sql
    Concurrent
    WebSockets
)

qt_standard_project_setup()

# =============================================================================
# 源文件配置
# =============================================================================

set(SOURCES
    src/main.cpp
    src/admin/AdminWindow.cpp
    src/admin/LoginDialog.cpp
    src/admin/DashboardWidget.cpp
    src/core/ChatServer.cpp
    src/core/SessionManager.cpp
    src/core/GroupManager.cpp
    src/core/ThreadManager.cpp
    src/core/ChatClientConnection.cpp
    src/core/ConnectionManager.cpp
    src/core/MessageEngine.cpp
    src/core/MessageHandlers.cpp
    src/database/Database.cpp
    src/database/DatabaseOptimizer.cpp
    src/database/DatabasePool.cpp
    src/cache/CacheManagerV2.cpp
    src/cache/MultiLevelCache.cpp
    src/cache/CacheStrategyManager.cpp
    src/cache/CachePreloader.cpp
    src/crypto/CryptoManager.cpp
    src/network/ProtocolParser.cpp
    src/network/QSslServer.cpp
    src/network/NetworkEventHandler.cpp
    src/network/NonBlockingConnectionManager.cpp
    src/services/EmailService.cpp
    src/services/EmailTemplate.cpp
    src/utils/AdminAuth.cpp
    src/utils/AdminManager.cpp
    src/utils/LogManager.cpp
    src/utils/AutoRecovery.cpp
    src/utils/StackTraceLogger.cpp
    src/config/ServerConfig.cpp
    src/utils/ThreadPool.cpp
)

# Check if all source files exist
foreach(SOURCE_FILE ${SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
        message(WARNING "Source file not found: ${SOURCE_FILE}")
    endif()
endforeach()

# Check if all source files exist
foreach(SOURCE_FILE ${SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
        message(WARNING "Source file not found: ${SOURCE_FILE}")
    endif()
endforeach()

set(HEADERS
    src/admin/AdminWindow.h
    src/admin/LoginDialog.h
    src/admin/DashboardWidget.h
    src/core/ChatServer.h
    src/core/SessionManager.h
    src/core/GroupManager.h
    src/core/ThreadManager.h
    src/core/ConnectionManager.h
    src/core/MessageEngine.h
    src/core/MessageHandlers.h
    src/database/Database.h
    src/database/DatabaseOptimizer.h
    src/database/DatabasePool.h
    src/cache/CacheManagerV2.h
    src/cache/MultiLevelCache.h
    src/cache/CacheStrategyManager.h
    src/cache/CachePreloader.h
    src/crypto/CryptoManager.h
    src/network/ProtocolParser.h
    src/network/QSslServer.h
    src/network/NetworkEventHandler.h
    src/network/NonBlockingConnectionManager.h
    src/services/EmailService.h
    src/services/EmailTemplate.h
    src/utils/AdminAuth.h
    src/utils/AdminManager.h
    src/utils/LogManager.h
    src/utils/AutoRecovery.h
    src/utils/StackTraceLogger.h
    src/config/ServerConfig.h
    src/utils/ThreadPool.h
    src/utils/LockFreeStructures.h
)

# Check if all header files exist
foreach(HEADER_FILE ${HEADERS})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE}")
        message(WARNING "Header file not found: ${HEADER_FILE}")
    endif()
endforeach()

set(UI_FILES
    src/admin/LoginDialog.ui
)

# Check if all UI files exist
foreach(UI_FILE ${UI_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${UI_FILE}")
        message(WARNING "UI file not found: ${UI_FILE}")
    endif()
endforeach()

# Add new enhancement modules
set(ENHANCEMENT_SOURCES
    src/core/ThreadSafetyEnhancements.cpp
    src/core/RobustnessManager.cpp
    src/core/StackTraceCollector.cpp
    src/core/ArchitectureOptimizer.cpp
    src/core/EnhancedChatServer.cpp
)

set(ENHANCEMENT_HEADERS
    src/core/ThreadSafetyEnhancements.h
    src/core/RobustnessManager.h
    src/core/StackTraceCollector.h
    src/core/ArchitectureOptimizer.h
    src/core/EnhancedChatServer.h
)

# Option to enable enhancements
option(ENABLE_ENHANCEMENTS "Enable server enhancements" ON)

if(ENABLE_ENHANCEMENTS)
    add_definitions(-DENABLE_ENHANCEMENTS)
    list(APPEND SOURCES ${ENHANCEMENT_SOURCES})
    list(APPEND HEADERS ${ENHANCEMENT_HEADERS})
    
    message(STATUS "Server enhancements enabled")
endif()

# Add required libraries for enhancements
find_package(Threads REQUIRED)

# =============================================================================
# 可执行文件配置
# =============================================================================

qt_add_executable(QKChatServer
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

# =============================================================================
# 依赖库配置
# =============================================================================

# 强制启用SSL支持
target_compile_definitions(QKChatServer PRIVATE
    QT_SSL_USE_OPENSSL
    QT_SSL_USE_OPENSSL_PRE
)

# Qt6库链接
target_link_libraries(QKChatServer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
)

# Windows平台特定的库
if(WIN32)
    target_link_libraries(QKChatServer PRIVATE
        kernel32.lib
        user32.lib
        gdi32.lib
        winspool.lib
        shell32.lib
        ole32.lib
        oleaut32.lib
        uuid.lib
        comdlg32.lib
        advapi32.lib
        ws2_32.lib
        dbghelp.lib
        psapi.lib
        pdh.lib
    )
endif()

# =============================================================================
# 编译配置
# =============================================================================

target_compile_definitions(QKChatServer PRIVATE
    QT_DEPRECATED_WARNINGS
    QKCHAT_SERVER_VERSION="${PROJECT_VERSION}"
)

target_include_directories(QKChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cache
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/admin
)

# =============================================================================
# 输出配置
# =============================================================================

set_target_properties(QKChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "_d"
)

# 复制证书文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/certs)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/certs 
         DESTINATION ${CMAKE_BINARY_DIR}/bin)
    message(STATUS "Certs copied to output directory")
endif()

# Deploy Qt dependencies using windeployqt on Windows
# Deploy Qt dependencies and copy certs directory on Windows
if(WIN32)
    # Copy the certs directory to the output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/certs
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/certs"
        COMMENT "Copying certs directory to output directory"
    )

    # Find and run windeployqt
    get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION_RELEASE)
    get_filename_component(QT_BIN_DIR ${QT_CORE_LOCATION} DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT_BIN_DIR})

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --verbose 1
                --dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                --plugindir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins"
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt to deploy Qt dependencies"
        )
    else()
        message(FATAL_ERROR "windeployqt not found. Please ensure it is in your PATH or Qt's bin directory.")
    endif()
endif()

# =============================================================================
# 资源文件配置
# =============================================================================

# 创建输出目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/config)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/logs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/cache)

# 复制配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/dev.conf 
    ${CMAKE_BINARY_DIR}/bin/config/dev.conf 
    COPYONLY
)

# 复制数据库初始化脚本
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/mysql_init.sql
    ${CMAKE_BINARY_DIR}/bin/data/mysql_init.sql 
    COPYONLY
)

# 创建证书目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/certs)

# 注意：在实际部署时，需要替换为真实的SSL证书
# 这里仅为了编译通过，创建了空的证书文件
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/certs/server.crt)
    file(WRITE ${CMAKE_BINARY_DIR}/bin/certs/server.crt "# 开发测试证书，请在生产环境中替换为真实证书\n")
else()
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/certs/server.crt
        ${CMAKE_BINARY_DIR}/bin/certs/server.crt
        COPYONLY
    )
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/certs/server.key)
    file(WRITE ${CMAKE_BINARY_DIR}/bin/certs/server.key "# 开发测试密钥，请在生产环境中替换为真实密钥\n")
else()
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/certs/server.key
        ${CMAKE_BINARY_DIR}/bin/certs/server.key
        COPYONLY
    )
endif()

# =============================================================================
# 调试配置
# =============================================================================

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(QKChatServer PRIVATE
        DEBUG_MODE
        LOG_LEVEL_DEBUG
    )
    message(STATUS "Debug mode enabled")
endif()

# =============================================================================
# 状态信息输出
# =============================================================================

message(STATUS "=== QK Chat Server Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt version: ${Qt6_VERSION}")
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
else()
    message(STATUS "OpenSSL: Not found")
endif()
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "===================================")
