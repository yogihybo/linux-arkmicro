import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1240
    height: 112
    visible: false
    property bool isImageFullScreen: false
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color:"transparent"
        Image{
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/toolBarBg.png"
        }
    }
    Button{
        id:zoomOutBtn
        anchors.right: zoomInBtn.left
        anchors.rightMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "zoomOutBtnObject"
        background: Rectangle{
            id:zoomOutBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:zoomOutBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/ImageZoomOutNormal.png"
        }
        onPressed: zoomOutBtnImage.source = "qrc:/images/ViodeWidget/ImageZoomOutPress.png"
        onReleased: zoomOutBtnImage.source = "qrc:/images/ViodeWidget/ImageZoomOutNormal.png"
    }
    Button{
        id:zoomInBtn
        anchors.right: prevBtn.left
        anchors.rightMargin:  40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "zoomInBtnObject"
        background: Rectangle{
            id:zoomInBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:zoomInBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/ImageZoomInNormal.png"
        }
        onPressed: zoomInBtnImage.source = "qrc:/images/ViodeWidget/ImageZoomInPress.png"
        onReleased: zoomInBtnImage.source = "qrc:/images/ViodeWidget/ImageZoomInNormal.png"
    }
    Button{
        id:prevBtn
        anchors.right: toggleBtn.left
        anchors.rightMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "prevBtnObject"
        background: Rectangle{
            id:prevBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:prevBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/PrevNormal.png"
        }
        onPressed: prevBtnImage.source  = "qrc:/images/ViodeWidget/PrevPress.png"
        onReleased: prevBtnImage.source = "qrc:/images/ViodeWidget/PrevNormal.png"
    }

    Button{
        id:toggleBtn
        anchors.centerIn: parent
        width: 72
        height: 72
        enabled: false
        objectName: "toggleBtnObject"
        property bool playStatus: false
        background: Rectangle{
            id:toggleBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:toggleBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/PauseNormal.png"
        }
        onPressed: {
            if(playStatus === false)
            {
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PausePress.png"
            }
            else{
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PlayPress.png"
            }
        }
        onReleased: {
            if(playStatus === false)
            {
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PauseNormal.png"
            }
            else{
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PlayNormal.png"
            }
        }
        onPlayStatusChanged: {
            if(playStatus === false)
            {
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PauseNormal.png"
            }
            else{
                toggleBtnImage.source = "qrc:/images/ViodeWidget/PlayNormal.png"
            }
        }
    }
    Button{
        id:nextBtn
        anchors.left: toggleBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "nextBtnObject"
        background: Rectangle{
            id:nextBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:nextBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/NextNormal.png"
        }
        onPressed: nextBtnImage.source = "qrc:/images/ViodeWidget/NextPress.png"
        onReleased:nextBtnImage.source = "qrc:/images/ViodeWidget/NextNormal.png"
    }

    Button{
        id:rotateBtn
        anchors.left: nextBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "rotateBtnObject"
        background: Rectangle{
            id:rotateBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:rotateBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/ImageRotateNormal.png"
        }
        onPressed:  rotateBtnImage.source = "qrc:/images/ViodeWidget/ImageRotatePress.png"
        onReleased: rotateBtnImage.source = "qrc:/images/ViodeWidget/ImageRotateNormal.png"
    }

    Button{
        id:fullScreenBtn
        anchors.left: rotateBtn.right
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 20
        width: 72
        height: 72
        objectName: "fullScreenBtnObject"
        property bool  fullStatus: false
        background: Rectangle{
            id:fullScreenBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:fullScreenBtnImage
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/FullScreenNormal.png"
        }
        onPressed: {
            if(fullStatus === false)
            {
                fullScreenBtnImage.source = "qrc:/images/ViodeWidget/FullScreenPress.png"
            }
            else
            {
                fullScreenBtnImage.source = "qrc:/images/ViodeWidget/ExitFullScreenPress.png"
            }
        }
        onReleased: {
            if(fullStatus === false)
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/FullScreenNormal.png"
            }
            else
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/ExitFullScreenNormal.png"
            }
        }
        onFullStatusChanged: {
            if(fullStatus === false)
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/FullScreenNormal.png"
            }
            else
            {
                fullScreenBtnImage.source =  "qrc:/images/ViodeWidget/ExitFullScreenNormal.png"
            }
        }
    }
}
