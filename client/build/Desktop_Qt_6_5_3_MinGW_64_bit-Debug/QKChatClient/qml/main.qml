import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Window 2.15
import QKChatClient 1.0

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
    property alias userController: userController
    
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
        
        onLoginSuccess: {
            console.log("登录成功")
            // TODO: 跳转到聊天主界面
            Qt.quit() // 临时退出，后续替换为跳转到聊天界面
        }
        
        onLoginFailed: {
            console.log("登录失败:", error)
        }
        
        onRegisterSuccess: {
            console.log("注册成功")
            showLogin = true
        }
        
        onRegisterFailed: {
            console.log("注册失败:", error)
        }
    }
    
    // 主容器
    StackView {
        id: stackView
        anchors.fill: parent
        
        initialItem: showLogin ? loginComponent : registerComponent
        
        // 监听showLogin变化，切换界面
        Connections {
            target: mainWindow
            function onShowLoginChanged() {
                if (showLogin) {
                    stackView.replace(loginComponent)
                } else {
                    stackView.replace(registerComponent)
                }
            }
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
        
        // 设置窗口居中
        x = (Screen.width - width) / 2
        y = (Screen.height - height) / 2
    }
}