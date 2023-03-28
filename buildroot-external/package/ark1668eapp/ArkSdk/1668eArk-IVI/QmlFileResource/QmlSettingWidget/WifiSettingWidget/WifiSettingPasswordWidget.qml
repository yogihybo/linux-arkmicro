import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    Button{
        anchors.fill:parent
        background: Rectangle{
            anchors.fill:parent
            color:"#000000"
        }
    }
    FocusScope {
        id:focusScope
        anchors.horizontalCenter: parent.horizontalCenter
        y:60
        width: 600
        height: 60
        objectName: "textInputObject"
        property alias textInputText: textInput.text
        signal clicked
        Rectangle{
            id:textInputRect
            anchors.fill: parent
            width: 600
            height: 60
            color: "transparent"
            border.color: "#FFFFFF"
            border.width: 2
            opacity: 0.2
            radius: 30
        }
        TextInput {
            id: textInput
            anchors.left:textInputRect.left
            anchors.leftMargin: 30
            anchors.top:textInputRect.top
            anchors.topMargin: 18
            width: 540
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
            text: qsTr("密码")
        }
        MouseArea{
            anchors.fill:parent
            onClicked: {
                //console.log("++++++++onClicked++++++++");
                focusScope.clicked();
            }
        }
    }
    Button{
        id:cancleBtn
        anchors.left: parent.left
        anchors.leftMargin: 250
        anchors.top:focusScope.bottom
        anchors.topMargin: 50
        width: 150
        height: 60
        objectName: "cancleBtnObject"
        background: Rectangle{
            id:cancleBtnBg
            anchors.fill:parent
            color:"#FFFFFF"
            opacity: 0.2
            radius: 50
        }
        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            opacity: 1
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("取消")
        }
        onPressed:cancleBtnBg.opacity = 0.6
        onReleased: cancleBtnBg.opacity = 0.2
        onClicked: {
            focusScope.textInputText=""
        }
    }

    Button{
        id:confirmBtnBtn
        anchors.left: cancleBtn.right
        anchors.leftMargin: 80
        anchors.top:focusScope.bottom
        anchors.topMargin: 50
        width: 150
        height: 60
        objectName: "confirmBtnBtnObject"
        background: Rectangle{
            id:confirmBtnBtnBg
            anchors.fill:parent
            color: "#0DA8FF"
            radius: 50
        }
        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            opacity: 1
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("确定")
        }
        onPressed:confirmBtnBtnBg.opacity = 0.4
        onReleased: confirmBtnBtnBg.opacity =1
    }

}
