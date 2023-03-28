import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property int showType: 0
    Rectangle{
        id:bgRect
        anchors.fill:parent
        opacity: 1
        color:"#000000"
    }
    Text{
        id:aboutTitle
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
        text:qsTr("关于")
    }

    Column{
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 126
        spacing: 48
        Repeater{
            model: ListModel{
                ListElement{name:QT_TR_NOOP("关于本机")}
                ListElement{name:QT_TR_NOOP("重启系统")}
                ListElement{name:QT_TR_NOOP("恢复出厂设置")}
            }
            Text {
                width: 197
                height: 40
                opacity: 1
                color: "#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:qsTr(name)
            }
        }
    }

    Button{
        id:checkBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:parent.top
        anchors.topMargin: 126
        width: 76
        height: 40
        background: Rectangle{
            id:checkBtnBg
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
            text:qsTr("查看")
        }
        onPressed: {
            checkBtnBg.color = "#0DA8FF"
            checkBtnBg.opacity = 0.5
        }
        onReleased: {
            checkBtnBg.color = "transparent"
            checkBtnBg.opacity = 1
        }
        onClicked: {
            root.showType = 1;
            checkBtn.enabled = false;
            resetBtn.enabled = false;
            recoveryBtn.enabled = false;
        }
    }

    Button{
        id:resetBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:parent.top
        anchors.topMargin: 214
        width: 76
        height: 40
        background: Rectangle{
            id:resetBtnBg
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
            text:qsTr("重启")
        }
        onPressed: {
            resetBtnBg.color = "#0DA8FF"
            resetBtnBg.opacity = 0.5
        }
        onReleased: {
            resetBtnBg.color = "transparent"
            resetBtnBg.opacity = 1
        }
        onClicked: {
            root.showType = 2;
            checkBtn.enabled = false;
            resetBtn.enabled = false;
            recoveryBtn.enabled = false;
        }
    }

    Button{
        id:recoveryBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:parent.top
        anchors.topMargin: 302
        width: 76
        height: 40
        background: Rectangle{
            id:recoveryBtnBg
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
            text:qsTr("恢复")
        }
        onPressed: {
            recoveryBtnBg.color = "#0DA8FF"
            recoveryBtnBg.opacity = 0.5
        }
        onReleased: {
            recoveryBtnBg.color = "transparent"
            recoveryBtnBg.opacity = 1
        }
        onClicked: {
            root.showType = 3;
            checkBtn.enabled = false;
            resetBtn.enabled = false;
            recoveryBtn.enabled = false;
        }
    }

    AboutSettingNativeInfoWidget{
        id:aboutSettingNativeInfoWidget
        x:40
        y:50
        visible: false
        objectName: "nativeInfoWidgetObject"
        onBtnClicked: {
            root.showType = 0;
        }
    }

    AboutSettingResetWidget{
        id:aboutSettingResetWidget
        x:140
        y:222
        visible: false
        objectName: "resetWidgetObject"
        onBtnClicked: {
            root.showType = 0;
        }
    }

    AboutSettingRecoveryWidget{
        id:aboutSettingRecoveryWidget
        x:140
        y:222
        objectName: "recoveryWidgetObject"
        visible: false
        onBtnClicked: {
            root.showType = 0;
        }
    }
    onShowTypeChanged: {
        switch(root.showType)
        {
            case 0:
                bgRect.visible =  true;
                bgRect.opacity = 1;
                bgRect.color   = "#000000";
                checkBtn.enabled = true;
                resetBtn.enabled = true;
                recoveryBtn.enabled = true;
                aboutSettingNativeInfoWidget.visible =false;
                aboutSettingResetWidget.visible = false;
                aboutSettingRecoveryWidget.visible = false;
                break;
             case 1:
                 bgRect.opacity = 0.3;
                 bgRect.color   = "#FFFFFF";
                 aboutSettingNativeInfoWidget.visible =true;
                 aboutSettingResetWidget.visible = false;
                 aboutSettingRecoveryWidget.visible = false;
                 break;
             case 2:
                 bgRect.opacity = 0.3;
                 bgRect.color   = "#FFFFFF";
                 aboutSettingNativeInfoWidget.visible =false;
                 aboutSettingResetWidget.visible = true;
                 aboutSettingRecoveryWidget.visible = false;
                 break;
             case 3:
                 bgRect.color   = "#FFFFFF";
                 bgRect.opacity = 0.3;
                 aboutSettingNativeInfoWidget.visible =false;
                 aboutSettingResetWidget.visible = false;
                 aboutSettingRecoveryWidget.visible = true;
                 break;
              default:
                  break;
        }
    }
}
