import QtQuick 2.0
import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: false
    Image{
        id:notConnectImage
        anchors.left: parent.left
        anchors.leftMargin: 218
        anchors.top:parent.top
        anchors.topMargin: 90
        width: 436
        height: 414
        source: "qrc:/images/MediaWidget/BtNotConnect.png"
    }
    Text{
        id:infoText
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top:parent.top
        anchors.topMargin: 540
        width: 880
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("蓝牙未连接，请到设置界面连接蓝牙设备")
        elide:Text.ElideRight
    }
}
