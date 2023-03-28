import QtQuick 2.0
import QtQuick.Controls 2.0
import "QmlVideoToolBarWidget"
import "QmlListWidget"
import "../QmlImageWidget"
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int  videoFullScreenType: 0
    property bool isImageFullScreen: false
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color:"transparent"
    }
    QmlImageWidget{
        id:imageWidget
        x:520
        y:0
        width: 1240
        height: 720
        objectName:"imageWidgetObject"
    }
    ListWidget{
        id:listWidget
        x:0
        y:0
        objectName: "listWidgetObject"
    }
    Button{
        id:videoBtn
        x:520
        y:0
        width: 1240
        height: 560
        enabled: false
        objectName: "videoBtnObject"
        background: Rectangle{
            anchors.fill:parent
            color: "transparent"
        }
    }

    VideoToolBarWidget{
        id:videoToolBarWidget
        x:520
        y:560
        objectName: "videoToolBarWidgetObject"
        onFullScreenTypeChanged: {
            root.videoFullScreenType = videoToolBarWidget.fullScreenType
        }
    }

    Text{
        id:msgText
        anchors.left: parent.left
        anchors.leftMargin: 740
        anchors.top:parent.top
        anchors.topMargin: 106
        width: 800
        height: 348
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "msgTextObject"
        visible: false
    }
    onVideoFullScreenTypeChanged: {
        if(root.videoFullScreenType === 0)
        {
            msgText.x = 740;
            videoBtn.x = 520;
            videoToolBarWidget.x = 520;
            listWidget.visible = true;
        }
        else
        {
            msgText.x = 400;
            videoBtn.x = 180;
            videoToolBarWidget.x = 180;
            listWidget.visible = false;
        }
    }
    QmlImageToolWidget{
        id:imageToolWidget
        x:520
        y:608
        objectName: "imageToolWidgetObject"
        onIsImageFullScreenChanged: {
            root.isImageFullScreen = imageToolWidget.isImageFullScreen
        }
    }
    onIsImageFullScreenChanged: {
        if(root.isImageFullScreen === false)
        {
            imageWidget.x=520
            imageWidget.width = 1240
            listWidget.visible = true
            imageToolWidget.x = 520
            imageToolWidget.width = 1240
        }
        else
        {
            imageWidget.x = 0
            imageWidget.width = 1920
            imageToolWidget.x = 0
            imageToolWidget.width = 1920
            listWidget.visible = false
        }
    }
}
