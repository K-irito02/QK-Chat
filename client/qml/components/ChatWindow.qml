import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import Qt.labs.platform 1.1
import QKChatClient 1.0

Item {
    id: chatWindow
    
    property var currentContact: null
    property var currentGroup: null
    property bool isGroupChat: currentGroup !== null
    property string chatTitle: isGroupChat ? (currentGroup?.name || "群聊") : (currentContact?.displayName || currentContact?.username || "聊天")
    property string chatSubtitle: isGroupChat ? 
        (currentGroup ? `${currentGroup.memberCount || 0}名成员` : "") :
        (currentContact?.isOnline ? "在线" : `最后上线: ${formatLastOnline(currentContact?.lastOnline)}`)
    
    // 信号定义
    signal sendMessage(string message, int messageType)
    
    // 消息列表模型
    ListModel {
        id: messagesModel
    }
    
    // 表情选择器状态
    property bool showEmojiPicker: false
    
    // 文件对话框
    FileDialog {
        id: fileDialog
        title: "选择要发送的文件"
        nameFilters: ["图片文件 (*.jpg *.png *.gif *.bmp)", "文档文件 (*.txt *.doc *.pdf)", "所有文件 (*)"]
        onAccepted: {
            if (selectedFile) {
                sendFile(selectedFile)
            }
        }
    }
    
    // 主布局
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 聊天头部
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Material.primary
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                
                // 联系人头像
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: 20
                    color: Material.accent
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: isGroupChat ? 
                            (currentGroup?.avatarUrl || "qrc:/icons/group.png") : 
                            (currentContact?.avatarUrl || "qrc:/icons/user.png")
                        fillMode: Image.PreserveAspectCrop
                        mipmap: true
                        
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            color: "transparent"
                            border.color: Material.background
                            border.width: 1
                        }
                    }
                    
                    // 在线状态指示器（仅私聊显示）
                    Rectangle {
                        visible: !isGroupChat && currentContact?.isOnline
                        width: 12
                        height: 12
                        radius: 6
                        color: "#4CAF50"
                        border.color: Material.background
                        border.width: 2
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                    }
                }
                
                // 聊天信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    
                    Text {
                        Layout.fillWidth: true
                        text: chatTitle
                        font.pointSize: 14
                        font.weight: Font.Medium
                        color: Material.primaryTextColor
                        elide: Text.ElideRight
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: chatSubtitle
                        font.pointSize: 11
                        color: Material.secondaryTextColor
                        elide: Text.ElideRight
                    }
                }
                
                // 功能按钮
                RowLayout {
                    spacing: 8
                    
                    // 语音通话按钮
                    RoundButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        icon.source: "qrc:/icons/phone.png"
                        icon.color: Material.primaryTextColor
                        flat: true
                        onClicked: startVoiceCall()
                        
                        ToolTip.visible: hovered
                        ToolTip.text: "语音通话"
                    }
                    
                    // 视频通话按钮
                    RoundButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        icon.source: "qrc:/icons/video.png"
                        icon.color: Material.primaryTextColor
                        flat: true
                        onClicked: startVideoCall()
                        
                        ToolTip.visible: hovered
                        ToolTip.text: "视频通话"
                    }
                    
                    // 更多选项按钮
                    RoundButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        icon.source: "qrc:/icons/more.png"
                        icon.color: Material.primaryTextColor
                        flat: true
                        onClicked: showChatOptions()
                        
                        ToolTip.visible: hovered
                        ToolTip.text: "更多选项"
                    }
                }
            }
        }
        
        // 消息列表区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Material.backgroundColor
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                
                ListView {
                    id: messagesList
                    model: messagesModel
                    spacing: 8
                    verticalLayoutDirection: ListView.BottomToTop
                    
                    delegate: MessageBubble {
                        width: messagesList.width
                        messageData: model
                        
                        // 消息操作
                        onClicked: selectMessage(model)
                        onLongPressed: showMessageMenu(model)
                    }
                    
                    // 滚动到底部的动画
                    Behavior on contentY {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutCubic
                        }
                    }
                }
                
                // 滚动到底部按钮
                RoundButton {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 16
                    width: 48
                    height: 48
                    visible: !messagesList.atYEnd
                    
                    icon.source: "qrc:/icons/arrow-down.png"
                    Material.background: Material.accent
                    
                    onClicked: {
                        messagesList.positionViewAtEnd()
                    }
                }
            }
        }
        
        // 输入区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(60, messageInput.contentHeight + 24)
            color: Material.dialogColor
            border.color: Material.dividerColor
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                
                // 表情按钮
                RoundButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    icon.source: showEmojiPicker ? "qrc:/icons/keyboard.png" : "qrc:/icons/emoji.png"
                    flat: true
                    
                    onClicked: {
                        showEmojiPicker = !showEmojiPicker
                    }
                    
                    ToolTip.visible: hovered
                    ToolTip.text: showEmojiPicker ? "显示键盘" : "显示表情"
                }
                
                // 消息输入框
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(messageInput.contentHeight + 16, 120)
                    
                    TextArea {
                        id: messageInput
                        placeholderText: "输入消息..."
                        wrapMode: TextArea.Wrap
                        selectByMouse: true
                        
                        // 发送快捷键
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (event.modifiers & Qt.ControlModifier) {
                                    // Ctrl+Enter 插入换行
                                    insert(cursorPosition, "\n")
                                } else {
                                    // Enter 发送消息
                                    sendCurrentMessage()
                                }
                                event.accepted = true
                            }
                        }
                        
                        // 输入提示
                        property bool isTyping: false
                        onTextChanged: {
                            if (text.length > 0 && !isTyping) {
                                isTyping = true
                                // 发送正在输入状态
                                chatController.sendTypingStatus(getCurrentChatId(), true)
                            } else if (text.length === 0 && isTyping) {
                                isTyping = false
                                chatController.sendTypingStatus(getCurrentChatId(), false)
                            }
                        }
                    }
                }
                
                // 附件按钮
                RoundButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    icon.source: "qrc:/icons/attach.png"
                    flat: true
                    
                    onClicked: fileDialog.open()
                    
                    ToolTip.visible: hovered
                    ToolTip.text: "发送文件"
                }
                
                // 语音消息按钮
                RoundButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    icon.source: "qrc:/icons/mic.png"
                    flat: true
                    
                    property bool isRecording: false
                    
                    onPressAndHold: startVoiceRecord()
                    onReleased: stopVoiceRecord()
                    
                    ToolTip.visible: hovered && !isRecording
                    ToolTip.text: "长按录音"
                }
                
                // 发送按钮
                RoundButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    icon.source: "qrc:/icons/send.png"
                    Material.background: messageInput.text.trim().length > 0 ? Material.accent : Material.buttonColor
                    enabled: messageInput.text.trim().length > 0
                    
                    onClicked: sendCurrentMessage()
                    
                    ToolTip.visible: hovered
                    ToolTip.text: "发送消息"
                }
            }
        }
        
        // 表情选择器
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: showEmojiPicker ? 200 : 0
            visible: showEmojiPicker
            color: Material.dialogColor
            border.color: Material.dividerColor
            border.width: 1
            
            Behavior on Layout.preferredHeight {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }
            
            EmojiPicker {
                anchors.fill: parent
                onEmojiSelected: (emoji) => {
                    messageInput.insert(messageInput.cursorPosition, emoji)
                }
            }
        }
    }
    
    // 正在输入指示器
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: showEmojiPicker ? 200 : 60
        anchors.left: parent.left
        anchors.leftMargin: 16
        
        width: typingText.width + 16
        height: 24
        radius: 12
        color: Material.accent
        visible: typingUsers.length > 0
        
        Text {
            id: typingText
            anchors.centerIn: parent
            text: getTypingText()
            color: Material.primaryTextColor
            font.pointSize: 10
        }
        
        property var typingUsers: []
    }
    
    // 方法实现
    function sendCurrentMessage() {
        const text = messageInput.text.trim()
        if (text.length === 0) return
        
        const messageData = {
            id: generateMessageId(),
            content: text,
            messageType: 0, // text
            timestamp: Date.now(),
            senderId: userController.userModel.userId,
            senderName: userController.userModel.displayName || userController.userModel.username,
            senderAvatar: userController.userModel.avatar,
            isOwn: true,
            deliveryStatus: "sending"
        }
        
        // 添加到本地列表
        messagesModel.insert(0, messageData)
        
        // 发射信号给父组件处理
        sendMessage(text, 0)
        
        // 清空输入框
        messageInput.text = ""
        
        // 滚动到底部
        messagesList.positionViewAtEnd()
    }
    
    function sendFile(fileUrl) {
        const fileName = fileUrl.toString().split('/').pop()
        
        const messageData = {
            id: generateMessageId(),
            content: fileName,
            messageType: getFileType(fileName),
            timestamp: Date.now(),
            senderId: userController.userModel.userId,
            senderName: userController.userModel.displayName || userController.userModel.username,
            senderAvatar: userController.userModel.avatar,
            isOwn: true,
            deliveryStatus: "sending",
            fileUrl: fileUrl
        }
        
        messagesModel.insert(0, messageData)
        
        // 发送文件
        if (isGroupChat) {
            chatController.sendFileToGroup(currentGroup.id, fileUrl.toString())
        } else {
            chatController.sendFile(currentContact.id, fileUrl.toString())
        }
        
        messagesList.positionViewAtEnd()
    }
    
    function loadMessages() {
        if (!currentContact && !currentGroup) return
        
        messagesModel.clear()
        
        let messages = []
        if (isGroupChat) {
            messages = chatController.getGroupMessageHistory(currentGroup.id, 50, 0)
        } else {
            messages = chatController.getMessageHistory(currentContact.id, 50, 0)
        }
        
        for (let i = messages.length - 1; i >= 0; i--) {
            messagesModel.append(messages[i])
        }
        
        messagesList.positionViewAtEnd()
    }
    
    function getCurrentChatId() {
        return isGroupChat ? currentGroup?.id : currentContact?.id
    }
    
    function generateMessageId() {
        return Date.now().toString() + Math.random().toString(36).substr(2, 9)
    }
    
    function getFileType(fileName) {
        const ext = fileName.split('.').pop().toLowerCase()
        if (['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp'].includes(ext)) {
            return 1 // image
        } else if (['mp4', 'avi', 'mkv', 'mov', 'wmv'].includes(ext)) {
            return 3 // video
        } else if (['mp3', 'wav', 'flac', 'aac', 'ogg'].includes(ext)) {
            return 4 // audio
        } else {
            return 2 // file
        }
    }
    
    function formatLastOnline(dateTime) {
        if (!dateTime) return "从未上线"
        
        const now = new Date()
        const lastOnline = new Date(dateTime)
        const diff = now - lastOnline
        
        if (diff < 60000) { // 1分钟内
            return "刚刚"
        } else if (diff < 3600000) { // 1小时内
            return Math.floor(diff / 60000) + "分钟前"
        } else if (diff < 86400000) { // 1天内
            return Math.floor(diff / 3600000) + "小时前"
        } else {
            return lastOnline.toLocaleDateString()
        }
    }
    
    function getTypingText() {
        if (typingUsers.length === 0) return ""
        if (typingUsers.length === 1) return `${typingUsers[0]} 正在输入...`
        if (typingUsers.length === 2) return `${typingUsers[0]} 和 ${typingUsers[1]} 正在输入...`
        return `${typingUsers[0]} 等 ${typingUsers.length} 人正在输入...`
    }
    
    function startVoiceCall() {
        // TODO: 实现语音通话
        console.log("Start voice call")
    }
    
    function startVideoCall() {
        // TODO: 实现视频通话
        console.log("Start video call")
    }
    
    function showChatOptions() {
        // TODO: 显示聊天选项菜单
        console.log("Show chat options")
    }
    
    function startVoiceRecord() {
        // TODO: 开始语音录制
        console.log("Start voice recording")
    }
    
    function stopVoiceRecord() {
        // TODO: 停止语音录制
        console.log("Stop voice recording")
    }
    
    function selectMessage(messageData) {
        // TODO: 选择消息
        console.log("Message selected:", messageData.id)
    }
    
    function showMessageMenu(messageData) {
        // TODO: 显示消息菜单
        console.log("Show message menu for:", messageData.id)
    }
    
    // 监听属性变化
    onCurrentContactChanged: {
        if (currentContact) {
            currentGroup = null
            loadMessages()
        }
    }
    
    onCurrentGroupChanged: {
        if (currentGroup) {
            currentContact = null
            loadMessages()
        }
    }
    
    // 连接ChatController信号
    Connections {
        target: chatController
        
        function onMessageReceived(senderId, content, messageType, timestamp) {
            if (isGroupChat) return
            if (currentContact && senderId === currentContact.id) {
                const messageData = {
                    id: generateMessageId(),
                    content: content,
                    messageType: messageType,
                    timestamp: timestamp,
                    senderId: senderId,
                    senderName: currentContact.displayName || currentContact.username,
                    senderAvatar: currentContact.avatarUrl,
                    isOwn: false,
                    deliveryStatus: "delivered"
                }
                
                messagesModel.insert(0, messageData)
                messagesList.positionViewAtEnd()
            }
        }
        
        function onGroupMessageReceived(groupId, senderId, content, messageType, timestamp) {
            if (!isGroupChat) return
            if (currentGroup && groupId === currentGroup.id) {
                // TODO: 获取发送者信息
                const messageData = {
                    id: generateMessageId(),
                    content: content,
                    messageType: messageType,
                    timestamp: timestamp,
                    senderId: senderId,
                    senderName: "群成员", // TODO: 获取真实姓名
                    senderAvatar: "qrc:/icons/user.png",
                    isOwn: senderId === userController.userModel.userId,
                    deliveryStatus: "delivered"
                }
                
                messagesModel.insert(0, messageData)
                messagesList.positionViewAtEnd()
            }
        }
        
        function onMessageSent(messageId) {
            // 更新消息状态为已发送
            for (let i = 0; i < messagesModel.count; i++) {
                if (messagesModel.get(i).id === messageId) {
                    messagesModel.setProperty(i, "deliveryStatus", "sent")
                    break
                }
            }
        }
        
        function onMessageDelivered(messageId) {
            // 更新消息状态为已送达
            for (let i = 0; i < messagesModel.count; i++) {
                if (messagesModel.get(i).id === messageId) {
                    messagesModel.setProperty(i, "deliveryStatus", "delivered")
                    break
                }
            }
        }
    }
}