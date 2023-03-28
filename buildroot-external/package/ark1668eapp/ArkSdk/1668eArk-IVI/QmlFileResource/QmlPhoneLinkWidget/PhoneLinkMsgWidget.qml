import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 600
    height: 360
    visible: true
    property int showType: 0
    Rectangle{
        anchors.fill:parent
        color:"#6e6e6e"
        radius: 20
    }
    Text{
        id:title
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 119
        width: 600
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("选择连接模式!")
    }
    Image{
         id:loaderImage
         anchors.horizontalCenter: parent.horizontalCenter
         anchors.top:parent.top
         anchors.topMargin: 80
         width: 93
         height:100
         visible: false
         source: "qrc:/images/ViodeWidget/ImageLoadingBackground.png"
         objectName: "loaderImageObject"
         property bool scanFinish: false
         transform: Rotation{
             id:rotation
             origin.x: loaderImage.sourceSize.width/2
             origin.y: loaderImage.sourceSize.height/2
             RotationAnimation on angle{
                 id:animation
                 running:false
                 from: 0
                 to: 360
                 duration: 2000
                 loops:Animation.Infinite
                 objectName:"animationObject"
             }
             onAngleChanged: {
                 if(loaderImage.scanFinish === true)
                 {
                     if(rotation.angle === 0 || rotation.angle === 360)
                     {
                         animation.running  = false;
                         loaderImage.scanFinish = false;
                     }
                 }
             }
         }
    }

    Rectangle{
        id:splitLine
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
        background: Rectangle{
            id:closeBtnBg
            anchors.fill:parent
            radius: 30
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

    Button{
        id:wireBtn
        anchors.left: parent.left
        anchors.leftMargin: 50
        anchors.top:parent.top
        anchors.topMargin: 280
        width: 150
        height: 60
        objectName: "wireBtnObject"
        background: Rectangle{
            id:wireBtnBg
            anchors.fill:parent
            radius: 30
            color: "#0DA8FF"
        }
        Text{
            id:wireBtnBgText
            anchors.left: wireBtn.left
            anchors.leftMargin: 20
            anchors.top:wireBtn.top
            anchors.topMargin: 18
            width: 110
            height: 24
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("有线")
        }
        onPressed:  wireBtnBg.opacity = 0.4
        onReleased: wireBtnBg.opacity = 1
    }
    Button{
        id:wirelessBtn
        anchors.left: parent.left
        anchors.leftMargin: 400
        anchors.top:parent.top
        anchors.topMargin: 280
        width: 150
        height: 60
        objectName: "wirelessBtnObject"
        background: Rectangle{
            id:wirelessBtnBg
            anchors.fill:parent
            radius: 30
            color: "#0DA8FF"
        }
        Text{
            id:wirelessBtnBgText
            anchors.left: wirelessBtn.left
            anchors.leftMargin: 20
            anchors.top:wirelessBtn.top
            anchors.topMargin: 18
            width: 110
            height: 24
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("无线")
        }
        onPressed:  wirelessBtnBg.opacity = 0.4
        onReleased: wirelessBtnBg.opacity = 1
    }
    onShowTypeChanged: {
        switch(root.showType){
            case 0:
                title.visible = true;
                loaderImage.visible = false;
                break;
            case 1:
                title.visible = false;
                loaderImage.visible = true;
                break;
            default:
                break;
        }
    }

}
