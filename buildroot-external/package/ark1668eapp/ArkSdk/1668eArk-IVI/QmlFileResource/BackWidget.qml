import QtQuick 2.0
import QtQuick.Controls 2.0
Rectangle {
    id:root
    width: 1920
    height: 100
    visible: true
    color: "#92d7f9"
    opacity: 0.6
    Button{
        id:backBtn
        width: 97
        height: 65
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.verticalCenter: parent.verticalCenter
        objectName: "backBtnObject"
        background: Rectangle{
            id:backBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image {
            id: name
            anchors.fill: parent
            source: "qrc:/images/HomeWidget/BackButton.png"
        }
        onPressed:backBtn.opacity = 0.4
        onReleased: backBtn.opacity = 1
    }
}
