import QtQuick 2.0

Item {
    id:root
    width: 400
    height: 659
    visible: true
    property int currentPage: 0
    signal phoneLinkMiniWidgetClicked
    Rectangle{
        id:phoneLinkRect
        x:0
        y:0
        width:  400
        height: 560
        radius: 20
        Image{
            id:phoneLinkImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/RectBgNormal.png"
        }
        MouseArea{
            x:0
            y:0
            width:400
            height: 330
            onPressed:  phoneLinkImage.source = "qrc:/images/HomeWidget/RectBgPress.png"
            onReleased: phoneLinkImage.source = "qrc:/images/HomeWidget/RectBgNormal.png"
            onClicked: {
                root.phoneLinkMiniWidgetClicked();
            }
        }

        Rectangle{
            id:linkRect
            anchors.left:parent.left
            anchors.leftMargin: 16
            anchors.top:parent.top
            anchors.topMargin: 80
            width: 369
            height: 240
            color:"transparent"
            Image{
                id:linkImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/PhoneLinkRect.png"
            }
        }

        Text {
            id:linkTypeName
            anchors.left: parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 360
            width: 273
            height: 39
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 32
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            objectName: "linkTypeNameObject"
        }

        Text {
            id:manualType
            anchors.left: parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 405
            width: 273
            height: 28
            opacity: 1
            color:"#B4B4B4"
            font.pixelSize: 24
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("手机互联")
        }
    }

    Text{
        id:widgetName
        anchors.left:phoneLinkRect.left
        anchors.leftMargin: 96
        anchors.top:phoneLinkRect.bottom
        anchors.topMargin: 30
        width: 208
        height: 69
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("手机互联")
    }
    onCurrentPageChanged: {
        switch(root.currentPage)
        {
            case 0:
                phoneLinkImage.source = "qrc:/images/HomeWidget/RectBgNormal.png";
                break;
            default:
                break;
        }
    }
}
