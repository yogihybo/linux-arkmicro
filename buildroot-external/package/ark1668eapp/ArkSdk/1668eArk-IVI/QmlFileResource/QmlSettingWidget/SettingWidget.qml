import QtQuick 2.0
import QtQuick.Controls 2.0
import "./BtSettingWidget"
import "./LightSettingWidget"
import "./SoundSettingWidget"
import "./MoreSettingWidget"
import "./AboutSettingWidget"
import "./WifiSettingWidget"
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int lastIndex: 0
    property int currentIndex: 0
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color: "#000000"
    }
    Rectangle{
        id:funcType
        anchors.left: parent.left
        anchors.leftMargin: 520
        anchors.top:parent.top
        width: 360
        height: 720
        color: "#161616"
        Column{
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.top:parent.top
            anchors.topMargin: 40
            spacing: 32
            Repeater{
                id:funcTypeRepeater
                model: ListModel
                {
                    ListElement{path:"qrc:/images/SettingWidget/Bt.png";name:QT_TR_NOOP("蓝牙")}
                    ListElement{path:"qrc:/images/SettingWidget/Light.png";name:QT_TR_NOOP("亮度")}
                    ListElement{path:"qrc:/images/SettingWidget/Sount.png";name:QT_TR_NOOP("声音")}
                    ListElement{path:"qrc:/images/SettingWidget/MoreSet.png";name:QT_TR_NOOP("更多设置")}
                    ListElement{path:"qrc:/images/SettingWidget/About.png";name:QT_TR_NOOP("关于")}
                    ListElement{path:"qrc:/images/SettingWidget/Wifi.png";name:QT_TR_NOOP("网络")}
                }

                Button{
                    id:degateBtn
                    width: 266
                    height: 48
                    property alias btnIconOpacity: btnIcon.opacity
                    property alias btnTextOpacity: btnText.opacity
                    background: Rectangle{
                        id:btnBgRect
                        anchors.fill:parent
                        color: "transparent"
                    }
                    Image{
                        id:btnIcon
                        anchors.left:degateBtn.left
                        anchors.top: degateBtn.top
                        opacity: 0.4
                        source: path
                    }
                    Text{
                        id:btnText
                        anchors.left:degateBtn.left
                        anchors.leftMargin: 65
                        anchors.top: degateBtn.top
                        anchors.topMargin: 4
                        width: 101
                        height: 40
                        opacity: 0.4
                        color:"#FFFFFF"
                        font.pixelSize: 24
                        font.family: "Alibaba PuHuiTi"
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text:qsTr(name)
                    }
                    onPressed: {
                        btnBgRect.color = "#0DA8FF"
                        btnBgRect.opacity = 0.5
                    }
                    onReleased: {
                        btnBgRect.color = "transparent"
                        btnBgRect.opacity = 1

                    }
                    onClicked: {
                        root.currentIndex = index;
                    }
                    Component.onCompleted: {
                        if(index === root.currentIndex)
                        {
                            btnIcon.opacity = 1;
                            btnText.opacity = 1;
                        }
                    }
                }
            }
        }
    }

    onCurrentIndexChanged: {
        funcTypeRepeater.itemAt(root.lastIndex).btnIconOpacity = 0.4
        funcTypeRepeater.itemAt(root.lastIndex).btnTextOpacity = 0.4
        funcTypeRepeater.itemAt(root.currentIndex).btnIconOpacity = 1
        funcTypeRepeater.itemAt(root.currentIndex).btnTextOpacity = 1
        root.lastIndex =  root.currentIndex
        switch(root.currentIndex){
            case 0:
                btSettingWidget.visible    = true;
                lightSettingWidget.visible = false;
                soundSettingWidget.visible = false;
                moreSettingWidget.visible  = false;
                aboutSettingWidget.visible = false;
                wifiSettingWidget.visible = false;
                break;
             case 1:
                 btSettingWidget.visible    = false;
                 lightSettingWidget.visible = true;
                 soundSettingWidget.visible = false;
                 moreSettingWidget.visible  = false;
                 aboutSettingWidget.visible = false;
                 wifiSettingWidget.visible = false;
                 break;
             case 2:
                 btSettingWidget.visible    = false;
                 lightSettingWidget.visible = false;
                 soundSettingWidget.visible = true;
                 moreSettingWidget.visible  = false;
                 aboutSettingWidget.visible = false;
                 wifiSettingWidget.visible = false;
                 break;
             case 3:
                 btSettingWidget.visible    = false;
                 lightSettingWidget.visible = false;
                 soundSettingWidget.visible = false;
                 moreSettingWidget.visible  = true;
                 aboutSettingWidget.visible = false;
                 wifiSettingWidget.visible = false;
                 break;
             case 4:
                 btSettingWidget.visible    = false;
                 lightSettingWidget.visible = false;
                 soundSettingWidget.visible = false;
                 moreSettingWidget.visible  = false;
                 aboutSettingWidget.visible = true;
                 wifiSettingWidget.visible = false;
                 break;
             case 5:
                 btSettingWidget.visible    = false;
                 lightSettingWidget.visible = false;
                 soundSettingWidget.visible = false;
                 moreSettingWidget.visible  = false;
                 aboutSettingWidget.visible = false;
                 wifiSettingWidget.visible = true;
                 break;
             default:
                 break;
        }
    }

    //LogoWidget
    LogoWidget{
        id:logoWidget
        anchors.left: parent.left
        anchors.top:parent.top
    }

    BtSettingWidget{
        id:btSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        objectName: "btSettingWidgetObject"
    }


    LightSettingWidget{
        id:lightSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        visible: false
        objectName: "lightSettingWidgetObject"
    }

    SoundSettingWidget{
        id:soundSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        visible: false
        objectName: "soundSettingWidgetObject"
    }

    MoreSettingWidget{
        id:moreSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        visible: false
        objectName: "moreSettingWidgetObject"
    }

    AboutSettingWidget{
        id:aboutSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        visible: false
        objectName: "aboutSettingWidgetObject"
    }
    WifiSettingWidget{
        id:wifiSettingWidget
        anchors.left: parent.left
        anchors.leftMargin: 880
        anchors.top:parent.top
        visible: false
        objectName: "wifiSettingWidgetObject"
    }
}
