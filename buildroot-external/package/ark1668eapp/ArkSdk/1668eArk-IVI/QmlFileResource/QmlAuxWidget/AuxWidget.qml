import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1920
    height: 720
    visible: true
    signal auxWidgetClicked
    Rectangle{
        anchors.fill: parent
        color: "#000000"
        MouseArea{
            anchors.fill :parent
            onClicked: {
                root.auxWidgetClicked();
            }
        }
    }
    Text{
        id:id_text
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: 200
        height: 40
        color: "#FFFFFF"
        opacity: 1
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("无信号...")
        objectName: "noSignalTextObject"
    }

}
