import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal prevPageBtnClicked
    Text{
        id:wallpaper
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 44
        width: 197
        height: 40
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("壁纸")
    }

    Button{
        id:wallpaperBtn
        anchors.left: parent.left
        anchors.leftMargin: 75
        anchors.top:wallpaper.bottom
        anchors.topMargin: 40
        width: 240
        height: 160
        background: Rectangle{
            id:wallpaperBtnBg
            color: "transparent"
        }
        Image{
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/Wallpaper.png"
        }
    }

    Text{
        id:phoneLinkTitle
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:wallpaperBtn.bottom
        anchors.topMargin: 40
        width: 197
        height: 40
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("手机互联")
    }

    Text{
        id:phoneLinkType
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:phoneLinkTitle.bottom
        anchors.topMargin: 24
        width: 453
        height: 40
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("CarLife + Carplay")
    }

    Button{
        id:phoneLinkSetBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:phoneLinkTitle.bottom
        anchors.topMargin: 24
        width: 76
        height: 40
        background: Rectangle{
            id:dataSetBtnBg
            anchors.fill:parent
            color:"transparent"
        }

        Text{
            anchors.fill:parent
            opacity: 1
            color: "#0DA8FF"
            font.pixelSize: 28
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            text:qsTr("设置")
        }
        onPressed: {
            dataSetBtnBg.color = "#0DA8FF"
            dataSetBtnBg.opacity = 0.5
        }
        onReleased: {
            dataSetBtnBg.color = "transparent"
            dataSetBtnBg.opacity = 1
        }
    }

    Button{
        id:prevPageBtn
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.top:parent.top
        anchors.topMargin: 336
        width: 48
        height: 48
        background:Rectangle {
            id:prevPageBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/ArrowLeft.png"
        }
        onPressed: {
            prevPageBtnBg.color = "#6e6e6e"
            prevPageBtnBg.opacity = 0.5
        }
        onReleased: {
            prevPageBtnBg.color = "transparent"
            prevPageBtnBg.opacity = 1
        }
        onClicked: {
            root.prevPageBtnClicked()
        }
    }
}
