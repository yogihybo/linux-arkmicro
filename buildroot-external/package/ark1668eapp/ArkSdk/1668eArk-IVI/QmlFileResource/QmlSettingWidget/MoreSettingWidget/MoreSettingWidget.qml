import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property int showType: 0
    property int languageType:0
    signal simpChineseBtnClicked
    signal traditionalChineseBtnClicked
    signal englishBtnBtnClicked
    Rectangle{
        id:bgRet
        anchors.fill:parent
        color:"#090909"
    }
    Column{
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 44
        spacing: 140
        Repeater{
            model: ListModel{
                ListElement{name:QT_TR_NOOP("日期和时间")}
                ListElement{name:QT_TR_NOOP("语言")}
                ListElement{name:QT_TR_NOOP("手机互联")}
            }
            Text {
                width: 197
                height: 40
                id: soundName
                opacity: 0.4
                color: "#FFFFFF"
                font.pixelSize: 24
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:qsTr(name)
            }
        }
    }

    Text{
        id:dataText
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 126
        width: 453
        height: 40
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("2021年11月10日  15:10")
        objectName: "dataTextObject"
    }

    Button{
        id:dataSetBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:parent.top
        anchors.topMargin: 126
        width: 76
        height: 40
        objectName: "dataSetBtnObject"
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

    Rectangle{
        id:language
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 304
        width: 740
        height: 60
        radius: 30
        color:"#262626"
        Button{
            id:simpChineseBtn
            anchors.left: parent.left
            anchors.top:parent.top
            width: 185
            height: 60
            background: Rectangle{
                id:simpChineseBtnBg
                color: "transparent"
                radius: 30
            }
            Text{
                id:simpChineseBtnText
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top:parent.top
                anchors.topMargin: 18
                width: 145
                height: 24
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:qsTr("中文（简体）")
            }
            onClicked: {
                root.simpChineseBtnClicked();
                root.languageType = 1
            }
        }

        Button{
            id:tradChineseBtn
            anchors.left: simpChineseBtn.right
            anchors.leftMargin: 93
            anchors.top:parent.top
            width: 185
            height: 60
            background: Rectangle{
                id:tradChineseBtnBg
                color: "transparent"
                radius: 30
            }
            Text{
                id:tradChineseBtnText
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top:parent.top
                anchors.topMargin: 18
                width: 145
                height: 24
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:qsTr("中文（繁体）")
            }
            onClicked: {
                root.traditionalChineseBtnClicked()
                root.languageType = 2
            }
        }

        Button{
            id:englishBtn
            anchors.left: tradChineseBtn.right
            anchors.leftMargin: 51
            anchors.top:parent.top
            width: 246
            height: 60
            background: Rectangle{
                id:englishBtnBtnBg
                color: "transparent"
                radius: 30
            }
            Text{
                id:englishBtnText
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top:parent.top
                anchors.topMargin: 18
                width: 206
                height: 24
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:qsTr("英文")
            }
            onClicked: {
                root.englishBtnBtnClicked();
                root.languageType = 0;
            }
        }
        Component.onCompleted: {
            simpChineseBtnBg.color = "#6e6e6e"
        } 
    }

    onLanguageTypeChanged:
    {
        switch(root.languageType)
        {
            case 0:
                simpChineseBtnBg.color = "transparent"
                tradChineseBtnBg.color = "transparent"
                englishBtnBtnBg.color  = "#6e6e6e"
                break;
            case 1:
                simpChineseBtnBg.color = "#6e6e6e"
                tradChineseBtnBg.color = "transparent"
                englishBtnBtnBg.color  = "transparent"
                break;
            case 2:
                simpChineseBtnBg.color = "transparent"
                tradChineseBtnBg.color = "#6e6e6e"
                englishBtnBtnBg.color  = "transparent"
                break;
            default:
                break;
        }
    }

    Text{
        id:phoneLinkType
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 480
        width: 453
        height: 40
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "phoneLinkTypeObject"
        text:qsTr("CarLife + Carplay")
    }

    Button{
        id:phoneLinkSetBtn
        anchors.left: parent.left
        anchors.leftMargin: 700
        anchors.top:phoneLinkType.top
        anchors.topMargin: 24
        width: 76
        height: 40
        objectName: "phoneLinkSetBtnObject"
        background: Rectangle{
            id:phoneLinkSetBtnBg
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
            phoneLinkSetBtnBg.color = "#0DA8FF"
            phoneLinkSetBtnBg.opacity = 0.5
        }
        onReleased: {
            phoneLinkSetBtnBg.color = "transparent"
            phoneLinkSetBtnBg.opacity = 1
        }

    }

    Button{
        id:systemSwitchBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:phoneLinkSetBtn.bottom
        anchors.topMargin: 50
        width: 250
        height: 60
        objectName: "systemSwitchBtnObject"
        background: Rectangle{
            id:systemSwitchBtnBg
            anchors.fill:parent
            color:"#0DA8FF"
            radius: 30
        }

        Text{
            anchors.fill:parent
            opacity: 1
            color: "#FFFFFF"
            font.pixelSize: 28
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("系统切换")
        }
        onPressed: {
            systemSwitchBtn.opacity = 0.4
        }
        onReleased: {
            systemSwitchBtn.opacity = 1
        }

    }


    MoreSettingDataTimeWidget{
        id:dataTime
        anchors.left: parent.left
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 160
        visible: false
        objectName: "moreSettingDataTimeWidgetObject";
        onConfirmBtnClicked:  {
            root.showType = 0;
        }
        onCancelBtnClicked: {
            root.showType = 0;
        }
    }
    MoreSettingPhoneLinkWidget{
        id:phoneLinkWidget
        anchors.left: parent.left
        anchors.leftMargin: 40
        anchors.top:parent.top
        anchors.topMargin: 160
        visible: false
        objectName: "phoneLinkWidgetObject"
        onBtnClicked: {
            root.showType = 0;
        }
    }
    onShowTypeChanged: {
        switch(root.showType)
        {
            case 0:
                bgRet.color   = "#090909"
                bgRet.opacity = 1;
                dataSetBtn.enabled = true;
                phoneLinkSetBtn.enabled = true;
                phoneLinkWidget.visible = false;
                dataTime.visible = false;
                break;
            case 1:
                bgRet.color   = "#FFFFFF"
                bgRet.opacity = 0.3;
                dataSetBtn.enabled = false;
                phoneLinkSetBtn.enabled = false;
                phoneLinkWidget.visible = false;
                dataTime.visible = true;
                break;
            case 2:
                bgRet.color   = "#FFFFFF"
                bgRet.opacity = 0.3;
                dataSetBtn.enabled = false;
                phoneLinkSetBtn.enabled = false;
                phoneLinkWidget.visible = true;
                dataTime.visible = false;
                break;
            default:
                break;
        }
    }
}
