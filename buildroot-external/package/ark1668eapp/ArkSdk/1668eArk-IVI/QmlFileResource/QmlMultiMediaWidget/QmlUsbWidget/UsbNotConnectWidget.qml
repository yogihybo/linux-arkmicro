import QtQuick 2.0

Item {
    id:root
    width: 880
    height: 720
    visible: true
    Image{
        id:notConnectImage
        anchors.left: parent.left
        anchors.leftMargin: 276
        anchors.top:parent.top
        anchors.topMargin: 112
        width: 324
        height: 341
        source: "qrc:/images/MediaWidget/UsbNotConnect.png"
    }
    Text{
        id:infoText
        anchors.left: parent.left
        anchors.leftMargin: 307
        anchors.top:parent.top
        anchors.topMargin: 504
        width: 265
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("未连接设备或者设备断开")
    }
}
