import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 600
    height: 360
    visible: true
    signal btnClicked
    Rectangle{
        anchors.fill:parent
        color:"#000000"
        radius: 20
    }
    Image{
        anchors.left: parent.left
        anchors.leftMargin: 80
        anchors.top:parent.top
        anchors.topMargin: 91
        width: 100
        height: 100
        source: "qrc:/images/SettingWidget/Warning.png"
    }
    Text{
        id:title
        anchors.left: parent.left
        anchors.leftMargin: 220
        anchors.top:parent.top
        anchors.topMargin: 120
        width: 380
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("恢复出厂将重置所有设置")
    }

    Rectangle{
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 260
        width: 600
        height:2
        color:"#FFFFFF"
    }

    Button{
        id:cancelBtn
        anchors.left: parent.left
        anchors.leftMargin: 110
        anchors.top:parent.top
        anchors.topMargin: 280
        width: 150
        height: 60
        background: Rectangle{
            id:cancelBtnBg
            anchors.fill:parent
            radius: 30
            opacity: 0.2
            color: "#FFFFFF"
        }
        Text{
            id:cancelBtnBgText
            anchors.left: cancelBtn.left
            anchors.leftMargin: 20
            anchors.top:cancelBtn.top
            anchors.topMargin: 18
            width: 110
            height: 24
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("取消")
        }
        onPressed: cancelBtnBg.opacity = 0.4
        onReleased: cancelBtnBg.opacity = 0.2
        onClicked: {
            root.visible = false
            root.btnClicked();
        }
    }

    Button{
        id:confirmBtn
        anchors.left: parent.left
        anchors.leftMargin: 340
        anchors.top:parent.top
        anchors.topMargin: 280
        width: 150
        height: 60
        objectName: "confirmBtnObject"
        background: Rectangle{
            id:confirmBtnBg
            anchors.fill:parent
            radius: 30
            color: "#0DA8FF"
        }
        Text{
            id:confirmBtnText
            anchors.left: confirmBtn.left
            anchors.leftMargin: 20
            anchors.top:confirmBtn.top
            anchors.topMargin: 18
            width: 110
            height: 24
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("确定")
        }
        onPressed: confirmBtnBg.opacity = 0.4
        onReleased: confirmBtnBg.opacity = 1
        onClicked: {
            root.visible = false;
            root.btnClicked();
        }
    }
}
