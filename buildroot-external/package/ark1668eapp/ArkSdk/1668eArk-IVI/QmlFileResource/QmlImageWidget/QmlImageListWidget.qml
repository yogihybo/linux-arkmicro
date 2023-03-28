import QtQuick 2.0
import QtQuick.Controls 2.0
import com.test.model 1.0
Item {
    id:root
    width: 520
    height: 638
    visible: false
    property int  lastIndex: -1
    property int  listViewCurrentIndex: -1
    property int  pressIndex: -1
    property bool itemClicked: false
    signal  imageListviewItemClicked(int type,int index)
    ListView{
        id:listView
        anchors.left:parent.left
        anchors.leftMargin: 20
        anchors.top:parent.top
        anchors.topMargin: 82
        model:myUsbImageModelData.getObjectModel()
        width: 480
        height: 638
        clip: true
        focus: true
        interactive: true
        objectName: "listviewObject"
        delegate:Rectangle{
            id:delegateRect
            width: 480
            height: 88
            color:"transparent"
            opacity:  1
            radius: 12
            property alias delegateRectColor:  delegateRect.color
            property alias delegateRectOpacity: delegateRect.opacity
            property alias titleColor:title.color
            Text{
                id:title
                anchors.left: delegateRect.left
                anchors.leftMargin: 20
                anchors.top:delegateRect.top
                anchors.topMargin: 22
                width: 253
                height: 40
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data
            }
            MouseArea{
                anchors.fill:parent
                onPressed: {
                    if(listView.currentIndex !== index)
                    {
                        delegateRect.color = "#0DA8FF"
                        delegateRect.opacity = 0.4
                        root.pressIndex = index
                        root.itemClicked = false
                    }
                }
                onReleased: {
                    if(listView.currentIndex !== index)
                    {
                        delegateRect.color   = "transparent"
                        delegateRect.opacity = 1
                    }
                }
                onClicked: {
                    listView.currentIndex     = index
                    root.listViewCurrentIndex = listView.currentIndex
                    root.imageListviewItemClicked(1,index)
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
                        listView.itemAtIndex(root.pressIndex).delegateRectColor = "transparent"
                        listView.itemAtIndex(root.pressIndex).delegateRectOpacity = 1
                        listView.itemAtIndex(root.pressIndex).titleColor = "#FFFFFF"
                    }
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
        listView.itemAtIndex(listView.currentIndex).delegateRectColor = "#FFFFFF"
        listView.itemAtIndex(listView.currentIndex).delegateRectOpacity = 0.4
        listView.itemAtIndex(listView.currentIndex).titleColor = "#0DA8FF"
        if(root.lastIndex >= 0)
        {
            if(listView.itemAtIndex(root.lastIndex))
            {
                listView.itemAtIndex(root.lastIndex).delegateRectColor = "transparent"
                listView.itemAtIndex(root.lastIndex).delegateRectOpacity = 1
                listView.itemAtIndex(root.lastIndex).titleColor = "#FFFFFF"
            }

        }
        root.lastIndex = listView.currentIndex
    }
}
