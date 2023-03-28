import QtQuick 2.0
import QtQuick.Controls 2.0
import com.QmlWidget.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal nextPageBtnClicked
    signal mediaSliderMoveFinish
    signal navigationMoveFinish
    signal telephoneMoveFinish
    Column{
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 44
        spacing: 140
        Repeater{
            model: ListModel{
                ListElement{name:QT_TR_NOOP("导航音量")}
                ListElement{name:QT_TR_NOOP("电话音量")}
                ListElement{name:QT_TR_NOOP("媒体音量")}
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
    Slider{
        id:navigation
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 124
        width: 740
        height: 60
        stepSize:1
        from: 0
        to: 50
        value: 20
        orientation:Qt.Horizontal
        snapMode:"SnapAlways"
        objectName: "navigationObject"
        property bool hoveredStatus: false
        background: Rectangle{
            x:navigation.leftPadding
            y:navigation.topPadding + navigation.availableHeight / 2 - height / 2
            width:  740
            height: 60
            radius: 30
            color:"#262626"
            Image{
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top:parent.top
                anchors.topMargin: 6
                source: "qrc:/images/SettingWidget/Sount.png"
            }
            Text{
                id:navigationValueText
                anchors.centerIn: parent
                color:"#FFFFFF"
                opacity: 1
                font.pixelSize: 24
                height: 24
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:navigation.value
                visible: false
            }
            Rectangle {
                id:navigationRect
                width: navigation.visualPosition * parent.width
                height: parent.height
                opacity: 1
                color:"#6e6e6e"
                radius: 30
                Text{
                    id:navigationValue
                    anchors.centerIn: parent
                    color:"#FFFFFF"
                    opacity: 1
                    font.pixelSize: 24
                    height: 24
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:navigation.value
                    visible: false
                }
                Component.onCompleted: {
                    if(navigationRect.width >= 50)
                    {
                        navigationValue.visible = true;
                    }
                    else
                    {
                        navigationValueText.visible = true
                    }
                }
                onWidthChanged: {
                    if(navigationRect.width >= 50)
                    {
                        navigationValue.visible = true;
                        navigationValueText.visible = false
                    }
                    else
                    {
                        navigationValue.visible = false;
                        navigationValueText.visible = true
                    }
                }
          }
        }
        handle: Rectangle {
            x: navigation.leftPadding + navigation.visualPosition * (navigation.availableWidth - width)
            y: navigation.topPadding + navigation.availableHeight / 2 - height / 2
            implicitWidth: 60
            implicitHeight:60
            color:"transparent"
        }
        onPressedChanged: {
            if(navigation.hoveredStatus === false)
            {
                navigation.hoveredStatus = true;
            }
            else{
                navigation.hoveredStatus = false;
                root.navigationMoveFinish();
            }
        }
        onValueChanged:
        {
            if(QmlWidget.getVolumeType() === 1)
            {
                muteBtn.muteStatus = false;
            }
        }
    }

    Slider{
        id:telephone
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 304
        width: 740
        height: 60
        stepSize:1
        from: 0
        to: 50
        value: 20
        orientation:Qt.Horizontal
        snapMode:"SnapAlways"
        objectName: "telephoneObject"
        property bool hoveredStatus: false
        background: Rectangle{
            x:telephone.leftPadding
            y:telephone.topPadding + telephone.availableHeight / 2 - height / 2
            width:  740
            height: 60
            radius: 30
            color:"#262626"
            Image{
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top:parent.top
                anchors.topMargin: 6
                source: "qrc:/images/SettingWidget/Sount.png"
            }
            Text{
                id:telephoneValueText
                anchors.centerIn: parent
                color:"#FFFFFF"
                opacity: 1
                font.pixelSize: 24
                height: 24
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:telephone.value
                visible: false
            }
            Rectangle {
                id:telephoneRect
                width: telephone.visualPosition * parent.width
                height: parent.height
                opacity: 1
                color:"#6e6e6e"
                radius: 30
                Text{
                    id:telephoneValue
                    anchors.centerIn: parent
                    color:"#FFFFFF"
                    opacity: 1
                    font.pixelSize: 24
                    height: 24
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:telephone.value
                    visible: false
                }
                Component.onCompleted: {
                    if(telephoneRect.width >= 50)
                    {
                        telephoneValue.visible = true;
                    }
                    else
                    {
                        telephoneValueText.visible = true
                    }
                }
                onWidthChanged: {
                    if(telephoneRect.width >= 50)
                    {
                        telephoneValue.visible = true;
                        telephoneValueText.visible = false
                    }
                    else
                    {
                        telephoneValue.visible = false;
                        telephoneValueText.visible = true
                    }
                }
          }
        }
        handle: Rectangle {
            x: telephone.leftPadding + telephone.visualPosition * (telephone.availableWidth - width)
            y: telephone.topPadding + telephone.availableHeight / 2 - height / 2
            implicitWidth: 60
            implicitHeight:60
            color:"transparent"
        }
        onPressedChanged: {
            if(telephone.hoveredStatus === false)
            {
                telephone.hoveredStatus = true;
            }
            else{
                telephone.hoveredStatus = false;
                root.telephoneMoveFinish();
            }
        }
        onValueChanged:
        {
            if(QmlWidget.getVolumeType() === 2)
            {
                muteBtn.muteStatus = false;
            }
        }
    }

    Slider{
        id:media
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 484
        width: 740
        height: 60
        stepSize:1
        from: 0
        to: 50
        value: 20
        orientation:Qt.Horizontal
        snapMode:"SnapAlways"
        objectName: "mediaObject"
        property bool hoveredStatus: false
        background: Rectangle{
            x:media.leftPadding
            y:media.topPadding + media.availableHeight / 2 - height / 2
            width:  740
            height: 60
            radius: 30
            color:"#262626"
            Image{
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top:parent.top
                anchors.topMargin: 6
                source: "qrc:/images/SettingWidget/Sount.png"
            }
            Text{
                id:mediaValueText
                anchors.centerIn: parent
                color:"#FFFFFF"
                opacity: 1
                font.pixelSize: 24
                height: 24
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:media.value
                visible: false
            }
            Rectangle {
                id:meidaRect
                width: media.visualPosition * parent.width
                height: parent.height
                opacity: 1
                color:"#6e6e6e"
                radius: 30
                Text{
                    id:meidaValue
                    anchors.centerIn: parent
                    color:"#FFFFFF"
                    opacity: 1
                    font.pixelSize: 24
                    height: 24
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:media.value
                    visible: false
                }
                Component.onCompleted: {
                    if(meidaRect.width >= 50)
                    {
                        meidaValue.visible = true;
                    }
                    else
                    {
                        mediaValueText.visible = true
                    }
                }
                onWidthChanged: {
                    if(meidaRect.width >= 50)
                    {
                        meidaValue.visible = true;
                        mediaValueText.visible = false
                    }
                    else
                    {
                        meidaValue.visible = false;
                        mediaValueText.visible = true
                    }
                }
          }
        }
        handle: Rectangle {
            x: media.leftPadding + media.visualPosition * (media.availableWidth - width)
            y: media.topPadding + media.availableHeight / 2 - height / 2
            implicitWidth: 60
            implicitHeight:60
            color:"transparent"
        }
        onPressedChanged: {
            if(media.hoveredStatus === false)
            {
                media.hoveredStatus = true;
            }
            else{
                media.hoveredStatus = false;
                root.mediaSliderMoveFinish();
            }
        }
        onValueChanged:
        {
            if(QmlWidget.getVolumeType() === 0)
            {
                muteBtn.muteStatus = false;
            }
        }
    }
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

    Button
    {
       id:muteBtn
       anchors.horizontalCenter: parent.horizontalCenter
       anchors.top:media.bottom
       anchors.topMargin: 50
       width: 300
       height: 60
       property bool muteStatus: false
       background: Rectangle{
            id:muteBtnBg
            anchors.fill: parent
            radius:30
            color:"red"
       }

       Text {
           id:muteText
           anchors.centerIn: parent
           color:"#FFFFFF"
           opacity: 1
           font.pixelSize: 48
           height: 48
           font.family: "Montserrat"
           horizontalAlignment: Text.AlignHCenter
           verticalAlignment: Text.AlignVCenter
           text:qsTr("静音")
        }
        onClicked:{
            if(muteBtn.muteStatus === false)
            {
                muteBtn.muteStatus = true
            }
            else{
                muteBtn.muteStatus = false
            }
           QmlWidget.requestMuteToggole();
        }
        onPressed:  muteBtn.opacity  = 0.4
        onReleased: muteBtn.opacity  = 1
        onMuteStatusChanged: {
            if(muteBtn.muteStatus === true)
            {
                muteBtnBg.color ="#0DA8FF"
                muteText.text = qsTr("恢复静音")
            }
            else{
                muteBtnBg.color ="red"
                muteText.text = qsTr("静音")
            }
        }
    }
}
