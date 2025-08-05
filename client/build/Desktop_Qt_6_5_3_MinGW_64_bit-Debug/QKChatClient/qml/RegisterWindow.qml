import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QKChatClient 1.0
import "components"

Page {
    id: registerPage
    
    signal switchToLogin()
    signal registrationSuccess(string username, string email, int userId)
    
    property alias userController: userController
    property string registeredEmail: ""
    property bool emailVerified: false
    property string statusMessage: ""
    property color statusColor: "black"

    // 独立的加载状态
    property bool sendingCode: false
    property bool verifyingCode: false
    
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
        anchors.margins: 16
        anchors.topMargin: 40
        spacing: 12
        
        // 标题
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 0
            Layout.bottomMargin: 5
            spacing: 4
                
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "创建账户"
                    font.pixelSize: 22
                    font.bold: true
                    color: Material.foreground
                }
                
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "欢迎加入QK Chat"
                    font.pixelSize: 13
                    color: Material.hintTextColor
                }
            }
            
        // 注册表单
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            
            // 头像选择区域
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 0
                Layout.bottomMargin: 0
                spacing: 4
                
                // 头像选择器
                AvatarSelector {
                    id: avatarSelector
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 90
                }
            }
                
                // 用户名输入
                CustomTextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "用户名"
                    iconSource: "qrc:/icons/user.png"
                    
                    property string validationError: ""
                    
                    onCustomTextChanged: {
                        if (text.length > 0) {
                            // 实时验证用户名
                            var validator = Qt.createQmlObject("import QKChatClient 1.0; Validator {}", registerPage);
                            if (validator) {
                                validationError = validator.getUsernameError(text);
                                errorMessage = validationError;
                            }
                        } else {
                            errorMessage = "";
                            validationError = "";
                        }
                    }
                    
                    onCustomEditingFinished: {
                        if (text.length > 0 && validationError === "") {
                            // 检查用户名可用性
                            userController.checkUsernameAvailability(text);
                        }
                    }
                }
                
                // 邮箱输入区域
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomTextField {
                        id: emailField
                        Layout.fillWidth: true
                        placeholderText: "邮箱地址"
                        iconSource: "qrc:/icons/email.png"

                        property string validationError: ""

                        onCustomTextChanged: {
                            if (text.length > 0) {
                                var validator = Qt.createQmlObject("import QKChatClient 1.0; Validator {}", registerPage);
                                if (validator) {
                                    validationError = validator.getEmailError(text);
                                    errorMessage = validationError;
                                }
                            } else {
                                errorMessage = "";
                                validationError = "";
                            }
                        }

                        onCustomEditingFinished: {
                            if (text.length > 0 && validationError === "") {
                                userController.checkEmailAvailability(text);
                            }
                        }
                    }

                    CustomButton {
                        id: sendCodeButton
                        Layout.preferredWidth: 120
                        text: sendingCode ? "发送中..." : "发送验证码"
                        enabled: !sendingCode &&
                                emailField.text.length > 0 &&
                                emailField.validationError === ""

                        onClicked: {
                            if (emailField.text.length > 0 && emailField.validationError === "") {
                                registeredEmail = emailField.text
                                sendingCode = true
                                console.log("Starting email verification for:", registeredEmail)
                                userController.sendEmailVerification(registeredEmail)
                            }
                        }
                    }
                }

                // 验证码输入区域
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomTextField {
                        id: verificationCodeField
                        Layout.fillWidth: true
                        placeholderText: "请输入6位验证码"
                        iconSource: "qrc:/icons/lock.png"
                        maximumLength: 6
                        enabled: registeredEmail.length > 0

                        onCustomTextChanged: {
                            // 清除错误信息
                            errorMessage = ""
                        }
                    }

                    CustomButton {
                        id: verifyCodeButton
                        Layout.preferredWidth: 120
                        text: verifyingCode ? "验证中..." : "验证"
                        enabled: !verifyingCode &&
                                verificationCodeField.text.length === 6 &&
                                registeredEmail.length > 0

                        onClicked: {
                            if (verificationCodeField.text.length === 6 && registeredEmail.length > 0) {
                                verifyingCode = true
                                console.log("Starting code verification for:", registeredEmail, "Code:", verificationCodeField.text)
                                userController.verifyEmailCode(registeredEmail, verificationCodeField.text)
                            }
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
                    
                    property string validationError: ""
                    
                    onCustomTextChanged: {
                        if (text.length > 0) {
                            var validator = Qt.createQmlObject("import QKChatClient 1.0; Validator {}", registerPage);
                            if (validator) {
                                validationError = validator.getPasswordError(text);
                                errorMessage = validationError;
                            }
                        } else {
                            errorMessage = "";
                            validationError = "";
                        }
                        
                        // 同时验证确认密码
                        if (confirmPasswordField.text.length > 0) {
                            confirmPasswordField.validateConfirm();
                        }
                    }
                }
                
                // 确认密码输入
                CustomTextField {
                    id: confirmPasswordField
                    Layout.fillWidth: true
                    placeholderText: "确认密码"
                    iconSource: "qrc:/icons/lock.png"
                    isPassword: true
                    
                    function validateConfirm() {
                        if (text.length > 0) {
                            if (text !== passwordField.text) {
                                errorMessage = "两次输入的密码不一致";
                            } else {
                                errorMessage = "";
                            }
                        } else {
                            errorMessage = "";
                        }
                    }
                    
                    onCustomTextChanged: validateConfirm();
                    
                    onCustomAccepted: {
                        if (registerButton.enabled) {
                            performRegister();
                        }
                    }
                }
                
            // 密码强度提示
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 2
                text: "密码要求：8-20个字符，包含大小写字母和数字"
                font.pixelSize: 11
                color: Material.hintTextColor
                wrapMode: Text.WordWrap
                visible: passwordField.activeFocus || passwordField.text.length > 0
            }
            
            // 状态信息显示
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 4
                Layout.bottomMargin: 2
                text: statusMessage
                color: statusColor
                font.pixelSize: 12
                visible: statusMessage.length > 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            // 错误信息显示
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 2
                Layout.bottomMargin: 4
                text: userController.errorMessage
                color: Material.color(Material.Red)
                font.pixelSize: 12
                visible: userController.errorMessage.length > 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            
            // 注册按钮
            CustomButton {
                id: registerButton
                Layout.fillWidth: true
                Layout.topMargin: 6
                text: userController.isLoading ? "注册中..." : "注册"
                enabled: !userController.isLoading &&
                        usernameField.text.length > 0 &&
                        emailField.text.length > 0 &&
                        passwordField.text.length > 0 &&
                        confirmPasswordField.text.length > 0 &&
                        usernameField.validationError === "" &&
                        passwordField.validationError === "" &&
                        confirmPasswordField.errorMessage === "" &&
                        emailVerified  // 必须完成邮箱验证
                isPrimary: true

                onClicked: performRegister();
            }
            
            // 登录链接
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                text: "已有账号？<font color='" + Material.accent + "'>立即登录</font>"
                font.pixelSize: 13
                color: Material.hintTextColor
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: switchToLogin();
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
    
    // 注册函数
    function performRegister() {
        if (userController.isLoading) return;
        if (!usernameField.text || !emailField.text || !passwordField.text) return;
        
        userController.registerUser(
            usernameField.text,
            emailField.text,
            passwordField.text,
            avatarSelector.selectedAvatar
        );
    }
    
    // 监听用户控制器信号
    Connections {
        target: userController
        
        function onRegisterFailed(error) {
            // 清空密码字段
            passwordField.text = "";
            confirmPasswordField.text = "";
        }
        
        function onUsernameAvailabilityResult(isAvailable) {
            if (!isAvailable) {
                usernameField.errorMessage = "用户名已被使用";
            }
        }
        
        function onEmailAvailabilityResult(isAvailable) {
            if (!isAvailable) {
                emailField.errorMessage = "邮箱已被注册";
            }
        }
        
        function onEmailVerificationSent(success, message) {
            sendingCode = false  // 重置发送状态
            console.log("Email verification sent result:", success, message)

            if (success) {
                statusMessage = "验证码已发送到您的邮箱，请查收"
                statusColor = "green"
                // 启用验证码输入框
                verificationCodeField.enabled = true
                verificationCodeField.forceActiveFocus()
            } else {
                statusMessage = "发送失败: " + message
                statusColor = "red"
                emailField.errorMessage = message
            }
        }

        function onEmailCodeVerified() {
            verifyingCode = false  // 重置验证状态
            console.log("Email code verification successful")

            statusMessage = "邮箱验证成功！"
            statusColor = "green"
            emailVerified = true
            // 验证成功后，不自动注册，让用户手动点击注册按钮
        }

        function onEmailCodeVerificationFailed(error) {
            verifyingCode = false  // 重置验证状态
            console.log("Email code verification failed:", error)

            statusMessage = "验证失败: " + error
            statusColor = "red"
            verificationCodeField.errorMessage = "验证码错误，请重试"
            emailVerified = false
        }
    }
    
    // 页面激活时的处理
    Component.onCompleted: {
        usernameField.forceActiveFocus();
    }

    // 重置表单状态
    function resetEmailVerification() {
        emailVerified = false
        registeredEmail = ""
        statusMessage = ""
        sendingCode = false
        verifyingCode = false
        verificationCodeField.text = ""
        verificationCodeField.enabled = false
    }

    // 清除状态信息
    function clearStatus() {
        statusMessage = ""
        statusColor = "black"
        sendingCode = false
        verifyingCode = false
    }
}
