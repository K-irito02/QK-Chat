import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: addPage
    
    signal contactAdded()
    signal groupCreated()
    
    color: Material.background
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 24
        
        // 页面标题
        Text {
            text: "添加联系人和群聊"
            font.pixelSize: 20
            font.bold: true
            color: Material.foreground
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 功能选项卡
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            
            TabButton {
                text: "添加联系人"
                font.pixelSize: 14
            }
            
            TabButton {
                text: "创建群聊"
                font.pixelSize: 14
            }
            
            TabButton {
                text: "加入群聊"
                font.pixelSize: 14
            }
        }
        
        // 内容区域
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            
            // 添加联系人页面
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20
                    
                    // 图标
                    Image {
                        source: "qrc:/icons/add-contact.png"
                        width: 80
                        height: 80
                        Layout.alignment: Qt.AlignHCenter
                        opacity: 0.7
                    }
                    
                    Text {
                        text: "添加新联系人"
                        font.pixelSize: 18
                        font.bold: true
                        color: Material.foreground
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    // 搜索框
                    TextField {
                        id: contactSearchField
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                        placeholderText: "输入用户名或邮箱"
                        font.pixelSize: 14
                        
                        background: Rectangle {
                            color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                            border.width: parent.activeFocus ? 2 : 1
                            border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                            radius: 12
                        }
                        
                        leftPadding: 16
                        rightPadding: 60
                        
                        Button {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: 44
                            height: 32
                            text: "搜索"
                            font.pixelSize: 12
                            
                            background: Rectangle {
                                color: parent.pressed ? Qt.darker(Material.primary, 1.2) : Material.primary
                                radius: 8
                            }
                            
                            onClicked: {
                                if (contactSearchField.text.trim().length > 0) {
                                    searchUser(contactSearchField.text.trim())
                                }
                            }
                        }
                        
                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Return) {
                                if (contactSearchField.text.trim().length > 0) {
                                    searchUser(contactSearchField.text.trim())
                                }
                            }
                        }
                    }
                    
                    // 搜索结果区域
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 200
                        color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                        radius: 12
                        border.width: 1
                        border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                        
                        ListView {
                            id: searchResultsList
                            anchors.fill: parent
                            anchors.margins: 8
                            model: searchResultsModel
                            delegate: searchResultDelegate
                            
                            // 空状态提示
                            Text {
                                anchors.centerIn: parent
                                text: searchResultsModel.count === 0 ? "输入用户名或邮箱进行搜索" : ""
                                font.pixelSize: 14
                                color: Material.theme === Material.Dark ? "#666666" : "#999999"
                                visible: searchResultsModel.count === 0
                            }
                        }
                    }
                    
                    Text {
                        text: "通过用户名或邮箱搜索并添加新联系人"
                        font.pixelSize: 12
                        color: Material.theme === Material.Dark ? "#888888" : "#666666"
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
            
            // 创建群聊页面
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20
                    
                    Image {
                        source: "qrc:/icons/create-group.png"
                        width: 80
                        height: 80
                        Layout.alignment: Qt.AlignHCenter
                        opacity: 0.7
                    }
                    
                    Text {
                        text: "创建新群聊"
                        font.pixelSize: 18
                        font.bold: true
                        color: Material.foreground
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    // 群聊名称输入
                    TextField {
                        id: groupNameField
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                        placeholderText: "群聊名称"
                        font.pixelSize: 14
                        
                        background: Rectangle {
                            color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                            border.width: parent.activeFocus ? 2 : 1
                            border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                            radius: 12
                        }
                        
                        leftPadding: 16
                        rightPadding: 16
                    }
                    
                    // 群聊描述输入
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 100
                        
                        TextArea {
                            id: groupDescField
                            placeholderText: "群聊描述（可选）"
                            wrapMode: TextArea.Wrap
                            font.pixelSize: 14
                            
                            background: Rectangle {
                                color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                                border.width: parent.activeFocus ? 2 : 1
                                border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                                radius: 12
                            }
                        }
                    }
                    
                    // 创建按钮
                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        width: 200
                        height: 44
                        text: "创建群聊"
                        font.pixelSize: 14
                        enabled: groupNameField.text.trim().length > 0
                        
                        background: Rectangle {
                            color: parent.enabled ? (parent.pressed ? Qt.darker(Material.primary, 1.2) : Material.primary) : Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                            radius: 22
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.enabled ? "white" : (Material.theme === Material.Dark ? "#888888" : "#999999")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            if (groupNameField.text.trim().length > 0) {
                                chatController.createGroup(groupNameField.text.trim(), groupDescField.text.trim())
                                groupNameField.text = ""
                                groupDescField.text = ""
                                groupCreated()
                            }
                        }
                    }
                    
                    Text {
                        text: "创建群聊后，您将成为群主，可以邀请其他人加入"
                        font.pixelSize: 12
                        color: Material.theme === Material.Dark ? "#888888" : "#666666"
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        Layout.maximumWidth: 300
                        wrapMode: Text.Wrap
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
            
            // 加入群聊页面
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20
                    
                    Image {
                        source: "qrc:/icons/join-group.png"
                        width: 80
                        height: 80
                        Layout.alignment: Qt.AlignHCenter
                        opacity: 0.7
                    }
                    
                    Text {
                        text: "加入群聊"
                        font.pixelSize: 18
                        font.bold: true
                        color: Material.foreground
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    TextField {
                        id: groupIdField
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                        placeholderText: "群聊ID或邀请码"
                        font.pixelSize: 14
                        
                        background: Rectangle {
                            color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                            border.width: parent.activeFocus ? 2 : 1
                            border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                            radius: 12
                        }
                        
                        leftPadding: 16
                        rightPadding: 60
                        
                        Button {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: 44
                            height: 32
                            text: "加入"
                            font.pixelSize: 12
                            
                            background: Rectangle {
                                color: parent.pressed ? Qt.darker(Material.primary, 1.2) : Material.primary
                                radius: 8
                            }
                            
                            onClicked: {
                                if (groupIdField.text.trim().length > 0) {
                                    joinGroup(groupIdField.text.trim())
                                }
                            }
                        }
                        
                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Return) {
                                if (groupIdField.text.trim().length > 0) {
                                    joinGroup(groupIdField.text.trim())
                                }
                            }
                        }
                    }
                    
                    Text {
                        text: "输入群聊ID或扫描邀请码加入群聊"
                        font.pixelSize: 12
                        color: Material.theme === Material.Dark ? "#888888" : "#666666"
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    
                    // 二维码扫描按钮
                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        width: 200
                        height: 44
                        text: "扫描二维码"
                        font.pixelSize: 14
                        
                        background: Rectangle {
                            color: parent.pressed ? Qt.alpha(Material.primary, 0.8) : "transparent"
                            border.width: 2
                            border.color: Material.primary
                            radius: 22
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: Material.primary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            // TODO: 实现二维码扫描功能
                            console.log("扫描二维码功能待实现")
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
    
    // 搜索结果数据模型
    ListModel {
        id: searchResultsModel
    }
    
    // 搜索结果委托
    Component {
        id: searchResultDelegate
        
        Rectangle {
            width: searchResultsList.width
            height: 60
            color: mouseArea.containsMouse ? Qt.alpha(Material.primary, 0.1) : "transparent"
            radius: 8
            
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    // TODO: 处理搜索结果点击
                    console.log("选择了:", model.name)
                }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12
                
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: 20
                    clip: true
                    color: "transparent"
                    
                    Image {
                        anchors.fill: parent
                        source: model.avatar || "qrc:/icons/avatar1.png"
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                }
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    
                    Text {
                        text: model.displayName || model.username
                        font.pixelSize: 14
                        font.bold: true
                        color: Material.foreground
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    
                    Text {
                        text: "@" + model.username
                        font.pixelSize: 12
                        color: Material.theme === Material.Dark ? "#888888" : "#666666"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
                
                Button {
                    text: model.isFriend ? "已添加" : "添加"
                    font.pixelSize: 12
                    enabled: !model.isFriend
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.pressed ? Qt.darker(Material.primary, 1.2) : Material.primary) : Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
                        radius: 16
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.enabled ? "white" : (Material.theme === Material.Dark ? "#888888" : "#999999")
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (!model.isFriend) {
                            addContact(model.username)
                        }
                    }
                }
            }
        }
    }
    
    // 搜索用户函数
    function searchUser(query) {
        searchResultsModel.clear()
        
        // TODO: 调用ChatController搜索用户
        // 这里添加示例数据
        var sampleUsers = [
            {
                "id": 1,
                "username": "zhangsan",
                "displayName": "张三",
                "avatar": "qrc:/icons/avatar1.png",
                "isFriend": false
            },
            {
                "id": 2,
                "username": "lisi",
                "displayName": "李四",
                "avatar": "qrc:/icons/avatar2.png",
                "isFriend": true
            }
        ]
        
        for (var i = 0; i < sampleUsers.length; i++) {
            var user = sampleUsers[i]
            if (user.username.includes(query.toLowerCase()) || 
                user.displayName.includes(query)) {
                searchResultsModel.append(user)
            }
        }
    }
    
    // 添加联系人函数
    function addContact(username) {
        chatController.addContact(username, "", "好友")
        
        // 更新搜索结果中的状态
        for (var i = 0; i < searchResultsModel.count; i++) {
            if (searchResultsModel.get(i).username === username) {
                searchResultsModel.setProperty(i, "isFriend", true)
                break
            }
        }
        
        contactAdded()
    }
    
    // 加入群聊函数
    function joinGroup(groupIdOrCode) {
        // TODO: 解析群聊ID或邀请码
        var groupId = parseInt(groupIdOrCode)
        if (groupId > 0) {
            chatController.joinGroup(groupId)
            groupIdField.text = ""
        } else {
            // 处理邀请码
            console.log("处理邀请码:", groupIdOrCode)
        }
    }
    
    // 监听ChatController信号
    Connections {
        target: chatController
        
        function onContactAdded(contact) {
            // 显示成功提示
            console.log("联系人添加成功:", contact.name)
        }
        
        function onGroupCreated(group) {
            // 显示成功提示
            console.log("群聊创建成功:", group.name)
        }
        
        function onGroupJoined(groupId) {
            // 显示成功提示
            console.log("成功加入群聊:", groupId)
        }
        
        function onError(message) {
            // 显示错误提示
            console.log("操作失败:", message)
        }
    }
}