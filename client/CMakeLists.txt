cmake_minimum_required(VERSION 3.16)

project(QKChatClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置Qt自动处理MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt6组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Qml
    Quick
    Network
    Sql
    QuickControls2
    Concurrent
    Multimedia
    OpenGL
)

qt_standard_project_setup()

# 定义源文件
set(SOURCES
    src/main.cpp
    src/controllers/UserController.cpp
    src/controllers/ChatController.cpp
    src/models/UserModel.cpp
    src/database/LocalDatabase.cpp
    src/network/NetworkClient.cpp
    src/utils/Validator.cpp
    src/utils/FileTransferManager.cpp
    src/utils/ThreadPool.cpp
    src/crypto/CryptoManager.cpp
    src/config/ConfigManager.cpp
)

# 定义头文件
set(HEADERS
    src/controllers/UserController.h
    src/controllers/ChatController.h
    src/models/UserModel.h
    src/database/LocalDatabase.h
    src/network/NetworkClient.h
    src/utils/Validator.h
    src/utils/FileTransferManager.h
    src/utils/ThreadPool.h
    src/crypto/CryptoManager.h
    src/config/ConfigManager.h
)

# 定义QML文件
set(QML_FILES
    qml/main.qml
    qml/LoginWindow.qml
    qml/RegisterWindow.qml
    qml/ChatMainWindow.qml
    qml/components/CustomButton.qml
    qml/components/CustomTextField.qml
    qml/components/AvatarSelector.qml
    qml/components/SideBarButton.qml
    qml/components/ChatWindow.qml
    qml/components/MessageBubble.qml
    qml/components/DefaultPage.qml
    qml/components/ContactsPage.qml
    qml/components/GroupsPage.qml
    qml/components/AddPage.qml
    qml/components/ProfilePage.qml
    qml/components/SettingsPage.qml
    qml/components/EmojiPicker.qml
)

# 创建可执行文件
qt_add_executable(QKChatClient
    ${SOURCES}
    ${HEADERS}
    Resource.qrc
)

# 添加QML模块
qt_add_qml_module(QKChatClient
    URI QKChatClient
    VERSION 1.0
    QML_FILES ${QML_FILES}
    RESOURCE_PREFIX "/QKChatClient"
)

# =============================================================================
# 依赖库配置
# =============================================================================

# 查找OpenSSL（用于加密功能）
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(QKChatClient PRIVATE 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
else()
    message(WARNING "OpenSSL not found - encryption features will be disabled")
    add_definitions(-DNO_OPENSSL)
endif()

# 链接Qt库
target_link_libraries(QKChatClient PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Sql
    Qt6::QuickControls2
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::OpenGL
)

# =============================================================================
# 编译配置
# =============================================================================

target_compile_definitions(QKChatClient PRIVATE
    QT_DEPRECATED_WARNINGS
    QKCHAT_CLIENT_VERSION="${PROJECT_VERSION}"
)

# 设置包含目录
target_include_directories(QKChatClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
)

# 设置编译选项
if(WIN32)
    target_compile_definitions(QKChatClient PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# 设置输出目录
set_target_properties(QKChatClient PROPERTIES
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
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/downloads)

# 复制配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/dev.ini 
    ${CMAKE_BINARY_DIR}/bin/config/dev.ini 
    COPYONLY
)

# 复制图标和资源文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icons)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons 
         DESTINATION ${CMAKE_BINARY_DIR}/bin)
    message(STATUS "Icons copied to output directory")
endif()

# =============================================================================
# 调试配置
# =============================================================================

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(QKChatClient PRIVATE
        DEBUG_MODE
        LOG_LEVEL_DEBUG
    )
    message(STATUS "Debug mode enabled")
endif()

# =============================================================================
# 状态信息输出
# =============================================================================

message(STATUS "=== QK Chat Client Configuration ===")
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

