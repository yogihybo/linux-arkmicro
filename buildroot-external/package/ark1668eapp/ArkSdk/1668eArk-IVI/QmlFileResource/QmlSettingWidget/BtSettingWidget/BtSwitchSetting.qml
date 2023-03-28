import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal nextPageBtnClicked
    Button{
        id:nextPageBtn
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.top:parent.top
        anchors.topMargin: 336
        width: 48
        height: 48
        background:Rectangle {
            id:nextPageBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/ArrowRight.png"
        }
        onPressed: {
            nextPageBtnBg.color = "#0DA8FF"
            nextPageBtnBg.opacity = 0.5
        }
        onReleased: {
            nextPageBtnBg.color = "transparent"
            nextPageBtnBg.opacity = 1
        }
        onClicked: {
            root.nextPageBtnClicked()
        }
    }
    Text{
        id:btTitle
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
        text:qsTr("蓝牙")
    }
    Text{
        id:btNameInput
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 88
        width: 318
        height: 25
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 20
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "btDeviceNameObject"
        text:qsTr("")
    }
    Text{
        id:pinCodeInput
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:btNameInput.bottom
        anchors.topMargin: 11
        width: 318
        height: 25
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 20
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "btPinCodeObject"
        text:qsTr("")
    }
    Column{
        anchors.left:parent.left
        anchors.leftMargin: 70
        anchors.top:pinCodeInput.bottom
        anchors.topMargin: 11
        spacing: 60
        Repeater{
            id:btNameRepeater
            model:ListModel{
                ListElement{name:QT_TR_NOOP("开启蓝牙")}
                ListElement{name:QT_TR_NOOP("自动连接")}
                ListElement{name:QT_TR_NOOP("自动接听")}
            }
            Text{
                id:delegateText
                width: 197
                height: 40
                opacity: 1
                color: "#FFFFFF"
                font.pixelSize: 24
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:qsTr(name)
            }
        }
    }

    Button{
        id:powerBtn
        anchors.left:parent.left
        anchors.leftMargin: 686
        anchors.top:pinCodeInput.bottom
        anchors.topMargin: 1
        width: 120
        height: 60
        objectName: "powerBtnObject"
        property int powerStatus: 0
        background: Rectangle{
            id:powerBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:powerBtnIcon
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/CloseSwitch.png"
        }
        onPressed:{
            //powerBtnBg.color = "#0DA8FF"
            powerBtn.opacity = 0.5
        }
        onReleased:{
            //powerBtnBg.color = "transparent"
            powerBtn.opacity = 1
        }
        onPowerStatusChanged: {
            switch(powerBtn.powerStatus){
                case 0:
                    powerBtnIcon.source = "qrc:/images/SettingWidget/CloseSwitch.png"
                    break;
                case 1:
                    powerBtnIcon.source = "qrc:/images/SettingWidget/OpenSwitch.png"
                    break;
                default:
                    break;
            }
        }
    }

    Button{
        id:autoConnectBtn
        anchors.left:parent.left
        anchors.leftMargin: 686
        anchors.top:powerBtn.bottom
        anchors.topMargin: 40
        width: 120
        height: 60
        objectName: "autoConnectBtnObject"
        property int autoConnectStatus: 0
        background: Rectangle{
            id:autoConnectBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:autoConnectBtnIcon
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/CloseSwitch.png"
        }
        onPressed:{
            //autoConnectBtnBg.color = "#0DA8FF"
            autoConnectBtn.opacity = 0.5
        }
        onReleased:{
            //autoConnectBtnBg.color = "transparent"
            autoConnectBtn.opacity = 1
        }
        onAutoConnectStatusChanged: {
            switch(autoConnectBtn.autoConnectStatus){
                case 0:
                    autoConnectBtnIcon.source = "qrc:/images/SettingWidget/CloseSwitch.png"
                    break;
                case 1:
                    autoConnectBtnIcon.source = "qrc:/images/SettingWidget/OpenSwitch.png"
                    break;
                default:
                    break;
            }
        }
    } 
    Button{
        id:autoAnswerBtn
        anchors.left:parent.left
        anchors.leftMargin: 686
        anchors.top:autoConnectBtn.bottom
        anchors.topMargin: 40
        width: 120
        height: 60
        objectName: "autoAnswerBtnObject"
        property int autoAnswerStatus: 0
        background: Rectangle{
            id:autoAnswerBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:autoAnswerBtnIcon
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/CloseSwitch.png"
        }
        onPressed:  {
            //autoAnswerBtnBg.color = "#0DA8FF"
            autoAnswerBtn.opacity = 0.5
        }
        onReleased: {
            //autoAnswerBtnBg.color = "transparent"
            autoAnswerBtn.opacity = 1
        }
        onAutoAnswerStatusChanged: {
            switch(autoAnswerBtn.autoAnswerStatus){
                case 0:
                    autoAnswerBtnIcon.source = "qrc:/images/SettingWidget/CloseSwitch.png"
                    break;
                case 1:
                    autoAnswerBtnIcon.source = "qrc:/images/SettingWidget/OpenSwitch.png"
                    break;
                default:
                    break;
            }
        }
    }
    Text {
        id:msgText
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:autoAnswerBtn.bottom
        anchors.topMargin: 20
        width: 880
        height: 40
        opacity: 1
        color: "#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"Carplay is connected, please disconnect it first"
        objectName: "msgTextObject"
    }
}
