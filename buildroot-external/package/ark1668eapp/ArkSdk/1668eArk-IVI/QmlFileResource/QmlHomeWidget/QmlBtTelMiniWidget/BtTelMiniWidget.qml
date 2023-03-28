import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 400
    height: 659
    visible: true
    property int currentPage: 0
    signal  btTelMiniWidgetClicked()
    Rectangle{
        id:btTelRect
        x:0
        y:0
        width: 400
        height: 560
        radius: 20
        Image{
            id:btTelRectImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/RectBgNormal.png"
        }
        MouseArea{
            x:0
            y:0
            width:400
            height: 330
            onPressed:  btTelRectImage.source = "qrc:/images/HomeWidget/RectBgPress.png"
            onReleased: btTelRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png"
            onClicked: {
                root.btTelMiniWidgetClicked();
            }
        }

        Rectangle{
            id:callerRect
            anchors.left:parent.left
            anchors.leftMargin: 70
            anchors.top:parent.top
            anchors.topMargin: 70
            width: 260
            height: 260
            color:"transparent"
            Image{
                id:callerImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/Clock.png"
            }
        }

        Text {
            id:callerName
            anchors.left: parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 341
            width: 273
            height: 39
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 32
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            objectName: "callerNameObject"
        }

        Text {
            id:callType
            anchors.left: parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 386
            width: 273
            height: 28
            opacity: 1
            color:"#B4B4B4"
            font.pixelSize: 24
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:"Idle Call"
            objectName: "callTypeObject"
        }

        Button{
            id:answerBtn
            anchors.left: parent.left
            anchors.leftMargin: 59
            anchors.top:parent.top
            anchors.topMargin: 433
            width: 88
            height: 88
            objectName: "answerBtnObject"
            enabled: false
            background: Rectangle{
                anchors.fill:parent
                color:"transparent"
            }
            Image{
                id:answerImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/AnswerNormal.png"

            }
            onPressed: answerImage.source  = "qrc:/images/HomeWidget/AnswerPress.png"
            onReleased: answerImage.source = "qrc:/images/HomeWidget/AnswerNormal.png"
        }

        Button{
            id:hangUpBtn
            anchors.left: parent.left
            anchors.leftMargin: 265
            anchors.top:parent.top
            anchors.topMargin: 433
            enabled: false
            background: Rectangle{
                anchors.fill:parent
                color: "transparent"
            }
            width: 88
            height: 88
            objectName: "hangUpBtnObject"
            Image{
                id:hangUpImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/HungUpNormal.png"
            }
            onPressed: hangUpImage.source = "qrc:/images/HomeWidget/HungUpPress.png"
            onReleased: hangUpImage.source = "qrc:/images/HomeWidget/HungUpNormal.png"
        }
    }

    Text{
        id:widgetName
        anchors.left:btTelRect.left
        anchors.leftMargin: 96
        anchors.top:btTelRect.bottom
        anchors.topMargin: 30
        width: 208
        height: 69
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("蓝牙电话")
    }
    onCurrentPageChanged:{
        switch(root.currentPage)
        {
            case 0:
                btTelRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png";
                answerImage.source = "qrc:/images/HomeWidget/AnswerNormal.png";
                hangUpImage.source = "qrc:/images/HomeWidget/HungUpNormal.png";
                break;
            default:
                break;
        }
    }

}
