import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.qkchat.network 1.0

Dialog {
    id: verificationDialog
    title: "邮箱验证"
    width: 400
    height: 300
    modal: true
    standardButtons: Dialog.Close
    closePolicy: Popup.NoAutoClose
    
    property string email: ""
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/icons/mail-verification.svg"
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
            smooth: true
        }
        
        Text {
            Layout.fillWidth: true
            text: "注册成功！"
            font.pixelSize: 18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            color: Material.primary
        }
        
        Text {
            Layout.fillWidth: true
            text: "我们已向您的邮箱 %1 发送了一封验证邮件。\n\n请检查您的邮箱并点击邮件中的链接完成验证。".arg(email)
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: Material.secondaryTextColor
        }
        
        Text {
            Layout.fillWidth: true
            text: "验证邮件可能在垃圾邮件文件夹中"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            font.italic: true
            color: Material.hintTextColor
        }
        
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                text: "重新发送"
                onClicked: {
                    networkClient.sendEmailVerification(email)
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: "确定"
                onClicked: {
                    verificationDialog.close()
                }
            }
        }
    }
    
    NetworkClient {
        id: networkClient
        
        onResendVerificationResponse: function(success, message) {
            if (success) {
                Qt.createQmlObject('import QtQuick 2.15; import QtQuick.Controls 2.15; 
                                   MessageDialog { text: "验证邮件已重新发送"; icon: StandardIcon.Information }', 
                                   verificationDialog, "dialog").open()
            } else {
                Qt.createQmlObject('import QtQuick 2.15; import QtQuick.Controls 2.15; 
                                   MessageDialog { text: message; icon: StandardIcon.Critical }', 
                                   verificationDialog, "dialog").open()
            }
        }
    }
}