import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int  msgWidgetVisible: 0
    property int  phoneLinkStatus: 0//在mainWidget.qml中使用
    property int  phoneLinkType: -1
    Rectangle{
        anchors.fill:parent
        color:"#161616"
    }
    CarLifeCarPlayWidget{
        id:carLifeCarPlayWidget
        anchors.fill:parent
        visible: false
        objectName: "CarLifeCarPlayWidgetObject"
    }
    AutoCarplayWidget{
        id:autoCarplayWidget
        anchors.fill:parent
        visible: false
        objectName: "autoCarplayWidgetObject"
    }
    EcLinkWidget{
        id:ecLinkWidget
        anchors.fill:parent
        visible: false
        objectName: "ecLinkWidgetObject"
    }
    HiCarWidget{
        id:hiCarWidget
        anchors.fill:parent
        visible: false
        objectName: "hiCarWidgetObject"
    }
    Timer{
        id:id_timer
        interval:30000
        repeat: false
        running: false
        triggeredOnStart: false
        onTriggered: {
	    console.log("++++msgWidgetVisible1111++++++");
            id_msg.showType = 0
            root.msgWidgetVisible = 0
            id_msg.visible = false
            id_timer.stop()
        }
        objectName: "timerObject"
    }

    PhoneLinkMsgWidget{
        id:id_msg
        anchors.centerIn: parent
        visible: false
        objectName: "PhoneLinkMsgWidgetObject"
        onVisibleChanged: {
            if(id_msg.visible === false)
            {
                id_msg.showType = 0
                root.msgWidgetVisible = 0
                id_msg.visible = false
                id_timer.stop()
            }
        }
    }
    PhoneLinkMsgShowWidget{
        id:id_msgShow
        anchors.centerIn: parent
        visible: false
        objectName: "phoneLinkMsgShowWidgetObject"
    }
    onMsgWidgetVisibleChanged: {
        console.log("+++++++root.msgWidgetVisible+++++++++",root.msgWidgetVisible);
        switch(root.msgWidgetVisible){
            case 1:
                id_msg.visible = true;
                id_timer.restart();
                break;
            case 0:
                id_msg.visible = false;
                id_msgShow.visible = false;
                id_timer.stop();
                break;
            default:
                break;
        }
    }
    onPhoneLinkTypeChanged: {
        switch(root.phoneLinkType)
        {
            case 0:
                carLifeCarPlayWidget.visible = true;
                autoCarplayWidget.visible = false;
                ecLinkWidget.visible = false;
                hiCarWidget.visible = false;
                break;
            case 1:
                carLifeCarPlayWidget.visible = false;
                autoCarplayWidget.visible = true;
                ecLinkWidget.visible = false;
                hiCarWidget.visible = false;
                break;
            case 2:
                ecLinkWidget.visible = true;
                hiCarWidget.visible = false;
                carLifeCarPlayWidget.visible = false;
                autoCarplayWidget.visible = false;
                break;
            case 3:
                hiCarWidget.visible = true;
                ecLinkWidget.visible = false;
                carLifeCarPlayWidget.visible = false;
                autoCarplayWidget.visible = false;
                break;
            default:
                break;
        }
    }
}
