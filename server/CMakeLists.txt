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
    src/database/Database.cpp
    src/database/DatabaseOptimizer.cpp
    src/cache/CacheManager.cpp
    src/crypto/CryptoManager.cpp
    src/network/ProtocolParser.cpp
    src/network/QSslServer.cpp
    src/utils/AdminAuth.cpp
    src/utils/AdminManager.cpp
    src/config/ServerConfig.cpp
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
    src/database/Database.h
    src/database/DatabaseOptimizer.h
    src/cache/CacheManager.h
    src/crypto/CryptoManager.h
    src/network/ProtocolParser.h
    src/utils/AdminAuth.h
    src/utils/AdminManager.h
    src/config/ServerConfig.h
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

# 查找OpenSSL
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(QKChatServer PRIVATE 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
else()
    message(WARNING "OpenSSL not found - SSL features will be disabled")
    add_definitions(-DNO_OPENSSL)
endif()

# Qt6库链接
target_link_libraries(QKChatServer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
)

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
