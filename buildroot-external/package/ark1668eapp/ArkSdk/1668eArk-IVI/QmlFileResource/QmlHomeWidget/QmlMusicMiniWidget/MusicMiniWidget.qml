import QtQuick 2.0
import QtQuick.Controls 2.0

Item {
    id:root
    width: 400
    height: 659
    visible: true
    property int currentPage: 0
    signal musicMiniWidgetClicked
    Rectangle{
        id:musicRect
        x:0
        y:0
        width: 400
        height: 560
        radius: 20
        Image{
            id:musicRectImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/RectBgNormal.png"
        }
        MouseArea{
            x:0
            y:0
            width:400
            height: 330
            onPressed:  musicRectImage.source = "qrc:/images/HomeWidget/RectBgPress.png"
            onReleased: musicRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png"
            onClicked: {
                root.musicMiniWidgetClicked();
            }
        }

        Rectangle{
            id:singerRect
            anchors.left:parent.left
            anchors.leftMargin: 70
            anchors.top:parent.top
            anchors.topMargin: 70
            width: 260
            height: 260
            color:"transparent"
            radius: 130
            Image{
                id:singerImage
                anchors.fill:parent
                cache:false
                source: "qrc:/images/HomeWidget/MusicMaster.png"
                fillMode: Image.PreserveAspectFit
                clip: true
            }
            Connections{
                target: CodeImage
                function onCallQmlRefreshImg(){
                    if(CodeImage.isNullConverImage() === true)
                    {
                        singerImage.source = "";
                        singerImage.source = "qrc:/images/HomeWidget/MusicMaster.png";
                    }
                    else{
                        singerImage.source = "";
                        singerImage.source = "image://CodeImg";
                    }
                }
            }

        }

        Button{
            id:prevBtn
            anchors.left: parent.left
            anchors.leftMargin: 50
            anchors.top:parent.top
            anchors.topMargin: 424
            width: 28
            height: 39
            objectName: "prevBtnObject"
            enabled: false
            background: Rectangle{
                anchors.fill:parent
                color:"transparent"
            }

            Image{
                id:previousImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/IcPreviousNormal.png"

            }
            onPressed: previousImage.source  = "qrc:/images/HomeWidget/IcPreviousPress.png"
            onReleased: previousImage.source = "qrc:/images/HomeWidget/IcPreviousNormal.png"
        }
        Text{
            id:musicName
            anchors.left: parent.left
            anchors.bottom: playbtn.top
            anchors.bottomMargin: 5
            width: 400
            height: 39
            opacity: 1
            color:"#FFFFFF"
            font.pixelSize: 32
            font.family: "Poppins"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            objectName: "musicNameObject"
            elide:Text.ElideRight
        }

        Button{
            id:playbtn
            anchors.left: parent.left
            anchors.leftMargin: 145
            anchors.top:parent.top
            anchors.topMargin: 390
            width: 108
            height: 108
            enabled: false
            objectName: "playbtnObject"
            property bool playstatus: false
            background: Rectangle{
                id:playbtnbg
                anchors.fill:parent
                color:"transparent"
            }
            Image{
                id:playImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/PauseNormal.png"
            }
            onPressed:{
               playbtnbg.opacity = 0.4
            }
            onReleased:{
                playbtnbg.opacity = 1
            }
            onPlaystatusChanged: {
                console.log("+++++playbtn.playstatus++++",playbtn.playstatus);
                if(playbtn.playstatus === false)
                {
                    playImage.source = "qrc:/images/HomeWidget/PauseNormal.png"
                }
                else{
                    playImage.source = "qrc:/images/HomeWidget/playButton.png"
                }
            }

        }
        Button{
            id:nextBtn
            anchors.left: parent.left
            anchors.leftMargin: 319
            anchors.top:parent.top
            anchors.topMargin: 424
            width: 28
            height: 39
            enabled: false
            objectName: "nextBtnObject"
            background:Rectangle{
                anchors.fill:parent
                color:"transparent"
            }
            Image{
                id:nextImage
                anchors.fill:parent
                source: "qrc:/images/HomeWidget/IcNextNormal.png"
            }
            onPressed: nextImage.source  = "qrc:/images/HomeWidget/IcNextPress.png"
            onReleased: nextImage.source = "qrc:/images/HomeWidget/IcNextNormal.png"
        }
    }

    Text{
        id:widgetName
        anchors.left:musicRect.left
        anchors.leftMargin: 96
        anchors.top:musicRect.bottom
        anchors.topMargin: 30
        width: 208
        height: 69
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text:qsTr("音乐")
    }
    onCurrentPageChanged: {
        switch(root.currentPage)
        {
            case 0:
                musicRectImage.source = "qrc:/images/HomeWidget/RectBgNormal.png";
                previousImage.source = "qrc:/images/HomeWidget/IcPreviousNormal.png";
                if(playbtn.playstatus === false)
                {
                    playImage.source = "qrc:/images/HomeWidget/PauseNormal.png";
                }
                else
                {
                    playImage.source = "qrc:/images/HomeWidget/playButton.png";
                }
                nextImage.source = "qrc:/images/HomeWidget/IcNextNormal.png";
                break;
             default:
                 break;
        }
    }
}
