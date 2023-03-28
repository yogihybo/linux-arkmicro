import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 520
    height: 720
    visible: true
    property int showType: 0
    Rectangle{
        anchors.fill:parent
        color:"#000000"
    }
    Rectangle{
        id:portraitRect
        x:0
        y:0
        width:  520
        height: 520
        color:"transparent"
        objectName: "portraitRectObject"
        Image{
            id:manIcon
            anchors.left: parent.left
            anchors.leftMargin: 130
            anchors.top:parent.top
            anchors.topMargin: 111
            width:  280
            height: 280
            source: "qrc:/images/TelephoneWidget/portrait.png"
        }
        Text{
            id:portraitRectPhoneNumber
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
            objectName: "portraitRectPhoneNumberObject"
            elide:Text.ElideRight
        }
        Text{
            id:inCommingText
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
            objectName: "inCommingTextObject"
            elide:Text.ElideRight
        }
    }

    Rectangle{
        id:keyBoardRect
        x:0
        y:0
        width: 520
        height: 520
        color:"transparent"
        property string phoneNumberStr:""
        property int pressIndex: -1
        visible: false
        objectName: "keyBoardRectObject"
        signal numberBtnClicked(string text)
        Text{
            id:inputText
            x:0
            y:18
            width:520
            height: 41
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 40
            font.family: "Helvetica LT Std"
            objectName: "keyBoardRectInputTextObject"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide:Text.ElideRight
        }
        Text{
            id:keyBoardPhoneNumber
            anchors.left: parent.left
            anchors.leftMargin: 90
            anchors.top:inputText.bottom
            anchors.topMargin: 5
            width: 339
            height: 41
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 40
            font.family: "Helvetica LT Std"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            objectName: "keyBoardRectPhoneNumberObject"
            elide:Text.ElideRight
        }
        Text{
            id:onCallTime
            anchors.left: parent.left
            anchors.top:keyBoardPhoneNumber.bottom
            anchors.topMargin: 5
            width: 520
            height: 28
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 24
            font.family: "Helvetica LT Std"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            objectName: "keyBoardRectOnCallTimeObject"
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
            objectName: "keyBoardRectDelBtnObject"
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
                keyBoardRect.phoneNumberStr = inputText.text
                if(keyBoardRect.phoneNumberStr.length > 0)
                {
                    keyBoardRect.phoneNumberStr = keyBoardRect.phoneNumberStr.substring(0,(keyBoardRect.phoneNumberStr.length-1));
                }
                inputText.text = keyBoardRect.phoneNumberStr
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
            onContentYChanged: {
                if(keyBoardRect.pressIndex != -1)
                {
                    if(dialView.itemAtIndex(keyBoardRect.pressIndex))
                    {
                        dialView.itemAtIndex(keyBoardRect.pressIndex).numberBtnBgColor = "transparent"
                        dialView.itemAtIndex(keyBoardRect.pressIndex).numberBtnBgOpacity = 1
                    }
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
                    keyBoardRect.pressIndex = index
                }
                onReleased: {
                    numberBtnBg.color = "transparent"
                    numberBtnBg.opacity = 1
                }
                onClicked: {
                    keyBoardRect.phoneNumberStr   = keyBoardRect.phoneNumberStr + numberText.text
                    inputText.text  = keyBoardRect.phoneNumberStr
                    keyBoardRect.numberBtnClicked(numberText.text)
                }
            }
        }
    }
    Button{
         id:hungUpBtn
         anchors.left: parent.left
         anchors.leftMargin: 24
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

    Button{
         id:keyboardBtn
         anchors.left: hungUpBtn.right
         anchors.leftMargin: 40
         anchors.bottom:parent.bottom
         anchors.bottomMargin: 80
         width: 88
         height: 88
         objectName: "keyboardBtnObject"
         background: Rectangle{
             id:keyboardBtnBg
             anchors.fill:parent
             color: "transparent"
         }
         Image {
             id: keyboardBtnIcon
             anchors.fill: parent
             source: "qrc:/images/TelephoneWidget/KeyboardNormal.png"
         }
         onPressed: {
             keyboardBtnIcon.source = "qrc:/images/TelephoneWidget/KeyboardPress.png"
         }
         onReleased: {
             keyboardBtnIcon.source = "qrc:/images/TelephoneWidget/KeyboardNormal.png"
         }
    }

    Button{
         id:microphoneBtn
         anchors.left: keyboardBtn.right
         anchors.leftMargin: 40
         anchors.bottom:parent.bottom
         anchors.bottomMargin: 80
         width: 88
         height: 88
         objectName: "microphoneBtnObject"
         background: Rectangle{
             id:microphoneBtnBg
             anchors.fill:parent
             color: "transparent"
         }
         Image {
             id:microphoneBtnIcon
             anchors.fill: parent
             source: "qrc:/images/TelephoneWidget/MuteIconNormal.png"
         }
         onPressed: {
             microphoneBtnIcon.source = "qrc:/images/TelephoneWidget/MuteIconPress.png"
         }
         onReleased: {
             microphoneBtnIcon.source = "qrc:/images/TelephoneWidget/MuteIconNormal.png"
         }
    }

    Button{
         id:transferBtn
         anchors.left:microphoneBtn.right
         anchors.leftMargin: 40
         anchors.bottom:parent.bottom
         anchors.bottomMargin: 80
         width: 88
         height: 88
         objectName: "transferBtnObject"
         background: Rectangle{
             id:transferBtnBg
             anchors.fill:parent
             color: "transparent"
         }
         Image {
             id:transferBtnIcon
             anchors.fill: parent
             source: "qrc:/images/TelephoneWidget/TransferNormal.png"
         }
         onPressed:{
             transferBtnIcon.source = "qrc:/images/TelephoneWidget/TransferPress.png"
         }
         onReleased:{
             transferBtnIcon.source = "qrc:/images/TelephoneWidget/TransferNormal.png"
         }
    }
    onShowTypeChanged: {
        switch(root.showType)
        {
            case 0:
                portraitRect.visible = true;
                keyBoardRect.visible = false;
                break;
            case 1:
                portraitRect.visible = false;
                keyBoardRect.visible = true;
                break;
            default:
                break;
        }
    }

}
