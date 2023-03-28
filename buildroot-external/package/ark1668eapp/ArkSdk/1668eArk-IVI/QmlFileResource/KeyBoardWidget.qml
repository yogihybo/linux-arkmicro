import QtQuick 2.0
import QtQuick.Controls 2.0
import com.QmlWidget.model 1.0
import com.test.model 1.0

Item {
    id:root
    width: 1920
    height: 450
    visible: true
    Button{
        anchors.fill:parent
        background: Rectangle{
            color: "#0E0E0E"
        }
    }
    Row{
        anchors.left: parent.left
        anchors.leftMargin: 60
        anchors.top:parent.top
        anchors.topMargin: 20
        spacing: 0
        Repeater{
            id:firstRowLetter
            model:myFirstRowModelData.objectModel()
            Button{
                id:firstDegateBtn
                width: 120
                height: 100
                background: Rectangle{
                    id:firstDegateBtnBg
                    anchors.fill: parent
                    color: "transparent"
                }
                Text{
                    id:firstRowText
                    anchors.fill: parent
                    color: "#FFFFFF"
                    font.pixelSize: 36
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:model.data
                }
                onPressed: {
                    firstDegateBtnBg.color = "#0DA8FF"
                    firstDegateBtnBg.opacity = 0.4
                }
                onReleased:{
                    firstDegateBtnBg.color = "transparent"
                    firstDegateBtnBg.opacity = 1
                }
                onClicked:
                {
                   QmlWidget.addKeyBoardInputStr(firstRowText.text);
                }
            }
        }
    }

    Row{
        anchors.left: parent.left
        anchors.leftMargin: 120
        anchors.top:parent.top
        anchors.topMargin: 120
        spacing: 0
        Repeater{
            id:secondRowLetter
            model:mySecondRowModelData.objectModel()
            Button{
                id:seconddegateBtn
                width: 120
                height: 100
                background: Rectangle{
                    id:secondDegateBtnBg
                    anchors.fill: parent
                    color: "transparent"
                }
                Text{
                    id:secondRowText
                    anchors.fill: parent
                    color: "#FFFFFF"
                    font.pixelSize: 36
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:model.data
                }
                onPressed: {
                    secondDegateBtnBg.color = "#0DA8FF"
                    secondDegateBtnBg.opacity = 0.4
                }
                onReleased:{
                    secondDegateBtnBg.color = "transparent"
                    secondDegateBtnBg.opacity = 1
                }
                onClicked: {
                    QmlWidget.addKeyBoardInputStr(secondRowText.text);
                }
            }
        }
    }

    Row{
        anchors.left: parent.left
        anchors.leftMargin: 180
        anchors.top:parent.top
        anchors.topMargin: 220
        spacing: 0
        Repeater{
            id:thirdRowLetter
            model:myThirdRowModelData.objectModel()
            Button{
                id:thirddegateBtn
                width: 120
                height: 100
                background: Rectangle{
                    id:thirdDegateBtnBg
                    anchors.fill: parent
                    color: "transparent"
                }
                Text{
                    id:thirdRowText
                    anchors.fill: parent
                    color: "#FFFFFF"
                    font.pixelSize: 36
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:model.data
                }
                onPressed: {
                    thirdDegateBtnBg.color = "#0DA8FF"
                    thirdDegateBtnBg.opacity = 0.4
                }
                onReleased:{
                    thirdDegateBtnBg.color = "transparent"
                    thirdDegateBtnBg.opacity = 1
                }
                onClicked: {
                    QmlWidget.addKeyBoardInputStr(thirdRowText.text);
                }
            }
        }
    }

    Button{
        id:caseSwitchBtn
        anchors.left: parent.left
        anchors.leftMargin: 60
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 180
        height: 60
        background: Rectangle{
            id:caseSwitchBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("大小写")
        }
        onPressed: {
            caseSwitchBtnBg.opacity = 0.6
        }
        onReleased:{
            caseSwitchBtnBg.opacity = 0.2
        }
        onClicked: {
            QmlWidget.caseSwitchBtnClicked();
        }
    }

    Button{
        id:chEhSwitchBtn
        anchors.left: parent.left
        anchors.leftMargin: 256
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 180
        height: 60
        enabled: false
        background: Rectangle{
            id:chEhSwitchBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("中/英")
        }
        onPressed: {
            chEhSwitchBtnBg.opacity = 0.6
        }
        onReleased:{
            chEhSwitchBtnBg.opacity = 0.2
        }
    }

    Button{
        id:spaceBtn
        anchors.left: parent.left
        anchors.leftMargin: 452
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 556
        height: 60
        background: Rectangle{
            id:spaceBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        onPressed: {
            spaceBtnBg.opacity = 0.6
        }
        onReleased:{
            spaceBtnBg.opacity = 0.2
        }
        onClicked: {
            QmlWidget.addKeyBoardInputStr(" ");
        }
    }

    Button{
        id:symbolsBtn
        anchors.left: parent.left
        anchors.leftMargin: 1024
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 170
        height: 60
        background: Rectangle{
            id:symbolsBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("?#&")
        }
        onPressed: {
            symbolsBtnBg.opacity = 0.6
        }
        onReleased:{
            symbolsBtnBg.opacity = 0.2
        }
        onClicked: {
            QmlWidget.symbolsBtnClicked();
        }
    }

    Button{
        id:backBtn
        anchors.left: parent.left
        anchors.leftMargin: 1210
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 107
        height: 60
        background: Rectangle{
            id:backBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("<")
        }
        onPressed: {
            backBtnBg.opacity = 0.6
        }
        onReleased:{
            backBtnBg.opacity = 0.2
        }
    }

    Button{
        id:nextBtn
        anchors.left: parent.left
        anchors.leftMargin: 1333
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 107
        height: 60
        background: Rectangle{
            id:nextBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr(">")
        }
        onPressed: {
            nextBtnBg.opacity = 0.6
        }
        onReleased:{
            nextBtnBg.opacity = 0.2
        }
    }

    Button{
        id:backSpaceBtn
        anchors.left: parent.left
        anchors.leftMargin: 1280
        anchors.top:parent.top
        anchors.topMargin: 40
        width: 160
        height: 60
        background: Rectangle{
            id:backSpaceBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("退格")
        }
        onPressed: {
            backSpaceBtnBg.opacity = 0.6
        }
        onReleased:{
            backSpaceBtnBg.opacity = 0.2
        }
        onClicked: {
            QmlWidget.subKeyBoardInputStr();
        }
    }

    Button{
        id:enterBtn
        anchors.left: parent.left
        anchors.leftMargin: 1220
        anchors.top:parent.top
        anchors.topMargin: 140
        width: 220
        height: 60
        background: Rectangle{
            id:enterBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill: parent
            color: "#FFFFFF"
            font.pixelSize: 24
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("回车")
        }
        onPressed: {
            enterBtnBg.opacity = 0.6
        }
        onReleased:{
            enterBtnBg.opacity = 0.2
        }
        onClicked: {
            QmlWidget.addKeyBoardInputStr("\r\n")
            QmlWidget.enterBtnClicked();
        }
    }


    Grid{
        anchors.right:parent.right
        anchors.rightMargin: 60
        anchors.top:parent.top
        anchors.topMargin: 20
        flow:Grid.LeftToRight
        spacing: 0
        width: 360
        height: 400
        clip: true
        Repeater{
            id:numGridRep
            model:["1","2","3","","4","5","6","","7","8","9","","","0",""]
            Button{
                width: 120
                height: 100
                background: Rectangle{
                    id:fourthDegateBtn
                    anchors.fill: parent
                    color: "transparent"
                }
                Text{
                    id:fourthDegateText
                    anchors.fill: parent
                    color: "#FFFFFF"
                    font.pixelSize: 36
                    font.family: "Montserrat"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text:modelData
                }
                onPressed: {
                    fourthDegateBtn.color = "#0DA8FF"
                    fourthDegateBtn.opacity = 0.4
                }
                onReleased:{
                    fourthDegateBtn.color = "transparent"
                    fourthDegateBtn.opacity = 1
                }
                onClicked: {
                    if(fourthDegateText.text != ""){
                        QmlWidget.addKeyBoardInputStr(fourthDegateText.text);
                    }
                }
            }
        }
    }
}
