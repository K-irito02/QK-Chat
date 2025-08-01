import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Item {
    id: emojiPicker
    
    signal emojiSelected(string emoji)
    
    // 表情分类
    property var emojiCategories: [
        {
            name: "常用",
            icon: "😊",
            emojis: ["😀", "😃", "😄", "😁", "😆", "😅", "😂", "🤣", "😊", "😇", "🙂", "🙃", "😉", "😌", "😍", "🥰", "😘", "😗", "😙", "😚", "😋", "😛", "😝", "😜", "🤪", "🤨", "🧐", "🤓", "😎", "🤩", "🥳"]
        },
        {
            name: "人物",
            icon: "👤",
            emojis: ["👶", "🧒", "👦", "👧", "🧑", "👨", "👩", "🧓", "👴", "👵", "👤", "👥", "👶🏻", "👶🏼", "👶🏽", "👶🏾", "👶🏿", "🧒🏻", "🧒🏼", "🧒🏽", "🧒🏾", "🧒🏿"]
        },
        {
            name: "动物",
            icon: "🐶",
            emojis: ["🐶", "🐱", "🐭", "🐹", "🐰", "🦊", "🐻", "🐼", "🐨", "🐯", "🦁", "🐮", "🐷", "🐽", "🐸", "🐵", "🙈", "🙉", "🙊", "🐒", "🐔", "🐧", "🐦", "🐤", "🐣", "🐥"]
        },
        {
            name: "食物",
            icon: "🍎",
            emojis: ["🍎", "🍊", "🍋", "🍌", "🍉", "🍇", "🍓", "🫐", "🍈", "🍒", "🍑", "🥭", "🍍", "🥥", "🥝", "🍅", "🍆", "🥑", "🥦", "🥬", "🥒", "🌶", "🫑", "🌽", "🥕", "🫒"]
        },
        {
            name: "活动",
            icon: "⚽",
            emojis: ["⚽", "🏀", "🏈", "⚾", "🥎", "🎾", "🏐", "🏉", "🥏", "🎱", "🪀", "🏓", "🏸", "🏒", "🏑", "🥍", "🏏", "🪃", "🥅", "⛳", "🪁", "🏹", "🎣", "🤿", "🥊", "🥋"]
        },
        {
            name: "符号",
            icon: "❤️",
            emojis: ["❤️", "🧡", "💛", "💚", "💙", "💜", "🖤", "🤍", "🤎", "💔", "❣️", "💕", "💞", "💓", "💗", "💖", "💘", "💝", "💟", "☮️", "✝️", "☪️", "🕉", "☸️", "✡️"]
        }
    ]
    
    property int currentCategory: 0
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 0
        
        // 分类标签栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "transparent"
            
            RowLayout {
                anchors.fill: parent
                spacing: 0
                
                Repeater {
                    model: emojiCategories
                    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: currentCategory === index ? Material.accent : "transparent"
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentCategory = index
                        }
                        
                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon
                            font.pointSize: 16
                        }
                        
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 2
                            color: Material.accent
                            visible: currentCategory === index
                        }
                    }
                }
            }
        }
        
        // 表情网格
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            GridView {
                id: emojiGrid
                cellWidth: 40
                cellHeight: 40
                model: emojiCategories[currentCategory].emojis
                
                delegate: Rectangle {
                    width: 40
                    height: 40
                    color: emojiMouseArea.pressed ? Material.accent : "transparent"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pointSize: 20
                    }
                    
                    MouseArea {
                        id: emojiMouseArea
                        anchors.fill: parent
                        onClicked: {
                            emojiPicker.emojiSelected(modelData)
                        }
                    }
                    
                    // 鼠标悬停效果
                    Rectangle {
                        anchors.fill: parent
                        color: Material.accent
                        opacity: 0.1
                        radius: parent.radius
                        visible: emojiMouseArea.containsMouse
                    }
                }
            }
        }
        
        // 最近使用的表情
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: recentEmojis.length > 0 ? 50 : 0
            color: Material.dialogColor
            visible: recentEmojis.length > 0
            
            property var recentEmojis: getRecentEmojis()
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                
                Text {
                    text: "最近使用:"
                    font.pointSize: 10
                    color: Material.secondaryTextColor
                }
                
                Repeater {
                    model: parent.parent.recentEmojis
                    
                    Rectangle {
                        width: 30
                        height: 30
                        color: recentMouseArea.pressed ? Material.accent : "transparent"
                        radius: 4
                        
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.pointSize: 16
                        }
                        
                        MouseArea {
                            id: recentMouseArea
                            anchors.fill: parent
                            onClicked: {
                                emojiPicker.emojiSelected(modelData)
                                addToRecentEmojis(modelData)
                            }
                        }
                    }
                }
                
                Item { Layout.fillWidth: true }
            }
        }
    }
    
    // 添加表情到最近使用
    function addToRecentEmojis(emoji) {
        let recent = getRecentEmojis()
        
        // 移除已存在的
        const index = recent.indexOf(emoji)
        if (index > -1) {
            recent.splice(index, 1)
        }
        
        // 添加到开头
        recent.unshift(emoji)
        
        // 保持最多10个
        if (recent.length > 10) {
            recent = recent.slice(0, 10)
        }
        
        // 保存到本地存储
        saveRecentEmojis(recent)
    }
    
    function getRecentEmojis() {
        // TODO: 从本地存储读取
        return ["😀", "😍", "👍", "❤️", "😂"]
    }
    
    function saveRecentEmojis(emojis) {
        // TODO: 保存到本地存储
        console.log("Save recent emojis:", emojis)
    }
} 