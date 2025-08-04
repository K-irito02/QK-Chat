import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Window 2.15
import QKChatClient 1.0
import "."

ApplicationWindow {
    id: mainWindow
    
    width: 400
    height: 600
    minimumWidth: 350
    minimumHeight: 500
    
    title: "QK Chat"
    visible: true
    
    // Material主题配置
    Material.theme: configManager.isDarkTheme ? Material.Dark : Material.Light
    Material.primary: configManager.primaryColor
    Material.accent: configManager.accentColor
    
    property bool showLogin: true
    property bool isLoggedIn: false
    property alias userController: userController
    property string prefillEmail: ""
    
    // 监听全局configManager的主题变化
    Connections {
        target: configManager
        function onIsDarkThemeChanged() {
            console.log("主题已切换到:", configManager.isDarkTheme ? "深色" : "浅色")
        }
    }
    
    // 全局用户控制器
    UserController {
        id: userController
    }
    
    // 监听用户控制器信号
    Connections {
        target: userController
        
        function onLoginSuccess() {
            console.log("登录成功")
            isLoggedIn = true
            // 调整窗口大小用于聊天界面
            width = 1200
            height = 800
            minimumWidth = 800
            minimumHeight = 600
            
            // 居中显示
            x = (Screen.width - width) / 2
            y = (Screen.height - height) / 2
        }
        
        function onLoginFailed(error) {
            console.log("登录失败:", error)
        }
        
        function onRegisterSuccess(username, email, userId) {
            console.log("注册成功")
            prefillEmail = email
            showLogin = true
        }
        
        function onRegisterFailed(error) {
            console.log("注册失败:", error)
        }
    }
    
    // 主容器
    StackView {
        id: stackView
        anchors.fill: parent
        
        initialItem: isLoggedIn ? chatMainComponent : (showLogin ? loginComponent : registerComponent)
        
        // 监听登录状态变化
        Connections {
            target: mainWindow
            function onIsLoggedInChanged() {
                if (isLoggedIn) {
                    stackView.replace(chatMainComponent)
                } else if (showLogin) {
                    stackView.replace(loginComponent)
                } else {
                    stackView.replace(registerComponent)
                }
            }
            
            function onShowLoginChanged() {
                if (!isLoggedIn) {
                    if (showLogin) {
                        stackView.replace(loginComponent)
                    } else {
                        stackView.replace(registerComponent)
                    }
                }
            }
        }
    }
    
    // 聊天主界面组件
    Component {
        id: chatMainComponent
        
        ChatMainWindow {
            // 聊天主界面会自动从userModel获取当前用户信息
        }
    }
    
    // 登录界面组件
    Component {
        id: loginComponent
        
        LoginWindow {
            onSwitchToRegister: {
                mainWindow.showLogin = false
            }
        }
    }
    
    // 注册界面组件
    Component {
        id: registerComponent
        
        RegisterWindow {
            onSwitchToLogin: {
                mainWindow.showLogin = true
            }
        }
    }
    
    // 主题切换功能
    function toggleTheme() {
        configManager.isDarkTheme = !configManager.isDarkTheme
    }
    
    // 窗口关闭事件
    onClosing: {
        // 保存配置
        configManager.saveConfig()
    }
    
    // 组件完成时的初始化
    Component.onCompleted: {
        // 尝试自动登录
        userController.tryAutoLogin()
        
        // 连接到服务器
        userController.connectToServer("localhost", 8443)
        
        // 设置窗口居中
        x = (Screen.width - width) / 2
        y = (Screen.height - height) / 2
    }
}