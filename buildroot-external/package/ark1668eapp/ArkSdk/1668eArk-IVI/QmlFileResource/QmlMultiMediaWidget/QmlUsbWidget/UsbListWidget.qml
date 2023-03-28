import QtQuick 2.0
import QtQuick.Controls 2.0
import com.usbMusic.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property int  lastIndex: -1
    property int  listViewCurrentIndex: -1
    property int  pressIndex:-1
    property bool itemClicked: false
    signal  usbMusicListviewItemClicked(int type,int index)
    FocusScope {
        id:focusScope
        x:20
        y:40
        width: 800
        height: 60
        Rectangle{
            id:textInputRect
            anchors.fill: parent
            width: 800
            height: 60
            color: "transparent"
            border.color: "#FFFFFF"
            border.width: 2
            opacity: 0.2
            radius: 30
        }
        Image{
            id:search
            anchors.left:textInputRect.left
            anchors.leftMargin: 4
            anchors.top:textInputRect.top
            anchors.topMargin: 6
            width: 48
            height: 48
            source: "qrc:/images/MediaWidget/search.png"

        }
        TextInput {
            id: textInput
            anchors.left:textInputRect.left
            anchors.leftMargin: 70
            anchors.top:textInputRect.top
            anchors.topMargin: 18
            width: 725
            height: 24
            color:"#FFFFFF"
            font.pixelSize: 20
            font.family: "Alibaba PuHuiTi"
            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter
        }
        Text{
            id:placeHold
            anchors.left:textInputRect.left
            anchors.leftMargin: 70
            anchors.top:textInputRect.top
            anchors.topMargin: 18
            width: 725
            height: 24
            font:textInput.font
            color:"#FFFFFF"
            opacity: textInput.length ? 0 : 0.6
            Behavior on opacity
            {
                NumberAnimation{ duration: 300 }
            }
            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter
            text: qsTr("输入关键字")
        }
    }

    ListView{
        id:listView
        model:myUsbMusicListModel.getObjectModel()
        anchors.left:parent.left
        anchors.leftMargin: 35
        anchors.top:parent.top
        anchors.topMargin: 115
        width: 800
        height: 605
        clip: true
        focus: true
        interactive: true
        objectName: "listViewObject"
        delegate:Rectangle{
            id:delegateRect
            width: 800
            height: 88
            color:"transparent"
            opacity: 1
            radius: 12
            property alias hornVisibel:horn.visible
            property alias delegateRectColor:  delegateRect.color
            property alias delegateRectOpacity: delegateRect.opacity
            property alias musicNameText:musicName.text
            Text{
                id:musicName
                anchors.left: delegateRect.left
                anchors.leftMargin: 9
                anchors.top:delegateRect.top
                anchors.topMargin: 10
                width: 791
                height: 40
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data1
            }
            Text{
                anchors.left: delegateRect.left
                anchors.leftMargin: 64
                anchors.top:delegateRect.top
                anchors.topMargin: 46
                width: 208
                height: 27
                opacity: 0.4
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data2
            }
            Rectangle{
                id:horn
                anchors.left: delegateRect.left
                anchors.leftMargin: 732
                anchors.top:delegateRect.top
                anchors.topMargin: 20
                width: 48
                height: 48
                color:"transparent"
                visible: false
                Image{
                    anchors.fill:parent
                    source: "qrc:/images/MediaWidget/horn.png"
                }
            }
            MouseArea{
                anchors.fill:parent
                onPressed: {
                    if(listView.currentIndex != index)
                    {
                        delegateRect.color = "#0DA8FF"
                        delegateRect.opacity = 0.4
                        root.itemClicked = false
                        root.pressIndex = index

                    }

                }
                onReleased: {
                    if(listView.currentIndex != index)
                    {
                        delegateRect.color = "transparent"
                        delegateRect.opacity = 1
                    }

                }
                onClicked: {
                    listView.currentIndex = index
                    root.listViewCurrentIndex = index
                    root.usbMusicListviewItemClicked(1,index)
                    root.itemClicked = true
                }
            }
        }
        onContentYChanged:
        {
            if(root.pressIndex != -1)
            {
                if(root.itemClicked === false)
                {
                    if(listView.itemAtIndex(root.pressIndex))
                    {
                        listView.itemAtIndex(root.pressIndex).hornVisibel = false
                        listView.itemAtIndex(root.pressIndex).delegateRectColor = "transparent"
                        listView.itemAtIndex(root.pressIndex).delegateRectOpacity = 1
                    }
                }
            }
            if(listView.contentY < 0)
            {
                listView.contentY = 0
            }
            root.pressIndex = -1
        }

    }
    onListViewCurrentIndexChanged: {
        listView.itemAtIndex(listView.currentIndex).hornVisibel = true
        listView.itemAtIndex(listView.currentIndex).delegateRectColor = "#FFFFFF"
        listView.itemAtIndex(listView.currentIndex).delegateRectOpacity = 0.4
        if(root.lastIndex != -1)
        {
            if(listView.itemAtIndex(root.lastIndex))
            {
                listView.itemAtIndex(root.lastIndex).hornVisibel = false
                listView.itemAtIndex(root.lastIndex).delegateRectColor = "transparent"
                listView.itemAtIndex(root.lastIndex).delegateRectOpacity = 1
            }
        }
        root.lastIndex = listView.currentIndex

        console.log("+++++++++Music Usb Listview count++++++++++++",listView.count);
        if(listView.count > 0)
        {
            console.log("+++++++++Music Usb Listview first music name++++++++++++",listView.itemAtIndex(0).musicNameText);
        }
    }

}
