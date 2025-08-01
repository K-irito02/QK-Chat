import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
        id: messageBubble
        
        property var messageData
        property bool isOwn: messageData ? messageData.isOwn : false
        property string senderName: messageData ? messageData.senderName : ""
        property url senderAvatar: messageData ? messageData.senderAvatar : ""
        property string message: messageData ? messageData.message : ""
        property int messageType: messageData ? messageData.messageType : 0 // 0: 文本, 1: 图片, 2: 文件
        property var timestamp: messageData ? messageData.timestamp : 0
        property bool isSent: messageData ? messageData.isSent : false
        
        signal clicked()
        signal longPressed()
        
        height: messageContainer.height + 16
    
    // 格式化时间
    function formatTime(timestamp) {
        var date = new Date(timestamp)
        var now = new Date()
        var today = new Date(now.getFullYear(), now.getMonth(), now.getDate())
        var messageDate = new Date(date.getFullYear(), date.getMonth(), date.getDate())
        
        if (messageDate.getTime() === today.getTime()) {
            // 今天，只显示时间
            return date.toLocaleTimeString(Qt.locale(), "hh:mm")
        } else if (messageDate.getTime() === today.getTime() - 86400000) {
            // 昨天
            return "昨天 " + date.toLocaleTimeString(Qt.locale(), "hh:mm")
        } else {
            // 其他日期
            return date.toLocaleDateString(Qt.locale(), "MM/dd hh:mm")
        }
    }
    
    Row {
        id: messageContainer
        anchors.left: isOwn ? undefined : parent.left
        anchors.right: isOwn ? parent.right : undefined
        anchors.leftMargin: isOwn ? 60 : 8
        anchors.rightMargin: isOwn ? 8 : 60
        anchors.top: parent.top
        anchors.topMargin: 8
        
        spacing: 8
        layoutDirection: isOwn ? Qt.RightToLeft : Qt.LeftToRight
        
        // 头像
        Image {
            width: 36
            height: 36
            source: senderAvatar
            fillMode: Image.PreserveAspectCrop
            visible: !isOwn
            
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: Material.theme === Material.Dark ? "#606060" : "#c0c0c0"
            }
        }
        
        // 消息内容区域
        Column {
            spacing: 4
            
            // 发送者名称（仅在群聊且不是自己的消息时显示）
            Text {
                text: senderName
                font.pixelSize: 12
                color: Material.theme === Material.Dark ? "#888888" : "#666666"
                visible: !isOwn && senderName !== ""
                anchors.left: parent.left
            }
            
            // 消息气泡
            Rectangle {
                id: bubbleBackground
                width: Math.min(messageContent.width + 24, messageBubble.width * 0.7)
                height: messageContent.height + 16
                
                color: {
                    if (isOwn) {
                        return Material.primary
                    } else {
                        return Material.theme === Material.Dark ? "#404040" : "#f0f0f0"
                    }
                }
                
                radius: 18
                
                // 尖角效果
                Canvas {
                    id: bubbleTail
                    width: 12
                    height: 12
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 8
                    anchors.left: isOwn ? undefined : parent.left
                    anchors.right: isOwn ? parent.right : undefined
                    anchors.leftMargin: isOwn ? 0 : -6
                    anchors.rightMargin: isOwn ? -6 : 0
                    
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.fillStyle = bubbleBackground.color
                        ctx.beginPath()
                        
                        if (isOwn) {
                            ctx.moveTo(6, 0)
                            ctx.lineTo(12, 6)
                            ctx.lineTo(0, 6)
                        } else {
                            ctx.moveTo(6, 0)
                            ctx.lineTo(0, 6)
                            ctx.lineTo(12, 6)
                        }
                        
                        ctx.closePath()
                        ctx.fill()
                    }
                }
                
                // 消息内容
                Item {
                    id: messageContent
                    anchors.centerIn: parent
                    width: contentLoader.width
                    height: contentLoader.height
                    
                    Loader {
                        id: contentLoader
                        sourceComponent: {
                            switch(messageType) {
                                case 0: return textMessageComponent
                                case 1: return imageMessageComponent
                                case 2: return fileMessageComponent
                                default: return textMessageComponent
                            }
                        }
                    }
                }
                
                // 添加鼠标区域处理点击和长按
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    
                    onClicked: {
                        messageBubble.clicked()
                    }
                    
                    onPressAndHold: {
                        messageBubble.longPressed()
                    }
                }
            }
            
            // 时间戳和状态
            Row {
                spacing: 4
                anchors.right: isOwn ? parent.right : undefined
                anchors.left: isOwn ? undefined : parent.left
                
                Text {
                    text: formatTime(timestamp)
                    font.pixelSize: 10
                    color: Material.theme === Material.Dark ? "#666666" : "#999999"
                }
                
                // 消息状态（仅自己的消息显示）
                Image {
                    width: 12
                    height: 12
                    source: {
                        if (!isOwn) return ""
                        if (isSent) return "qrc:/icons/message-sent.png"
                        return "qrc:/icons/message-sending.png"
                    }
                    visible: isOwn
                }
            }
        }
    }
    
    // 文本消息组件
    Component {
        id: textMessageComponent
        
        Text {
            text: message
            font.pixelSize: 14
            color: isOwn ? "white" : Material.foreground
            wrapMode: Text.Wrap
            maximumLineCount: 50
            
            // 支持链接点击
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }
            
            // 自动检测链接
            textFormat: Text.RichText
        }
    }
    
    // 图片消息组件
    Component {
        id: imageMessageComponent
        
        Column {
            spacing: 8
            
            Image {
                source: message // message字段存储图片路径
                fillMode: Image.PreserveAspectFit
                width: Math.min(sourceSize.width, 200)
                height: Math.min(sourceSize.height, 200)
                
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.width: 1
                    border.color: Material.theme === Material.Dark ? "#606060" : "#c0c0c0"
                    radius: 8
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // 点击放大图片
                        imagePreview.source = message
                        imagePreview.open()
                    }
                }
            }
            
            Text {
                text: "点击查看大图"
                font.pixelSize: 12
                color: isOwn ? "white" : Material.foreground
                opacity: 0.7
            }
        }
    }
    
    // 文件消息组件
    Component {
        id: fileMessageComponent
        
        Row {
            spacing: 12
            
            Rectangle {
                width: 40
                height: 40
                color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                radius: 8
                
                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "qrc:/icons/file.png"
                }
            }
            
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                
                Text {
                    text: message // message字段存储文件名
                    font.pixelSize: 14
                    color: isOwn ? "white" : Material.foreground
                    maximumLineCount: 2
                    wrapMode: Text.Wrap
                }
                
                Text {
                    text: "点击下载"
                    font.pixelSize: 12
                    color: isOwn ? "white" : Material.foreground
                    opacity: 0.7
                }
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // 处理文件下载
                    console.log("下载文件:", message)
                }
            }
        }
    }
    
    // 图片预览弹窗
    Popup {
        id: imagePreview
        anchors.centerIn: Overlay.overlay
        width: Math.min(previewImage.sourceSize.width + 40, parent.width * 0.9)
        height: Math.min(previewImage.sourceSize.height + 40, parent.height * 0.9)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        property alias source: previewImage.source
        
        background: Rectangle {
            color: Material.theme === Material.Dark ? "#2b2b2b" : "#ffffff"
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#404040" : "#e0e0e0"
            radius: 8
        }
        
        Image {
            id: previewImage
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            width: Math.min(sourceSize.width, imagePreview.width - 40)
            height: Math.min(sourceSize.height, imagePreview.height - 40)
        }
    }
}