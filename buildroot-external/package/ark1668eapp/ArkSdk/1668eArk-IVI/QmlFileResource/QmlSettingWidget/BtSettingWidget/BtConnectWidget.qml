import QtQuick 2.0
import QtQuick.Controls 2.0
import com.test.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: false
    signal prevPageBtnClicked
    property int  lastIndex: -1
    property int  listViewCurrentIndex: -1
    property int  pressIndex: -1
    property bool itemClicked: false
    property int  btConnectStatus: 0
    signal  listviewItemClicked(int index)
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
            prevPageBtnBg.color = "#0DA8FF"
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
        objectName: "btTitleObject"
        text:qsTr("")
    }

    Text{
        id:btSwitchName
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
        text:qsTr("开启蓝牙")
    }

    Button{
       id:btPowerBtn
       anchors.left: parent.left
       anchors.leftMargin: 686
       anchors.top:parent.top
       anchors.topMargin: 114
       width: 120
       height: 60
       objectName: "btPowerBtnObject"
       property int btPowerStatus: 0
       background: Rectangle{
            id:btPowerBtnBg
            anchors.fill:parent
            color:"transparent"
       }
       Image{
           id:btPowerBtnIcon
           anchors.fill:parent
           source: "qrc:/images/SettingWidget/CloseSwitch.png"
       }
       onPressed:{
            //btPowerBtnBg.color = "#0DA8FF"
            btPowerBtn.opacity = 0.5
       }
       onReleased: {
           //btPowerBtnBg.color = "transparent"
           btPowerBtn.opacity = 1
       }
       onBtPowerStatusChanged: {
            switch(btPowerBtn.btPowerStatus){
                case 0:
                    btPowerBtnIcon.source = "qrc:/images/SettingWidget/CloseSwitch.png";
                    break;
                case 1:
                    btPowerBtnIcon.source = "qrc:/images/SettingWidget/OpenSwitch.png";
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
        anchors.top:btPowerBtn.bottom
        anchors.topMargin: 10
        width: 880
        height: 30
        opacity: 1
        color: "#FFFFFF"
        font.pixelSize: 24
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:"Carplay is connected, please disconnect it first"
        objectName: "msgTextObject"
    }

    Text{
        id:availableDevices
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
        text:qsTr("可用设备")
    }
    Button{
        id:btScanBtn
        anchors.left: parent.left
        anchors.leftMargin: 752
        anchors.top:parent.top
        anchors.topMargin: 234
        width: 48
        height: 48
        objectName: "btScanBtnObject"
        background: Rectangle{
            id:btScanBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:btScanBtnIcon
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/Refresh.png"
            transform: Rotation{
                id:rotation
                objectName: "rotationObject"
                origin.x: btScanBtnIcon.sourceSize.width/2
                origin.y: btScanBtnIcon.sourceSize.height/2
                property bool scanFinish: false
                RotationAnimation on angle{
                    id:animation
                    running: false
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                    objectName: "btScanBtnIconRotationAnimationObject"
                }
                onAngleChanged: {
                    if(rotation.scanFinish == true)
                    {
                        animation.running  = false;
                        rotation.scanFinish = false;
                    }
                }
            }
        }
        onPressed: {
            btScanBtnBg.color = "#0DA8FF"
            btScanBtn.opacity = 0.5
        }
        onReleased: {
            btScanBtnBg.color = "transparent"
            btScanBtn.opacity = 1
        }
    }

    ListView{
        id:listView
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
        model:myBtNameModelData.getObjectModel()
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
        id:btConnectedBtn
        anchors.left: parent.left
        anchors.leftMargin: 35
        anchors.top:parent.top
        anchors.topMargin: 212
        width: 800
        height: 88
        visible: false
        objectName: "btConnectedBtnObject"
        background: Rectangle{
            id:btConnectedBtnBg
            anchors.fill:parent
            radius: 12
            color:"#FFFFFF"
            opacity: 0.4
        }
        Text{
            id:btConnectName
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
            objectName: "btConnectedBtnTextObject"
        }
        onPressed: {
            btConnectedBtnBg.color = "#0DA8FF"
        }
        onReleased: {
            btConnectedBtnBg.color = "#FFFFFF"
        }
    }
    onBtConnectStatusChanged: {
        if(root.btConnectStatus >= 3)
        {
            availableDevices.anchors.topMargin = 346;
            btScanBtn.anchors.topMargin = 342;
            listView.anchors.topMargin = 427;
            btConnectedBtn.visible = true;
        }
        else
        {
            availableDevices.anchors.topMargin = 238;
            btScanBtn.anchors.topMargin = 234;
            listView.anchors.topMargin = 319;
            btConnectedBtn.visible = false;
        }
    }
}
