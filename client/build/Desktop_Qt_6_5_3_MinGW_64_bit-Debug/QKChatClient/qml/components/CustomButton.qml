import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Button {
    id: root
    
    property bool isPrimary: false
    property bool isLoading: false
    property color customColor: isPrimary ? Material.accent : "transparent"
    property color customTextColor: isPrimary ? "white" : Material.accent
    
    implicitHeight: 50
    implicitWidth: 120
    
    font.pixelSize: 16
    font.bold: isPrimary
    
    Material.background: customColor
    Material.foreground: customTextColor
    
    // 按钮背景
    background: Rectangle {
        id: backgroundRect
        color: root.enabled ? customColor : Material.hintTextColor
        radius: 8
        border.width: isPrimary ? 0 : 1
        border.color: root.enabled ? Material.accent : Material.hintTextColor
        
        // 悬停效果
        opacity: root.enabled ? (root.hovered ? 0.9 : 1.0) : 0.6
        
        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        
        // 涟漪效果
        Rectangle {
            id: ripple
            anchors.centerIn: parent
            width: 0
            height: 0
            radius: width / 2
            color: isPrimary ? "white" : Material.accent
            opacity: 0
            
            PropertyAnimation {
                id: rippleAnimation
                target: ripple
                properties: "width,height"
                to: Math.max(root.width, root.height) * 2
                duration: 300
                easing.type: Easing.OutQuad
            }
            
            PropertyAnimation {
                id: rippleOpacityAnimation
                target: ripple
                property: "opacity"
                from: 0.3
                to: 0
                duration: 300
                easing.type: Easing.OutQuad
            }
        }
    }
    
    // 按钮内容
    contentItem: Item {
        anchors.fill: parent
        
        // 加载指示器
        BusyIndicator {
            anchors.centerIn: parent
            visible: isLoading
            width: 20
            height: 20
            Material.foreground: customTextColor
        }
        
        // 按钮文本
        Text {
            anchors.centerIn: parent
            text: root.text
            font: root.font
            color: root.enabled ? customTextColor : Material.hintTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            visible: !isLoading
        }
    }
    
    // 点击效果
    onClicked: {
        if (root.enabled && !isLoading) {
            ripple.width = 0
            ripple.height = 0
            ripple.opacity = 0.3
            rippleAnimation.start()
            rippleOpacityAnimation.start()
        }
    }
    
    // 按压状态
    onPressed: {
        backgroundRect.scale = 0.98
    }
    
    onReleased: {
        backgroundRect.scale = 1.0
    }
    
    // 缩放动画
    Behavior on scale {
        NumberAnimation { duration: 100 }
    }
} 