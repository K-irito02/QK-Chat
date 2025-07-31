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
    Qml
    Quick
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
    src/database/Database.cpp
    src/network/ProtocolParser.cpp
    src/utils/AdminAuth.cpp
    src/config/ServerConfig.cpp
)

set(HEADERS
    src/admin/AdminWindow.h
    src/admin/LoginDialog.h
    src/admin/DashboardWidget.h
    src/core/ChatServer.h
    src/core/SessionManager.h
    src/database/Database.h
    src/network/ProtocolParser.h
    src/utils/AdminAuth.h
    src/config/ServerConfig.h
)

set(UI_FILES
    src/admin/LoginDialog.ui
)

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
    Qt6::Qml
    Qt6::Quick
    Qt6::Concurrent
)

# =============================================================================
# 编译配置
# =============================================================================

target_compile_definitions(QKChatServer PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_include_directories(QKChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# =============================================================================
# 输出配置
# =============================================================================

set_target_properties(QKChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# =============================================================================
# 资源文件配置
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/dev.conf 
    ${CMAKE_BINARY_DIR}/bin/config/dev.conf 
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/mysql_init.sql
    ${CMAKE_BINARY_DIR}/bin/data/mysql_init.sql 
    COPYONLY
)

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
message(STATUS "=====================================")
