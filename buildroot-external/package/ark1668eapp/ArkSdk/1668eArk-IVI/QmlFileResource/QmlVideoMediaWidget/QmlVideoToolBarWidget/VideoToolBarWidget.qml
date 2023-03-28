import QtQuick 2.0
import QtQuick.Controls 2.0
import "./../../QmlUserWidget"
Item {
    id:root
    width: 1240
    height: 160
    visible: false
    signal mousePressed
    property int fullScreenType: 0
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color:"transparent"
        Image{
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/toolBarBg.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:{
                root.mousePressed()
            }
        }
    }
    Text{
        id:endTimeText
        anchors.left: parent.left
        anchors.leftMargin: 72
        anchors.top:parent.top
        anchors.topMargin: 89
        width: 233
        height: 30
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:"0:00"
        objectName: "endTimeObject"
    }
    Text{
        id:remaTime
        anchors.right: parent.right
        anchors.rightMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 89
        width: 233
        height: 30
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text:"-0:00"
        objectName: "remaTimeObject"
    }
    Button{
        id:stopBtn
        anchors.left:parent.left
        anchors.leftMargin: 360
        anchors.top:parent.top
        anchors.topMargin: 68
        width: 72
        height: 72
        objectName: "stopBtnObject"
        background: Rectangle {
            id:stopBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            id:stopBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/StopNormal.png"
        }
        onPressed:  stopBtnImage.source =  "qrc:/images/ViodeWidget/StopPress.png"
        onReleased: stopBtnImage.source =  "qrc:/images/ViodeWidget/StopNormal.png"
    }

    Button{
        id:prevBtn
        anchors.left:stopBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 68
        width: 72
        height: 72
        objectName: "prevBtnObject"
        background: Rectangle {
            id:prevBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            id:prevBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/PrevNormal.png"
        }
        onPressed:  prevBtnImage.source =  "qrc:/images/ViodeWidget/PrevPress.png"
        onReleased: prevBtnImage.source =  "qrc:/images/ViodeWidget/PrevNormal.png"
    }

    Button{
        id:toggleBtn
        anchors.left:prevBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 68
        width: 72
        height: 72
        objectName: "toggleBtnObject"
        property bool playStatus: false
        background: Rectangle {
            id:toggleBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            id:toggleBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/PauseNormal.png"
        }
        onPressed:{
            if(playStatus === false)
            {
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PauseNormal.png"
            }
            else{
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PlayPress.png"
            }
        }
        onReleased:{
            if(playStatus === false)
            {
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PauseNormal.png"
            }
            else{
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PlayNormal.png"
            }
        }
        onPlayStatusChanged: {
            if(playStatus === false)
            {
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PauseNormal.png"
            }
            else{
                toggleBtnImage.source =  "qrc:/images/ViodeWidget/PlayNormal.png"
            }
        }
    }

    Button{
        id:nextBtn
        anchors.left:toggleBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 68
        width: 72
        height: 72
        objectName: "nextBtnObject"
        background: Rectangle {
            id:nextBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            id:nextBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/NextNormal.png"
        }
        onPressed:  nextBtnImage.source =  "qrc:/images/ViodeWidget/NextPress.png"
        onReleased: nextBtnImage.source =  "qrc:/images/ViodeWidget/NextNormal.png"
    }

    Button{
        id:fullScreenBtn
        anchors.left:nextBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 68
        width: 72
        height: 72
        objectName: "fullScreenBtnObject"
        property bool fullScreenStatus: false
        background: Rectangle {
            id:fullScreenBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            id:fullScreenBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/FullScreenNormal.png"
        }
        onPressed:{
            if(fullScreenStatus === false)
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/FullScreenPress.png"
            }
            else
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/ExitFullScreenPress.png"
            }
        }
        onReleased:{
            if(fullScreenStatus === false)
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/FullScreenNormal.png"
            }
            else
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/ExitFullScreenNormal.png"
            }
        }
        onFullScreenStatusChanged: {
            if(fullScreenStatus === false)
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/FullScreenNormal.png"
            }
            else
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/ExitFullScreenNormal.png"
            }
        }
    }
    UserSlider{
       id:control
       x:71
       y:44
       width: 1099
       height: 4
       fromValue: 0
       toValue:100
       value: 0
       objectName: "sliderObject"
       background: Rectangle{
           x:control.leftPadding
           y:control.topPadding + control.availableHeight / 2 - height / 2
           width:  1099
           height: 4
           radius: 2
           color:"#FFFFFF"
           Rectangle {
               width: control.visualPosition * parent.width
               height: parent.height
               opacity: 0.6
               color:"#000000"
               radius: 2
         }
       }
    }

}
