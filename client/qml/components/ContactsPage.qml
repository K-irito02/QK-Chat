import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: contactsPage
    
    signal contactSelected(var contact)
    
    color: Material.background
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16
        
        // 页面标题
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "联系人"
                font.pixelSize: 18
                font.bold: true
                color: Material.foreground
                Layout.fillWidth: true
            }
            
            ToolButton {
                icon.source: "qrc:/icons/add.png"
                icon.width: 20
                icon.height: 20
                ToolTip.text: "添加联系人"
                ToolTip.visible: hovered
                
                onClicked: {
                    addContactDialog.open()
                }
            }
        }
        
        // 搜索框
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "搜索联系人..."
            
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
                contactsModel.filterContacts(text)
            }
        }
        
        // 联系人分组选择
        Row {
            Layout.fillWidth: true
            spacing: 8
            
            Repeater {
                model: ["全部", "好友", "同事", "家人", "其他"]
                
                Button {
                    text: modelData
                    flat: true
                    checkable: true
                    checked: index === 0
                    
                    background: Rectangle {
                        color: parent.checked ? Qt.alpha(Material.primary, 0.2) : "transparent"
                        border.width: parent.checked ? 1 : 0
                        border.color: Material.primary
                        radius: 16
                    }
                    
                    onClicked: {
                        // 切换分组过滤
                        for (var i = 0; i < parent.children.length; i++) {
                            parent.children[i].checked = (i === index)
                        }
                        contactsModel.filterByGroup(modelData === "全部" ? "" : modelData)
                    }
                }
            }
        }
        
        // 联系人列表
        ListView {
            id: contactsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            model: contactsModel
            delegate: contactDelegate
            
            section.property: "firstLetter"
            section.criteria: ViewSection.FullString
            section.delegate: sectionDelegate
            
            ScrollBar.vertical: ScrollBar {
                active: true
            }
        }
        
        // 空状态提示
        Column {
            Layout.alignment: Qt.AlignCenter
            spacing: 16
            visible: contactsModel.count === 0
            
            Image {
                source: "qrc:/icons/contacts-empty.png"
                width: 64
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }
            
            Text {
                text: "暂无联系人"
                font.pixelSize: 14
                color: Material.theme === Material.Dark ? "#666666" : "#999999"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Button {
                text: "添加联系人"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: addContactDialog.open()
            }
        }
    }
    
    // 联系人数据模型
    ListModel {
        id: contactsModel
        
        property var originalData: []
        
        Component.onCompleted: {
            loadContacts()
        }
        
        function filterContacts(searchText) {
            clear()
            for (var i = 0; i < originalData.length; i++) {
                var contact = originalData[i]
                if (searchText === "" || 
                    contact.name.toLowerCase().includes(searchText.toLowerCase()) ||
                    contact.username.toLowerCase().includes(searchText.toLowerCase())) {
                    append(contact)
                }
            }
        }
        
        function filterByGroup(group) {
            clear()
            for (var i = 0; i < originalData.length; i++) {
                var contact = originalData[i]
                if (group === "" || contact.group === group) {
                    append(contact)
                }
            }
        }
    }
    
    // 分组标题委托
    Component {
        id: sectionDelegate
        
        Rectangle {
            width: contactsList.width
            height: 32
            color: Material.theme === Material.Dark ? "#353535" : "#fafafa"
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: section
                font.pixelSize: 14
                font.bold: true
                color: Material.primary
            }
        }
    }
    
    // 联系人项目委托
    Component {
        id: contactDelegate
        
        Rectangle {
            width: contactsList.width
            height: 64
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
                    contactSelected(model)
                }
            }
            
            // 右键菜单
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.contact = model
                        contextMenu.popup()
                    }
                }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                
                // 头像
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    color: "transparent"
                    radius: 20
                    clip: true
                    
                    Image {
                        anchors.fill: parent
                        source: model.avatar || "qrc:/icons/avatar1.png"
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                    
                    // 在线状态指示器
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: model.isOnline ? "#4CAF50" : "#757575"
                        border.width: 2
                        border.color: Material.background
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                    }
                }
                
                // 联系人信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Text {
                            text: model.name || "未知"
                            font.pixelSize: 14
                            font.bold: true
                            color: Material.foreground
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        
                        Rectangle {
                            color: Qt.alpha(Material.primary, 0.1)
                            radius: 10
                            width: groupText.contentWidth + 16
                            height: groupText.contentHeight + 4
                            
                            Text {
                                id: groupText
                                anchors.centerIn: parent
                                text: model.group || "好友"
                                font.pixelSize: 12
                                color: Material.primary
                            }
                        }
                    }
                    
                    Text {
                        text: model.signature || "这个人很懒，什么都没写"
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
                        ToolTip.text: "发送消息"
                        ToolTip.visible: hovered
                        
                        onClicked: {
                            contactSelected(model)
                        }
                    }
                    
                    ToolButton {
                        icon.source: "qrc:/icons/phone.png"
                        icon.width: 16
                        icon.height: 16
                        ToolTip.text: "语音通话"
                        ToolTip.visible: hovered
                        
                        onClicked: {
                            // 发起语音通话
                        }
                    }
                }
            }
        }
    }
    
    // 右键菜单
    Menu {
        id: contextMenu
        
        property var contact: null
        
        MenuItem {
            text: "发送消息"
            icon.source: "qrc:/icons/chat.png"
            onClicked: {
                if (contextMenu.contact) {
                    contactSelected(contextMenu.contact)
                }
            }
        }
        
        MenuItem {
            text: "查看资料"
            icon.source: "qrc:/icons/info.png"
            onClicked: {
                if (contextMenu.contact) {
                    contactInfoDialog.loadContact(contextMenu.contact)
                    contactInfoDialog.open()
                }
            }
        }
        
        MenuItem {
            text: "设置备注"
            icon.source: "qrc:/icons/edit.png"
            onClicked: {
                if (contextMenu.contact) {
                    editContactDialog.loadContact(contextMenu.contact)
                    editContactDialog.open()
                }
            }
        }
        
        MenuSeparator { }
        
        MenuItem {
            text: "删除联系人"
            icon.source: "qrc:/icons/delete.png"
            onClicked: {
                if (contextMenu.contact) {
                    deleteContactDialog.contactId = contextMenu.contact.id
                    deleteContactDialog.contactName = contextMenu.contact.name
                    deleteContactDialog.open()
                }
            }
        }
    }
    
    // 添加联系人对话框
    Popup {
        id: addContactDialog
        
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
                id: usernameField
                Layout.fillWidth: true
                placeholderText: "输入用户名或邮箱"
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            TextField {
                id: remarkField
                Layout.fillWidth: true
                placeholderText: "备注名称（可选）"
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            ComboBox {
                id: groupField
                Layout.fillWidth: true
                model: ["好友", "同事", "家人", "其他"]
                currentIndex: 0
            }
            
            Text {
                text: "输入对方的用户名或邮箱地址来添加联系人"
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
                onClicked: addContactDialog.close()
            }
            
            Button {
                text: "添加"
                onClicked: {
                    if (usernameField.text.trim().length > 0) {
                        // 发送添加联系人请求
                        chatController.addContact(usernameField.text.trim(), remarkField.text.trim(), groupField.currentText)
                        usernameField.text = ""
                        remarkField.text = ""
                        groupField.currentIndex = 0
                        addContactDialog.close()
                    }
                }
            }
        }
    }
    
    // 联系人信息对话框
    Dialog {
        id: contactInfoDialog
        
        anchors.centerIn: parent
        width: 320
        height: 400
        title: "联系人信息"
        modal: true
        
        property var contactData: null
        
        function loadContact(contact) {
            contactData = contact
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            // 头像和基本信息
            Column {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    clip: true
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "transparent"
                    
                    Image {
                        anchors.fill: parent
                        source: contactInfoDialog.contactData ? contactInfoDialog.contactData.avatar : ""
                        fillMode: Image.PreserveAspectCrop
                        
                        layer.enabled: true
                    }
                }
                
                Text {
                    text: contactInfoDialog.contactData ? contactInfoDialog.contactData.name : ""
                    font.pixelSize: 18
                    font.bold: true
                    color: Material.foreground
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: contactInfoDialog.contactData ? ("@" + contactInfoDialog.contactData.username) : ""
                    font.pixelSize: 14
                    color: Material.theme === Material.Dark ? "#888888" : "#666666"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            
            // 详细信息
            Column {
                Layout.fillWidth: true
                spacing: 12
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: infoRowComponent
                    onLoaded: {
                        item.label = "分组"
                        item.value = contactInfoDialog.contactData ? contactInfoDialog.contactData.group : ""
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: infoRowComponent
                    onLoaded: {
                        item.label = "个性签名"
                        item.value = contactInfoDialog.contactData ? contactInfoDialog.contactData.signature : ""
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: infoRowComponent
                    onLoaded: {
                        item.label = "状态"
                        item.value = contactInfoDialog.contactData ? (contactInfoDialog.contactData.isOnline ? "在线" : "离线") : ""
                    }
                }
            }
        }
        
        standardButtons: Dialog.Close
    }
    
    // 编辑联系人对话框
    Dialog {
        id: editContactDialog
        
        anchors.centerIn: parent
        width: 320
        height: 250
        title: "编辑联系人"
        modal: true
        
        property var contactData: null
        
        function loadContact(contact) {
            contactData = contact
            remarkEditField.text = contact.remark || contact.name
            groupEditField.currentIndex = Math.max(0, groupEditField.model.indexOf(contact.group))
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            
            TextField {
                id: remarkEditField
                Layout.fillWidth: true
                placeholderText: "备注名称"
                
                background: Rectangle {
                    color: Material.theme === Material.Dark ? "#404040" : "#f5f5f5"
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Material.primary : (Material.theme === Material.Dark ? "#606060" : "#c0c0c0")
                    radius: 8
                }
            }
            
            ComboBox {
                id: groupEditField
                Layout.fillWidth: true
                model: ["好友", "同事", "家人", "其他"]
            }
        }
        
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        onAccepted: {
            if (contactData && remarkEditField.text.trim().length > 0) {
                // 更新联系人信息
                chatController.updateContact(contactData.id, remarkEditField.text.trim(), groupEditField.currentText)
                loadContacts() // 重新加载联系人列表
            }
        }
    }
    
    // 删除联系人确认对话框
    Dialog {
        id: deleteContactDialog
        
        anchors.centerIn: parent
        width: 320
        height: 150
        title: "删除联系人"
        modal: true
        
        property int contactId: 0
        property string contactName: ""
        
        Text {
            anchors.fill: parent
            text: "确定要删除联系人 \"" + deleteContactDialog.contactName + "\" 吗？\n\n删除后将无法恢复聊天记录。"
            wrapMode: Text.Wrap
            color: Material.foreground
        }
        
        standardButtons: Dialog.Yes | Dialog.No
        
        onAccepted: {
            if (contactId > 0) {
                // 删除联系人
                chatController.removeContact(contactId)
                loadContacts() // 重新加载联系人列表
            }
        }
    }
    
    // 信息行组件
    Component {
        id: infoRowComponent
        
        RowLayout {
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
    }
    
    // 加载联系人数据
    function loadContacts() {
        contactsModel.clear()
        contactsModel.originalData = []
        
        // 这里添加示例数据，实际应该从数据库或服务器加载
        var sampleContacts = [
            {
                "id": 1,
                "username": "zhangsan",
                "name": "张三",
                "remark": "张三",
                "avatar": "qrc:/icons/avatar1.png",
                "isOnline": true,
                "group": "好友",
                "signature": "生活不止眼前的苟且，还有诗和远方",
                "firstLetter": "Z"
            },
            {
                "id": 2,
                "username": "lisi",
                "name": "李四",
                "remark": "李四",
                "avatar": "qrc:/icons/avatar2.png",
                "isOnline": false,
                "group": "同事",
                "signature": "代码改变世界",
                "firstLetter": "L"
            },
            {
                "id": 3,
                "username": "wangwu",
                "name": "王五",
                "remark": "王五",
                "avatar": "qrc:/icons/avatar3.png",
                "isOnline": true,
                "group": "家人",
                "signature": "平凡而不平庸",
                "firstLetter": "W"
            }
        ]
        
        contactsModel.originalData = sampleContacts
        for (var i = 0; i < sampleContacts.length; i++) {
            contactsModel.append(sampleContacts[i])
        }
    }
}