import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 880
    height: 720
    visible: true
    signal nextPageBtnClicked
    signal prevPageBtnClicked

    SoundValueSettingWidget{
        id:soundValueSettingWidget
        objectName: "soundValueSettingWidgetObject"
        onNextPageBtnClicked: {
            root.nextPageBtnClicked()
        }
    }
    SoundBalanceSettingWidget{
        id:soundBalanceSettingWidget
        objectName: "soundBalanceSettingWidgetObject"
        visible: false
        onPrevPageBtnClicked: {
            root.prevPageBtnClicked()
        }
    }
    onPrevPageBtnClicked: {
        soundValueSettingWidget.visible  = true;
        soundBalanceSettingWidget.visible = false;
    }
    onNextPageBtnClicked: {
        soundValueSettingWidget.visible  = false;
        soundBalanceSettingWidget.visible = true;
    }

}
