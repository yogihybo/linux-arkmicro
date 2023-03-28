import QtQuick 2.0
import "./QmlMultiMediaPlayWidget"
import "./QmlUsbWidget"
import "./QmlSdWidget "
import "./QmlBtWidget"
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int  mutilMediaIndex: 0
    property int  lastMutilMediaIndex: -1
    property int  mutilMediaType: -1
    property int  usbWidgetType: 0
    property int  sdWidgetType: 0
    property int  btWidgetType: 0
    signal musicTypeChanged
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color:"#000000"
    }
    //中间类型选择列表
    Rectangle{
        id:mutilMediaRect
        x:520
        y:0
        width: 360
        height: 720
        color:"#161616"
        Column{
            anchors.left:mutilMediaRect.left
            anchors.top:mutilMediaRect.top
            anchors.topMargin: 44
            spacing: 40
            Repeater{
                id:mutilMediaReapter
                model: ListModel{
                    ListElement{name:"USB"}
                    ListElement{name:QT_TR_NOOP("SD卡")}
                    ListElement{name:QT_TR_NOOP("蓝牙")}
                    ListElement{name:QT_TR_NOOP("收音")}
                }
                Rectangle{
                    id:deltgateRect
                    width: 360
                    height: 40
                    color: "transparent"
                    property alias mutilMediaNameOpacity : mutilMediaName.opacity
                    //property alias refreshVisibel : refresh.visible
                    Text{
                        id:mutilMediaName
                        anchors.left: deltgateRect.left
                        anchors.leftMargin: 40
                        anchors.top:deltgateRect.top
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
//                    Rectangle{
//                        id:refresh
//                        anchors.left: deltgateRect.left
//                        anchors.leftMargin: 292
//                        anchors.top:deltgateRect.top
//                        width: 48
//                        height: 48
//                        color: "transparent"
//                        visible: false
//                        Image{
//                            id:refreshImage
//                            anchors.fill:parent
//                            source: "qrc:/images/MediaWidget/Refresh.png"
//                        }
//                        MouseArea{
//                            anchors.fill:parent
//                            onPressed:  refreshImage.source = "qrc:/images/MediaWidget/RefreshPress.png"
//                            onReleased: refreshImage.source = "qrc:/images/MediaWidget/Refresh.png"
//                        }
//                    }
                    MouseArea{
                        anchors.left:deltgateRect.left
                        anchors.top:deltgateRect.top
                        width: 292
                        height: 40
                        onPressed: {
                            deltgateRect.color = "#0DA8FF"
                            deltgateRect.opacity = 0.5
                        }
                        onReleased: {
                            deltgateRect.color = "transparent"
                            deltgateRect.opacity = 1
                        }
                        onClicked: {
                            root.mutilMediaIndex = index;
                            if(root.lastMutilMediaIndex != root.mutilMediaIndex)
                            {
                                //console.log("++++++root.lastMutilMediaIndex++++++++",root.lastMutilMediaIndex)
                                root.musicTypeChanged();
                            }
                            root.lastMutilMediaIndex = index;
                        }
                    }
                    Component.onCompleted:{
                        if(root.mutilMediaIndex === index)
                        {
                           mutilMediaName.opacity = 1
                          // refresh.visible = true;
                        }
                   }
                }
            }
        }
    }
    onMutilMediaIndexChanged: {
        for(var i = 0;i<4;i++)
        {
            //console.log("++++++++++root.mutilMediaIndex+++++",root.mutilMediaIndex);
            if(i === root.mutilMediaIndex)
            {
                mutilMediaReapter.itemAt(i).mutilMediaNameOpacity = 1;
                //mutilMediaReapter.itemAt(i).refreshVisibel = true;
            }
            else
            {
                mutilMediaReapter.itemAt(i).mutilMediaNameOpacity = 0.4;
                //mutilMediaReapter.itemAt(i).refreshVisibel =false;
            }
        }
//        console.log("++++[mutilMediaWidget.qml:mutilMediaIndex]+++++",mutilMediaIndex);
//        console.log("++++[mutilMediaWidget.qml:usbWidgetType]+++++",usbWidgetType);
        switch(root.mutilMediaIndex){
            case 0:
                if(root.usbWidgetType === 0)
                {
                    usbNotConnectWidget.visible = true;
                    usbScanWidget.visible = false;
                    usbListWidget.visible = false;
                    sdNotConnectWidget.visible = false;
                    sdScanWidget.visible = false;
                    sdListWidget.visible = false;
                    btNotConnect.visible =false;
                    btMusciListWidget.visible = false;
                }
                else if(root.usbWidgetType === 1)
                {
                    usbNotConnectWidget.visible = false;
                    usbScanWidget.visible = true;
                    usbListWidget.visible = false;
                    sdNotConnectWidget.visible = false;
                    sdScanWidget.visible = false;
                    sdListWidget.visible = false;
                    btNotConnect.visible =false;
                    btMusciListWidget.visible = false;
                }
                else if(root.usbWidgetType === 2)
                {
                    usbNotConnectWidget.visible = false;
                    usbScanWidget.visible = false;
                    usbListWidget.visible = true;
                    sdNotConnectWidget.visible = false;
                    sdScanWidget.visible = false;
                    sdListWidget.visible = false;
                    btNotConnect.visible =false;
                    btMusciListWidget.visible = false;
                }
                break;
             case 1:
                 if(root.sdWidgetType === 0)
                 {
                     usbNotConnectWidget.visible = false;
                     usbScanWidget.visible = false;
                     usbListWidget.visible = false;
                     sdNotConnectWidget.visible = true;
                     sdScanWidget.visible = false;
                     sdListWidget.visible = false;
                     btNotConnect.visible =false;
                     btMusciListWidget.visible = false;
                 }
                 else if(root.sdWidgetType === 1)
                 {
                     usbNotConnectWidget.visible = false;
                     usbScanWidget.visible = false;
                     usbListWidget.visible = false;
                     sdNotConnectWidget.visible = false;
                     sdScanWidget.visible = true;
                     sdListWidget.visible = false;
                     btNotConnect.visible =false;
                     btMusciListWidget.visible = false;
                 }
                 else if(root.sdWidgetType === 2)
                 {
                     usbNotConnectWidget.visible = false;
                     usbScanWidget.visible = false;
                     usbListWidget.visible = false;
                     sdNotConnectWidget.visible = false;
                     sdScanWidget.visible = false;
                     sdListWidget.visible = true;
                     btNotConnect.visible =false;
                     btMusciListWidget.visible = false;
                 }
                 break;
            case 2:
                if(root.btWidgetType === 0)
                {
                    usbNotConnectWidget.visible = false;
                    usbScanWidget.visible = false;
                    usbListWidget.visible = false;
                    sdNotConnectWidget.visible = false;
                    sdScanWidget.visible = false;
                    sdListWidget.visible = false;
                    btNotConnect.visible = true;
                    btMusciListWidget.visible = false;
                }
                if(root.btWidgetType === 1)
                {
                    usbNotConnectWidget.visible = false;
                    usbScanWidget.visible = false;
                    usbListWidget.visible = false;
                    sdNotConnectWidget.visible = false;
                    sdScanWidget.visible = false;
                    sdListWidget.visible = false;
                    btNotConnect.visible = false;
                    btMusciListWidget.visible = true;
                }
                break;
             default:
                 break;
        }
    }
    //多媒体(USB音乐 SD卡音乐 BT音乐)界面
    MultiMediaPlayWidget{
        id:multiMediaPlayWidget
        x:0
        y:0
        objectName: "multiMediaPlayWidgetObject"
    }
    //Usb未连接界面 
    UsbNotConnectWidget{
        id:usbNotConnectWidget
        x:880
        y:0
        visible: true
        objectName: "usbNotConnectWidgetObject"
    }

    UsbScanWidget{
        id:usbScanWidget
        x:880
        y:0
        visible: false
        objectName:"usbScanWidgetObject"
    }

    UsbListWidget{
        id:usbListWidget
        x:880
        y:0
        visible: false
        objectName:"usbListWidgetObject"
    }

    SdNotConnectWidget{
        id:sdNotConnectWidget
        x:880
        y:0
        visible: false
        objectName:"sdNotConnectWidgetObject"
    }

    SdScanWidget{
        id:sdScanWidget
        x:880
        y:0
        visible: false
        objectName:"sdScanWidgetObject"
    }
    SdListWidget{
        id:sdListWidget
        x:880
        y:0
        visible: false
        objectName:"sdListWidgetObject"
    }

    BluetoothNotConnect{
        id:btNotConnect
        x:880
        y:0
        visible: false
    }
    BluetoothMusicListWidget{
        id:btMusciListWidget
        x:880
        y:0
        visible: false
        objectName: "btMusciListWidgetObject"
    }
    onMutilMediaTypeChanged: {
        widgetChanged();
    }
    function widgetChanged(){
        switch(root.mutilMediaType){
            case 0:
                root.mutilMediaIndex = 0;
                root.usbWidgetType   = 0;
                usbNotConnectWidget.visible = true;
                usbScanWidget.visible = false;
                usbListWidget.visible = false;
                sdNotConnectWidget.visible = false;
                sdScanWidget.visible = false;
                sdListWidget.visible = false;
                btNotConnect.visible =false;
                btMusciListWidget.visible = false;
                break;
             case 1:
                 root.mutilMediaIndex = 0;
                 root.usbWidgetType   = 1;
                 usbNotConnectWidget.visible = false;
                 usbScanWidget.visible = true;
                 usbListWidget.visible = false;
                 sdNotConnectWidget.visible = false;
                 sdScanWidget.visible = false;
                 sdListWidget.visible = false;
                 btNotConnect.visible =false;
                 btMusciListWidget.visible = false;
                 break;
              case 2:
                  root.mutilMediaIndex = 0;
                  root.usbWidgetType   = 2;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = true;
                  sdNotConnectWidget.visible = false;
                  sdScanWidget.visible = false;
                  sdListWidget.visible = false;
                  btNotConnect.visible =false;
                  btMusciListWidget.visible = false;
                  break;
              case 3:
                  root.mutilMediaIndex = 1;
                  root.sdWidgetType   = 0;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = false;
                  sdNotConnectWidget.visible = true;
                  sdScanWidget.visible = false;
                  sdListWidget.visible = false;
                  btNotConnect.visible =false;
                  btMusciListWidget.visible = false;
                  break;
              case 4:
                  root.mutilMediaIndex = 1;
                  root.sdWidgetType   = 1;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = false;
                  sdNotConnectWidget.visible = false;
                  sdScanWidget.visible = true;
                  sdListWidget.visible = false;
                  btNotConnect.visible =false;
                  btMusciListWidget.visible = false;
                  break;
              case 5:
                  root.mutilMediaIndex = 1;
                  root.sdWidgetType   = 2;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = false;
                  sdNotConnectWidget.visible = false;
                  sdScanWidget.visible = false;
                  sdListWidget.visible = true;
                  btNotConnect.visible =false;
                  btMusciListWidget.visible = false;
                  break;
              case 6:
                  root.mutilMediaIndex = 2;
                  root.btWidgetType   = 0;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = false;
                  sdNotConnectWidget.visible = false;
                  sdScanWidget.visible = false;
                  sdListWidget.visible = false;
                  btNotConnect.visible =true;
                  btMusciListWidget.visible = false;
                  break;
              case 7:
                  root.mutilMediaIndex = 2;
                  root.btWidgetType   = 1;
                  usbNotConnectWidget.visible = false;
                  usbScanWidget.visible = false;
                  usbListWidget.visible = false;
                  sdNotConnectWidget.visible = false;
                  sdScanWidget.visible = false;
                  sdListWidget.visible = false;
                  btNotConnect.visible = false;
                  btMusciListWidget.visible = true;
                  break;
              default:
                  break;
        }
    }
    onMusicTypeChanged:
    {
        multiMediaPlayWidget.musicTypeChanged();
    }
}
