import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    Text {
        id:title
        anchors.fill:parent
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("功能保留页面二!")
    }
}
