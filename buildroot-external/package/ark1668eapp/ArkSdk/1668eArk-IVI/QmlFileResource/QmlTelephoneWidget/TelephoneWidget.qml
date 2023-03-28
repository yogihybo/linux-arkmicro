import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int  telTypeIndex: 0
    property int  btConnectStatus: 0
    property string  rootPhoneNumber: ""
    signal listviewItemClicked()
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color:"#000000"
    }
    Rectangle{
        id:telType
        x:520
        y:0
        width: 360
        height: 720
        color:"#161616"
    }
    Button{
        id:callLogBtn
        anchors.left: parent.left
        anchors.leftMargin: 520
        anchors.top:parent.top
        anchors.topMargin: 44
        width: 200
        height: 40
        objectName:"callLogBtnObject"
        property alias textOpacity: callLogText.opacity
        background: Rectangle
        {
            id:callLogBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Text{
            id:callLogText
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.top:parent.top
            width: 101
            height: 40
            opacity: 0.4
            color:"#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:qsTr("通话记录")
        }
        onPressed: {
            callLogBtnBg.color = "#0DA8FF"
            callLogBtn.opacity = 0.5
        }
        onReleased: {
            callLogBtnBg.color = "transparent"
            callLogBtn.opacity = 1
        }
    }
    Button{
       id:callLogRefBtn
       anchors.right:telType.right
       anchors.rightMargin: 20
       anchors.top:parent.top
       anchors.topMargin: 40
       width: 48
       height: 48
       visible: false
       objectName: "callLogRefBtnObject"
       background: Rectangle{
            id:callLogRefBtnBg
            anchors.fill:parent
            color:"transparent"
       }
       Image{
           id:callLogRefBtnIcon
           anchors.fill:parent
           source: "qrc:/images/MediaWidget/Refresh.png"
           transform: Rotation{
               id:rotation
               objectName: "rotationObject"
               origin.x: callLogRefBtnIcon.sourceSize.width/2
               origin.y: callLogRefBtnIcon.sourceSize.height/2
               property bool scanFinish: false
               RotationAnimation on angle{
                   id:animation
                   running: false
                   from: 0
                   to: 360
                   duration: 1000
                   loops: Animation.Infinite
                   objectName: "callLogRefBtnIconAnimationObject"
               }
               onAngleChanged: {
                   if(rotation.scanFinish == true)
                   {
                       if(rotation.angle === 0 || rotation.angle === 360)
                       {
                           animation.running  = false;
                           rotation.scanFinish = false;
                       }
                   }
               }
           }
       }
       onPressed: {
            callLogRefBtnBg.color = "#0DA8FF"
            callLogRefBtn.opacity  = 0.5
       }
       onReleased: {
           callLogRefBtnBg.color = "transparent"
           callLogRefBtn.opacity  = 1
       }
    }

    Button{
        id:phoneBookBtn
        anchors.left:parent.left
        anchors.leftMargin: 520
        anchors.top:callLogBtn.bottom
        anchors.topMargin: 40
        width: 200
        height: 40
        objectName: "phoneBookBtnObject"
        property alias textOpacity: phoneBookText.opacity
        background: Rectangle
        {
            id:phoneBookBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Text{
            id:phoneBookText
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.top:parent.top
            width: 101
            height: 40
            opacity: 0.4
            color:"#FFFFFF"
            font.pixelSize: 24
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text:qsTr("通讯录")
        }
        onPressed: {
            phoneBookBtnBg.color = "#0DA8FF"
            phoneBookBtn.opacity = 0.5
        }
        onReleased: {
            phoneBookBtnBg.color = "transparent"
            phoneBookBtn.opacity = 1
        }
    }

    Button{
       id:phoneBookRefBtn
       anchors.right:telType.right
       anchors.rightMargin: 20
       anchors.top:callLogRefBtn.bottom
       anchors.topMargin: 32
       width: 48
       height: 48
       visible: false
       objectName: "phoneBookRefBtnObject"
       background: Rectangle{
            id:phoneBookRefBtnBg
            anchors.fill:parent
            color:"transparent"
       }
       Image{
           id:phoneBookRefBtnIcon
           anchors.fill:parent
           source: "qrc:/images/MediaWidget/Refresh.png"
           transform: Rotation{
               id:phoneBookRefBtnIconRotation
               objectName: "rotationObject"
               origin.x: phoneBookRefBtnIcon.sourceSize.width/2
               origin.y: phoneBookRefBtnIcon.sourceSize.height/2
               property bool scanFinish: false
               RotationAnimation on angle{
                   id:phoneBookRefBtnIconAnimation
                   running: false
                   from: 0
                   to: 360
                   duration: 1000
                   loops: Animation.Infinite
                   objectName: "phoneBookRefBtnIconAnimationObject"
               }
               onAngleChanged: {
                   if(phoneBookRefBtnIconRotation.scanFinish == true)
                   {
                       if(phoneBookRefBtnIconRotation.angle === 0 || phoneBookRefBtnIconRotation.angle === 360)
                       {
                           phoneBookRefBtnIconAnimation.running  = false;
                           phoneBookRefBtnIconRotation.scanFinish = false;
                       }
                   }
               }
           }
       }
       onPressed: {
           phoneBookRefBtnBg.color = "#0DA8FF"
           phoneBookRefBtn.opacity = 0.5
       }
       onReleased: {
           phoneBookRefBtnBg.color = "transparent"
           phoneBookRefBtn.opacity = 1
       }
    }


    onTelTypeIndexChanged : {
        switch(root.telTypeIndex)
        {
            case 0:
                telephoneCallLogWidget.visible     = false;
                telephoneAddressBookWidget.visible = false;
                btNotConnectWidget.visible = true;
                callLogBtn.textOpacity = 0.4;
                phoneBookBtn.textOpacity = 0.4;
                callLogRefBtn.visible   = false;
                phoneBookRefBtn.visible = false;
                break;
             case 1:
                 telephoneCallLogWidget.visible     = true;
                 telephoneAddressBookWidget.visible = false;
                 btNotConnectWidget.visible = false;
                 callLogBtn.textOpacity = 1;
                 phoneBookBtn.textOpacity = 0.4;
                 callLogRefBtn.visible   = true;
                 phoneBookRefBtn.visible = false;
                 break;
             case 2:
                 telephoneCallLogWidget.visible     = false;
                 telephoneAddressBookWidget.visible = true;
                 btNotConnectWidget.visible = false;
                 callLogBtn.textOpacity = 0.4;
                 phoneBookBtn.textOpacity = 1;
                 callLogRefBtn.visible   = false;
                 phoneBookRefBtn.visible = true;
                 break;
             default:
                 break;
        }
    }

    TelephoneDialerWidget{
        id:telephoneDialerWidget
        x:0
        y:0
        visible: true
        objectName: "dialerWidgetObject"
    }
    QmlTelephoneCommingWidget{
        id:inCommingWidget
        x:0
        y:0
        visible: false
        objectName: "inCommingWidgetObject"
    }
    QmlTelephoneOnCallWidget{
        id:onCallWidget
        x:0
        y:0
        visible: false
        objectName: "onCallWidgetObject"
    }
    QmlTelephoneBtNotConnect{
        id:btNotConnectWidget
        x:880
        y:0
        visible: true
        objectName:"btNotConnectObject"
    }
    QmlTelephoneAddressBookWidget{
        id:telephoneAddressBookWidget
        x:880
        y:0
        visible: false
        objectName:"telephoneAddressBookWidgetObject"
        onListViewItemClicked: {
            root.rootPhoneNumber = telephoneAddressBookWidget.rootPhoneNumber;
            root.listviewItemClicked();
        }
    }

    TelephoneCallLogWidget{
        id:telephoneCallLogWidget
        x:880
        y:0
        visible: false
        objectName:"telephoneCallLogWidgetObject"
        onListViewItemClicked: {
            root.rootPhoneNumber = telephoneCallLogWidget.phoneNumber;
            root.listviewItemClicked();
        }
    }

    onBtConnectStatusChanged: {
        switch(root.btConnectStatus){
            case 0:
                telephoneDialerWidget.visible = true;
                inCommingWidget.visible = false;
                onCallWidget.visible = false;
                break;
            case 5:
                telephoneDialerWidget.visible = false;
                inCommingWidget.visible = true;
                onCallWidget.visible = false;
                break;
            case 6:
                telephoneDialerWidget.visible = false;
                inCommingWidget.visible = false;
                onCallWidget.visible = true;
                break;
             default:
                 telephoneDialerWidget.visible = true;
                 inCommingWidget.visible = false;
                 onCallWidget.visible = false;
                 break;
        }
    }
    onListviewItemClicked: {
        telephoneDialerWidget.listViewItemClicked(root.rootPhoneNumber);
    }
}
