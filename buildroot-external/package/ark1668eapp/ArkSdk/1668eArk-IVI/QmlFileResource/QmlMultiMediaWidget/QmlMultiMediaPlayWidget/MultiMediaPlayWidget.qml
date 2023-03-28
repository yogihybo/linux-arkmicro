import QtQuick 2.0
import QtQuick.Controls 2.1
import "./../../QmlUserWidget"

Item {
    id:root
    width: 520
    height: 720
    visible: true
    signal musicTypeChanged
    Rectangle{
        anchors.left: parent.left
        anchors.leftMargin: 53
        anchors.top:parent.top
        anchors.topMargin: 58
        width: 416
        height: 416
        color: "transparent"
        Image{
            id:albumCoveImage
            anchors.fill:parent
            cache:false
            objectName: "albumCoveImageObject"
            source: "qrc:/images/MediaWidget/albumCover.png"
        }
        Connections{
            target: CodeImage
            function onCallQmlRefreshImg(){
                //console.log("+++++++onCallQmlRefreshImg1111+++++++++",CodeImage.isNullConverImage())
                if(CodeImage.isNullConverImage() === true)
                {
                    albumCoveImage.source = "";
                    albumCoveImage.source = "qrc:/images/MediaWidget/albumCover.png";
                }
                else{
                    albumCoveImage.source = "";
                    albumCoveImage.source = "image://CodeImg";

                }
            }
        }
    }


    Text{
        id:title
        anchors.left: parent.left
        anchors.leftMargin: 50
        anchors.top:parent.top
        anchors.topMargin: 488
        width: 407
        height: 32
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 26
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "titleObject"
        elide:Text.ElideRight
    }
    Text{
        id:artist
        anchors.left: parent.left
        anchors.leftMargin: 50
        anchors.top:parent.top
        anchors.topMargin: 525
        width: 184
        height: 25
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 20
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "artistObject"
        elide:Text.ElideRight
    }

    Text{
        id:album
        anchors.left: parent.left
        anchors.leftMargin: 50
        anchors.top:parent.top
        anchors.topMargin: 555
        width: 299
        height: 19
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 15
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        objectName: "albumObject"
        elide:Text.ElideRight
    }

    Text{
        id:remaTime
        anchors.right: parent.right
        anchors.rightMargin: 51
        anchors.top:parent.top
        anchors.topMargin: 567
        width: 40
        height: 19
        opacity: 0.4
        color:"#FFFFFF"
        font.pixelSize: 15
        font.family: "Montserrat"
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text:"-0:00"
        objectName: "remaTimeObject"
    }

    UserSlider{
        id:control
        anchors.left: parent.left
        anchors.leftMargin: 53
        anchors.top:parent.top
        anchors.topMargin: 590
        width: 416
        height: 4
        fromValue: 0
        toValue:100
        value: 0
        enabled: false
        objectName: "sliderObject"
        background: Rectangle{
            x:control.leftPadding
            y:control.topPadding + control.availableHeight / 2 - height / 2
            width:  416
            height: 4
            radius: 2
            color:"#FFFFFF"
            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                opacity: 0.6
                color:"#000000"
                radius: 2
          }
        }
    }
    Button{
        id:preBtn
        anchors.left: parent.left
        anchors.leftMargin: 53
        anchors.top:parent.top
        anchors.topMargin: 616
        width: 48
        height:48
        objectName: "preBtnObject"
        background: Rectangle{
            id:preBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:preBtnImage
            anchors.fill:parent
            source: "qrc:/images/MediaWidget/PrevNormal.png"
        }
        onPressed: preBtnImage.source = "qrc:/images/MediaWidget/PrevPress.png"
        onReleased: preBtnImage.source = "qrc:/images/MediaWidget/PrevNormal.png"
    }

    Button{
        id:playBtn
        anchors.left: parent.left
        anchors.leftMargin: 175
        anchors.top:parent.top
        anchors.topMargin: 616
        width: 48
        height:48
        objectName: "playBtnObject"
        property bool playStatus: false
        background: Rectangle{
            id:playBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:playBtnImage
            anchors.fill:parent
            source: "qrc:/images/MediaWidget/PauseNormal.png"
        }
        onPressed: {
            if(playStatus === true)
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PlayPress.png"
            }
            else
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PausePress.png"
            }
        }
        onReleased:{
            if(playStatus === true)
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PlayNormal.png"
            }
            else
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PauseNormal.png"
            }
        }
        onPlayStatusChanged:{
            if(playStatus === true)
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PlayNormal.png"
            }
            else
            {
                playBtnImage.source = "qrc:/images/MediaWidget/PauseNormal.png"
            }
        }
    }

    Button{
        id:nextBtn
        anchors.left: parent.left
        anchors.leftMargin: 297
        anchors.top:parent.top
        anchors.topMargin: 616
        width: 48
        height:48
        objectName: "nextBtnObject"
        background: Rectangle{
            id:nextBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:nextBtnImage
            anchors.fill:parent
            source: "qrc:/images/MediaWidget/NextNormal.png"
        }
        onPressed: nextBtnImage.source = "qrc:/images/MediaWidget/NextPress.png"
        onReleased: nextBtnImage.source = "qrc:/images/MediaWidget/NextNormal.png"
    }


    Button{
        id:modeBtn
        anchors.left: parent.left
        anchors.leftMargin: 419
        anchors.top:parent.top
        anchors.topMargin: 616
        width: 48
        height:48
        objectName: "modeBtnObject"
        property int playModeType: 0
        background: Rectangle{
            id:modeBtnBg
            anchors.fill:parent
            color:"transparent"
        }
        Image{
            id:modeBtnImage
            anchors.fill:parent
            source: "qrc:/images/MediaWidget/MultimediaAllNormal.png"
        }
        onPlayModeTypeChanged: {
            switch(playModeType){
                case 0:
                    modeBtnImage.source = "qrc:/images/MediaWidget/MultimediaAllNormal.png";
                    break;
                case 1:
                    modeBtnImage.source = "qrc:/images/MediaWidget/MultimediaRandom.png";
                    break;
                case 2:
                    modeBtnImage.source = "qrc:/images/MediaWidget/MultimediaSingle.png";
                    break;
                default:
                    break;
            }
        }
    }
}
