import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal nextPageBtnClicked
    signal prevPageBtnClicked 
    BtSwitchSetting{
        id:btSwitchSetting
        objectName: "btSwitchSettingObject"
        onNextPageBtnClicked: {
            root.nextPageBtnClicked()
        }
    }
    BtConnectWidget{
        id:btConnectWidget
        objectName: "btConnectWidgetObject"
        visible: false
        onPrevPageBtnClicked: {
            root.prevPageBtnClicked()
        }
    }

    onPrevPageBtnClicked: {
        btSwitchSetting.visible = true;
        btConnectWidget.visible = false;
    }
    onNextPageBtnClicked: {
        btConnectWidget.visible = true;
        btSwitchSetting.visible = false;
    }
}
