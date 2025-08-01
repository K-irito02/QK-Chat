import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: settingsPage
    
    signal themeChanged(string newTheme)
    
    color: Material.background
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        
        ColumnLayout {
            width: settingsPage.width - 32
            spacing: 24
            
            // 页面标题
            Text {
                text: "设置"
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
                Layout.alignment: Qt.AlignHCenter
            }
            
            // 外观设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: appearanceColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: appearanceColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "外观设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    // 主题设置
                    SettingItem {
                        title: "主题模式"
                        subtitle: "选择浅色或深色主题"
                        
                        content: Row {
                            spacing: 8
                            
                            RadioButton {
                                text: "浅色"
                                checked: configManager.theme === "light"
                                onToggled: {
                                    if (checked) {
                                        configManager.setTheme("light")
                                        themeChanged("light")
                                    }
                                }
                            }
                            
                            RadioButton {
                                text: "深色"
                                checked: configManager.theme === "dark"
                                onToggled: {
                                    if (checked) {
                                        configManager.setTheme("dark")
                                        themeChanged("dark")
                                    }
                                }
                            }
                            
                            RadioButton {
                                text: "跟随系统"
                                checked: configManager.theme === "system"
                                onToggled: {
                                    if (checked) {
                                        configManager.setTheme("system")
                                        themeChanged("system")
                                    }
                                }
                            }
                        }
                    }
                    
                    // 字体大小
                    SettingItem {
                        title: "字体大小"
                        subtitle: "调整界面字体大小"
                        
                        content: Row {
                            spacing: 12
                            
                            Text {
                                text: "小"
                                font.pixelSize: 12
                                color: Material.foreground
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Slider {
                                width: 120
                                from: 12
                                to: 18
                                stepSize: 1
                                value: 14
                                
                                onValueChanged: {
                                    // TODO: 保存字体大小设置
                                }
                            }
                            
                            Text {
                                text: "大"
                                font.pixelSize: 16
                                color: Material.foreground
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                    
                    // 主色调
                    SettingItem {
                        title: "主色调"
                        subtitle: "选择应用的主色调"
                        
                        content: Row {
                            spacing: 8
                            
                            Repeater {
                                model: [
                                    {"name": "蓝色", "color": "#2196F3"},
                                    {"name": "绿色", "color": "#4CAF50"},
                                    {"name": "紫色", "color": "#9C27B0"},
                                    {"name": "橙色", "color": "#FF9800"},
                                    {"name": "红色", "color": "#F44336"}
                                ]
                                
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: modelData.color
                                    border.width: configManager.primaryColor === modelData.color ? 3 : 1
                                    border.color: configManager.primaryColor === modelData.color ? Material.foreground : Qt.alpha(Material.foreground, 0.3)
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            configManager.setPrimaryColor(modelData.color)
                                        }
                                    }
                                    
                                    ToolTip {
                                        visible: parent.MouseArea.containsMouse
                                        text: modelData.name
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // 聊天设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: chatColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: chatColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "聊天设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    SettingItem {
                        title: "发送快捷键"
                        subtitle: "选择发送消息的快捷键"
                        
                        content: ComboBox {
                            width: 120
                            model: ["Enter", "Ctrl+Enter", "Shift+Enter"]
                            currentIndex: 0
                            
                            onCurrentTextChanged: {
                                // TODO: 保存发送快捷键设置
                            }
                        }
                    }
                    
                    SettingSwitchItem {
                        title: "显示时间戳"
                        subtitle: "在消息旁显示发送时间"
                        checked: true
                    }
                    
                    SettingSwitchItem {
                        title: "消息预览"
                        subtitle: "在通知中显示消息内容"
                        checked: true
                    }
                    
                    SettingSwitchItem {
                        title: "自动下载图片"
                        subtitle: "自动下载聊天中的图片"
                        checked: true
                    }
                    
                    SettingSwitchItem {
                        title: "表情包动画"
                        subtitle: "播放动态表情包"
                        checked: true
                    }
                }
            }
            
            // 通知设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: notificationColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: notificationColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "通知设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    SettingSwitchItem {
                        title: "桌面通知"
                        subtitle: "显示桌面弹窗通知"
                        checked: true
                    }
                    
                    SettingSwitchItem {
                        title: "声音提醒"
                        subtitle: "播放消息提示音"
                        checked: true
                    }
                    
                    SettingItem {
                        title: "免打扰时间"
                        subtitle: "设置免打扰的时间段"
                        
                        content: Row {
                            spacing: 8
                            
                            TextField {
                                width: 60
                                text: "22:00"
                                horizontalAlignment: TextInput.AlignHCenter
                                
                                background: Rectangle {
                                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                                    border.width: parent.activeFocus ? 2 : 1
                                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                                    radius: 4
                                }
                            }
                            
                            Text {
                                text: "至"
                                color: Material.foreground
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                width: 60
                                text: "08:00"
                                horizontalAlignment: TextInput.AlignHCenter
                                
                                background: Rectangle {
                                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                                    border.width: parent.activeFocus ? 2 : 1
                                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                                    radius: 4
                                }
                            }
                        }
                    }
                    
                    SettingSwitchItem {
                        title: "群聊@提醒"
                        subtitle: "被@时单独提醒"
                        checked: true
                    }
                }
            }
            
            // 安全设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: securityColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: securityColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "安全设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    SettingSwitchItem {
                        title: "自动锁定"
                        subtitle: "长时间未操作时自动锁定"
                        checked: false
                    }
                    
                    SettingItem {
                        title: "自动锁定时间"
                        subtitle: "设置自动锁定的时间"
                        enabled: false // 当自动锁定开启时才启用
                        
                        content: ComboBox {
                            width: 120
                            model: ["5分钟", "10分钟", "30分钟", "1小时"]
                            currentIndex: 1
                            enabled: false
                        }
                    }
                    
                    SettingSwitchItem {
                        title: "消息加密"
                        subtitle: "对聊天消息进行端到端加密"
                        checked: true
                    }
                    
                    SettingActionItem {
                        title: "清除聊天记录"
                        subtitle: "删除所有本地聊天记录"
                        actionText: "清除"
                        isDestructive: true
                        
                        onActionClicked: {
                            clearDataDialog.open()
                        }
                    }
                }
            }
            
            // 存储设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: storageColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: storageColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "存储设置"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    SettingItem {
                        title: "缓存大小"
                        subtitle: "当前使用 256 MB"
                        
                        content: Button {
                            text: "清理缓存"
                            
                            background: Rectangle {
                                color: parent.pressed ? Qt.alpha(Material.primary, 0.8) : "transparent"
                                border.width: 1
                                border.color: Material.primary
                                radius: 16
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: Material.primary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: {
                                // TODO: 清理缓存
                            }
                        }
                    }
                    
                    SettingItem {
                        title: "自动清理"
                        subtitle: "定期清理过期的缓存文件"
                        
                        content: ComboBox {
                            width: 120
                            model: ["从不", "每周", "每月"]
                            currentIndex: 1
                        }
                    }
                    
                    SettingSwitchItem {
                        title: "WiFi下自动下载"
                        subtitle: "仅在WiFi环境下自动下载文件"
                        checked: true
                    }
                }
            }
            
            // 关于信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: aboutColumn.height + 48
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 16
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                
                Column {
                    id: aboutColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "关于"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }
                    
                    SettingActionItem {
                        title: "版本信息"
                        subtitle: "QK Chat v1.0.0"
                        actionText: "检查更新"
                        
                        onActionClicked: {
                            // TODO: 检查更新
                        }
                    }
                    
                    SettingActionItem {
                        title: "使用帮助"
                        subtitle: "查看使用说明和常见问题"
                        actionText: "查看"
                        
                        onActionClicked: {
                            // TODO: 打开帮助页面
                        }
                    }
                    
                    SettingActionItem {
                        title: "反馈建议"
                        subtitle: "向我们反馈问题或建议"
                        actionText: "反馈"
                        
                        onActionClicked: {
                            // TODO: 打开反馈页面
                        }
                    }
                }
            }
        }
    }
    
    // 清除数据确认对话框
    Popup {
        id: clearDataDialog
        
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
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            Text {
                text: "此操作将永久删除所有本地聊天记录，无法恢复。"
                font.pixelSize: 14
                color: Material.foreground
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            
            Text {
                text: "确定要继续吗？"
                font.pixelSize: 14
                font.bold: true
                color: "#f44336"
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
                onClicked: clearDataDialog.close()
            }
            
            Button {
                text: "确定"
                onClicked: {
                    // TODO: 清除聊天记录
                    console.log("清除聊天记录")
                    clearDataDialog.close()
                }
            }
        }
    }
    
    // 设置项基础组件
    component SettingItem: ColumnLayout {
        property string title: ""
        property string subtitle: ""
        property alias content: contentLoader.sourceComponent
        property bool enabled: true
        
        Layout.fillWidth: true
        spacing: 8
        opacity: enabled ? 1.0 : 0.5
        
        RowLayout {
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
            
            Loader {
                id: contentLoader
                Layout.alignment: Qt.AlignRight
            }
        }
    }
    
    // 开关设置项组件
    component SettingSwitchItem: RowLayout {
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
    
    // 操作设置项组件
    component SettingActionItem: RowLayout {
        property string title: ""
        property string subtitle: ""
        property string actionText: ""
        property bool isDestructive: false
        
        signal actionClicked()
        
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
        
        Button {
            text: actionText
            
            background: Rectangle {
                color: parent.pressed ? (isDestructive ? Qt.darker("#f44336", 1.2) : Qt.alpha(Material.primary, 0.8)) : "transparent"
                border.width: 1
                border.color: isDestructive ? "#f44336" : Material.primary
                radius: 16
            }
            
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: isDestructive ? "#f44336" : Material.primary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: actionClicked()
        }
    }
}