import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    Image{
        id:notConnectImage
        anchors.left: parent.left
        anchors.leftMargin: 244
        anchors.top:parent.top
        anchors.topMargin: 114
        width: 393
        height: 398
        source: "qrc:/images/MediaWidget/Scan.png"
    }
    Text{
        id:infoText
        anchors.left: parent.left
        anchors.leftMargin: 290
        anchors.top:parent.top
        anchors.topMargin: 540
        width: 300
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("正在扫描Usb，请稍候...")
    }
    Timer{
        id:tm
        interval: 500
        repeat: true
        triggeredOnStart: true
        running: false
        objectName: "timeObject"
        property int  tt: 0
        property bool isIncrease: true
        onTriggered:
        {
            if(isIncrease === true)
            {
                tt++;
                if(tt === 3)
                {
                    isIncrease = false
                }
            }
            else
            {
                tt--;
                if(tt === 0)
                {
                    isIncrease = true
                }
            }

            if(tt === 0)
            {
                infoText .text = qsTr("正在扫描Usb，请稍候...");
            }
            else if(tt === 1)
            {
                infoText .text = qsTr("正在扫描Usb，请稍候..");
            }
            else if(tt === 2)
            {
                infoText .text = qsTr("正在扫描Usb，请稍候.");
            }
            else if(tt === 3)
            {
                infoText .text = qsTr("正在扫描Usb，请稍候");
            }
        }
    }
}
