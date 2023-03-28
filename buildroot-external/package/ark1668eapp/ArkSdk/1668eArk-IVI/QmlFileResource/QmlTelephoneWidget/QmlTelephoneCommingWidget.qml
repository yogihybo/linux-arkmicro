import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 520
    height: 720
    visible: false
    Rectangle{
        anchors.fill:parent
        color:"#000000"
    }
    Image{
        id:manIcon
        anchors.left: parent.left
        anchors.leftMargin: 130
        anchors.top:parent.top
        anchors.topMargin: 111
        width: 280
        height: 280
        source: "qrc:/images/TelephoneWidget/portrait.png"
    }
    Text{
        id:phoneNumber
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:parent.top
        anchors.topMargin: 432
        width: 273
        height: 39
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 32
        font.family: "Poppins"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "phoneNumberObject"
        elide:Text.ElideRight
    }

    Text{
        id:inConingText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:parent.top
        anchors.topMargin: 477
        width: 273
        height: 28
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 32
        font.family: "Poppins"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        objectName: "InConingTextObject"
        elide:Text.ElideRight
    }
    Button{
         id:answerBtn
         anchors.left: parent.left
         anchors.leftMargin: 130
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

    Button{
         id:hungUpBtn
         anchors.left: parent.left
         anchors.leftMargin: 322
         anchors.bottom:parent.bottom
         anchors.bottomMargin: 80
         width: 88
         height: 88
         objectName: "hungUpBtnObject"
         background: Rectangle{
             id:hungUpBtnBg
             anchors.fill:parent
             color: "transparent"
         }
         Image {
             id: hungUpBtnIcon
             anchors.fill: parent
             source: "qrc:/images/TelephoneWidget/HungUpNormal.png"
         }
         onPressed: {
             hungUpBtnIcon.source = "qrc:/images/TelephoneWidget/HungUpPress.png"
         }
         onReleased: {
             hungUpBtnIcon.source = "qrc:/images/TelephoneWidget/HungUpNormal.png"
         }
    }
}
