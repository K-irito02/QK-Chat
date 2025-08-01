import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1

Item {
    id: root
    
    property url selectedAvatar: "qrc:/icons/avatar1.png"
    property var defaultAvatars: [
        "qrc:/icons/avatar1.png",
        "qrc:/icons/avatar2.png",
        "qrc:/icons/avatar3.png",
        "qrc:/icons/avatar4.png",
        "qrc:/icons/avatar5.png"
    ]
    property int selectedIndex: 0
    
    signal avatarChanged(url avatar)
    
    implicitWidth: 280
    implicitHeight: 60
    
    // 头像选择行
    Row {
        anchors.centerIn: parent
        spacing: 8
        
        Repeater {
            model: defaultAvatars
            
            Rectangle {
                property bool isSelected: selectedIndex === index
                property bool isCustomAvatar: selectedAvatar.toString().indexOf("qrc:/icons/avatar") === -1
                
                width: isSelected ? 60 : 40
                height: isSelected ? 60 : 40
                radius: width / 2
                border.width: isSelected ? 3 : 1
                border.color: isSelected ? Material.accent : Material.hintTextColor
                color: "transparent"
                
                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: (isSelected && isCustomAvatar) ? selectedAvatar : modelData
                    fillMode: Image.PreserveAspectCrop
                    
                    layer.enabled: true
                    layer.effect: Rectangle {
                        radius: parent.width / 2
                    }
                }
                
                // 上传按钮（仅在选中的头像上显示）
                Button {
                    visible: isSelected
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: 20
                    height: 20
                    
                    background: Rectangle {
                        radius: 10
                        color: Material.accent
                    }
                    
                    icon.source: "qrc:/icons/edit.png"
                    icon.width: 10
                    icon.height: 10
                    icon.color: "white"
                    
                    onClicked: fileDialog.open()
                    
                    ToolTip {
                        text: "上传自定义头像"
                        visible: parent.hovered
                        delay: 500
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        selectedIndex = index
                        if (!isCustomAvatar || selectedAvatar.toString().indexOf("qrc:/icons/avatar") !== -1) {
                            selectedAvatar = modelData
                        }
                        avatarChanged(selectedAvatar)
                    }
                    cursorShape: Qt.PointingHandCursor
                }
                
                // 选中状态动画
                Behavior on width {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
                
                Behavior on height {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
                
                Behavior on border.width {
                    NumberAnimation { duration: 200 }
                }
                
                Behavior on border.color {
                    ColorAnimation { duration: 200 }
                }
            }
        }
    }
    
    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "选择头像图片"
        folder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg)"]
        
        onAccepted: {
            var filePath = fileDialog.fileUrl.toString()
            
            // 验证文件
            var validator = Qt.createQmlObject("import QKChatClient 1.0; Validator {}", root)
            if (validator) {
                if (!validator.isValidImageFile(filePath)) {
                    errorDialog.text = "请选择有效的图片文件（JPG/PNG格式）"
                    errorDialog.open()
                    return
                }
                
                if (!validator.isValidImageSize(filePath, 2)) {
                    errorDialog.text = "图片大小不能超过2MB"
                    errorDialog.open()
                    return
                }
            }
            
            // 设置为自定义头像
            selectedAvatar = filePath
            avatarChanged(selectedAvatar)
        }
    }
    
    // 错误提示对话框
    Popup {
        id: errorDialog
        
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
        
        property alias text: errorText.text
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16
            
            Text {
                id: errorText
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Material.foreground
            }
            
            Button {
                text: "确定"
                Layout.alignment: Qt.AlignRight
                onClicked: errorDialog.close()
            }
        }
    }
}