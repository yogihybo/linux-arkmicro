import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 800
    height: 600
    visible: false
    signal btnClicked
    Rectangle{
        anchors.fill:parent
        color:"#000000"
        radius: 20
    }
    Text{
        id:title
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 800
        height: 50
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("本机信息")
    }

    Text{
        id:bspVersion
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 175
        width: 800
        height: 50
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 42
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "bspVersionObject"
    }

    Text{
        id:appVersion
        anchors.left: parent.left
        anchors.top:bspVersion.bottom
        anchors.topMargin: 50
        width: 800
        height: 50
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 42
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "appVersionObject"
    }

    Text{
        id:mcuVersion
        anchors.left: parent.left
        anchors.top:appVersion.bottom
        anchors.topMargin: 50
        width: 800
        height: 50
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 42
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "mcuVersionObject"
        text:qsTr("mcu:暂时没有")
    }


    Rectangle{
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 500
        width: 800
        height: 1
        color:"#FFFFFF"
    }
    Button{
        id:closeBtn
        anchors.left: parent.left
        anchors.leftMargin: 375
        anchors.top:parent.top
        anchors.topMargin: 520
        width: 150
        height: 60
        objectName: "closeBtnObject"
        background: Rectangle{
            id:closeBtnBg
            anchors.fill:parent
            radius: 30
            color: "#0DA8FF"
        }
        Text{
            id:closeBtnText
            anchors.left: closeBtn.left
            anchors.leftMargin: 20
            anchors.top:closeBtn.top
            anchors.topMargin: 18
            width: 110
            height: 24
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("关闭")
        }
        onPressed:  closeBtnBg.opacity = 0.4
        onReleased: closeBtnBg.opacity = 1
        onClicked: {
            root.visible = false;
            root.btnClicked();
        }
    }
}
