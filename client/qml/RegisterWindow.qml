import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QKChatClient 1.0
import "components"

Page {
    id: registerPage
    
    signal switchToLogin()
    signal registrationSuccess(string username, int userId)
    
    property alias userController: userController
    property string statusMessage: ""
    property color statusColor: "black"
    
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
                
                // 邮箱输入
                CustomTextField {
                    id: emailField
                    Layout.fillWidth: true
                    placeholderText: "邮箱"
                    iconSource: "qrc:/icons/email.png"
                    
                    property string validationError: ""
                    
                    onCustomTextChanged: {
                        if (text.length > 0) {
                            // 实时验证邮箱
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
                        console.log("=== EMAIL FIELD EDITING FINISHED ===");
                        console.log("Email text:", text);
                        console.log("Validation error:", validationError);
                        
                        if (text.length > 0 && validationError === "") {
                            console.log("Calling checkEmailAvailability");
                            // 检查邮箱可用性
                            userController.checkEmailAvailability(text);
                        } else {
                            console.log("Not calling checkEmailAvailability - text length:", text.length, "validation error:", validationError);
                        }
                        
                        console.log("=== END EMAIL FIELD EDITING FINISHED ===");
                    }
                }
                
                // 验证码输入区域
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    CustomTextField {
                        id: verificationCodeField
                        Layout.fillWidth: true
                        placeholderText: "验证码"
                        iconSource: "qrc:/icons/security.png"
                        maximumLength: 6
                        
                        property string validationError: ""
                        
                        onCustomTextChanged: {
                            if (text.length > 0) {
                                if (text.length !== 6) {
                                    validationError = "验证码必须是6位数字";
                                    errorMessage = validationError;
                                } else {
                                    errorMessage = "";
                                    validationError = "";
                                }
                            } else {
                                errorMessage = "";
                                validationError = "";
                            }
                        }
                    }
                    
                    CustomButton {
                        id: sendCodeButton
                        Layout.preferredWidth: 80
                        text: "发送验证码"
                        enabled: emailField.text.length > 0 && 
                                emailField.validationError === "" &&
                                !userController.isLoading
                        
                        onClicked: {
                            console.log("=== QML: SEND VERIFICATION CODE BUTTON CLICKED ===");
                            console.log("Email field text:", emailField.text);
                            console.log("Email field length:", emailField.text.length);
                            console.log("UserController exists:", userController !== null);
                            console.log("UserController type:", typeof userController);
                            console.log("UserController object:", userController);
                            
                            // 测试UserController的其他方法
                            if (userController) {
                                console.log("UserController isLoading:", userController.isLoading);
                                console.log("UserController errorMessage:", userController.errorMessage);
                                
                                // 测试一个简单的Q_INVOKABLE方法
                                try {
                                    console.log("Testing testMethod...");
                                    var testResult = userController.testMethod("test input");
                                    console.log("testMethod result:", testResult);
                                } catch (e) {
                                    console.error("Error calling testMethod:", e);
                                }
                                
                                // 测试validateEmail方法
                                try {
                                    console.log("Testing validateEmail method...");
                                    var result = userController.validateEmail(emailField.text);
                                    console.log("validateEmail result:", result);
                                } catch (e) {
                                    console.error("Error calling validateEmail:", e);
                                }
                                
                                // 测试sendEmailVerificationCode方法
                                if (typeof userController.sendEmailVerificationCode === 'function') {
                                    console.log("Calling userController.sendEmailVerificationCode");
                                    console.log("Method exists, calling with email:", emailField.text);
                                    try {
                                        userController.sendEmailVerificationCode(emailField.text);
                                        sendCodeButton.enabled = false;
                                        // 60秒后重新启用
                                        timer.start();
                                        console.log("Button disabled and timer started");
                                    } catch (e) {
                                        console.error("Error calling sendEmailVerificationCode:", e);
                                    }
                                } else {
                                    console.error("sendEmailVerificationCode method not available");
                                    console.log("Available methods:", Object.getOwnPropertyNames(userController));
                                    console.log("All properties:", Object.keys(userController));
                                }
                            } else {
                                console.error("UserController is null");
                            }
                            
                            console.log("=== END QML: SEND VERIFICATION CODE BUTTON CLICKED ===");
                        }
                        
                        Timer {
                            id: timer
                            interval: 60000
                            repeat: false
                            onTriggered: {
                                sendCodeButton.enabled = true;
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
                        verificationCodeField.text.length > 0 &&
                        passwordField.text.length > 0 &&
                        confirmPasswordField.text.length > 0 &&
                        usernameField.validationError === "" &&
                        emailField.validationError === "" &&
                        verificationCodeField.validationError === "" &&
                        passwordField.validationError === "" &&
                        confirmPasswordField.errorMessage === ""
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
        if (!usernameField.text || !emailField.text || !verificationCodeField.text || !passwordField.text) return;
        
        userController.registerUser(
            usernameField.text,
            emailField.text,
            verificationCodeField.text,
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
                emailField.errorMessage = "邮箱已被使用";
            }
        }
        
        function onEmailVerificationCodeSent(success, message) {
            if (success) {
                statusMessage = "验证码已发送到您的邮箱";
                statusColor = Material.color(Material.Green);
                // 发送成功后启用验证码输入框
                verificationCodeField.enabled = true;
                verificationCodeField.forceActiveFocus();
            } else {
                statusMessage = "验证码发送失败：" + message;
                statusColor = Material.color(Material.Red);
                // 发送失败后重新启用发送按钮
                sendCodeButton.enabled = true;
                timer.stop();
            }
        }
        
        function onEmailVerificationCodeVerified(success, message) {
            if (success) {
                statusMessage = "验证码验证成功";
                statusColor = Material.color(Material.Green);
            } else {
                statusMessage = "验证码验证失败：" + message;
                statusColor = Material.color(Material.Red);
            }
        }
        

    }
    
    // 页面激活时的处理
    Component.onCompleted: {
        usernameField.forceActiveFocus();
    }

    // 清除状态信息
    function clearStatus() {
        statusMessage = ""
        statusColor = "black"
    }
}
