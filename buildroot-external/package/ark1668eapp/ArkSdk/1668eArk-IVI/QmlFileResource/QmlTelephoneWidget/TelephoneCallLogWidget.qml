import QtQuick 2.0
import QtQuick.Controls 2.0
import com.AllCall.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property int lastIndex: -1
    property int  pressIndex: -1
    property string phoneNumber: ""
    signal  listViewItemClicked()
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
            text: qsTr("搜索联系人")
        }
    }

    ListView{
        id:listView
        anchors.left:parent.left
        anchors.leftMargin: 35
        anchors.top:parent.top
        anchors.topMargin: 115
        width: 800
        height: 605
        clip: true
        focus: true
        interactive: true
        model: myAllCallLogModelData.getObjectModel()
        property bool itemClicked: false
        property int listCurrentIndex: -1
        delegate:Button{
            id:delegateBtn
            width: 800
            height: 88
            property alias delegateBtnBgColor: delegateBtnBg.color
            property alias delegateBtnBgOpacity: delegateBtnBg.opacity
            background: Rectangle{
                id:delegateBtnBg
                anchors.fill:parent
                color:"transparent"
            }
            Text{
                id:phoneName
                anchors.left: parent.left
                anchors.leftMargin: 19
                anchors.top:parent.top
                anchors.topMargin: 10
                width: 500
                height: 40
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data2
                elide:Text.ElideRight
            }
            Image{
                id:phoneTypeIcon
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.top:parent.top
                anchors.topMargin: 12
                width: 36
                height: 36
            }
            Text{
                id:phoneNumber
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.top:parent.top
                anchors.topMargin: 52
                width: 500
                height: 27
                opacity: 0.4
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:model.data3
                elide:Text.ElideRight
            }
            Text{
                id:dateTime
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.top:parent.top
                anchors.topMargin: 52
                width: 240
                height: 27
                opacity: 0.4
                color:"#FFFFFF"
                font.pixelSize: 20
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text:model.data4 + " " + model.data5
                elide:Text.ElideRight
            }

            Component.onCompleted: {
                if(String(model.data1)==="2")
                {
                    phoneTypeIcon.source = "qrc:/images/TelephoneWidget/Incoming.png"
                    phoneName.color   = "#FFFFFF"
                    phoneNumber.color = "#FFFFFF"
                    dateTime.color    = "#FFFFFF"
                }
                else if(String(model.data1)==="3"){
                    phoneTypeIcon.source = "qrc:/images/TelephoneWidget/Outcoming.png"
                    phoneName.color   = "#FFFFFF"
                    phoneNumber.color = "#FFFFFF"
                    dateTime.color    = "#FFFFFF"
                }
                else if(String(model.data1)==="4"){
                    phoneTypeIcon.source = "qrc:/images/TelephoneWidget/HangUp.png"
                    phoneName.color   = "#FF0000"
                    phoneNumber.color = "#FF0000"
                    dateTime.color    = "#FF0000"
                }
            }

            onPressed: {
                if(listView.currentIndex != index)
                {
                    delegateBtnBg.color = "#0DA8FF"
                    delegateBtnBg.opacity = 0.4
                    root.pressIndex = index
                    listView.itemClicked = false
                }
            }
            onReleased: {
                if(listView.currentIndex != index)
                {
                    delegateBtnBg.color = "transparent"
                    delegateBtnBg.opacity = 1
                }
            }
            onClicked: {
                listView.currentIndex = index
                root.phoneNumber = phoneNumber.text
                root.listViewItemClicked()
                listView.itemClicked = true
                listView.listCurrentIndex = index
            }
        }
        onContentYChanged:
        {
            if(root.pressIndex != -1)
            {
                if(listView.itemClicked === false){
                    if(listView.itemAtIndex(root.pressIndex))
                    {
                        listView.itemAtIndex(root.pressIndex).delegateBtnBgColor = "transparent"
                        listView.itemAtIndex(root.pressIndex).delegateBtnBgOpacity = 1
                    }
                }
            }

            if(listView.contentY < 0)
            {
                listView.contentY = 0
            }
            root.pressIndex = -1;
        }
        onListCurrentIndexChanged: {
            listView.itemAtIndex(listView.currentIndex).delegateBtnBgColor = "#FFFFFF"
            listView.itemAtIndex(listView.currentIndex).delegateBtnBgOpacity = 0.4
            if(root.lastIndex != -1)
            {
                if(listView.itemAtIndex(root.lastIndex))
                {
                    listView.itemAtIndex(root.lastIndex).delegateBtnBgColor = "transparent"
                    listView.itemAtIndex(root.lastIndex).delegateBtnBgOpacity = 1
                }
            }
            root.lastIndex = listView.currentIndex
        }
    }    
}
