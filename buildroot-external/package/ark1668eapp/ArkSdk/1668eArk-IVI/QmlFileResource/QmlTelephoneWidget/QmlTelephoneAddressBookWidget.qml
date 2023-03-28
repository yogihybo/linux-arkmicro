 import QtQuick 2.0
import QtQuick.Controls 2.0
import com.phoneBook.model 1.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    property string rootPhoneNumber: ""
    property int  lastIndex: -1
    signal listViewItemClicked()
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
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: qsTr("搜索联系人")
        }
    }

    ListView{
        id:contactsListview
        anchors.left:parent.left
        anchors.leftMargin: 44
        anchors.top:parent.top
        anchors.topMargin: 134
        width: 791
        height: 586
        clip: true
        focus: true
        model:myPhoneBookModelData.getObjectModel()
        interactive: true
        property int  pressIndex: -1
        property bool itemClicked: false
        property int  listCurrentIndex: -1
        //property alias delegateBtnBg: value
        delegate: Button{
            width: 791
            height: 88
            id:delegateBtn
            property alias delegateBtnBgColor: delegateBtnBg.color
            property alias delegateBtnBgOpacity: delegateBtnBg.opacity
            background: Rectangle{
                id:delegateBtnBg
                anchors.fill:parent
                color:"transparent"
            }
            Text{
                id:nameText
                anchors.left:parent.left
                anchors.top:parent.top
                anchors.topMargin: 22
                width: 267
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

            Text{
                id:phoneNumber
                anchors.right:parent.right
                anchors.top:parent.top
                anchors.topMargin: 22
                width: 300
                height: 40
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: TextInput.AlignLeft
                verticalAlignment: TextInput.AlignVCenter
                text:model.data3
                elide:Text.ElideRight
            }
            onPressed: {
                if(contactsListview.currentIndex != index)
                {
                    delegateBtnBg.color = "#0DA8FF"
                    delegateBtnBg.opacity = 0.4
                    contactsListview.pressIndex = index
                    contactsListview.itemClicked = false
                }
            }
            onReleased:{
                if(contactsListview.currentIndex != index)
                {
                    delegateBtnBg.color = "transparent"
                    delegateBtnBg.opacity = 1
                }
            }
            onClicked: {
                contactsListview.currentIndex = index
                contactsListview.listCurrentIndex = index
                root.rootPhoneNumber = phoneNumber.text
                root.listViewItemClicked()
                contactsListview.itemClicked = true
            }
        }
        section.property: "data1"
        section.criteria: ViewSection.FirstCharacter
        section.delegate: sectionHeader
        onContentYChanged: {
            if(contactsListview.pressIndex != -1)
            {
                if(contactsListview.itemClicked === false)
                {
                    if(contactsListview.itemAtIndex(contactsListview.pressIndex))
                    {
                        contactsListview.itemAtIndex(contactsListview.pressIndex).delegateBtnBgColor = "transparent"
                        contactsListview.itemAtIndex(contactsListview.pressIndex).delegateBtnBgOpacity = 1
                    }
                }
            }

            if(contactsListview.contentY < 0)
            {
                contactsListview.contentY = 0
            }
            contactsListview.pressIndex = -1
        }
        onListCurrentIndexChanged: {
            contactsListview.itemAtIndex(contactsListview.currentIndex).delegateBtnBgColor = "#FFFFFF"
            contactsListview.itemAtIndex(contactsListview.currentIndex).delegateBtnBgOpacity = 0.4
            if(root.lastIndex != -1)
            {
                if(contactsListview.itemAtIndex(root.lastIndex))
                {
                    contactsListview.itemAtIndex(root.lastIndex).delegateBtnBgColor = "transparent"
                    contactsListview.itemAtIndex(root.lastIndex).delegateBtnBgOpacity = 1
                }
            }
            root.lastIndex = contactsListview.currentIndex
        }
    }

    Component{
        id: sectionHeader
        Rectangle{
            width: 791
            height: 88
            color: "transparent"

            Text{
                anchors.fill:parent
                opacity: 0.4
                color:"#FFFFFF"
                text: section
                font.pixelSize: 36
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    ListView{
        id:letter_Listview
        anchors.right: parent.right
        anchors.rightMargin: 7
        anchors.top:parent.top
        anchors.topMargin: 117
        width: 38
        height: 605
        clip: true
        focus: true
        property int pressIndex: -1
        property int lastIndex: -1
        property int listCurrentIndex: -1
        property bool itemClicked: false
        model: ["A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
            "P","Q","R","S","T","U","V","W","X","Y","Z","#"]
        delegate:Button{
            width: 38
            height: 38
            property alias delegateRectColor: id_Bg.color
            background: Rectangle{
                id:id_Bg
                anchors.fill: parent
                color:"transparent"
                radius: 19
            }
            Text{
                anchors.fill: parent
                color:"#FFFFFF"
                opacity: 0.6
                font.pixelSize: 26
                font.family: "Eurostile LT ExtendedTwo"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
            onPressed: {
                if(letter_Listview.currentIndex != index)
                {
                    letter_Listview.pressIndex = index
                    id_Bg.color = "#35AEFD"
                    letter_Listview.itemClicked = false
                }
            }
            onReleased: {
                if(letter_Listview.currentIndex != index)
                {
                    id_Bg.color = "transparent"
                }
            }
            onClicked: {
                var idx = myPhoneBookModelData.getModelDataHead(modelData);
                contactsListview.positionViewAtIndex(idx, ListView.Beginning);
                letter_Listview.itemClicked      = true;
                letter_Listview.currentIndex     = index;
                letter_Listview.listCurrentIndex = index;
            }
        }
        onContentYChanged: {
            if(letter_Listview.pressIndex != -1)
            {
                if(letter_Listview.itemClicked === false)
                {
                    if(letter_Listview.itemAtIndex(letter_Listview.pressIndex))
                    {
                        letter_Listview.itemAtIndex(letter_Listview.pressIndex).delegateRectColor = "transparent"
                    }
                }

            }
            if(letter_Listview.contentY < 0)
            {
                letter_Listview.contentY = 0
            }
            if(letter_Listview.contentY >= 421){
                letter_Listview.contentY = 421
            }
            letter_Listview.pressIndex = -1
        }
        onListCurrentIndexChanged: {
            letter_Listview.itemAtIndex(letter_Listview.currentIndex).delegateRectColor = "#35AEFD"
            if(letter_Listview.lastIndex != -1)
            {
                if(letter_Listview.itemAtIndex(letter_Listview.lastIndex))
                {
                    letter_Listview.itemAtIndex(letter_Listview.lastIndex).delegateRectColor = "transparent"
                }
            }
            letter_Listview.lastIndex = letter_Listview.currentIndex
        }
    }
    function getIndexFromLab(lab)
    {
       var i;
       for (i=0;i< myPhoneBookModelData.modelDataCount();i++)
       {
           if (myPhoneBookModelData.getModelDataHead(i) === lab)
           {
               break;
           }
       }
       return i;
    }

}
