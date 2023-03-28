import QtQuick 2.0
import "./../QmlBtTelMiniWidget"
import "./../QmlMusicMiniWidget"
import "./../QmlPhoneLinkMiniWidget"
import "./../QmlVideoMiniWidget"
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int currentPage: 0
    BtTelMiniWidget{
        id:btTelMiniWidget
        x:40
        y:56
        objectName: "btTelMiniWidgetObject"
    }
    MusicMiniWidget{
        id:musicMiniWidget
        x:467
        y:56
        objectName: "musicMiniWidgetObject"
    }

    PhoneLinkMiniWidget{
        id:phoneLinkMiniWidget
        x:894
        y:56
        objectName: "phoneLinkMiniWidgetObject"
    }

    VideoMiniWidget{
        id:videoMiniWidget
        x:1320
        y:56
        objectName: "auxMiniWidgetObject"
    }
    onCurrentPageChanged: {
        switch(root.currentPage)
        {
            case 0:
                btTelMiniWidget.currentPage = 0;
                musicMiniWidget.currentPage = 0;
                phoneLinkMiniWidget.currentPage = 0;
                videoMiniWidget.currentPage = 0;
                break;
            case 1:
                btTelMiniWidget.currentPage = 1;
                musicMiniWidget.currentPage = 1;
                phoneLinkMiniWidget.currentPage = 1;
                videoMiniWidget.currentPage = 1;
                break;
            case 2:
                btTelMiniWidget.currentPage = 2;
                musicMiniWidget.currentPage = 2;
                phoneLinkMiniWidget.currentPage = 2;
                videoMiniWidget.currentPage = 2;
                break;
           default:
               break;

        }
    }
}
