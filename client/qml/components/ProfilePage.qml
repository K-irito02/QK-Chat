import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1

Rectangle {
    id: profilePage
    
    property var userInfo: userModel
    
    color: Material.background
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        
        ColumnLayout {
            width: profilePage.width - 32
            spacing: 24
            
            // 页面标题
            Text {
                text: "个人中心"
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
                Layout.alignment: Qt.AlignHCenter
            }
            
            // 头像和基本信息区域
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16
                    
                    // 头像
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 100
                        height: 100
                        color: "transparent"
                        radius: 50
                        clip: true
                        
                        Image {
                            id: avatarImage
                            anchors.fill: parent
                            source: userInfo ? userInfo.avatar : "qrc:/icons/avatar1.png"
                            fillMode: Image.PreserveAspectCrop
                            
                            layer.enabled: true
                        }
                        
                        // 编辑头像按钮
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15
                            color: Material.primary
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            border.width: 2
                            border.color: Material.background
                            
                            Image {
                                anchors.centerIn: parent
                                width: 16
                                height: 16
                                source: "qrc:/icons/edit.png"
                                fillMode: Image.PreserveAspectFit
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    avatarDialog.open()
                                }
                            }
                        }
                    }
                    
                    // 用户名和状态
                    Column {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 4
                        
                        Text {
                            text: userInfo ? userInfo.username : "用户名"
                            font.pixelSize: 18
                            font.bold: true
                            color: Material.foreground
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "在线"
                            font.pixelSize: 14
                            color: Material.theme === Material.Dark ? "#81C784" : "#4CAF50"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
            
            // 个人信息编辑区域
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: infoColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: infoColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "个人信息"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    // 用户名
                    ProfileInfoItem {
                        label: "用户名"
                        value: userInfo ? userInfo.username : ""
                        editable: false
                        onEditClicked: {
                            // 用户名不可编辑
                        }
                    }
                    
                    // 邮箱
                    ProfileInfoItem {
                        label: "邮箱"
                        value: userInfo ? userInfo.email : ""
                        editable: true
                        onEditClicked: {
                            editEmailDialog.currentValue = value
                            editEmailDialog.open()
                        }
                    }
                    
                    // 个性签名
                    ProfileInfoItem {
                        label: "个性签名"
                        value: userInfo ? (userInfo.signature || "这个人很懒，什么都没写") : ""
                        editable: true
                        onEditClicked: {
                            editSignatureDialog.currentValue = value
                            editSignatureDialog.open()
                        }
                    }
                }
            }
            
            // 账户设置区域
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: accountColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: accountColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "账户设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    // 修改密码
                    ProfileActionItem {
                        icon: "qrc:/icons/lock.png"
                        title: "修改密码"
                        subtitle: "定期修改密码以保护账户安全"
                        onClicked: {
                            changePasswordDialog.open()
                        }
                    }
                    
                    // 隐私设置
                    ProfileActionItem {
                        icon: "qrc:/icons/privacy.png"
                        title: "隐私设置"
                        subtitle: "管理您的隐私和安全选项"
                        onClicked: {
                            privacyDialog.open()
                        }
                    }
                    
                    // 消息设置
                    ProfileActionItem {
                        icon: "qrc:/icons/message-settings.png"
                        title: "消息设置"
                        subtitle: "配置消息通知和提醒"
                        onClicked: {
                            messageSettingsDialog.open()
                        }
                    }
                }
            }
            
            // 退出登录按钮
            Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
                width: 200
                height: 44
                text: "退出登录"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: parent.pressed ? Qt.darker("#f44336", 1.2) : "#f44336"
                    radius: 22
                }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    logoutDialog.open()
                }
            }
        }
    }
    
    // 头像选择对话框
    Popup {
        id: avatarDialog
        
        anchors.centerIn: parent
        width: 400
        height: 300
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            Text {
                text: "选择一个头像或上传自定义头像"
                font.pixelSize: 14
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.fillWidth: true
            }
            
            // 默认头像选择
            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                cellWidth: 80
                cellHeight: 80
                
                model: ListModel {
                    ListElement { source: "qrc:/icons/avatar1.png" }
                    ListElement { source: "qrc:/icons/avatar2.png" }
                    ListElement { source: "qrc:/icons/avatar3.png" }
                    ListElement { source: "qrc:/icons/avatar4.png" }
                    ListElement { source: "qrc:/icons/avatar5.png" }
                }
                
                delegate: Rectangle {
                    width: 70
                    height: 70
                    color: "transparent"
                    radius: 30
                    clip: true
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 5
                        source: model.source
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                updateAvatar(model.source)
                                avatarDialog.close()
                            }
                        }
                    }
                    
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 5
                        color: "transparent"
                        border.width: avatarImage.source == model.source ? 3 : 0
                        border.color: Material.primary
                        radius: 30
                    }
                }
            }
            
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "上传自定义头像"
                onClicked: {
                    selectAvatarFile()
                }
            }
        }
        
        // 取消按钮
        Button {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            text: "取消"
            onClicked: avatarDialog.close()
        }
    }
    
    // 编辑邮箱对话框
    Popup {
        id: editEmailDialog
        
        anchors.centerIn: parent
        width: 350
        height: 200
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        property string currentValue: ""
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            TextField {
                id: emailField
                Layout.fillWidth: true
                placeholderText: "输入新邮箱地址"
                text: editEmailDialog.currentValue
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            Text {
                text: "修改邮箱后需要重新验证"
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.fillWidth: true
            }
        }
        
        // 按钮区域
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8
            
            Button {
                text: "取消"
                onClicked: editEmailDialog.close()
            }
            
            Button {
                text: "确定"
                onClicked: {
                    if (emailField.text.trim().length > 0) {
                        updateUserInfo("email", emailField.text.trim())
                    }
                    editEmailDialog.close()
                }
            }
        }
    }
    
    // 编辑个性签名对话框
    Popup {
        id: editSignatureDialog
        
        anchors.centerIn: parent
        width: 400
        height: 250
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        property string currentValue: ""
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                TextArea {
                    id: signatureField
                    placeholderText: "输入个性签名..."
                    text: editSignatureDialog.currentValue
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    
                    background: Rectangle {
                        color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                        border.width: parent.activeFocus ? 2 : 1
                        border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                        radius: 8
                    }
                }
            }
            
            Text {
                text: signatureField.length + "/100"
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.alignment: Qt.AlignRight
            }
        }
        
        // 按钮区域
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8
            
            Button {
                text: "取消"
                onClicked: editSignatureDialog.close()
            }
            
            Button {
                text: "确定"
                onClicked: {
                    updateUserInfo("signature", signatureField.text)
                    editSignatureDialog.close()
                }
            }
        }
    }
    
    // 修改密码对话框
    Popup {
        id: changePasswordDialog
        
        anchors.centerIn: parent
        width: 400
        height: 300
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            TextField {
                id: currentPasswordField
                Layout.fillWidth: true
                placeholderText: "当前密码"
                echoMode: TextInput.Password
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            TextField {
                id: newPasswordField
                Layout.fillWidth: true
                placeholderText: "新密码"
                echoMode: TextInput.Password
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            TextField {
                id: confirmPasswordField
                Layout.fillWidth: true
                placeholderText: "确认新密码"
                echoMode: TextInput.Password
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            Text {
                text: "密码长度应为8-20位，包含大小写字母和数字"
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
        }
        
        // 按钮区域
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8
            
            Button {
                text: "取消"
                onClicked: changePasswordDialog.close()
            }
            
            Button {
                text: "确定"
                onClicked: {
                    if (newPasswordField.text === confirmPasswordField.text) {
                        changePassword(currentPasswordField.text, newPasswordField.text)
                        currentPasswordField.text = ""
                        newPasswordField.text = ""
                        confirmPasswordField.text = ""
                        changePasswordDialog.close()
                    } else {
                        // 显示密码不匹配错误
                    }
                }
            }
        }
    }
    
    // 隐私设置对话框
    Popup {
        id: privacyDialog
        
        anchors.centerIn: parent
        width: 400
        height: 350
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 20
            
            PrivacySettingItem {
                title: "允许陌生人添加我为好友"
                subtitle: "关闭后只能通过搜索用户名添加"
                checked: true
            }
            
            PrivacySettingItem {
                title: "显示在线状态"
                subtitle: "让其他人看到您的在线状态"
                checked: true
            }
            
            PrivacySettingItem {
                title: "允许群聊邀请"
                subtitle: "允许好友邀请您加入群聊"
                checked: true
            }
            
            PrivacySettingItem {
                title: "消息已读回执"
                subtitle: "让发送者知道您已读取了消息"
                checked: true
            }
        }
        
        // 按钮区域
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8
            
            Button {
                text: "取消"
                onClicked: privacyDialog.close()
            }
            
            Button {
                text: "确定"
                onClicked: privacyDialog.close()
            }
        }
    }
    
    // 消息设置对话框
    Popup {
        id: messageSettingsDialog
        
        anchors.centerIn: parent
        width: 400
        height: 400
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 20
            
            MessageSettingItem {
                title: "新消息通知"
                subtitle: "接收新消息时显示通知"
                checked: true
            }
            
            MessageSettingItem {
                title: "声音提醒"
                subtitle: "接收消息时播放提示音"
                checked: true
            }
            
            MessageSettingItem {
                title: "震动提醒"
                subtitle: "接收消息时震动提醒"
                checked: false
            }
            
            MessageSettingItem {
                title: "群聊消息通知"
                subtitle: "接收群聊消息时显示通知"
                checked: true
            }
            
            MessageSettingItem {
                title: "免打扰模式"
                subtitle: "临时关闭所有通知"
                checked: false
            }
            
            // 按钮区域
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Layout.topMargin: 20
                spacing: 8
                
                Button {
                    text: "取消"
                    onClicked: messageSettingsDialog.close()
                }
                
                Button {
                    text: "确定"
                    onClicked: messageSettingsDialog.close()
                }
            }
        }
    }
    
    // 退出登录确认对话框
    Popup {
        id: logoutDialog
        
        anchors.centerIn: parent
        width: 300
        height: 150
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16
            
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "确定要退出登录吗？"
                font.pixelSize: 14
                color: Material.foreground
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8
                
                Button {
                    text: "取消"
                    onClicked: logoutDialog.close()
                }
                
                Button {
                    text: "退出"
                    onClicked: {
                        logout()
                        logoutDialog.close()
                    }
                }
            }
        }
    }
    
    // 文件选择对话框
    // 文件选择对话框（使用 Qt.labs.platform）
    // 注意：在实际应用中可能需要使用原生文件对话框或自定义实现
    property string selectedAvatarPath: ""
    
    function selectAvatarFile() {
        // TODO: 实现文件选择逻辑
        // 可以使用 Qt.labs.platform.FileDialog 或其他方式
        console.log("选择头像文件")
    }
    
    // 个人信息项组件
    component ProfileInfoItem: RowLayout {
        property string label: ""
        property string value: ""
        property bool editable: true
        
        signal editClicked()
        
        Layout.fillWidth: true
        spacing: 12
        
        Text {
            text: label + ":"
            font.pixelSize: 14
            color: Material.theme === Material.Dark ? "#888888" : "#666666"
            Layout.preferredWidth: 80
        }
        
        Text {
            text: value || "未设置"
            font.pixelSize: 14
            color: Material.foreground
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        
        ToolButton {
            icon.source: "qrc:/icons/edit.png"
            icon.width: 16
            icon.height: 16
            visible: editable
            
            onClicked: editClicked()
        }
    }
    
    // 个人操作项组件
    component ProfileActionItem: Rectangle {
        property string icon: ""
        property string title: ""
        property string subtitle: ""
        
        signal clicked()
        
        Layout.fillWidth: true
        height: 60
        color: mouseArea.containsMouse ? Qt.alpha(Material.primary, 0.1) : "transparent"
        radius: 8
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.clicked()
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12
            
            Rectangle {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                color: Material.foreground
                radius: 2
                
                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: icon
                    fillMode: Image.PreserveAspectFit
                }
            }
            
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                
                Text {
                    text: title
                    font.pixelSize: 14
                    font.bold: true
                    color: Material.foreground
                    Layout.fillWidth: true
                }
                
                Text {
                    text: subtitle
                    font.pixelSize: 12
                    color: Material.theme === Material.Dark ? "#888888" : "#666666"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }
            }
            
            Rectangle {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                color: Material.theme === Material.Dark ? "#666666" : "#999999"
                radius: 1
                
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    source: "qrc:/icons/arrow-right.png"
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }
    
    // 隐私设置项组件
    component PrivacySettingItem: RowLayout {
        property string title: ""
        property string subtitle: ""
        property bool checked: false
        
        Layout.fillWidth: true
        spacing: 12
        
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            
            Text {
                text: title
                font.pixelSize: 14
                font.bold: true
                color: Material.foreground
                Layout.fillWidth: true
            }
            
            Text {
                text: subtitle
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
        }
        
        Switch {
            checked: parent.checked
            onToggled: {
                parent.checked = checked
                // TODO: 保存设置
            }
        }
    }
    
    // 消息设置项组件
    component MessageSettingItem: RowLayout {
        property string title: ""
        property string subtitle: ""
        property bool checked: false
        
        Layout.fillWidth: true
        spacing: 12
        
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            
            Text {
                text: title
                font.pixelSize: 14
                font.bold: true
                color: Material.foreground
                Layout.fillWidth: true
            }
            
            Text {
                text: subtitle
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
        }
        
        Switch {
            checked: parent.checked
            onToggled: {
                parent.checked = checked
                // TODO: 保存设置
            }
        }
    }
    
    // 工具函数
    function updateAvatar(avatarUrl) {
        // TODO: 调用userController更新头像
        console.log("更新头像:", avatarUrl)
    }
    
    function updateUserInfo(field, value) {
        // TODO: 调用userController更新用户信息
        console.log("更新用户信息:", field, value)
    }
    
    function changePassword(currentPassword, newPassword) {
        // TODO: 调用userController修改密码
        console.log("修改密码")
    }
    
    function logout() {
        // TODO: 调用userController退出登录
        userController.logout()
    }
}