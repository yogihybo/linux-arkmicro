import QtQuick 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: false
    Image{
        id:notConnectImage
        anchors.left: parent.left
        anchors.leftMargin: 276
        anchors.top:parent.top
        anchors.topMargin: 112
        width: 324
        height: 341
        source: "qrc:/images/MediaWidget/SdNotConnect.png"
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
        text:qsTr("未插入SD卡")
    }
}
