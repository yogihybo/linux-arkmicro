#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "BusinessLogic/BusinessLogicUtility.h"
#include "AudioServiceProxy.h"
#include "Utility.h"
#include "UserInterfaceUtility.h"
#include <QDBusConnection>
#include <QProcess>
//#include <QGuiApplication>
#include "ArkApplication.h"
#include <QSettings>
#include <QFile>
static const QString audiofilepath("/data/Audio.ini");
class AudioPrivate
{
public:
    explicit AudioPrivate(Audio* parent);
    ~AudioPrivate();
    void initialize();
    void connectAllSlots();
    AudioSource m_Multimedia;
    AudioSource m_Aux;
    AudioSource m_Phone;
    Local::DbusServer::Audio* m_AudioServiceProxy;
    MuteItem m_MuteItem;
    EqualizerItem m_EqualizerItem;
private:
    Audio* m_Parent;
};

bool Audio::requestAudioSource(const AudioSource source)
{
    qDebug() << "Audio::requestAudioSource" << source << m_Private->m_Phone << m_Private->m_Multimedia;
    initializePrivate();
    static bool flag(false);
    bool ret = true;
    if (!flag) {
        flag = true;
        QStringList cmd;
        cmd << QString("-t") << AudioApplication;
        bool result = QProcess::startDetached(ArkApplication::applicationFilePath(), cmd);
        qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "result" << result;
    } else {
        if ((AS_BluetoothPhone != source)
                && (AS_CarplayPhone != source)
                && (AS_CarlifePhone != source)
                && (AS_AutoPhone != source)) {
            if (AS_Idle == m_Private->m_Phone) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                if (AS_Idle == m_Private->m_Aux) {
                    if (source != m_Private->m_Multimedia) {
                        qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                        AudioSource multimedia = m_Private->m_Multimedia;
                        if (AS_Aux == source) {
                            m_Private->m_Aux = AS_Aux;
                        } else {
                            m_Private->m_Aux = AS_Idle;
                            m_Private->m_Multimedia = source;
                        }
                        qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                        onHolderChange(multimedia, source);
                    }
                    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestAudioSource(source);
                    reply.waitForFinished();
                    if (reply.isError()) {
                        qDebug() << "method call" << __PRETTY_FUNCTION__ << __LINE__ << " failed" << reply.error();
                    }
                } else {
                    if (AS_Aux != source) {
                        m_Private->m_Multimedia = source;
                        ret = false;
                    }
                }
            } else {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                if (AS_Aux == source) {
                    m_Private->m_Aux = AS_Aux;
                } else {
                    m_Private->m_Multimedia = source;
                }
                ret = false;
            }
        } else {
            qDebug() << __PRETTY_FUNCTION__ << __LINE__;
            if ((AS_BluetoothPhone == source)
                    && (AS_BluetoothPhone != m_Private->m_Phone)) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                m_Private->m_Phone = AS_BluetoothPhone;
                onHolderChange(m_Private->m_Multimedia, source);
                QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestAudioSource(source);
                reply.waitForFinished();
                if (reply.isError()) {
                    qDebug() << "method call" << __PRETTY_FUNCTION__ << __LINE__ << " failed" << reply.error();
                }
            } else if ((AS_CarplayPhone == source)
                       && (AS_CarplayPhone != m_Private->m_Phone)) {
                m_Private->m_Phone = AS_CarplayPhone;
                onHolderChange(m_Private->m_Multimedia, source);
                QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestAudioSource(source);
                reply.waitForFinished();
                if (reply.isError()) {
                    qDebug() << "method call" << __PRETTY_FUNCTION__ << __LINE__ << " failed" << reply.error();
                }
            } else if ((AS_CarlifePhone == source)
                       && (AS_CarlifePhone != m_Private->m_Phone)) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                m_Private->m_Phone = AS_CarlifePhone;
                onHolderChange(m_Private->m_Multimedia, source);
                QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestAudioSource(source);
                reply.waitForFinished();
                if (reply.isError()) {
                    qDebug() << "method call" << __PRETTY_FUNCTION__ << __LINE__ << " failed" << reply.error();
                }
            } else if ((AS_AutoPhone == source)
                       && (AS_AutoPhone != m_Private->m_Phone)) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__;
                m_Private->m_Phone = AS_AutoPhone;
                onHolderChange(m_Private->m_Multimedia, source);
                QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestAudioSource(source);
                reply.waitForFinished();
                if (reply.isError()) {
                    qDebug() << "method call" << __PRETTY_FUNCTION__ << __LINE__ << " failed" << reply.error();
                }
            }
        }
    }
    if (!ret) {
        qDebug() << "Audio::requestAudioSource fail" << source << m_Private->m_Phone << m_Private->m_Multimedia;
    }
    return ret;
}

void Audio::releaseAudioSource(const AudioSource source)
{
    qDebug() << "Audio::releaseAudioSource" << source << m_Private->m_Multimedia;
    if ((AS_BluetoothPhone == source)
            || (AS_CarplayPhone == source)
            || (AS_CarlifePhone == source)
            || (AS_AutoPhone == source)) {
        qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
        if ((AS_BluetoothPhone == m_Private->m_Phone)
                || (AS_CarplayPhone == m_Private->m_Phone)
                || (AS_CarlifePhone == m_Private->m_Phone)
                || (AS_AutoPhone == m_Private->m_Phone)) {
            m_Private->m_Phone = AS_Idle;
            qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
            if (AS_Aux == m_Private->m_Aux) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
                onHolderChange(source, AS_Aux);
            } else {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
                if (AS_Idle != m_Private->m_Multimedia) {
                    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << m_Private->m_Multimedia;
                    onHolderChange(source, m_Private->m_Multimedia);
                }
            }
        }
    } else if (AS_Aux == source) {
        qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
        if (AS_Aux == m_Private->m_Aux) {
            m_Private->m_Aux = AS_Idle;
            qDebug() << __PRETTY_FUNCTION__ << __LINE__ ;
            if (AS_Idle != m_Private->m_Multimedia) {
                qDebug() << __PRETTY_FUNCTION__ << __LINE__ << m_Private->m_Multimedia;
                onHolderChange(source, m_Private->m_Multimedia);
            }
        }
    }
}
//调用amixer重新释放系统音量
void Audio::releaseVolume()
{
    system("amixer cset numid=24,iface=MIXER,name='PA Volume' 52");
    system("amixer cset numid=13,iface=MIXER,name='PA Input-Gain' 3");
}

int Audio::getcurrentvolumevalue()
{
    QFile filename(audiofilepath);
    int value = 0;
    if(filename.exists()){
        QSettings *volume_configfile = new QSettings(audiofilepath,QSettings::IniFormat);
        value = volume_configfile->value("Volume").toInt();
        delete volume_configfile;
    }else{
        qDebug()<<audiofilepath<<" is not exist";
    }
    qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"volume value :"<<value;
    return value;
}

void Audio::reset()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->reqesetReset();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::setvolumebalance()
{
    pid_t status = system("kmem 0xe4000008 0x7474");//调整音量增益幅度
    if (-1 == status)
    {
        qDebug("system error!");
    }
    else
    {
        qDebug("exit status value = [0x%x]\n", status);

        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                qDebug("'kmem 0xe4000008 0x7474' run shell script successfully.\n");
            }
            else
            {
                qDebug("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
            }
        }
        else
        {
            qDebug("exit status = [%d]\n", WEXITSTATUS(status));
        }
    }
}

void Audio::requestMuteToggole()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestMuteToggole();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << __PRETTY_FUNCTION__ << reply.error();
    }
}

void Audio::requestSpeaker(const int item)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestSpeaker(item);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << __PRETTY_FUNCTION__ << reply.error();
    }
}

void Audio::requestMute(const int item)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestMute(item);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << __PRETTY_FUNCTION__ << reply.error();
    }
}


void Audio::requestIncreaseVolume()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestIncreaseVolume();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::requestDecreaseVolume()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestDecreaseVolume();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::requestSetVolume(const int volume)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->requestSetVolume(volume);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::setEqualizerItem(const int item, const int bass, const int middle, const int treble)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->setEqualizerItem(item, bass, middle, treble);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::EqualizerItemTogglePrevious()
{
    initializePrivate();
    if (EI_Undefine == m_Private->m_EqualizerItem) {
        m_Private->m_EqualizerItem = AudioPersistent::getEqualizerItem(true);
    }
    EqualizerItem desireItem = EI_Undefine;
    switch (m_Private->m_EqualizerItem) {
    case EI_Standard: {
        desireItem = EI_Custom;
        break;
    }
    case EI_Popular: {
        desireItem = EI_Standard;
        break;
    }
    case EI_Classic: {
        desireItem = EI_Popular;
        break;
    }
    case EI_Jazz: {
        desireItem = EI_Classic;
        break;
    }
    default: {
        desireItem = EI_Jazz;
        break;
    }
    }
    g_Audio->setEqualizerItem(desireItem, EI_Undefine, EI_Undefine, EI_Undefine);

}

void Audio::EqualizerItemToggleNext()
{
    initializePrivate();
    if (EI_Undefine == m_Private->m_EqualizerItem) {
        m_Private->m_EqualizerItem = AudioPersistent::getEqualizerItem(true);
    }
    EqualizerItem desireItem = EI_Undefine;
    switch (m_Private->m_EqualizerItem) {
    case EI_Standard: {
        desireItem = EI_Popular;
        break;
    }
    case EI_Popular: {
        desireItem = EI_Classic;
        break;
    }
    case EI_Classic: {
        desireItem = EI_Jazz;
        break;
    }
    case EI_Jazz: {
        desireItem = EI_Custom;
        break;
    }
    default: {
        desireItem = EI_Standard;
        break;
    }
    }
    g_Audio->setEqualizerItem(desireItem, EI_Undefine, EI_Undefine, EI_Undefine);
}

void Audio::setSoundItem(const int item, const int left, const int right)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->setSoundItem(item, left, right);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

AudioSource Audio::getPhoneSource()
{
    return m_Private->m_Phone;
}

AudioSource Audio::getMultimediaSource()
{
    return m_Private->m_Multimedia;
}

AudioSource Audio::getAudioSource()
{
    bool ret1 = (AS_BluetoothPhone == m_Private->m_Phone)
            || (AS_CarplayPhone == m_Private->m_Phone)
            || (AS_CarlifePhone == m_Private->m_Phone)
            || (AS_AutoPhone == m_Private->m_Phone);
    bool ret2 = (!ret1) && (AS_Aux == m_Private->m_Aux);
//    qDebug()<<"++++++++ret1+++++++"<<ret1;
//    qDebug()<<"++++++++ret2+++++++"<<ret2;
//    qDebug()<<"++++++++m_Private->m_Phone+++++++"<<m_Private->m_Phone;
//    qDebug()<<"++++++++m_Private->m_Aux+++++++"<<m_Private->m_Aux;
//    qDebug()<<"++++++++m_Private->m_Multimedia+++++++"<<m_Private->m_Multimedia;
    AudioSource ret = (ret1? m_Private->m_Phone: (ret2? m_Private->m_Aux: m_Private->m_Multimedia));
//    qDebug()<<"++++++++ret+++++++"<<ret;
    return ret;
}

MuteItem Audio::getMute()
{
    return m_Private->m_MuteItem;
}


void Audio::faderOut()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->faderOut();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call faderOut failed" << reply.error();
    }
}

void Audio::AudioSwitchNone()
{
//    struct ArkControl control;
//    memset((void*)&control, 0, sizeof(struct ArkControl));
//    struct ArkProtocol protocol;
//    memset((void*)&protocol, 0, sizeof(struct ArkProtocol));
//    protocol.type = AT_Control;
//    protocol.direction = AD_Setter;
//    protocol.data = &control;
//    control.type = ACLT_RequestSourceSwitch;
//    unsigned char data[1] = {0x00};
//    control.data = data;
//    control.length = 1;
//    g_Setting->setArkProtocol(ArkProtocolWrapper(protocol));
}

int Audio::open_amixer_mode(int stream_name, int ctrlname)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->open_amixer_mode(stream_name,ctrlname);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::close_amixer_mode()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->close_amixer_mode();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call faderOut failed" << reply.error();
    }
}

int Audio::get_amixersoftmaster_volume()
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->get_amixersoftmaster_volume();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call faderOut failed" << reply.error();
    }
}

void Audio::set_amixersoftmaster_volume(int volume)
{
    initializePrivate();
    QDBusPendingReply<> reply = m_Private->m_AudioServiceProxy->set_amixersoftmaster_volume(volume);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "method call customEvent failed" << reply.error();
    }
}

void Audio::onServiceUnregistered(const QString &service)
{
    qDebug() << "Audio::onServiceUnregistered" << service;
    if (ArkMicroAudioService == service) {
        g_DbusService->startService(ArkMicroAudioService);
    }
}

void Audio::onMuteChangeHandler(const MuteItem mute)
{
    m_Private->m_MuteItem = mute;
    onMuteChange(m_Private->m_MuteItem);
}

void Audio::onEqualizerItemChangeHandler(const int item, const int bass, const int middle, const int treble)
{
    m_Private->m_EqualizerItem = item;
}

Audio::Audio(QObject *parent)
    : QObject(parent)
    , m_Private(new AudioPrivate(this))
{
}

Audio::~Audio()
{
}

void Audio::initializePrivate()
{
    if (NULL == m_Private) {
        m_Private.reset(new AudioPrivate(this));
    }
}
AudioPrivate::AudioPrivate(Audio *parent)
    : m_Parent(parent)
{
#if defined(ENTER_HOME_FIRST)
    m_Multimedia = AS_Idle;
#else
	m_Multimedia = AS_Radio;
#endif
    //m_Multimedia = AS_Radio;AS_Idle
    m_Aux = AS_Idle;
    m_Phone = AS_Idle;
    m_AudioServiceProxy = NULL;
    m_MuteItem = MI_Unmute;
    m_EqualizerItem = EI_Undefine;
    initialize();
    connectAllSlots();
}

AudioPrivate::~AudioPrivate()
{
}

void AudioPrivate::initialize()
{
    if (NULL == m_AudioServiceProxy) {
        qDebug()<<"+++++++++AudioPrivate::initialize()+++++++++++";
        m_AudioServiceProxy = new Local::DbusServer::Audio(ArkMicroAudioService,
                                                           ArkMicroAudioPath,
                                                           QDBusConnection::sessionBus(),
                                                           m_Parent);
    }
}

void AudioPrivate::connectAllSlots()
{
    if (NULL != m_AudioServiceProxy) {
        connectSignalAndSignalByNamesake(m_AudioServiceProxy, m_Parent, ARKSENDER(onVolumeChange(const int)));
        connectSignalAndSignalByNamesake(m_AudioServiceProxy, m_Parent, ARKSENDER(onEqualizerItemChange(const int, const int, const int, const int)));
        connectSignalAndSignalByNamesake(m_AudioServiceProxy, m_Parent, ARKSENDER(onSoundItemChange(const int, const int, const int)));
        QObject::connect(m_AudioServiceProxy, ARKSENDER(onMuteChange(const int)),
                         m_Parent,            ARKRECEIVER(onMuteChangeHandler(const int)));
        QObject::connect(m_AudioServiceProxy, ARKSENDER(onEqualizerItemChange(const int, const int, const int, const int)),
                         m_Parent,            ARKRECEIVER(onEqualizerItemChangeHandler(const int, const int, const int, const int)));

    }
}

