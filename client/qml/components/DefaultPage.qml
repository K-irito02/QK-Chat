import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: defaultPage
    
    signal contactSelected(var contact)
    signal groupSelected(var group)
    
    color: Material.background
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16
        
        // 页面标题
        Text {
            text: "最近聊天"
            font.pixelSize: 18
            font.bold: true
            color: Material.foreground
            Layout.fillWidth: true
        }
        
        // 搜索框
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "搜索联系人或群聊..."
            
            background: Rectangle {
                color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                border.width: searchField.activeFocus ? 2 : 1
                border.color: searchField.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                radius: 8
            }
            
            leftPadding: 40
            
            Image {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                source: "qrc:/icons/search.png"
                opacity: 0.6
            }
        }
        
        // 最近聊天列表
        ListView {
            id: recentChatsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            model: recentChatsModel
            delegate: recentChatDelegate
            
            ScrollBar.vertical: ScrollBar {
                active: true
            }
        }
        
        // 空状态提示
        Column {
            Layout.alignment: Qt.AlignCenter
            spacing: 16
            visible: recentChatsModel.count === 0
            
            Image {
                source: "qrc:/icons/chat-empty.png"
                width: 64
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }
            
            Text {
                text: "暂无最近聊天记录"
                font.pixelSize: 14
                color: Material.theme === Material.Dark ? "#666666" : "#999999"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "去联系人或群聊页面开始聊天吧"
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#555555" : "#aaaaaa"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    
    // 最近聊天数据模型
    ListModel {
        id: recentChatsModel
        
        Component.onCompleted: {
            // 从数据库加载最近聊天记录
            loadRecentChats()
        }
    }
    
    // 最近聊天项目委托
    Component {
        id: recentChatDelegate
        
        Rectangle {
            width: recentChatsList.width
            height: 72
            color: mouseArea.containsMouse ? Qt.alpha(Material.primary, 0.1) : "transparent"
            radius: 8
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
            
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (model.isGroup) {
                        groupSelected(model)
                    } else {
                        contactSelected(model)
                    }
                }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                
                // 头像
                Rectangle {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    color: "transparent"
                    radius: 24
                    clip: true
                    
                    Image {
                        anchors.fill: parent
                        source: model.avatar || "qrc:/icons/avatar1.png"
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                    
                    // 在线状态指示器（仅联系人）
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: model.isOnline ? "#4CAF50" : "#757575"
                        border.width: 2
                        border.color: Material.background
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        visible: !model.isGroup
                    }
                    
                    // 群聊成员数量指示器
                    Rectangle {
                        width: 20
                        height: 12
                        radius: 6
                        color: Material.primary
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        visible: model.isGroup
                        
                        Text {
                            anchors.centerIn: parent
                            text: model.memberCount || 0
                            font.pixelSize: 8
                            color: "white"
                        }
                    }
                }
                
                // 聊天信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Text {
                            text: model.name || "未知"
                            font.pixelSize: 16
                            font.bold: model.unreadCount > 0
                            color: Material.foreground
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        
                        Text {
                            text: formatMessageTime(model.lastMessageTime)
                            font.pixelSize: 12
                            color: Material.theme === Material.Dark ? "#888888" : "#666666"
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Text {
                            text: model.lastMessage || "暂无消息"
                            font.pixelSize: 14
                            color: Material.theme === Material.Dark ? "#aaaaaa" : "#777777"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                        
                        // 未读消息数量
                        Rectangle {
                            width: 20
                            height: 16
                            radius: 8
                            color: "#f44336"
                            visible: model.unreadCount > 0
                            
                            Text {
                                anchors.centerIn: parent
                                text: model.unreadCount > 99 ? "99+" : model.unreadCount
                                font.pixelSize: 10
                                color: "white"
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 工具函数
    function formatMessageTime(timestamp) {
        if (!timestamp) return ""
        
        var date = new Date(timestamp)
        var now = new Date()
        var today = new Date(now.getFullYear(), now.getMonth(), now.getDate())
        var messageDate = new Date(date.getFullYear(), date.getMonth(), date.getDate())
        
        if (messageDate.getTime() === today.getTime()) {
            return date.toLocaleTimeString(Qt.locale(), "hh:mm")
        } else if (messageDate.getTime() === today.getTime() - 86400000) {
            return "昨天"
        } else if (now.getTime() - messageDate.getTime() < 7 * 86400000) {
            return date.toLocaleDateString(Qt.locale(), "ddd")
        } else {
            return date.toLocaleDateString(Qt.locale(), "MM/dd")
        }
    }
    
    function loadRecentChats() {
        // 从本地数据库加载最近聊天记录
        // 这里添加示例数据，实际应该从数据库加载
        recentChatsModel.append({
            "id": 1,
            "name": "张三",
            "avatar": "qrc:/icons/avatar1.png",
            "isGroup": false,
            "isOnline": true,
            "lastMessage": "你好，最近怎么样？",
            "lastMessageTime": Date.now() - 300000, // 5分钟前
            "unreadCount": 2
        })
        
        recentChatsModel.append({
            "id": 2,
            "name": "前端开发群",
            "avatar": "qrc:/icons/group.png",
            "isGroup": true,
            "memberCount": 15,
            "lastMessage": "李四: 大家看看这个新框架",
            "lastMessageTime": Date.now() - 1800000, // 30分钟前
            "unreadCount": 5
        })
        
        recentChatsModel.append({
            "id": 3,
            "name": "李四",
            "avatar": "qrc:/icons/avatar2.png",
            "isGroup": false,
            "isOnline": false,
            "lastMessage": "明天见！",
            "lastMessageTime": Date.now() - 86400000, // 昨天
            "unreadCount": 0
        })
    }
    
    // 搜索功能
    Connections {
        target: searchField
        onTextChanged: {
            // TODO: 实现搜索逻辑，根据 searchField.text 过滤 recentChatsModel
        }
    }
}