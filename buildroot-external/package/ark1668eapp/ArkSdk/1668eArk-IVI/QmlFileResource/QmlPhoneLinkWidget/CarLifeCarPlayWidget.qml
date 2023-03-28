import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    Button{
        id:carLifeBtn
        anchors.left: parent.left
        anchors.leftMargin: 400
        anchors.top: parent.top
        anchors.topMargin: 237
        width: 192
        height: 192
        objectName: "carLifeBtnObject"
        background: Rectangle{
            id:btn1Bg
            color: "transparent"
        }
        Image{
            id:btn1Icon
            anchors.fill:parent
            source: "qrc:/images/PhoneLink/CarlifeNormal.png"
        }
        onPressed: btn1Icon.source = "qrc:/images/PhoneLink/CarlifePress.png"
        onReleased:btn1Icon.source = "qrc:/images/PhoneLink/CarlifeNormal.png"
    }
    Text{
        id:btn1Name
        anchors.left: parent.left
        anchors.leftMargin: 428
        anchors.top: parent.top
        anchors.topMargin: 439
        width: 121
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"CarLife"
    }

    Button{
        id:carPlayBtn
        anchors.left: parent.left
        anchors.leftMargin: 1168
        anchors.top: parent.top
        anchors.topMargin: 237
        width: 192
        height: 192
        objectName: "carPlayBtnObject"
        background: Rectangle{
            id:btn2Bg
            color: "transparent"
        }
        Image{
            id:btn2Icon
            anchors.fill:parent
            source: "qrc:/images/PhoneLink/CarplayNormal.png"
        }
        onPressed: btn2Icon.source = "qrc:/images/PhoneLink/CarplayPress.png"
        onReleased: btn2Icon.source = "qrc:/images/PhoneLink/CarplayNormal.png"
    }
    Text{
        id:btn2Name
        anchors.left: parent.left
        anchors.leftMargin: 1189
        anchors.top: parent.top
        anchors.topMargin: 439
        width: 121
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"Carplay"
    }
}
