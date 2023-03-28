import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 800
    height: 420
    visible: true
    property int  currentIndex: -1
    signal btnClicked
    Rectangle{
        anchors.fill:parent
        color:"#0E0E0E"
        radius: 20
    }

    Column{
        id:phoneTypeColumn
        anchors.left:parent.left
        anchors.top:parent.top
        anchors.topMargin: 48
        width: 800
        height: 233
        spacing: 27
        Repeater{
            id:phoneTypeRepeater
            model: ["CarLife+Carplay","Auto+Carplay",QT_TR_NOOP("亿连手机互联"),"HiCar"]
            Button{
                width: 800
                height: 38
                property alias textOpacity:phoneTypeText.opacity
                background: Rectangle{
                    anchors.fill: parent
                    color:"transparent"
                }
                Text{
                   id:phoneTypeText
                   anchors.fill:parent
                   color: "#FFFFFF"
                   opacity: 0.4
                   font.pixelSize: 28
                   font.family: "Alibaba PuHuiTi"
                   horizontalAlignment: Text.AlignHCenter
                   verticalAlignment: Text.AlignVCenter
                   text: modelData
                }
                onPressed: phoneTypeText.opacity = 0.2
                onReleased:phoneTypeText.opacity = 0.4
                onClicked: {
                    //console.log("+++++++index+++++++",index);
                    root.currentIndex = index;
                }
            }
        }
    }
    onCurrentIndexChanged: {
        if(root.visible){
            for(var i=0;i<4;i++)
            {
                if(i=== root.currentIndex){
                    phoneTypeRepeater.itemAt(i).textOpacity = 1;
                }
                else{
                    phoneTypeRepeater.itemAt(i).textOpacity = 0.4;
                }
            }
        }
    }

    Rectangle{
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 320
        width: 800
        height: 2
        color: "#FFFFFF"
    }
    Button{
        id:cancleBtn
        anchors.left: parent.left
        anchors.leftMargin: 210
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 150
        height: 60
        objectName: "cancleBtnObject"
        background: Rectangle{
            id:cancleBtnBg
            anchors.fill: parent
            color: "#FFFFFF"
            opacity: 0.2
            radius: 50
        }

        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("取消")
        }
        onPressed:  cancleBtnBg.opacity = 0.4
        onReleased: cancleBtnBg.opacity = 0.2
        onClicked: {
            root.visible = false;
            root.btnClicked();
        }
    }

    Button{
        id:confirmBtn
        anchors.left: parent.left
        anchors.leftMargin: 440
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 150
        height: 60
        objectName: "confirmBtnObject"
        background: Rectangle{
            id:confirmBtnBg
            anchors.fill: parent
            color: "#0DA8FF"
            radius: 50
        }

        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("确认")
        }
        onPressed:  cancleBtnBg.opacity = 0.4
        onReleased: cancleBtnBg.opacity = 1
        onClicked: {
            root.visible = false;
            root.btnClicked();
        }
    }
}
