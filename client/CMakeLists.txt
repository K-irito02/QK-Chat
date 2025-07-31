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
    QuickControls2Impl
)

qt_standard_project_setup()

# 定义源文件
set(SOURCES
    src/main.cpp
    src/controllers/UserController.cpp
    src/models/UserModel.cpp
    src/database/LocalDatabase.cpp
    src/network/NetworkClient.cpp
    src/utils/Validator.cpp
    src/config/ConfigManager.cpp
)

# 定义头文件
set(HEADERS
    src/controllers/UserController.h
    src/models/UserModel.h
    src/database/LocalDatabase.h
    src/network/NetworkClient.h
    src/utils/Validator.h
    src/config/ConfigManager.h
)

# 定义QML文件
set(QML_FILES
    qml/main.qml
    qml/LoginWindow.qml
    qml/RegisterWindow.qml
    qml/components/CustomButton.qml
    qml/components/CustomTextField.qml
    qml/components/AvatarSelector.qml
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
)

# 链接Qt库
target_link_libraries(QKChatClient PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Sql
    Qt6::QuickControls2
    Qt6::QuickControls2Impl
)

# 设置包含目录
target_include_directories(QKChatClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
)

# =============================================================================
# 资源文件配置
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/dev.ini 
    ${CMAKE_BINARY_DIR}/bin/config/dev.ini 
    COPYONLY
)

# =============================================================================
# 状态信息输出
# =============================================================================

message(STATUS "=== QK Chat Client Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "=====================================")

