import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: groupsPage
    
    signal groupSelected(var group)
    
    color: Material.background
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16
        
        // 页面标题
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "群聊"
                font.pixelSize: 18
                font.bold: true
                color: Material.foreground
                Layout.fillWidth: true
            }
            
            ToolButton {
                icon.source: "qrc:/icons/add.png"
                icon.width: 20
                icon.height: 20
                ToolTip.text: "创建群聊"
                ToolTip.visible: hovered
                
                onClicked: {
                    createGroupDialog.open()
                }
            }
        }
        
        // 搜索框
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "搜索群聊..."
            
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
            
            onTextChanged: {
                groupsModel.filterGroups(text)
            }
        }
        
        // 群聊列表
        ListView {
            id: groupsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            model: groupsModel
            delegate: groupDelegate
            
            ScrollBar.vertical: ScrollBar {
                active: true
            }
        }
        
        // 空状态提示
        Column {
            Layout.alignment: Qt.AlignCenter
            spacing: 16
            visible: groupsModel.count === 0
            
            Image {
                source: "qrc:/icons/groups-empty.png"
                width: 64
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }
            
            Text {
                text: "暂未加入任何群聊"
                font.pixelSize: 14
                color: Material.theme === Material.Dark ? "#666666" : "#999999"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Button {
                text: "创建群聊"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: createGroupDialog.open()
            }
        }
    }
    
    // 群聊数据模型
    ListModel {
        id: groupsModel
        
        property var originalData: []
        
        Component.onCompleted: {
            loadGroups()
        }
        
        function filterGroups(searchText) {
            clear()
            for (var i = 0; i < originalData.length; i++) {
                var group = originalData[i]
                if (searchText === "" || 
                    group.name.toLowerCase().includes(searchText.toLowerCase())) {
                    append(group)
                }
            }
        }
    }
    
    // 群聊项目委托
    Component {
        id: groupDelegate
        
        Rectangle {
            width: groupsList.width
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
                    groupSelected(model)
                }
            }
            
            // 右键菜单
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.group = model
                        contextMenu.popup()
                    }
                }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                
                // 群头像
                Rectangle {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    color: "transparent"
                    radius: 8 // 群聊头像用圆角矩形
                    clip: true
                    
                    Image {
                        anchors.fill: parent
                        source: model.avatar || "qrc:/icons/group.png"
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                    
                    // 成员数量指示器
                    Rectangle {
                        width: 20
                        height: 14
                        radius: 7
                        color: Material.primary
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        
                        Text {
                            anchors.centerIn: parent
                            text: model.memberCount || 0
                            font.pixelSize: 9
                            color: "white"
                            font.bold: true
                        }
                    }
                }
                
                // 群聊信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    
                    Text {
                        text: model.name || "未知群聊"
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    
                    Text {
                        text: model.description || "暂无群描述"
                        font.pixelSize: 12
                        color: Material.theme === Material.Dark ? "#aaaaaa" : "#777777"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }
                
                // 操作按钮
                Row {
                    spacing: 8
                    
                    ToolButton {
                        icon.source: "qrc:/icons/chat.png"
                        icon.width: 16
                        icon.height: 16
                        ToolTip.text: "进入群聊"
                        ToolTip.visible: hovered
                        
                        onClicked: {
                            groupSelected(model)
                        }
                    }
                    
                    ToolButton {
                        icon.source: "qrc:/icons/info.png"
                        icon.width: 16
                        icon.height: 16
                        ToolTip.text: "群聊信息"
                        ToolTip.visible: hovered
                        
                        onClicked: {
                            groupInfoDialog.loadGroup(model)
                            groupInfoDialog.open()
                        }
                    }
                }
            }
        }
    }
    
    // 右键菜单
    Menu {
        id: contextMenu
        
        property var group: null
        
        MenuItem {
            text: "进入群聊"
            icon.source: "qrc:/icons/chat.png"
            onClicked: {
                if (contextMenu.group) {
                    groupSelected(contextMenu.group)
                }
            }
        }
        
        MenuItem {
            text: "群聊信息"
            icon.source: "qrc:/icons/info.png"
            onClicked: {
                if (contextMenu.group) {
                    groupInfoDialog.loadGroup(contextMenu.group)
                    groupInfoDialog.open()
                }
            }
        }
        
        MenuItem {
            text: "邀请成员"
            icon.source: "qrc:/icons/invite.png"
            onClicked: {
                if (contextMenu.group) {
                    inviteMemberDialog.groupId = contextMenu.group.id
                    inviteMemberDialog.groupName = contextMenu.group.name
                    inviteMemberDialog.open()
                }
            }
        }
        
        MenuSeparator { }
        
        MenuItem {
            text: "退出群聊"
            icon.source: "qrc:/icons/exit.png"
            onClicked: {
                if (contextMenu.group) {
                    leaveGroupDialog.groupId = contextMenu.group.id
                    leaveGroupDialog.groupName = contextMenu.group.name
                    leaveGroupDialog.open()
                }
            }
        }
    }
    
    // 创建群聊对话框
    Popup {
        id: createGroupDialog
        
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
            spacing: 16
            
            TextField {
                id: groupNameField
                Layout.fillWidth: true
                placeholderText: "群聊名称"
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                TextArea {
                    id: groupDescField
                    placeholderText: "群聊描述（可选）"
                    wrapMode: TextArea.Wrap
                    
                    background: Rectangle {
                        color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                        border.width: parent.activeFocus ? 2 : 1
                        border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                        radius: 8
                    }
                }
            }
            
            Text {
                text: "输入群聊名称来创建新的群聊，您将成为群主"
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
                onClicked: createGroupDialog.close()
            }
            
            Button {
                text: "创建"
                onClicked: {
                    if (groupNameField.text.trim().length > 0) {
                        chatController.createGroup(groupNameField.text.trim(), groupDescField.text.trim())
                        groupNameField.text = ""
                        groupDescField.text = ""
                        createGroupDialog.close()
                    }
                }
            }
        }
    }
    
    // 群聊信息对话框
    Popup {
        id: groupInfoDialog
        
        anchors.centerIn: parent
        width: 400
        height: 500
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        property var groupData: null
        
        function loadGroup(group) {
            groupData = group
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            // 群头像和基本信息
            Column {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                
                Rectangle {
                    width: 80
                    height: 80
                    radius: 12
                    clip: true
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "transparent"
                    
                    Image {
                        anchors.fill: parent
                        source: groupInfoDialog.groupData ? groupInfoDialog.groupData.avatar : ""
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                }
                
                Text {
                    text: groupInfoDialog.groupData ? groupInfoDialog.groupData.name : ""
                    font.pixelSize: 18
                    font.bold: true
                    color: Material.foreground
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: groupInfoDialog.groupData ? (groupInfoDialog.groupData.memberCount + " 名成员") : ""
                    font.pixelSize: 14
                    color: Material.theme === Material.Dark ? "#888888" : "#666666"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            
            // 详细信息
            Column {
                Layout.fillWidth: true
                spacing: 12
                
                GroupInfoRow {
                    label: "群描述"
                    value: groupInfoDialog.groupData ? groupInfoDialog.groupData.description : ""
                }
                
                GroupInfoRow {
                    label: "创建时间"
                    value: groupInfoDialog.groupData ? Qt.formatDateTime(groupInfoDialog.groupData.createdAt, "yyyy-MM-dd hh:mm") : ""
                }
            }
            
            // 成员列表预览
            Text {
                text: "群成员"
                font.pixelSize: 14
                font.bold: true
                color: Material.foreground
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
                radius: 8
                
                Text {
                    anchors.centerIn: parent
                    text: "点击查看所有成员"
                    color: Material.theme === Material.Dark ? "#888888" : "#666666"
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // TODO: 显示群成员列表
                    }
                }
            }
        }
        
        // 关闭按钮
        Button {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            text: "关闭"
            onClicked: groupInfoDialog.close()
        }
    }
    
    // 邀请成员对话框
    Popup {
        id: inviteMemberDialog
        
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
        
        property int groupId: 0
        property string groupName: ""
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            Text {
                text: "邀请成员加入群聊 \"" + inviteMemberDialog.groupName + "\""
                font.pixelSize: 14
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            
            TextField {
                id: inviteUsernameField
                Layout.fillWidth: true
                placeholderText: "输入用户名"
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
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
                onClicked: inviteMemberDialog.close()
            }
            
            Button {
                text: "邀请"
                onClicked: {
                    if (inviteUsernameField.text.trim().length > 0 && groupId > 0) {
                        // TODO: 根据用户名查找用户ID，然后邀请
                        inviteUsernameField.text = ""
                        inviteMemberDialog.close()
                    }
                }
            }
        }
    }
    
    // 退出群聊确认对话框
    Popup {
        id: leaveGroupDialog
        
        anchors.centerIn: parent
        width: 320
        height: 150
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: Material.dialogColor
            radius: 8
            border.width: 1
            border.color: Material.theme === Material.Dark ? "#505050" : "#e0e0e0"
        }
        
        property int groupId: 0
        property string groupName: ""
        
        Text {
            anchors.fill: parent
            text: "确定要退出群聊 \"" + leaveGroupDialog.groupName + "\" 吗？\n\n退出后将不再接收群消息。"
            wrapMode: Text.Wrap
            color: Material.foreground
        }
        
        // 按钮区域
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8
            
            Button {
                text: "取消"
                onClicked: leaveGroupDialog.close()
            }
            
            Button {
                text: "退出"
                onClicked: {
                    if (groupId > 0) {
                        chatController.leaveGroup(groupId)
                        loadGroups() // 重新加载群聊列表
                        leaveGroupDialog.close()
                    }
                }
            }
        }
    }
    
    // 群信息行组件
    component GroupInfoRow: RowLayout {
        property string label: ""
        property string value: ""
        
        Layout.fillWidth: true
        
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
            wrapMode: Text.Wrap
        }
    }
    
    // 加载群聊数据
    function loadGroups() {
        groupsModel.clear()
        groupsModel.originalData = []
        
        // 这里添加示例数据，实际应该从ChatController获取
        var sampleGroups = [
            {
                "id": 1,
                "name": "前端开发群",
                "avatar": "qrc:/icons/group.png",
                "memberCount": 15,
                "description": "前端技术交流群",
                "createdAt": new Date()
            },
            {
                "id": 2,
                "name": "项目讨论组",
                "avatar": "qrc:/icons/group.png",
                "memberCount": 8,
                "description": "项目开发讨论",
                "createdAt": new Date()
            }
        ]
        
        groupsModel.originalData = sampleGroups
        for (var i = 0; i < sampleGroups.length; i++) {
            groupsModel.append(sampleGroups[i])
        }
    }
    
    // 监听ChatController的群聊变化
    Connections {
        target: chatController
        
        function onGroupsChanged() {
            loadGroups()
        }
        
        function onGroupCreated(group) {
            // 刷新群聊列表
            chatController.refreshGroups()
        }
        
        function onGroupJoined(groupId) {
            // 刷新群聊列表
            chatController.refreshGroups()
        }
        
        function onGroupLeft(groupId) {
            // 刷新群聊列表
            chatController.refreshGroups()
        }
    }
}