import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Button {
    id: sideBarButton
    
    property string iconSource: ""
    property bool selected: false
    
    height: 44
    flat: true
    
    background: Rectangle {
        color: {
            if (selected) {
                return Qt.alpha(Material.primary, 0.2)
            } else if (sideBarButton.hovered) {
                return Qt.alpha(Material.foreground, 0.08)
            } else {
                return "transparent"
            }
        }
        
        border.width: selected ? 2 : 0
        border.color: selected ? Material.primary : "transparent"
        radius: 8
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    contentItem: Row {
        spacing: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 12
        
        Rectangle {
            width: 20
            height: 20
            color: selected ? Material.primary : Material.foreground
            radius: 2
            
            Image {
                anchors.fill: parent
                anchors.margins: 2
                source: iconSource
                fillMode: Image.PreserveAspectFit
            }
        }
        
        Text {
            text: sideBarButton.text
            color: selected ? Material.primary : Material.foreground
            font.pixelSize: 14
            font.weight: selected ? Font.Medium : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    // 悬停效果
    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
    }
}