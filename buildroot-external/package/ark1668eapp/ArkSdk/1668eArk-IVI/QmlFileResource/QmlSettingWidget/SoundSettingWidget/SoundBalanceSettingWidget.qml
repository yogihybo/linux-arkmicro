import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal prevPageBtnClicked
    signal mousePressed(double x,double y)
    signal mouseRelease(double x,double y)
    signal mouseMove(double x,double y)
    Text{
        id:soundTitle
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
        text:qsTr("音效平衡")
    }

    Image {
        anchors.left: parent.left
        anchors.leftMargin: 312
        anchors.top:parent.top
        anchors.topMargin: 138
        source: "qrc:/images/SettingWidget/0 1.png"
    }

    Image {
        anchors.left: parent.left
        anchors.leftMargin: 304
        anchors.top:parent.top
        anchors.topMargin: 147
        source: "qrc:/images/SettingWidget/CSYS.png"
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
    Image{
        x:415
        y:300
        objectName: "tickImageObject"
        source: "qrc:/images/SettingWidget/SettingSoundSoundTicks.png"
    }
    Rectangle{
       anchors.fill:parent
       color: "transparent"
       MouseArea{
           anchors.fill:parent
           onPressed: {
              // console.log("++++++onPressed++++++");
               root.mousePressed(mouseX,mouseY);
           }
           onReleased: {
               root.mouseRelease(mouseX,mouseY);
           }
           onPositionChanged: {
               root.mouseMove(mouseX,mouseY)
           }
       }
    }
}
