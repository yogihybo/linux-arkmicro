import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 520
    height: 720
    visible: true
    property string phoneNumberStr:""
    property int pressIndex: -1
    signal listViewItemClicked(string phoneNumber)
    Rectangle{
        anchors.fill:parent
        color:"#000000"
    }
    Text{
        id:phoneNumber
        anchors.left: parent.left
        anchors.leftMargin: 90
        anchors.top:parent.top
        anchors.topMargin: 60
        width: 340
        height: 50
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "phoneNumberObject"
        elide:Text.ElideRight
    }
    Button{
        id:delBtn
        anchors.left: parent.left
        anchors.leftMargin: 432
        anchors.top:parent.top
        anchors.topMargin: 61
        width: 48
        height: 48
        background: Rectangle{
            id:delBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image{
            anchors.fill:parent
            source: "qrc:/images/TelephoneWidget/Backspace.png"
        }
        onPressed: {
            delBtnBg.color = "#0DA8FF"
            delBtnBg.opacity = 0.4
        }
        onReleased: {
            delBtnBg.color = "transparent"
            delBtnBg.opacity = 1
        }
        onClicked: {
            phoneNumberStr = phoneNumber.text
            if(phoneNumberStr.length > 0)
            {
                phoneNumberStr = phoneNumberStr.substring(0,(phoneNumberStr.length-1));

            }
            phoneNumber.text = phoneNumberStr
        }
    }
    GridView{
        id:dialView
        anchors.left: parent.left
        anchors.leftMargin: 80
        anchors.top: parent.top
        anchors.topMargin: 112
        clip: true
        width: 360
        height: 400
        model:12
        cellWidth: 120
        cellHeight: 100
        interactive:false
        delegate: numberDelegate
    }
    Timer{
        id:id_timer
        interval: 100
        repeat: false
        running: false
        triggeredOnStart:false
        onTriggered: {
            dialView.itemAtIndex(root.pressIndex).numberBtnBgColor = "transparent"
            dialView.itemAtIndex(root.pressIndex).numberBtnBgOpacity = 1
            root.pressIndex = -1
        }
    }
    onPressIndexChanged: {
        if(root.pressIndex != -1)
        {
            if(dialView.itemAtIndex(root.pressIndex))
            {
               id_timer.restart()
            }
        }
    }
    Component {
        id: numberDelegate
        Button {
            id:numberBtn
            width: 120
            height: 100
            property alias numberBtnBgColor: numberBtnBg.color
            property alias numberBtnBgOpacity: numberBtnBg.opacity
            background: Rectangle{
                id:numberBtnBg
                anchors.fill:parent
                color: "transparent"
            }
            Text {
                id:numberText
                anchors.fill:parent
                opacity: 1
                color:"#FFFFFF"
                font.pixelSize: 36
                font.family: "Montserrat"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                //text:"1"
            }
            Component.onCompleted: {
                if(index === 9)
                {
                    numberText.text = "*"
                }
                else if(index === 10)
                {
                    numberText.text  = "0"
                }
                else if(index === 11)
                {
                    numberText.text  ="#"
                }
                else
                {
                     numberText.text  = (index+1).toString()
                }
            }
            onPressed: {
                numberBtnBg.color = "#0DA8FF"
                numberBtnBg.opacity = 0.4
                root.pressIndex = index
            }
            onReleased: {
                numberBtnBg.color = "transparent"
                numberBtnBg.opacity = 1
            }
            onClicked: {
                if(phoneNumber.text.length < 16)
                {
                    phoneNumberStr   = phoneNumberStr + numberText.text
                    phoneNumber.text = phoneNumberStr
                }
            }
        }
    }

   Button{
        id:answerBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom:parent.bottom
        anchors.bottomMargin: 80
        width: 88
        height: 88
        objectName: "answerBtnObject"
        background: Rectangle{
            id:answerBtnBg
            anchors.fill:parent
            color: "transparent"
        }
        Image {
            id: answerBtnIcon
            anchors.fill: parent
            source: "qrc:/images/TelephoneWidget/AnswerNormal.png"
        }
        onPressed: {
            answerBtnIcon.source = "qrc:/images/TelephoneWidget/AnswerPress.png"
        }
        onReleased: {
            answerBtnIcon.source = "qrc:/images/TelephoneWidget/AnswerNormal.png"
        }
   }

}
