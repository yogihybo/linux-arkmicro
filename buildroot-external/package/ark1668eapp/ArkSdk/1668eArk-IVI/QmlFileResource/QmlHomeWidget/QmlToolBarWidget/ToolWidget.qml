import QtQuick 2.0
import com.test.model 1.0
Item {
    id:root
    width: 160
    height: 720
    visible: true
    property bool mediaClickedStatus: false
    property bool phoneLinkClickedStatus: false
    property bool telClickedStatus: false
    property bool auxClickedStatus: false
    property bool settingClickedStatus: false
    signal  multiMediaBtnClicked
    signal  auxBtnClicked
    signal  settingBtnClicked
    signal  phoneLinkClicked
    signal  telClicked
    Rectangle{
        id:bgRect
        anchors.fill:parent
        color: "#0A2749"
        objectName: "bgRectObject"
    }
    Text{
        id:timeText
        x:25
        y:26
        width: 169
        height: 41
        opacity: 1
        color:"#FFFFFF"
        font.pixelSize: 48
        font.weight: Font.Normal
        font.family: "Helvetica LT Std"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:"00:00"
        objectName: "tiemObject"
    }
    Row{
        x:0
        y:74
        spacing: 4
        Repeater{
            model: myStatusBar.objectModel()
            Rectangle{
                color: "transparent"
                width: 36
                height: 36
                Image {
                    id: statusImage
                    anchors.fill:parent
                    source: model.data
                }
            }
        }
    }

    Rectangle{
        id:multiMediaBtn
        x:40
        y:130
        width: 80
        height: 80
        color:"transparent"
        Image{
            id:mediaImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/MusicIconNormal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  mediaImage.source === "qrc:/images/HomeWidget/MusicIconPress.png"
            onClicked: {
                if(root.mediaClickedStatus === false)
                {
                    root.mediaClickedStatus = true;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                else
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                root.multiMediaBtnClicked();
            }
        }
    }

    Rectangle{
        x:40
        y:250
        width: 80
        height: 80
        color:"transparent"
        Image{
            id:phoneLinkImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/PhoneLinkIconNormal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  phoneLinkImage.source = "qrc:/images/HomeWidget/PhoneLinkIconPress.png"
            onClicked: {
                if(root.phoneLinkClickedStatus === false)
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = true;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                else
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                root.phoneLinkClicked()
            }
        }
    }

    Rectangle{
        x:40
        y:370
        width: 80
        height: 80
        color:"transparent"
        Image{
            id:telImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/TelIconNormal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  telImage.source = "qrc:/images/HomeWidget/TelIconPress.png"
            onClicked: {
                if(root.telClickedStatus === false)
                {
                    root.mediaClickedStatus = false
                    root.phoneLinkClickedStatus = false
                    root.telClickedStatus = true
                    root.auxClickedStatus = false
                    root.settingClickedStatus= false
                }
                else
                {
                    root.mediaClickedStatus = false
                    root.phoneLinkClickedStatus = false
                    root.telClickedStatus = false
                    root.auxClickedStatus = false
                    root.settingClickedStatus= false
                }
                root.telClicked()
            }
        }
    }

    Rectangle{
        x:40
        y:490
        width: 80
        height: 80
        color:"transparent"
        Image{
            id:auxImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/AuxIconNomal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  auxImage.source = "qrc:/images/HomeWidget/AuxIconPress.png"
            onClicked: {
                if(auxClickedStatus === false)
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = true;
                    root.settingClickedStatus= false;
                }
                else
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                root.auxBtnClicked();
            }
        }
    }

    Rectangle{
        x:40
        y:610
        width: 80
        height: 80
        color:"transparent"
        Image{
            id:settingImage
            anchors.fill:parent
            source: "qrc:/images/HomeWidget/SettingIconNormal.png"
        }
        MouseArea{
            anchors.fill:parent
            onPressed:  settingImage.source = "qrc:/images/HomeWidget/SettingIconPress.png"
            onClicked: {
                if(root.settingClickedStatus === false)
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= true;
                }
                else
                {
                    root.mediaClickedStatus = false;
                    root.phoneLinkClickedStatus = false;
                    root.telClickedStatus = false;
                    root.auxClickedStatus = false;
                    root.settingClickedStatus= false;
                }
                root.settingBtnClicked();
            }
        }
    }

    onMediaClickedStatusChanged:{
        if(root.mediaClickedStatus === true)
        {
            mediaImage.source = "qrc:/images/HomeWidget/MusicIconPress.png"
        }
        else
        {
            mediaImage.source = "qrc:/images/HomeWidget/MusicIconNormal.png"
        }
    }
    onPhoneLinkClickedStatusChanged: {
        if(root.phoneLinkClickedStatus === true)
        {
            phoneLinkImage.source = "qrc:/images/HomeWidget/PhoneLinkIconPress.png"
        }
        else
        {
            phoneLinkImage.source = "qrc:/images/HomeWidget/PhoneLinkIconNormal.png"
        }
    }

    onTelClickedStatusChanged: {

        if(root.telClickedStatus === true)
        {
            telImage.source = "qrc:/images/HomeWidget/TelIconPress.png"
        }
        else
        {
            telImage.source = "qrc:/images/HomeWidget/TelIconNormal.png"
        }
    }

    onAuxClickedStatusChanged: {
        if(root.auxClickedStatus === true)
        {
            auxImage.source = "qrc:/images/HomeWidget/AuxIconPress.png"
        }
        else
        {
            auxImage.source = "qrc:/images/HomeWidget/AuxIconNomal.png"
        }
    }
    onSettingClickedStatusChanged: {

        if(root.settingClickedStatus === true)
        {
            settingImage.source = "qrc:/images/HomeWidget/SettingIconPress.png"
        }
        else
        {
            settingImage.source = "qrc:/images/HomeWidget/SettingIconNormal.png"
        }
    }
}
