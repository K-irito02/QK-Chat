import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    
    property alias text: textField.text
    property alias placeholderText: textField.placeholderText
    property alias maximumLength: textField.maximumLength
    property string iconSource: ""
    property bool isPassword: false
    property string errorMessage: ""
    property bool hasError: errorMessage.length > 0
    property int inputMethodHints: Qt.ImhNone
    
    signal customTextChanged()
    signal customAccepted()
    signal customEditingFinished()
    
    function forceActiveFocus() {
        textField.forceActiveFocus()
    }
    
    function clear() {
        textField.clear()
    }
    
    implicitHeight: column.implicitHeight
    implicitWidth: 250
    
    Column {
        id: column
        width: parent.width
        spacing: 5
        
        // 输入框容器
        Rectangle {
            width: parent.width
            height: 50
            radius: 8
            border.width: textField.activeFocus ? 2 : 1
            border.color: {
                if (hasError) return Material.color(Material.Red)
                if (textField.activeFocus) return Material.accent
                return Material.hintTextColor
            }
            color: Material.backgroundColor
            
            // 过渡动画
            Behavior on border.color {
                ColorAnimation { duration: 200 }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10
                
                // 图标
                Image {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    source: iconSource
                    visible: iconSource.length > 0
                    opacity: textField.activeFocus ? 1.0 : 0.6
                    
                    Behavior on opacity {
                        NumberAnimation { duration: 200 }
                    }
                }
                
                // 文本输入框
                TextField {
                    id: textField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    font.pixelSize: 15
                    color: Material.foreground
                    placeholderTextColor: Material.hintTextColor
                    selectByMouse: true
                    verticalAlignment: TextInput.AlignVCenter
                    inputMethodHints: root.inputMethodHints
                    
                    echoMode: isPassword ? TextInput.Password : TextInput.Normal
                    
                    background: Rectangle {
                        color: "transparent"
                    }
                    
                    onTextChanged: root.customTextChanged()
                    onAccepted: root.customAccepted()
                    onEditingFinished: root.customEditingFinished()
                    
                    // 输入验证动画
                    PropertyAnimation {
                        id: shakeAnimation
                        target: textField
                        property: "x"
                        duration: 100
                        from: 0; to: 5
                        loops: 4
                        alwaysRunToEnd: true
                        easing.type: Easing.InOutQuad
                        
                        onFinished: {
                            textField.x = 0
                        }
                    }
                }
                
                // 密码显示/隐藏按钮
                Button {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    visible: isPassword
                    flat: true
                    
                    icon.source: textField.echoMode === TextInput.Password ? 
                               "qrc:/icons/eye-off.png" : 
                               "qrc:/icons/eye.png"
                    icon.width: 20
                    icon.height: 20
                    icon.color: Material.hintTextColor
                    
                    onClicked: {
                        textField.echoMode = textField.echoMode === TextInput.Password ? 
                                           TextInput.Normal : TextInput.Password
                    }
                    
                    background: Rectangle {
                        color: "transparent"
                        radius: 12
                    }
                }
            }
        }
        
        // 错误信息
        Text {
            width: parent.width
            text: errorMessage
            color: Material.color(Material.Red)
            font.pixelSize: 12
            visible: hasError
            wrapMode: Text.WordWrap
            
            // 错误信息显示动画
            opacity: hasError ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }
    
    // 错误状态时的震动效果
    function showError(message) {
        errorMessage = message
        shakeAnimation.start()
    }
    
    // 清除错误状态
    function clearError() {
        errorMessage = ""
    }
}