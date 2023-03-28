import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property bool btOpenStatus: false
    signal sliderMoveFinish()
    Text{
        id:lightTitle
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
        text:qsTr("亮度")
    }
    Text{
        id:lightSwitchName
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 124
        width: 197
        height: 40
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("自动调整亮度")
    }

    Button{
       id:lightSwitchBtn
       anchors.left: parent.left
       anchors.leftMargin: 686
       anchors.top:parent.top
       anchors.topMargin: 114
       width: 120
       height: 60
       enabled: false
       background: Rectangle{
            id:lightSwitchBtnBgRect
            anchors.fill:parent
            color:"transparent"
       }
       Image{
           id:lightSwitchBtnBg
           anchors.fill:parent
           source: "qrc:/images/SettingWidget/CloseSwitch.png"
       }
       onClicked: {
            if(root.btOpenStatus === false)
            {
                lightSwitchBtnBg.source = "qrc:/images/SettingWidget/OpenSwitch.png"
                root.btOpenStatus = true
            }
            else
            {
                lightSwitchBtnBg.source = "qrc:/images/SettingWidget/CloseSwitch.png"
                root.btOpenStatus = false
            }
       }

    }

    Slider{
        id:control
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 244
        width: 740
        height: 60
        stepSize:1
        from: 0
        to: 100
        value: 30
        orientation:Qt.Horizontal
        snapMode:"SnapAlways"
        objectName: "brightnessSliderObject"

        background: Rectangle{
            x:control.leftPadding
            y:control.topPadding + control.availableHeight / 2 - height / 2
            width:  740
            height: 60
            radius: 30
            color:"#262626"
            Image{
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top:parent.top
                anchors.topMargin: 6
                source: "qrc:/images/SettingWidget/Brightness.png"
            }
            Text{
                id:lightValueText
                anchors.centerIn: parent
                color:"#FFFFFF"
                opacity: 1
                font.pixelSize: 24
                height: 24
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:control.value + "%"
                visible: false
            }
            Rectangle {
                id:lightRect
                width: control.visualPosition * parent.width
                height: parent.height
                opacity: 1
                color:"#6e6e6e"
                radius: 30
                Text{
                    id:lightValue
                    anchors.centerIn: parent
                    color:"#FFFFFF"
                    opacity: 1
                    font.pixelSize: 24
                    height: 24
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:control.value + "%"
                    visible: false
                }
                Component.onCompleted: {
                    if(lightRect.width >= 50)
                    {
                        lightValue.visible = true;
                    }
                    else
                    {
                        lightValueText.visible = true
                    }
                }
                onWidthChanged: {
                    if(lightRect.width >= 50)
                    {
                        lightValue.visible = true;
                        lightValueText.visible = false
                    }
                    else
                    {
                        lightValue.visible = false;
                        lightValueText.visible = true
                    }
                }
          }
        }
        handle: Rectangle {
            x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: 60
            implicitHeight:60
            color:"transparent"
        }
        MouseArea{
            anchors.fill:parent
            onReleased: {
                root.sliderMoveFinish();
            }
        }

    }
}
