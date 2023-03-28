import QtQuick 2.0
import QtQuick.Controls 2.0
import "../../QmlImageWidget"
Item {
    id:root
    width: 520
    height: 720
    visible: true
    Rectangle{
        id:bg;
        anchors.fill: parent
        color: "#000000"
    }
    Rectangle{
        id:selectRect
        x:29
        y:18
        width: 240
        height: 64
        color: "transparent"
        Image{
            id:selectRectBg
            anchors.fill:parent
            source: "qrc:/images/ViodeWidget/Bg.png"
        }
    }
    Button{
        id:sdTypeBtn
        anchors.left:selectRect.left
        anchors.top:selectRect.top
        width: 120
        height: 64
        enabled: false
        objectName: "sdTypeBtnObject"
        property bool sdSelect: false
        background: Rectangle{
            id:sdTypeBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:sdTypeBtnIcon
            anchors.fill:parent
        }
        Text {
            id: sdTypeBtnText
            anchors.left:parent.left
            anchors.leftMargin: 34
            anchors.top:parent.top
            anchors.topMargin: 16
            width: 51
            height: 33
            opacity: 1
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:"sd"
        }
        onSdSelectChanged: {
            if(sdSelect === false)
            {
                sdTypeBtnIcon.source = ""
            }
            else
            {
                sdTypeBtnIcon.source = "qrc:/images/ViodeWidget/SecletBtn.png"
            }
        }
    }

    Button{
        id:usbTypeBtn
        anchors.left:selectRect.left
        anchors.leftMargin: 120
        anchors.top:selectRect.top
        width: 120
        height: 64
        enabled: false
        objectName: "usbTypeBtnObject"
        property bool usbSelect: false
        background: Rectangle{
            id:usbTypeBtnBg
            anchors.fill: parent
            color: "transparent"
        }
        Image{
            id:usbTypeBtnIcon
            anchors.fill:parent
        }
        Text {
            id: usbTypeBtnText
            anchors.left:parent.left
            anchors.leftMargin: 34
            anchors.top:parent.top
            anchors.topMargin: 16
            width: 51
            height: 33
            opacity: 1
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:"usb"
        }
        onUsbSelectChanged: {
            if(usbSelect === false)
            {
                usbTypeBtnIcon.source = ""
            }
            else
            {
                usbTypeBtnIcon.source = "qrc:/images/ViodeWidget/SecletBtn.png"
            }
        }
    }

    Button{
        id:videoBtn
        x:318
        y:34
        width: 100
        height: 40
        objectName: "videoBtnObject"
        property bool videoSelect: false
        background: Rectangle{
            anchors.fill:parent
            color: "transparent"
        }
        Text {
            id: videoBtnText
            anchors.fill:parent
            opacity: 0.4
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:qsTr("视屏")
        }
        onVideoSelectChanged:{
            if(videoSelect === false)
            {
                videoBtnText.opacity = 0.4
            }
            else
            {
                videoBtnText.opacity = 1
            }
        }
    }

    Button{
        id:imageBtn
        x:429
        y:34
        width: 100
        height: 40
        objectName: "imageBtnObject"
        property bool  pixmapSelect: false
        background: Rectangle{
            anchors.fill:parent
            color: "transparent"
        }
        Text {
            id: imageBtnText
            anchors.fill:parent
            opacity: 0.4
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:qsTr("图片")
        }
        onPixmapSelectChanged: {
            if(pixmapSelect === false)
            {
                imageBtnText.opacity = 0.4
            }
            else{
                imageBtnText.opacity = 1
            }
        }
    }
    VideoListWidget{
        id:videoListWidget
        x:0
        y:82
        objectName: "videoListWidgetObject"
    }
    SdVideoListWidget{
        id:sdVideoListWidget
        x:0
        y:82
        objectName: "sdVideoListWidgetObject"
    }

    QmlImageListWidget{
        id:imageListWidget
        x:0
        y:82
        objectName: "imageListWidgetObject"
    }
    SDQmlImageWidget{
        id:sDimageListWidget
        x:0
        y:82
        objectName: "sDimageListWidgetObject"
    }
}
