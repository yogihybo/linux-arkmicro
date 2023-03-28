import QtQuick 2.0
import QtQuick.Controls 2.0

import "./QmlHomeWidget/QmlToolBarWidget"
import "./QmlHomeWidget"
import "./QmlMultiMediaWidget"
import "./QmlVideoMediaWidget"
import "./QmlSettingWidget"
import "./QmlPhoneLinkWidget"
import "./QmlTelephoneWidget"
import "./QmlHomeWidget/QMlSwipeViewHomeWidget/"
import "./QmlAuxWidget"
Item {
    id:root
    width: 1920
    height: 720
    visible: true
    property int  mediaStatus: 0
    property int  typeStatus: 0
    property int  mainFullScreenType: 0
    property bool isImageFullScreen: false
    property int  phoneLinkStatus: 0
    property int  auxStatus: 0
    signal mousePressed(double x,double y)
    signal mouseRelease(double x,double y)
    signal mouseMove(double x,double y)
    signal multiMediaComponentLoaderComplete
    signal phoneLinkComponentLoaderComplete
    signal telephoneComponentLoaderComplete
    signal videoMediaComponentLoaderComplete
    signal settingComponentLoaderComplete
    signal keyBoardComponentLoaderComplete
    signal auxComponentLoaderComplete
    signal backComponentLoaderComplete
    Rectangle{
        id:bgRect
        anchors.fill:parent
        Image{
            id:bgRectImage
            anchors.fill:parent
            objectName:"bgRectObject"
            source:"qrc:/images/HomeWidget/HomeBg.png"
        }
    }
    FooterWidget{
        id:footerWidget
        x:950
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
    }
    HomeWidget{
        id:homeWidget
        x:160
        y:0
        visible: true
        objectName: "homeWidgetObject"
        onSwipeViewCurrentIndexChanged: {
            footerWidget.currentIndex = homeWidget.swipeViewCurrentIndex;
        }
    }

    ToolWidget{
        id:toolWidget
        x:0
        y:0
        objectName: "toolWidgetObject"
    }
    Loader{
        id:globalRect_loader
        anchors.fill:parent
        visible: false
        asynchronous: true
    }
    Component{
        id:globalRectComponent
        Rectangle{
            id:globalRect
            color: "transparent"
            MouseArea{
                anchors.fill:parent
                onPressed: {
                    root.mousePressed(mouseX,mouseY);
                }
                onReleased: {
                    root.mouseRelease(mouseX,mouseY);
                }
                onPositionChanged: {
                    root.mouseMove(mouseX,mouseY)
                }
            }
        }
    }
    Loader{
        id:multiMedia_loader
        x:160
        y:0
        visible: false
        asynchronous: true
    }
    Component{
        id:multiMediaComponent
        MultiMediaWidget{
            id:multiMediaWidget
            visible: true
            objectName: "multiMediaWidgetObject"
        }
    }

    Loader
    {
        id:phoneLink_loader
        x:160
        y:0
        visible: false
        asynchronous: true
    }
    Component
    {
        id:phoneLinkComponent
        PhoneLinkWidget{
            id:phoneLinkWidget
            visible: true
            objectName: "phoneLinkWidgetObject"
            onPhoneLinkStatusChanged:
            {
                root.phoneLinkStatus = phoneLinkWidget.phoneLinkStatus
            }
        }
    }
    Loader
    {
        id:telephone_loader
        x:160
        y:0
        visible: false
        asynchronous: true
    }
    Component
    {
        id:telephoneComponent
        TelephoneWidget{
            id:telephoneWidget
            visible: true
            objectName: "telephoneWidgetObject"
        }

    }

    Loader
    {
        id:videoMedia_loader
        x:160
        y:0
        visible: false
        asynchronous: true
    }
    Component
    {
        id:videoMediaComponent
        VideoMediaWidget{
            id:videoMediaWidget
            visible: true
            objectName: "videoMediaWidgetObject"
            onVideoFullScreenTypeChanged:{
                root.mainFullScreenType = videoMediaWidget.videoFullScreenType
            }
            onIsImageFullScreenChanged: {
                root.isImageFullScreen = videoMediaWidget.isImageFullScreen
            }
        }
    }

    Loader
    {
        id:setting_loader
        x:160
        y:0
        visible: false
        asynchronous: true
    }
    Component
    {
        id:settingComponent
        SettingWidget{
            id:settingWidget
            visible: true
            objectName: "settingWidgetObject"
        }
    }


    Loader
    {
        id:keyBoard_loader
        x:0
        y:270
        visible: false
        asynchronous: true
        objectName: "keyBoardLoaderObject"
    }
    Component
    {
        id:keyBoardComponent
        KeyBoardWidget{
            id:keyBoardWidget
            visible: true
            objectName: "keyBoardWidgetObject"
        }
    }

    Loader
    {
        id:aux_loader
        x:0
        y:0
        visible: false
        asynchronous: true
        objectName: "auxLoaderObject"
    }
    Component
    {
        id:auxComponent
        AuxWidget{
            id:auxWidget
            visible: true
            objectName: "auxWidgetObject"
        }
    }

    Loader
    {
        id:back_loader
        x:0
        y:0
        visible: false
        asynchronous: true
        objectName: "backLoaderObject"
    }
    Component
    {
        id:backComponent
        BackWidget{
            id:backWidget
            visible: true
            objectName: "backWidgetObject"
        }
    }

    function onMultiMediaBtnClicked()
    {
        if(root.typeStatus != 1)
        {
            root.typeStatus = 1
        }
        else
        {
            root.typeStatus = 0
        }
    }
    function onPhoneLinkClicked()
    {
        if(root.typeStatus != 2)
        {
            root.typeStatus = 2
        }
        else
        {
            root.typeStatus = 0
        }
    }

    function onTelClicked()
    {
        if(root.typeStatus != 3)
        {
            root.typeStatus = 3
        }
        else
        {
            root.typeStatus = 0
        }
    }

    function onAuxBtnClicked()
    {
        if(root.typeStatus != 4)
        {
            root.typeStatus = 4
        }
        else
        {
            root.typeStatus = 0
        }
    }

    function onSettingBtnClicked()
    {
        if(root.typeStatus != 5)
        {
            root.typeStatus = 5
        }
        else
        {
            root.typeStatus = 0
        }
    }
    onTypeStatusChanged: {
        console.log("+++++++typeStatus:+++++++++",typeStatus);
        switch(root.typeStatus){
            case 0:
                homeWidget.visible = true;
                bgRect.visible = true;
                footerWidget.visible = true;
                multiMedia_loader.visible = false;
                phoneLink_loader.visible = false;
                telephone_loader.visible = false;
                videoMedia_loader.visible = false;
                setting_loader.visible = false;
                keyBoard_loader.visible =false;
                back_loader.visible = false;
                aux_loader.visible = false;
                break;
             case 1:
                 homeWidget.visible = false;
                 bgRect.visible = false;
                 footerWidget.visible = false;
                 multiMedia_loader.visible = true;
                 phoneLink_loader.visible = false;
                 telephone_loader.visible = false;
                 videoMedia_loader.visible = false;
                 setting_loader.visible = false;
                 keyBoard_loader.visible =false;
                 back_loader.visible = false;
                 aux_loader.visible = false;
                 break;
             case 2:
                 homeWidget.visible = false;
                 bgRect.visible = false;
                 multiMedia_loader.visible = false;
                 phoneLink_loader.visible = true;
                 footerWidget.visible = false;
                 telephone_loader.visible = false;
                 videoMedia_loader.visible = false;
                 setting_loader.visible = false;
                 keyBoard_loader.visible =false;
                 back_loader.visible = false;
                 aux_loader.visible = false;
                 break;
             case 3:
                 homeWidget.visible = false;
                 bgRect.visible = false;
                 footerWidget.visible = false;
                 multiMedia_loader.visible = false;
                 phoneLink_loader.visible = false;
                 telephone_loader.visible = true;
                 videoMedia_loader.visible = false;
                 setting_loader.visible = false;
                 keyBoard_loader.visible =false;
                 back_loader.visible = false;
                 aux_loader.visible = false;
                 break;
             case 4:
                 homeWidget.visible = false;
                 bgRect.visible = false;
                 footerWidget.visible = false;
                 multiMedia_loader.visible = false;
                 phoneLink_loader.visible = false;
                 telephone_loader.visible = false;
                 videoMedia_loader.visible = true;
                 setting_loader.visible = false;
                 keyBoard_loader.visible =false;
                 back_loader.visible = false;
                 aux_loader.visible = false;
                 break;
             case 5:
                 homeWidget.visible = false;
                 bgRect.visible = false;
                 footerWidget.visible = false;
                 multiMedia_loader.visible = false;
                 phoneLink_loader.visible = false;
                 telephone_loader.visible = false;
                 videoMedia_loader.visible = false;
                 setting_loader.visible = true;
                 keyBoard_loader.visible =false;
                 back_loader.visible = false;
                 aux_loader.visible = false;
                 break;
             default:
                 break;
        }
    }
    onMainFullScreenTypeChanged: {
        if(root.mainFullScreenType === 0)
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible = true;
            videoMedia_loader.visible = true;
            setting_loader.visible = false;
            back_loader.visible = false;
            aux_loader.visible = false;
        }
        else
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible = false;
            videoMedia_loader.visible = true;
            setting_loader.visible = false;
            back_loader.visible = false;
            aux_loader.visible = false;
        }
    }
    onIsImageFullScreenChanged: {
        if(root.isImageFullScreen === false)
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            videoMedia_loader.x = 160
            videoMedia_loader.width = 1920
            toolWidget.visible = true;
            videoMedia_loader.visible = true;
            setting_loader.visible = false;
            back_loader.visible = false;
            aux_loader.visible = false;
        }
        else
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible = false;
            videoMedia_loader.x = 0
            videoMedia_loader.width = 1760
            videoMedia_loader.visible = true;
            setting_loader.visible = false;
            back_loader.visible = false;
            aux_loader.visible = false;
        }
    }

    onPhoneLinkStatusChanged: {
        switch(root.phoneLinkStatus)
        {
            case 0:
                homeWidget.visible = true;
                bgRect.visible = true;
                footerWidget.visible = true;
                multiMedia_loader.visible = false;
                phoneLink_loader.visible = false;
                telephone_loader.visible = false;
                toolWidget.visible = true;
                videoMedia_loader.visible = false;
                setting_loader.visible = false;
                globalRect_loader.visible  = false;
                aux_loader.visible = false;
                back_loader.visible = false;
                break;
            case 1:
                homeWidget.visible = false;
                bgRect.visible = false;
                footerWidget.visible = false;
                multiMedia_loader.visible = false;
                phoneLink_loader.visible = true;
                telephone_loader.visible = false;
                toolWidget.visible = true;
                videoMedia_loader.visible = false;
                setting_loader.visible = false;
                globalRect_loader.visible  = false;
                aux_loader.visible = false;
                back_loader.visible = false;
                break;
            case 2:
                homeWidget.visible = false;
                bgRect.visible = false;
                footerWidget.visible = false;
                multiMedia_loader.visible = false;
                phoneLink_loader.visible = false;
                telephone_loader.visible = false;
                toolWidget.visible = false;
                videoMedia_loader.visible = false;
                setting_loader.visible = false;
                globalRect_loader.visible  = true;
                aux_loader.visible = false;
                back_loader.visible = false;
                break;
            default:
                break;
        }
    }

    onAuxStatusChanged: {
        if(root.auxStatus == 0)
        {
            homeWidget.visible = true;
            bgRect.visible = true;
            footerWidget.visible = true;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible = true;
            videoMedia_loader.visible = false;
            setting_loader.visible = false;
            globalRect_loader.visible  = false;
            aux_loader.visible = false;
            keyBoard_loader.visible =false;
            back_loader.visible = false;
        }
        else if(root.auxStatus == 1)
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible      = false;
            videoMedia_loader.visible = false;
            setting_loader.visible  = false;
            globalRect_loader.visible     = false;
            aux_loader.visible      = true;
            back_loader.visible     = true;
            keyBoard_loader.visible =false;
        }
        else if(root.auxStatus == 2)
        {
            homeWidget.visible = false;
            bgRect.visible = false;
            footerWidget.visible = false;
            multiMedia_loader.visible = false;
            phoneLink_loader.visible = false;
            telephone_loader.visible = false;
            toolWidget.visible = false;
            videoMedia_loader.visible = false;
            setting_loader.visible = false;
            globalRect_loader.visible  = false;
            aux_loader.visible = false;
            keyBoard_loader.visible =false;
            back_loader.visible = true;
        }
    }
    function onMultiMediaLoader()
    {
        if(multiMedia_loader.status === Loader.Ready)
        {
            root.multiMediaComponentLoaderComplete();
        }
    }

    function onPhoneLinkLoader()
    {
        if(phoneLink_loader.status === Loader.Ready)
        {
            root.phoneLinkComponentLoaderComplete();
        }
    }
    function onTelephoneLoader()
    {
        if(telephone_loader.status === Loader.Ready)
        {
            root.telephoneComponentLoaderComplete();
        }
    }

    function onVideoMediaLoader()
    {
        if(videoMedia_loader.status === Loader.Ready)
        {
            root.videoMediaComponentLoaderComplete();
        }
    }

    function onSettingLoader()
    {
        if(setting_loader.status === Loader.Ready)
        {
            root.settingComponentLoaderComplete();
        }
    }

    function onKeyBoardLoader()
    {
        if(keyBoard_loader.status === Loader.Ready)
        {
            root.keyBoardComponentLoaderComplete();
        }
    }

    function onAuxLoader()
    {
        if(aux_loader.status === Loader.Ready)
        {
            root.auxComponentLoaderComplete()
        }
    }

    function onBackLoader()
    {
        if(back_loader.status === Loader.Ready)
        {
            root.backComponentLoaderComplete()
        }
    }
    Component.onCompleted: {
        multiMedia_loader.sourceComponent = multiMediaComponent;
        multiMedia_loader.statusChanged.connect(onMultiMediaLoader);
        phoneLink_loader.sourceComponent  = phoneLinkComponent;
        phoneLink_loader.statusChanged.connect(onPhoneLinkLoader);
        telephone_loader.sourceComponent  = telephoneComponent;
        telephone_loader.statusChanged.connect(onTelephoneLoader);
        videoMedia_loader.sourceComponent = videoMediaComponent;
        videoMedia_loader.statusChanged.connect(onVideoMediaLoader);
        setting_loader.sourceComponent    = settingComponent;
        setting_loader.statusChanged.connect(onSettingLoader);
        keyBoard_loader.sourceComponent   = keyBoardComponent;
        keyBoard_loader.statusChanged.connect(onKeyBoardLoader);
        aux_loader.sourceComponent        = auxComponent;
        aux_loader.statusChanged.connect(onAuxLoader);
        back_loader.sourceComponent       = backComponent;
        back_loader.statusChanged.connect(onBackLoader);
        globalRect_loader.sourceComponent = globalRectComponent;
        toolWidget.multiMediaBtnClicked.connect(onMultiMediaBtnClicked)
        toolWidget.auxBtnClicked.connect(onAuxBtnClicked)
        toolWidget.settingBtnClicked.connect(onSettingBtnClicked)
        toolWidget.phoneLinkClicked.connect(onPhoneLinkClicked)
        toolWidget.telClicked.connect(onTelClicked)
    }

}
