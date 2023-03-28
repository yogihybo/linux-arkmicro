import QtQuick 2.0
import QtQuick.Controls 2.0
import com.test.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property int  lastIndex: -1
    property int  listViewCurrentIndex: -1
    property int  pressIndex: -1
    property bool itemClicked: false
    property int  wifiConnectStatus: -1
    property int wifiPowerStatus: 0
    signal  listviewItemClicked(int index)
    Text{
        id:wifiTitle
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
        text:qsTr("WIFI")
    }

    Text{
        id:wifiSwitchName
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
        text:qsTr("开启WIFI")
    }

    Button{
       id:wifiPowerBtn
       anchors.left: parent.left
       anchors.leftMargin: 686
       anchors.top:parent.top
       anchors.topMargin: 114
       width: 120
       height: 60
       objectName: "wifiPowerBtnObject"
       background: Rectangle{
            id:wifiPowerBtnBg
            anchors.fill:parent
            color:"transparent"
       }
       Image{
           id:wifiPowerBtnIcon
           anchors.fill:parent
           source: "qrc:/images/SettingWidget/CloseSwitch.png"
       }
       onPressed:{
            wifiPowerBtn.opacity = 0.4
       }
       onReleased: {
           wifiPowerBtn.opacity = 1
       }

    }
    onWifiPowerStatusChanged: {
         switch(root.wifiPowerStatus){
             case 0:
                 wifiPowerBtnIcon.source = "qrc:/images/SettingWidget/CloseSwitch.png";
                 nearbyWifi.visible  = false;
                 wifiScanBtn.visible = false;
                 listView.visible    = false;
                 wifiConnectedBtn.visible = false;
                 break;
             case 1:
                 wifiPowerBtnIcon.source = "qrc:/images/SettingWidget/OpenSwitch.png";
                 nearbyWifi.visible  = true;
                 wifiScanBtn.visible = true;
                 listView.visible    = true;
                 break;
             default:
                 break;
         }
    }

    Text{
        id:nearbyWifi
        visible:  false
        anchors.left: parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 238
        width: 197
        height: 40
        color:"#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("选取附近的WIFI")
    }

    Button{
        id:wifiScanBtn
        visible:  false
        anchors.left: parent.left
        anchors.leftMargin: 752
        anchors.top:parent.top
        anchors.topMargin: 234
        width: 48
        height: 48
        objectName: "wifiScanBtnObject"
        background: Rectangle{
            id:wifiScanBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:wifiScanBtnIcon
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/Refresh.png"
            transform: Rotation{
                id:rotation
                objectName: "rotationObject"
                origin.x: wifiScanBtnIcon.sourceSize.width/2
                origin.y: wifiScanBtnIcon.sourceSize.height/2
                property bool scanFinish: false
                RotationAnimation on angle{
                    id:animation
                    running: false
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                    objectName: "wifiScanBtnIconRotationAnimationObject"
                }
                onAngleChanged: {
                    if(rotation.scanFinish === true)
                    {
                        animation.running  = false;
                        rotation.scanFinish = false;
                    }
                }
            }
        }
        onPressed: {
            wifiScanBtnBg.color = "#0DA8FF"
            wifiScanBtn.opacity = 0.5
        }
        onReleased: {
            wifiScanBtnBg.color = "transparent"
            wifiScanBtn.opacity = 1
        }
    }

    ListView{
        id:listView
        visible:  false
        anchors.left:parent.left
        anchors.leftMargin: 70
        anchors.top:parent.top
        anchors.topMargin: 319
        width: 736
        height: 400
        clip: true
        focus: true
        interactive: true
        objectName: "listViewObject"
        model:myWifiListModelData.objectModel()
        delegate: Button{
            id:delegateBtn
            width: 736
            height: 88
            property alias delegateRectColor:   delegateBgRect.color
            property alias delegateRectOpacity: delegateBtn.opacity
            background: Rectangle{
                id:delegateBgRect
                anchors.fill:parent
                color:"transparent"
            }
            Text{
                id:delegateBtnText
                anchors.left: parent.left
                anchors.top:parent.top
                anchors.topMargin: 22
                width: 736
                height: 40
                color:"#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data
            }
            onPressed: {
                delegateBgRect.color = "#0DA8FF"
                delegateBtn.opacity = 0.4
                root.pressIndex = index
                root.itemClicked = false
            }
            onReleased: {
                delegateBgRect.color   = "transparent"
                delegateBtn.opacity = 1
            }
            onClicked: {
                listView.currentIndex     = index
                root.listViewCurrentIndex = listView.currentIndex
                root.listviewItemClicked(index)
                root.itemClicked = true
            }
        }
        onContentYChanged:
        {
            if(root.pressIndex != -1)
            {

                if(listView.itemAtIndex(root.pressIndex))
                {
                    listView.itemAtIndex(root.pressIndex).delegateRectColor = "transparent"
                    listView.itemAtIndex(root.pressIndex).delegateRectOpacity = 1
                }

            }

            if(listView.contentY < 0)
            {
                listView.contentY = 0
            }
            root.pressIndex = -1;
        }
    }
    onListViewCurrentIndexChanged: {
        if(root.lastIndex >= 0)
        {
            if(listView.itemAtIndex(root.lastIndex))
            {
                listView.itemAtIndex(root.lastIndex).delegateRectColor = "transparent"
                listView.itemAtIndex(root.lastIndex).delegateRectOpacity = 1
            }
        }
        root.lastIndex = listView.currentIndex
    }

    Button{
        id:wifiConnectedBtn
        visible:  false
        anchors.left: parent.left
        anchors.leftMargin: 35
        anchors.top:parent.top
        anchors.topMargin: 212
        width: 800
        height: 88
        objectName: "wifiConnectedBtnObject"
        background: Rectangle{
            id:wifiConnectedBtnBg
            anchors.fill:parent
            radius: 12
            color:"#FFFFFF"
            opacity: 0.4
        }
        Text{
            id:wifiConnectName
            anchors.left: parent.left
            anchors.leftMargin: 35
            anchors.top:parent.top
            anchors.topMargin: 22
            width: 736
            height: 40
            color:"#FFFFFF"
            font.pixelSize: 28
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            objectName: "wifiConnectedBtnTextObject"
        }
        onPressed: wifiConnectedBtnBg.opacity = 0.8
        onReleased:wifiConnectedBtnBg.opacity = 0.4
    }
    Text{
        id:connectText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: 200
        height: 40
        color: "#FFFFFF"
        opacity: 1
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
    Timer{
        id:time
        interval :20000
        repeat : false
        running :false
        triggeredOnStart :false
        onTriggered: {
            connectText.visible = false;
            if(root.wifiConnectStatus === 1)
            {
                nearbyWifi.anchors.topMargin = 346;
                wifiScanBtn.anchors.topMargin = 342;
                listView.anchors.topMargin = 427;
                wifiConnectedBtn.visible = true;
            }
            else
            {
                nearbyWifi.anchors.topMargin = 238;
                wifiScanBtn.anchors.topMargin = 234;
                listView.anchors.topMargin = 319;
                wifiConnectedBtn.visible = false;
            }
        }
    }

    Timer{
        id:id_time
        interval :500
        repeat : false
        running :false
        triggeredOnStart :false
        onTriggered: {
            connectText.visible = false;
            if(root.wifiConnectStatus === 1)
            {
                nearbyWifi.anchors.topMargin = 346;
                wifiScanBtn.anchors.topMargin = 342;
                listView.anchors.topMargin = 427;
                wifiConnectedBtn.visible = true;
            }
            else
            {
                nearbyWifi.anchors.topMargin = 238;
                wifiScanBtn.anchors.topMargin = 234;
                listView.anchors.topMargin = 319;
                wifiConnectedBtn.visible = false;
            }
        }
    }

    WifiSettingPasswordWidget{
        x:0
        y:0
        visible: false
        objectName: "passwordWidgetObject"
    }
    onWifiConnectStatusChanged: {
        console.log("==========root.wifiConnectStatus=========",root.wifiConnectStatus);
        switch(root.wifiConnectStatus)
        {
            case 0:
                connectText.text = qsTr("正在连接,请稍后...");
                connectText.visible = true;
                time.restart();
                break;
            case 1:
                connectText.text = qsTr("连接成功！");
                connectText.visible = true;
                id_time.restart();
                break;
            case 2:
                connectText.text = qsTr("连接失败！");
                connectText.visible = true;
                id_time.restart();
                break;
            case 3:
                connectText.text = qsTr("断开连接！");
                connectText.visible = true;
                id_time.restart();
                break;
            case 4:
                connectText.text = qsTr("密码错误！");
                connectText.visible = true;
                id_time.restart();
                break;
        }
    }


}
