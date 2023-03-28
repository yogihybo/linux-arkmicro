import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    Text{
        id:pinCode
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 150
        width: 1760
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"Huawei HiCar 连接码:955699"
        objectName: "pinCodeObject"
    }
    Text{
        id:pinCode1
        anchors.left: parent.left
        anchors.top: pinCode.bottom
        anchors.topMargin:  10
        width: 1760
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"手机开启蓝牙靠近中控，发现汽车后输入连接码,"
        objectName: "pinCodeObject"
    }

    Text{
        id:pinCode2
        anchors.left: parent.left
        anchors.top: pinCode1.bottom
        anchors.topMargin:  10
        width: 1760
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"进行首次验证.您也可以通过USB线连接您的手机"
        objectName: "pinCodeObject"
    }

    Text{
        id:pinCode3
        anchors.left: parent.left
        anchors.top: pinCode2.bottom
        anchors.topMargin: 10
        width: 1760
        height: 49
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 36
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"注：支持部分EMUI10.0及以上版本或Magic UI 3.0及以上版本的手机"
        objectName: "pinCodeObject"
    }

    Button{
        id:hicarBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: pinCode3.bottom
        anchors.topMargin: 50
        width: 100
        height: 60
        objectName: "hicarBtnObject"
        background: Rectangle{
            id:btn1Bg
            color: "#0DA8FF"
            radius: 20
        }
        Text{
            anchors.fill:parent
            width: 1760
            height: 49
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 36
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:"广播"
        }
        onPressed: hicarBtn.opacity = 0.4
        onReleased:hicarBtn.opacity = 1
    }

    Image{
         id:loaderImage
         anchors.centerIn: parent
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
}
