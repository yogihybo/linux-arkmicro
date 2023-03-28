#include "QmlLauncher.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/StatusBar.h"
#include "UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModel.h"
#include "UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModel.h"
#include "UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModelData.h"
#include "UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModelData.h"
#include "UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusic/UsbMusicListModelData.h"
#include "UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdMusic/SdMusicListModelDdata.h"
#include "UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbVideo/UsbVideoModelData.h"
#include "UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdVideo/SdVideoModelData.h"
#include "UserInterface/MainWidget/DiskWidget/MusicInformation/MusicInformationWidget.h"
#include "UserInterface/MainWidget/DiskWidget/VideoWidget/ImageWidget/ImageWidget.h"
#include "UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbImage/UsbImageModelData.h"
#include "UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdImage/SdImageModelData.h"
#include "UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothConnectWidget/BluetoothNameModelData.h"
#include "BusinessLogic/Widget.h"
#include "UserInterface/MainWidget/KeyBoardModelData/KeyBoardFirstRowModelData.h"
#include "UserInterface/MainWidget/KeyBoardModelData/KeyBoardSecondRowModelData.h"
#include "UserInterface/MainWidget/KeyBoardModelData/KeyBoardThirdRowModelData.h"
#include "UserInterface/MainWidget/SettingWidget/WifiSettingWidget/WifiSettingWidgetModelData.h"
#include <QDebug>
QmlLauncher::QmlLauncher(QObject *parent) : QObject(parent)
{
    m_Engine = (QQmlApplicationEngine*)parent;
    QObject::connect(g_Setting,SIGNAL(onLanguageChanged()),this,SLOT(onLanguageChanged()));
}
void QmlLauncher::onLanguageChanged()
{
    qDebug()<<"+++++++onLanguageChanged++++++++++++";
    m_Engine->retranslate();
}
void QmlLauncher::qmlLauncherInit()
{
    //将homeWidget/ToolWidget/StatusBar的model加入到qml上下文
    setStatusBarContextProperty();
}

void QmlLauncher::setStatusBarContextProperty()
{
    qmlRegisterUncreatableType<myModel, 1>("com.test.model", 1, 0,
                                           "MyModel",
                                           "Cannot create MyModel");
    StatusBar* myStatusBar = new StatusBar(this);
    m_Engine->rootContext()->setContextProperty("myStatusBar",myStatusBar);

    qmlRegisterUncreatableType<UsbMusicListModel>("com.usbMusic.model", 1, 0,
                                           "UsbMusicListModel",
                                           "Cannot create UsbMusicListModel");
    UsbMusicListModelData* myUsbMusicListModelData = new UsbMusicListModelData(this);
    m_Engine->rootContext()->setContextProperty("myUsbMusicListModel",myUsbMusicListModelData);

    SdMusicListModelDdata* mySdMusicListModelData = new SdMusicListModelDdata(this);
    m_Engine->rootContext()->setContextProperty("mySdMusicListModelData",mySdMusicListModelData);

    UsbVideoModelData* myUsbVideoModelData = new UsbVideoModelData(this);
    m_Engine->rootContext()->setContextProperty("myUsbVideoModelData",myUsbVideoModelData);

    SdVideoModelData* mySdVideoModelData   = new SdVideoModelData(this);
    m_Engine->rootContext()->setContextProperty("mySdVideoModelData",mySdVideoModelData);

    UsbImageModelData* myUsbImageModelData = new UsbImageModelData(this);
    m_Engine->rootContext()->setContextProperty("myUsbImageModelData",myUsbImageModelData);

    SdImageModelData* mySdImageModelData   = new SdImageModelData(this);
    m_Engine->rootContext()->setContextProperty("mySdImageModelData",mySdImageModelData);

    BluetoothNameModelData* myBtNameModelData = new BluetoothNameModelData(this);
    m_Engine->rootContext()->setContextProperty("myBtNameModelData",myBtNameModelData);

    qmlRegisterUncreatableType<PhoneBookModel>("com.phoneBook.model", 1, 0,
                                           "PhoneBookModel",
                                           "Cannot create PhoneBookModel");
    PhoneBookModelData* myPhoneBookModelData = new PhoneBookModelData(this);
    m_Engine->rootContext()->setContextProperty("myPhoneBookModelData",myPhoneBookModelData);
    qmlRegisterUncreatableType<AllCallLogModel>("com.AllCall.model", 1, 0,
                                           "AllCallLogModel",
                                           "Cannot create AllCallLogModel");
    AllCallLogModelData* myAllCallLogModelData = new AllCallLogModelData(this);
    m_Engine->rootContext()->setContextProperty("myAllCallLogModelData",myAllCallLogModelData);
    MusicInformationWidget* CodeImage = new MusicInformationWidget(this);
    m_Engine->rootContext()->setContextProperty("CodeImage", CodeImage);
    m_Engine->addImageProvider(QLatin1String("CodeImg"), CodeImage->getImageProvider());

    KeyBoardFirstRowModelData* myFirstRowModelData = new KeyBoardFirstRowModelData(this);
    m_Engine->rootContext()->setContextProperty("myFirstRowModelData",myFirstRowModelData);

    KeyBoardSecondRowModelData* mySecondRowModelData = new KeyBoardSecondRowModelData(this);
    m_Engine->rootContext()->setContextProperty("mySecondRowModelData",mySecondRowModelData);

    KeyBoardThirdRowModelData* myThirdRowModelData = new KeyBoardThirdRowModelData(this);
    m_Engine->rootContext()->setContextProperty("myThirdRowModelData",myThirdRowModelData);

    WifiSettingWidgetModelData* myWifiListModelData = new WifiSettingWidgetModelData(this);
    m_Engine->rootContext()->setContextProperty("myWifiListModelData",myWifiListModelData);

}
