import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    Button{
        id:ecLinkBtn
        anchors.centerIn: parent
        width: 192
        height: 192
        enabled: false
        objectName: "ecLinkBtnObject"
        background: Rectangle{
            id:btn1Bg
            color: "transparent"
        }
        Image{
            id:btn1Icon
            anchors.fill:parent
            source: "qrc:/images/PhoneLink/EcLink.png"
        }
        onPressed: ecLinkBtn.opacity = 0.4
        onReleased:ecLinkBtn.opacity = 1
    }
    Text{
        id:btn1Name
        anchors.left: parent.left
        anchors.top: ecLinkBtn.bottom
        anchors.topMargin: 10
        width: 1760
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("由于亿连版本太多，暂时没有合入，亿连各版本单独做了Demo")
    }
}
