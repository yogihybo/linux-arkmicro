import QtQuick 2.0
import QtQuick.Controls 2.0
Item{
    id:root
    width: 600
    height: 360
    visible: false
    Rectangle{
        anchors.fill:parent
        color:"#6e6e6e"
        radius: 20
    }
    Text{
        id:linkMsg
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 119
        width: 600
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        objectName: "linkMsgObject"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    Rectangle{
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 260
        width: 600
        height:2
        color:"#FFFFFF"
    }

    Button{
        id:closeBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:parent.top
        anchors.topMargin: 280
        width: 150
        height: 60
        objectName: "closeBtnObject"
        background: Rectangle{
            id:closeBtnBg
            anchors.fill:parent
            radius: 30
            opacity:1
            color: "#0DA8FF"
        }
        Text{
            id:closeBtnBgText
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
        onClicked:{
            root.visible = false;
        }
    }
    Timer{
        id:id_timer
        interval: 3000
        repeat: false
        running: false
        triggeredOnStart: false
        onTriggered: {
            root.visible = false
        }
    }

    onVisibleChanged: {
        if(root.visible === true)
        {
            id_timer.restart();
        }
        else
        {
            id_timer.stop();
        }
    }

}
