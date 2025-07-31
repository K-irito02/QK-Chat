import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QKChatClient 1.0
import "components"

Page {
    id: loginPage
    
    signal switchToRegister()
    
    property alias userController: userController
    
    UserController {
        id: userController
    }
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: Material.backgroundColor
        
        gradient: Gradient {
            GradientStop { position: 0.0; color: Material.primary }
            GradientStop { position: 1.0; color: Material.backgroundColor }
        }
        opacity: 0.1
    }
    
    // 主题切换按钮
    Button {
        id: themeButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 16
        width: 40
        height: 40
        
        background: Rectangle {
            radius: 20
            color: Material.accent
        }
        
        icon.source: configManager.isDarkTheme ? 
            "qrc:/icons/sun.png" : 
            "qrc:/icons/moon.png"
        icon.width: 20
        icon.height: 20
        icon.color: "white"
        
        onClicked: {
            configManager.isDarkTheme = !configManager.isDarkTheme;
            configManager.saveConfig();
        }
        
        HoverHandler {
            id: themeButtonHover
        }
        
        ToolTip {
            text: configManager.isDarkTheme ? "切换到浅色主题" : "切换到深色主题"
            visible: themeButtonHover.hovered
        }
    }
    
    // 主内容区域
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        anchors.topMargin: 80
        spacing: 20
        
        // Logo和标题
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 120
            spacing: 10
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "QK Chat"
                font.pixelSize: 32
                font.bold: true
                color: Material.foreground
            }
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "欢迎回来"
                font.pixelSize: 18
                color: Material.hintTextColor
            }
        }
        
        // 登录表单
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 280
            Layout.alignment: Qt.AlignHCenter
            spacing: 15
            
            // 用户名或邮箱输入
            CustomTextField {
                id: usernameField
                Layout.fillWidth: true
                placeholderText: "用户名或邮箱"
                iconSource: "qrc:/icons/user.png"
                
                onCustomTextChanged: {
                    if (text.length > 0) {
                        errorMessage = ""
                    }
                }
            }
            
            // 密码输入
            CustomTextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "密码"
                iconSource: "qrc:/icons/lock.png"
                isPassword: true
                
                onCustomTextChanged: {
                    if (text.length > 0) {
                        errorMessage = ""
                    }
                }
                
                onCustomAccepted: {
                    if (loginButton.enabled) {
                        performLogin()
                    }
                }
            }
            
            // 验证码区域（密码错误3次后显示）
            RowLayout {
                Layout.fillWidth: true
                visible: userController.needCaptcha
                spacing: 10
                
                CustomTextField {
                    id: captchaField
                    Layout.fillWidth: true
                    placeholderText: "验证码"
                    iconSource: "qrc:/icons/captcha.png"
                    maximumLength: 6
                }
                
                Rectangle {
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    border.width: 1
                    border.color: Material.hintTextColor
                    radius: 4
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: userController.captchaImage
                        fillMode: Image.PreserveAspectFit
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: userController.refreshCaptcha()
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
            
            // 错误信息显示
            Text {
                Layout.fillWidth: true
                text: userController.errorMessage
                color: Material.color(Material.Red)
                font.pixelSize: 14
                visible: userController.errorMessage.length > 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            
            // 记住密码选项
            RowLayout {
                Layout.fillWidth: true
                
                CheckBox {
                    id: rememberCheckbox
                    text: "记住密码"
                    font.pixelSize: 14
                }
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: "忘记密码？"
                    font.pixelSize: 14
                    color: Material.accent
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // TODO: 实现忘记密码功能
                            console.log("忘记密码")
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
            
            // 登录按钮
            CustomButton {
                id: loginButton
                Layout.fillWidth: true
                text: userController.isLoading ? "登录中..." : "登录"
                enabled: !userController.isLoading && 
                        usernameField.text.length > 0 && 
                        passwordField.text.length > 0 &&
                        (!userController.needCaptcha || captchaField.text.length > 0)
                isPrimary: true
                
                onClicked: performLogin()
            }
            
            // 注册链接
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 15
                text: "还没有账号？<font color='" + Material.accent + "'>立即注册</font>"
                font.pixelSize: 14
                color: Material.hintTextColor
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: switchToRegister()
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
    
    // 登录函数
    function performLogin() {
        if (userController.isLoading) return
        if (!usernameField.text || !passwordField.text) return
        
        var captcha = userController.needCaptcha ? captchaField.text : ""
        userController.login(usernameField.text, passwordField.text, captcha)
        
        // 保存登录凭据（如果选择记住密码）
        if (rememberCheckbox.checked) {
            userController.saveLoginCredentials(usernameField.text, passwordField.text, true)
        }
    }
    
    // 监听用户控制器信号
    Connections {
        target: userController
        
        function onLoginFailed(error) {
            // 清空密码字段
            passwordField.text = ""
            if (userController.needCaptcha) {
                captchaField.text = ""
            }
        }
        
        function onNeedCaptchaChanged() {
            if (userController.needCaptcha) {
                userController.refreshCaptcha()
            }
        }
    }
    
    // 页面激活时的处理
    Component.onCompleted: {
        usernameField.forceActiveFocus()
    }
}
