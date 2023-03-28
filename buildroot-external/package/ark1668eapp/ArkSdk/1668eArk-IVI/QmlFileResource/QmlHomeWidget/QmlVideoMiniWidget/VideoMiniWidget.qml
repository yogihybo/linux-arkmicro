import QtQuick 2.0

Item {
    id:root
    width: 400
    height: 659
    visible: true
    property int currentPage: 0
    signal auxWidgetClicked

    Rectangle{
        id:videoRect
        x:0
        y:0
        width: 400
        height: 560
        radius: 20
        Image{
            id:videoRectImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/RectBgNormal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  videoRectImage.source = "qrc:/images/HomeWidget/RectBgPress.png"
            onReleased: videoRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png"
            onClicked: {
                root.auxWidgetClicked();
            }
        }
        Rectangle{
            id:videoScreenShotRect
            anchors.left:parent.left
            anchors.leftMargin: 16
            anchors.top:parent.top
            anchors.topMargin: 80
            width: 369
            height: 240
            color:"transparent"
            Image{
                id:videoScreenShotImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/unsplash.png"
            }
        }

        Text {
            id:rectTitle
            anchors.left: parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 361
            width: 273
            height: 39
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 32
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("视频输入")
        }
    }

    Text{
        id:widgetName
        anchors.left:videoRect.left
        anchors.leftMargin: 96
        anchors.top:videoRect.bottom
        anchors.topMargin: 30
        width: 208
        height: 69
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("AUX")
    }
    onCurrentPageChanged: {
        switch(root.currentPage)
        {
            case 0:
                videoRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png"
                break;
            default:
                break;
        }
    }

}
