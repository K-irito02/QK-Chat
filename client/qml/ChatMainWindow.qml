import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import "components"

ApplicationWindow {
    id: chatMainWindow
    width: 1200
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: "QK Chat"
    
    Material.theme: configManager.theme === "dark" ? Material.Dark : Material.Light
    Material.primary: configManager.primaryColor
    Material.accent: configManager.accentColor
    
    property string currentUser: userModel.username
    property url currentUserAvatar: userModel.avatar
    property int currentPage: 0  // 0: 默认页面, 1: 联系人, 2: 群聊, 3: 添加, 4: 个人中心, 5: 设置
    property var selectedContact: null
    property var selectedGroup: null
    property bool isGroupChat: false
    
    // 页面类型枚举
    readonly property int pageDefault: 0
    readonly property int pageContacts: 1
    readonly property int pageGroups: 2
    readonly property int pageAdd: 3
    readonly property int pageProfile: 4
    readonly property int pageSettings: 5
    
    // 初始化聊天控制器
    Component.onCompleted: {
        chatController.initialize()
    }
    
    RowLayout {
        anchors.fill: parent
        spacing: 0
        
        // 左侧边栏
        Rectangle {
            Layout.preferredWidth: 240
            Layout.fillHeight: true
            color: Material.theme === Material.Dark ? "#2b2b2b" : "#f5f5f5"
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
            
            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4
                
                // 头像区域
                Rectangle {
                    width: parent.width
                    height: 80
                    color: "transparent"
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 12
                        
                        Rectangle {
                            width: 48
                            height: 48
                            radius: 24
                            clip: true
                            color: "transparent"
                            border.width: 2
                            border.color: Material.primary
                            
                            Image {
                                anchors.fill: parent
                                source: currentUserAvatar
                                fillMode: Image.PreserveAspectCrop
                                
                                layer.enabled: true
                            }
                        }
                        
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            
                            Text {
                                text: currentUser
                                font.pixelSize: 16
                                font.bold: true
                                color: Material.foreground
                            }
                            
                            Text {
                                text: "在线"
                                font.pixelSize: 12
                                color: Material.theme === Material.Dark ? "#81C784" : "#4CAF50"
                            }
                        }
                    }
                }
                
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
                }
                
                // 导航按钮区域
                Column {
                    width: parent.width
                    spacing: 2
                    
                    SideBarButton {
                        width: parent.width
                        text: "默认页面"
                        iconSource: "qrc:/icons/home.png"
                        selected: currentPage === pageDefault
                        onClicked: {
                            currentPage = pageDefault
                            selectedContact = null
                            selectedGroup = null
                        }
                    }
                    
                    SideBarButton {
                        width: parent.width
                        text: "联系人页面"
                        iconSource: "qrc:/icons/user.png"
                        selected: currentPage === pageContacts
                        onClicked: {
                            currentPage = pageContacts
                            selectedGroup = null
                        }
                    }
                    
                    SideBarButton {
                        width: parent.width
                        text: "群聊页面"
                        iconSource: "qrc:/icons/group.png"
                        selected: currentPage === pageGroups
                        onClicked: {
                            currentPage = pageGroups
                            selectedContact = null
                        }
                    }
                    
                    SideBarButton {
                        width: parent.width
                        text: "添加或创建页面"
                        iconSource: "qrc:/icons/add.png"
                        selected: currentPage === pageAdd
                        onClicked: {
                            currentPage = pageAdd
                            selectedContact = null
                            selectedGroup = null
                        }
                    }
                }
                
                Item {
                    width: parent.width
                    height: 20
                }
                
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
                }
                
                // 底部按钮区域
                Column {
                    width: parent.width
                    spacing: 2
                    
                    SideBarButton {
                        width: parent.width
                        text: "个人中心"
                        iconSource: "qrc:/icons/profile.png"
                        selected: currentPage === pageProfile
                        onClicked: {
                            currentPage = pageProfile
                            selectedContact = null
                            selectedGroup = null
                        }
                    }
                    
                    SideBarButton {
                        width: parent.width
                        text: "设置"
                        iconSource: "qrc:/icons/settings.png"
                        selected: currentPage === pageSettings
                        onClicked: {
                            currentPage = pageSettings
                            selectedContact = null
                            selectedGroup = null
                        }
                    }
                }
            }
        }
        
        // 中间内容区域
        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: Material.background
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
            
            Loader {
                id: middlePaneLoader
                anchors.fill: parent
                anchors.margins: 1
                
                sourceComponent: {
                    switch(currentPage) {
                        case pageDefault: return defaultPageComponent
                        case pageContacts: return contactsPageComponent
                        case pageGroups: return groupsPageComponent
                        case pageAdd: return addPageComponent
                        case pageProfile: return profilePageComponent
                        case pageSettings: return settingsPageComponent
                        default: return defaultPageComponent
                    }
                }
            }
        }
        
        // 右侧聊天区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Material.background
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
            
            ChatWindow {
                id: chatWindow
                anchors.fill: parent
                anchors.margins: 1
                
                // 根据选中的联系人或群组显示聊天内容
                visible: selectedContact !== null || selectedGroup !== null
                currentContact: selectedContact
                currentGroup: selectedGroup
                isGroupChat: chatMainWindow.isGroupChat
                
                onSendMessage: function(message, messageType) {
                    // 发送消息逻辑
                    if (isGroupChat && selectedGroup) {
                        chatController.sendGroupMessage(selectedGroup.id, message, messageType)
                    } else if (selectedContact) {
                        chatController.sendMessage(selectedContact.id, message, messageType)
                    }
                }
            }
            
            // 默认显示页面
            Column {
                anchors.centerIn: parent
                spacing: 20
                visible: selectedContact === null && selectedGroup === null
                
                Image {
                    source: "qrc:/icons/logo.png"
                    width: 120
                    height: 120
                    anchors.horizontalCenter: parent.horizontalCenter
                    opacity: 0.6
                }
                
                Text {
                    text: "根据用户点击的联系人或群聊，显示聊天界面"
                    font.pixelSize: 16
                    color: Material.theme === Material.Dark ? "#888888" : "#666666"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "选择一个联系人或群组开始聊天"
                    font.pixelSize: 14
                    color: Material.theme === Material.Dark ? "#666666" : "#999999"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
    
    // 页面组件定义
    Component {
        id: defaultPageComponent
        DefaultPage {
            onContactSelected: function(contact) {
                selectedContact = contact
                selectedGroup = null
                isGroupChat = false
            }
            onGroupSelected: function(group) {
                selectedGroup = group
                selectedContact = null
                isGroupChat = true
            }
        }
    }
    
    Component {
        id: contactsPageComponent
        ContactsPage {
            onContactSelected: function(contact) {
                selectedContact = contact
                selectedGroup = null
                isGroupChat = false
            }
        }
    }
    
    Component {
        id: groupsPageComponent
        GroupsPage {
            onGroupSelected: function(group) {
                selectedGroup = group
                selectedContact = null
                isGroupChat = true
            }
        }
    }
    
    Component {
        id: addPageComponent
        AddPage {
            onContactAdded: {
                // 刷新联系人列表
                currentPage = pageContacts
            }
            onGroupCreated: {
                // 刷新群组列表
                currentPage = pageGroups
            }
        }
    }
    
    Component {
        id: profilePageComponent
        ProfilePage {
            userInfo: userModel
        }
    }
    
    Component {
        id: settingsPageComponent
        SettingsPage {
            onThemeChanged: function(newTheme) {
                configManager.setTheme(newTheme)
            }
        }
    }
    
    // 连接控制器信号
    Connections {
        target: chatController
        
        function onMessageReceived(senderId, message, messageType, timestamp) {
            // 处理接收到的消息
            if (selectedContact && selectedContact.id === senderId) {
                chatWindow.addMessage(senderId, message, messageType, timestamp, false)
            }
        }
        
        function onGroupMessageReceived(groupId, senderId, message, messageType, timestamp) {
            // 处理接收到的群组消息
            if (selectedGroup && selectedGroup.id === groupId) {
                chatWindow.addGroupMessage(groupId, senderId, message, messageType, timestamp, false)
            }
        }
    }
}