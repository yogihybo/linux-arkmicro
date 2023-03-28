#include "AudioService.h"
#include "audiocontrol.h"
#include <QDebug>
#include <QTimer>
#include <limits>
#include <cmath>
#include <QDebug>

#define VOL_MAX_LEVEL 50
static int VOL_TABLE[]=
{
    0,  2,  3,  4,  6,  8, 10, 12, 14, 16,
    18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
    39, 42, 45, 48, 51, 54, 57, 60, 63, 66,
    69, 72, 75, 78, 81, 84, 87, 90, 95, 99,
};

static int Radio_VOL_Gain[] = {
    0,          //0
    2,2,2,   //1-3
    2,2,2,   //4-6
    3,3,3,   //7-9
    6,6,6,   //10-12
    5,5,5,   //13-15
    7,7,7,   //16-18
    7,7,7,   //19-21
    7,7,7,   //22-24
    7,7,7,   //25-27
    7,7,7    //28-30
};

static int Navi_VOL_Gain[] = {
    0,       //0
    0,0,0,   //1-3
    0,0,0,   //4-6
    0,0,0,   //7-9
    0,0,0,   //10-12
    0,0,0,   //13-15
    0,0,0,   //16-18
    0,0,0,   //19-21
    0,0,0,   //22-24
    0,0,0,   //25-27
    0,0,0    //28-30
};

static int BT_VOL_Gain[] = {
    0,          //0
    1,1,1,   //1-3
    5,5,5,   //4-6
    5,5,5,   //7-9
    5,5,5,   //10-12
    5,5,5,   //13-15
    5,5,5,   //16-18
    5,5,5,   //19-21
    5,5,5,   //22-24
    5,5,5,   //25-27
    5,5,5    //28-30
};



enum AudioCardType {
    ACT_Undefine = -1,
    ACT_Default,
    ACT_Ark169,
    ACT_Yaoxi,
    ACT_Luyuan169,
};

char* painputswitch_id = "";
char* paoutputvolume_id = "";
char* pamuteswitch_id = "";
char* paoutputfl_id = "";
char* paoutputfr_id = "";
char* paoutputrl_id = "";
char* paoutputrr_id = "";
char* paoutputbass_id = "";
char* paoutputmiddle_id = "";
char* paoutputtreble_id = "";
char* paoutputgain_id = "";

class AudioServicePrivate
{
public:
    explicit AudioServicePrivate(AudioService* parent);
    ~AudioServicePrivate();
    void initialize();
    void initializeArk169();
    void initializeYaoxi();
    void initializeLuyuan169();
    void initializeDefault();
    void controlMute(int value);
    void controlGain(int value);
    void controlBass(int value);
    void controlMiddle(int value);
    void controlTreble(int value);
    bool insmodKO(const QString& path);
    void SetInputAudio(AudioSource source);
    void SetOutputAudio(int volume);
    void SetOutputVolume(int volume);
    int pos2volume(int pos/* 0 ~ VOL_MAX_LEVEL*/);
    void setSoundItem(const int item, const int left, const int right, const bool write = true);
    void initializeFaderTimer();
    void faderOut();
private:
    Q_DECLARE_PUBLIC(AudioService)
    AudioService* const q_ptr;
    int	m_MinVol;
    int	m_MaxVol;
    float m_Step;
    int m_CurrentVolume;
    int m_CurrentSoundLeft;
    int m_CustomSoundLeft;
    int m_CurrentSoundRight;
    int m_CustomSoundRight;
    int m_CurrentBass;
    int m_CustomBass;
    int m_CurrentMiddle;
    int m_CustomMiddle;
    int m_CurrentTreble;
    int m_CustomTreble;
    AudioSource m_AudioSource;
    AudioCardType m_AudioCardType;
    MuteItem m_MuteItem;
    SpeakerSoundItem m_SpeakerSoundItem;
    QTimer* m_FaderTimer;
    float m_FaderStep;
};

AudioService::AudioService(QObject *parent)
    : QObject(parent)
    , d_ptr(new AudioServicePrivate(this))
{

}

AudioService::~AudioService()
{
    if (NULL != d_ptr) {
        delete d_ptr;
    }
}

void AudioService::requestAudioSource(const AudioSource source)
{
    Q_D(AudioService);
    if (source != d->m_AudioSource) {
        if (NULL != d->m_FaderTimer) {
            d->m_FaderTimer->stop();
        }
        d->m_AudioSource = source;
        if (SSI_PowerOn == d->m_SpeakerSoundItem) {
            if (MI_Unmute == d->m_MuteItem) {
                //                d->SetOutputVolume(d->m_CurrentVolume / 2);
                //                d->SetOutputVolume(d->m_CurrentVolume / 4);
                //                d->SetOutputVolume(d->m_CurrentVolume / 8);
                d->controlMute(1);
                //                d->SetOutputVolume(0);
            }
            d->SetInputAudio(source);
            if (MI_Unmute == d->m_MuteItem) {
                d->controlMute(0);
                d->faderOut();
                //                d->SetOutputVolume(d->m_CurrentVolume / 8);
                //                d->SetOutputVolume(d->m_CurrentVolume / 4);
                //                d->SetOutputVolume(d->m_CurrentVolume / 2);
                //                d->SetOutputVolume(d->m_CurrentVolume);
            }
        } else {
            d->SetInputAudio(source);
        }
    }
}

void AudioService::releaseAudioSource(const AudioSource source)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (source == d->m_AudioSource) {
        d->m_AudioSource = AS_Idle;
        d->SetInputAudio(d->m_AudioSource);
    }
}

void AudioService::reqesetReset()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    d->controlMute(1);
    AudioPersistent::reset();
}

void AudioService::requestMuteToggole()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (MI_Unmute == d->m_MuteItem) {
        d->m_MuteItem = MI_Mute;
        if (SSI_PowerOn == d->m_SpeakerSoundItem) {
            d->controlMute(1);
        }
    } else if (MI_Mute == d->m_MuteItem) {
        d->m_MuteItem = MI_Unmute;
        if (SSI_PowerOn == d->m_SpeakerSoundItem) {
            d->controlMute(0);
        }
        onVolumeChange(d->m_CurrentVolume);
    }
    onMuteChange(d->m_MuteItem);
}

void AudioService::requestSpeaker(const int item)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (item != d->m_SpeakerSoundItem) {
        d->m_SpeakerSoundItem = item;
        if (SSI_PowerOff == item) {
            if (ACT_Ark169 == d->m_AudioCardType) {
                d->controlMute(1);
            } else {
                d->SetOutputVolume(0);
            }
        } else if (SSI_PowerOn == item) {
            if (MI_Unmute == d->m_MuteItem) {
                d->SetOutputVolume(d->m_CurrentVolume);
                if (ACT_Ark169 == d->m_AudioCardType) {
                    d->controlMute(0);
                }
            } else {
                if (ACT_Ark169 == d->m_AudioCardType) {
                    d->controlMute(1);
                } else {
                    d->SetOutputVolume(0);
                }
            }
        }
    }
}

void AudioService::requestMute(const int item)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (item != d->m_MuteItem) {
        if (MI_Unmute == item) {
            d->m_MuteItem = MI_Unmute;
            if (ACT_Ark169 == d->m_AudioCardType) {
                if (SSI_PowerOn == d->m_SpeakerSoundItem) {
                    d->controlMute(0);
                }
            } else {
                if (SSI_PowerOn == d->m_SpeakerSoundItem) {
                    d->SetOutputVolume(d->m_CurrentVolume);
                }
            }
        } else if (MI_Mute == item) {
            d->m_MuteItem = MI_Mute;
            if (ACT_Ark169 == d->m_AudioCardType) {
                if (SSI_PowerOn == d->m_SpeakerSoundItem) {
                    d->controlMute(1);
                }
            } else {
                if (SSI_PowerOn == d->m_SpeakerSoundItem) {
                    d->SetOutputVolume(0);
                }
            }
        }
    }
}

void AudioService::requestIncreaseHalfVolume()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (ACT_Default == d->m_AudioCardType) {
        d->SetOutputVolume(d->m_CurrentVolume);
    }
}

void AudioService::requestDecreaseHalfVolume()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (ACT_Default == d->m_AudioCardType) {
        d->SetOutputVolume(d->m_CurrentVolume / 2);
    }
}

void AudioService::requestIncreaseVolume()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (MI_Unmute == d->m_MuteItem) {
        if (d->m_CurrentVolume < VOL_MAX_LEVEL) {
            ++d->m_CurrentVolume;
        }
        requestSetVolume(d->m_CurrentVolume);
    } else {
        requestMuteToggole();
    }
}

void AudioService::requestDecreaseVolume()
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (MI_Unmute == d->m_MuteItem) {
        if (d->m_CurrentVolume > 0) {
            --d->m_CurrentVolume;
        }
        requestSetVolume(d->m_CurrentVolume);
    } else {
        requestMuteToggole();
    }
}

void AudioService::requestSetVolume(const int volume)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    if (volume > VOL_MAX_LEVEL) {
        d->m_CurrentVolume = VOL_MAX_LEVEL;
    } else if (volume < 0) {
        d->m_CurrentVolume = 0;
    } else {
        d->m_CurrentVolume = volume;
    }
    if (SSI_PowerOn == d->m_SpeakerSoundItem) {
        d->SetOutputVolume(d->m_CurrentVolume);
    }
    AudioPersistent::setVolume(d->m_CurrentVolume);
    onVolumeChange(d->m_CurrentVolume);
    if (MI_Mute == d->m_MuteItem) {
        d->m_MuteItem = MI_Unmute;
        if (SSI_PowerOn == d->m_SpeakerSoundItem) {
            d->controlMute(0);
        }
    }
}

void AudioService::setEqualizerItem(const int item, const int bass, const int middle, const int treble)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    int bassValue = bass;
    int middleValue = middle;
    int trebleValue = treble;
    AudioPersistent::setEqualizerItem(item);
    switch (item) {
    case EI_Standard: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 7;
        break;
    }
    case EI_Popular: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 11;
        break;
    }
    case EI_Classic: {
        bassValue = 11;
        middleValue = 9;
        trebleValue = 2;
        break;
    }
    case EI_Jazz: {
        bassValue = 10;
        middleValue = 11;
        trebleValue = 11;
        break;
    }
    default: {
        Q_D(AudioService);
        if (-1 == bassValue) {
            if (AudioPersistent::getBassEqualizer() != d->m_CustomBass) {
                d->m_CustomBass = AudioPersistent::getBassEqualizer();
            }
            bassValue = d->m_CustomBass;
        }
        AudioPersistent::setBassEqualizer(bassValue);
        if (-1 == middleValue) {
            if (AudioPersistent::getMiddleEqualizer() != d->m_CustomMiddle) {
                d->m_CustomMiddle = AudioPersistent::getMiddleEqualizer();
            }
            middleValue = d->m_CustomMiddle;
        }
        AudioPersistent::setMiddleEqualizer(middleValue);
        if (-1 == trebleValue) {
            if (AudioPersistent::getTrebleEqualizer() != d->m_CustomTreble) {
                d->m_CustomTreble = AudioPersistent::getTrebleEqualizer();
            }
            trebleValue = d->m_CustomTreble;
        }
        AudioPersistent::setTrebleEqualizer(trebleValue);
        break;
    }
    }
    bool change = false;
    if (d->m_CurrentBass != bassValue) {
        d->m_CurrentBass = bassValue;
        change = true;
    }
    if (d->m_CurrentMiddle != middleValue) {
        d->m_CurrentMiddle = middleValue;
        change = true;
    }
    if (d->m_CurrentTreble != trebleValue) {
        d->m_CurrentTreble = trebleValue;
        change = true;
    }
    if (change) {
        Q_D(AudioService);
        switch (d->m_AudioCardType) {
        case ACT_Ark169: {
            d->controlBass(bassValue * 2);
            d->controlMiddle(middleValue * 2);
            d->controlTreble(trebleValue * 2);
            break;
        }
        case ACT_Yaoxi:
        case ACT_Luyuan169: {
            d->controlBass(bassValue);
            d->controlMiddle(middleValue);
            d->controlTreble(trebleValue);
            break;
        }
        default: {
            break;
        }
        }
    }
    onEqualizerItemChange(item, bassValue, middleValue, trebleValue);
}

void AudioService::setSoundItem(const int item, const int left, const int right)
{
    Q_D(AudioService);
    if (NULL != d->m_FaderTimer) {
        d->m_FaderTimer->stop();
    }
    d->setSoundItem(item, left, right);
}

void AudioService::faderOut()
{
    Q_D(AudioService);
    d->faderOut();
}

int AudioService::open_amixer_mode(int stream_name, int ctrlname)
{
     AudioControl control;
     control.ALSAMixerOpen(stream_name,ctrlname);
     return 0;
}

void AudioService::close_amixer_mode()
{
    AudioControl control;
    control.ALSAMixerClose();
}

int AudioService::get_amixersoftmaster_volume()
{
    AudioControl control;
    int volume = control.ArkALSADuckMixerGetVolume();
    return volume;
}

void AudioService::set_amixersoftmaster_volume(int volume)
{
    AudioControl control;
    control.ArkALSADuckMixerSetVolume(volume);
}

void AudioService::onTimeout()
{
    Q_D(AudioService);
    d->m_FaderStep += 0.1f;
    d->SetOutputVolume(d->m_CurrentVolume * d->m_FaderStep);
    if (fabs(1.0f - d->m_FaderStep) > std::numeric_limits<float>::epsilon()) {
        d->m_FaderTimer->start();
    }
}

AudioServicePrivate::AudioServicePrivate(AudioService *parent)
    : q_ptr(parent)
{
    m_MinVol = 0;
    m_MaxVol = 120;
    m_Step = 1.0f;
    m_CurrentVolume = 15;
    m_CustomSoundLeft = AudioPersistent::getLeftSound();
    m_CurrentSoundLeft = m_CustomSoundLeft;
    m_CustomSoundRight = AudioPersistent::getRightSound();
    m_CurrentSoundRight = m_CustomSoundRight;
    m_CurrentBass = 0;
    m_CustomBass = AudioPersistent::getBassEqualizer();
    m_CurrentMiddle = 0;
    m_CustomMiddle = AudioPersistent::getMiddleEqualizer();
    m_CurrentTreble = 0;
    m_CustomTreble = AudioPersistent::getTrebleEqualizer();
    m_AudioSource = AS_Idle;
    m_AudioCardType = ACT_Default;
    m_MuteItem = MI_Unmute;
    m_SpeakerSoundItem = SSI_PowerOn;
    m_FaderTimer = NULL;
    m_FaderStep = 1.0f;
    initialize();
}

AudioServicePrivate::~AudioServicePrivate()
{
}

void AudioServicePrivate::initialize()
{
    Q_Q(AudioService);
    //qDebug()<<"++++[AudioServicePrivate::initialize():544]++++="<<QString(qgetenv("PROTOCOL_ID")).size();
    bool ret = QDBusConnection::sessionBus().registerService(ArkMicroAudioService);
    ret = ret && QDBusConnection::sessionBus().registerObject(ArkMicroAudioPath,
                                                              q,
                                                              QDBusConnection::ExportNonScriptableSignals
                                                              | QDBusConnection::ExportNonScriptableSlots);
    if (!ret) {
        qCritical() << "AudioService Register QDbus failed!";
        exit(EXIT_FAILURE);
    }
//    bool exists(QFile::exists("/tmp/audio"));
//    ret = !exists;
//    ret = ret && insmodKO(QString("/lib/modules/3.4.0/kernel/drivers/ark/audio/ark-cs4334-codec.ko"));
//    ret = ret && insmodKO(QString("/lib/modules/3.4.0/kernel/drivers/ark/audio/ark-sddac-codec.ko"));
//    ret = ret && insmodKO(QString("/lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-i2s.ko"));
//    ret = ret && insmodKO(QString("/lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-pcm-dma.ko"));
//    ret = ret && insmodKO(QString("/lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-sddac.ko"));
        if (QString(qgetenv("PROTOCOL_ID")).contains(QString("ark169"))
                || QString(qgetenv("PROTOCOL_ID")).contains(QString("dacheng169"))
                || QString(qgetenv("PROTOCOL_ID")).contains(QString("mingshang"))
                || QString(qgetenv("PROTOCOL_ID")).contains(QString("huixin"))
                || QString(qgetenv("PROTOCOL_ID")).size() == 0) {
            m_AudioSource = AS_Idle;//modify by wandz at20190729 for radio first mute
            m_AudioCardType = ACT_Ark169;
            m_Step = 2.000000f;
            m_CurrentVolume = AudioPersistent::getVolume();
            qDebug()<<"++++++++m_CurrentVolume++++++"<<m_CurrentVolume;
            if (m_CurrentVolume > 20) {
                m_CurrentVolume = 20;
            }
            AudioPersistent::setVolume(m_CurrentVolume);
            if (QString(qgetenv("PROTOCOL_ID")).contains(QString("mingshang"))) {
                AudioPersistent::setVolume(m_CurrentVolume);
            }
            painputswitch_id = "numid=14,iface=MIXER,name='PA Input Select'";
            paoutputvolume_id = "numid=24,iface=MIXER,name='PA Volume'";
            pamuteswitch_id = "numid=22,iface=MIXER,name='PA Mute'";
            paoutputfl_id = "numid=16,iface=MIXER,name='PA Fader-FL'";
            paoutputfr_id = "numid=17,iface=MIXER,name='PA Fader-FR'";
            paoutputrl_id = "numid=18,iface=MIXER,name='PA Fader-RL'";
            paoutputrr_id = "numid=19,iface=MIXER,name='PA Fader-RR'";
            paoutputbass_id = "numid=4,iface=MIXER,name='EQ Bass'";
            paoutputmiddle_id = "numid=7,iface=MIXER,name='EQ Middle'";
            paoutputtreble_id = "numid=10,iface=MIXER,name='EQ Treble'";
            paoutputgain_id = "numid=13,iface=MIXER,name='PA Input-Gain'";
            initializeArk169();
        } else if (QString(qgetenv("PROTOCOL_ID")).contains(QString("yaoxi"))) {
            m_AudioSource = AS_Radio;
            m_AudioCardType = ACT_Yaoxi;
            m_Step = 3.175000f;
            m_CurrentVolume   = AudioPersistent::getVolume();
            painputswitch_id  = "numid=6,iface=MIXER,name='PA Input-Select'";
            paoutputvolume_id = "numid=5,iface=MIXER,name='PA Volume'";
            paoutputfl_id     = "numid=8,iface=MIXER,name='Speaker FL'";
            paoutputfr_id     = "numid=9,iface=MIXER,name='Speaker FR'";
            paoutputrl_id     = "numid=10,iface=MIXER,name='Speaker RL'";
            paoutputrr_id     = "numid=11,iface=MIXER,name='Speaker RR'";
            paoutputbass_id   = "numid=3,iface=MIXER,name='EQ Bass'";
            paoutputtreble_id = "numid=4,iface=MIXER,name='EQ Treble'";
            initializeYaoxi();
        } else if (QString(qgetenv("PROTOCOL_ID")).contains(QString("luyuan169"))) {
            m_AudioSource = AS_Radio;
            m_AudioCardType = ACT_Luyuan169;
            m_CurrentVolume = AudioPersistent::getVolume();
            if (m_CurrentVolume > 20) {
                m_CurrentVolume = 20;
                AudioPersistent::setVolume(m_CurrentVolume);
            }
            painputswitch_id = "numid=6,iface=MIXER,name='PA Input-Select'";
            paoutputvolume_id = "numid=5,iface=MIXER,name='PA Volume'";
            pamuteswitch_id = "";
            paoutputfl_id = "numid=8,iface=MIXER,name='Speaker FL'";
            paoutputfr_id = "numid=9,iface=MIXER,name='Speaker FR'";
            paoutputrl_id = "numid=10,iface=MIXER,name='Speaker RL'";
            paoutputrr_id = "numid=11,iface=MIXER,name='Speaker RR'";
            paoutputbass_id = "numid=3,iface=MIXER,name='EQ Bass'";
            paoutputmiddle_id = "";
            paoutputtreble_id = "numid=4,iface=MIXER,name='EQ Treble'";
            paoutputgain_id = "";
            initializeLuyuan169();
        } else {
            m_Step = 3.175000f;
            m_CurrentVolume = AudioPersistent::getVolume();
            initializeDefault();
        }
}

void AudioServicePrivate::initializeArk169()
{
    controlMute(1);
    AudioControl control;
    SetOutputVolume(m_CurrentVolume);
    setSoundItem(AudioPersistent::getSoundItem(), m_CustomSoundLeft, m_CustomSoundRight, false);
    EqualizerItem equalizerItem = AudioPersistent::getEqualizerItem();
    int bassValue;
    int middleValue;
    int trebleValue;
    switch (equalizerItem) {
    case EI_Standard: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 7;
        break;
    }
    case EI_Popular: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 11;
        break;
    }
    case EI_Classic: {
        bassValue = 11;
        middleValue = 9;
        trebleValue = 2;
        break;
    }
    case EI_Jazz: {
        bassValue = 10;
        middleValue = 11;
        trebleValue = 11;
        break;
    }
    default: {
        bassValue = m_CustomBass;
        middleValue = m_CustomMiddle;
        trebleValue = m_CustomTreble;
        break;
    }
    }
    m_CurrentBass = bassValue;
    m_CurrentMiddle = middleValue;
    m_CurrentTreble = trebleValue;
    controlBass(bassValue * 2);
    controlMiddle(middleValue * 2);
    controlTreble(trebleValue * 2);
    controlGain(0);
    control.Set(PAINPUTSWITCH_AUDIO, 0);
    controlMute(1);
}

void AudioServicePrivate::initializeYaoxi()
{
    AudioControl control;
    SetOutputVolume(m_CurrentVolume);
    setSoundItem(AudioPersistent::getSoundItem(), m_CustomSoundLeft, m_CustomSoundRight, false);

    EqualizerItem equalizerItem = AudioPersistent::getEqualizerItem();
    int bassValue;
    int middleValue;
    int trebleValue;
    switch (equalizerItem) {
    case EI_Standard: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 7;
        break;
    }
    case EI_Popular: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 11;
        break;
    }
    case EI_Classic: {
        bassValue = 11;
        middleValue = 9;
        trebleValue = 2;
        break;
    }
    case EI_Jazz: {
        bassValue = 10;
        middleValue = 11;
        trebleValue = 11;
        break;
    }
    default: {
        bassValue = m_CustomBass;
        middleValue = m_CustomMiddle;
        trebleValue = m_CustomTreble;
        break;
    }
    }
    controlBass(bassValue);
    controlMiddle(middleValue);
    controlTreble(trebleValue);
    control.Set(PAINPUTSWITCH_AUDIO, 1);
}

void AudioServicePrivate::initializeLuyuan169()
{
    AudioControl control;
    controlMute(1);
    SetOutputVolume(m_CurrentVolume);
    setSoundItem(AudioPersistent::getSoundItem(), m_CustomSoundLeft, m_CustomSoundRight, false);

    EqualizerItem equalizerItem = AudioPersistent::getEqualizerItem();
    int bassValue;
    int middleValue;
    int trebleValue;
    switch (equalizerItem) {
    case EI_Standard: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 7;
        break;
    }
    case EI_Popular: {
        bassValue = 7;
        middleValue = 7;
        trebleValue = 11;
        break;
    }
    case EI_Classic: {
        bassValue = 11;
        middleValue = 9;
        trebleValue = 2;
        break;
    }
    case EI_Jazz: {
        bassValue = 10;
        middleValue = 11;
        trebleValue = 11;
        break;
    }
    default: {
        bassValue = m_CustomBass;
        middleValue = m_CustomMiddle;
        trebleValue = m_CustomTreble;
        break;
    }
    }
    m_CurrentBass = bassValue;
    m_CurrentMiddle = middleValue;
    m_CurrentTreble = trebleValue;
    controlBass(bassValue);
    controlMiddle(middleValue);
    controlTreble(trebleValue);
    controlGain(10);
    control.Set(PAINPUTSWITCH_AUDIO, 1);
    controlMute(0);
}

void AudioServicePrivate::initializeDefault()
{
    AudioControl control;
    int min, max;
    control.GetRange(LEFT_AUDIO,&min, &max);
    m_MinVol = min;
    m_MaxVol = max;
    float distance = m_MaxVol - m_MinVol;
    m_Step = distance / VOL_MAX_LEVEL;
    control.Set(LEFT_AUDIO, max);
    control.GetRange(RIGHT_AUDIO,&min, &max);
    control.Set(RIGHT_AUDIO, max);
    control.Set(IRSTATUS_AUDIO, 0);
    control.Set(OUTPUT_AUDIO, 0);
    control.Set(FM_AUDIO, 885);
    SetInputAudio(AS_Idle);
    SetOutputAudio(m_CurrentVolume);
}

void AudioServicePrivate::controlMute(int value)
{
    switch (m_AudioCardType) {
    case ACT_Yaoxi:
    case ACT_Luyuan169: {
        break;
    }
    default: {
        AudioControl control;
        control.Set(PAMUTESWITCH_AUDIO, value);
    }
    }
}

void AudioServicePrivate::controlGain(int value)
{
   // qDebug()<<"++++++++0000value++++++++++"<<value;
    switch (m_AudioCardType) {
    case ACT_Yaoxi:
    case ACT_Luyuan169: {
        break;
    }
    default: {
        AudioControl control;
        control.Set(PAOUTPUTGAIN_AUDIO, value);
        break;
    }
    }
}

void AudioServicePrivate::controlBass(int value)
{
    switch (m_AudioCardType) {
    default: {
        AudioControl eqControl;
        eqControl.Set(PAOUTPUTBASS_AUDIO, value);
        break;
    }
    }
}

void AudioServicePrivate::controlMiddle(int value)
{
    switch (m_AudioCardType) {
    case ACT_Yaoxi:
    case ACT_Luyuan169: {
        break;
    }
    default: {
        AudioControl eqControl;
        eqControl.Set(PAOUTPUTMIDDLE_AUDIO, value);
        break;
    }
    }
}

void AudioServicePrivate::controlTreble(int value)
{
    switch (m_AudioCardType) {
    default: {
        AudioControl eqControl;
        eqControl.Set(PAOUTPUTTREBLE_AUDIO, value);
        break;
    }
    }
}

bool AudioServicePrivate::insmodKO(const QString &path)
{
    bool flag(QFile::exists(path));
    if (flag) {
        QString cmd = QString("insmod ") + QString(" ") + path;
        if (0 != system(cmd.toLocal8Bit().data())) {
            flag = false;
        }
    }
    return flag;
}

//unused                                    Item #0 'A_SINGLE'
//unused                                    Item #1 'B_SINGLE'
//bluetooth                                 Item #2 'C_SINGLE:BT'
//music,video,carplay,carlife...            Item #3 'D_SINGLE:NAVI'
//radio                                     Item #4 'E1_SINGLE:RADIO'
//aux in                                    Item #5 'E2_SINGLE:AUX'
//unused Item #6 'D_DIFF'
//unused Item #7 'E_FULL_DIFF'

void AudioServicePrivate::SetInputAudio(int source)
{
    AudioControl control;
    switch(source)  {
    case AS_Idle: {
        if (ACT_Ark169 == m_AudioCardType) {
        } else {
            controlMute(1);
            control.Set(STRAM_AUDIO, 2);
        }
    }
    case AS_Aux: {
        if (ACT_Ark169 == m_AudioCardType) {
            control.Set(PAINPUTSWITCH_AUDIO, 1);
        } else if ((ACT_Yaoxi == m_AudioCardType)
                   || (ACT_Luyuan169 == m_AudioCardType)) {
            control.Set(PAINPUTSWITCH_AUDIO, 3);
        } else {
            controlMute(1);
            control.Set(STRAM_AUDIO, 2);
        }
        break;
    }
    case AS_Radio: {
        if (ACT_Ark169 == m_AudioCardType) {
            control.Set(PAINPUTSWITCH_AUDIO, 1);
        } else if ((ACT_Yaoxi == m_AudioCardType)
                   || (ACT_Luyuan169 == m_AudioCardType)) {
            control.Set(PAINPUTSWITCH_AUDIO, 1);
        }
        break;
    }
    case AS_Music:
    case AS_Video:
    case AS_CarplayMusic:
    case AS_CarplayPhone:
    case AS_AutoMusic:
    case AS_CarlifeMusic:
    case AS_ECLinkMusic:
    case AS_HiCarMusic: {
        if (ACT_Ark169 == m_AudioCardType) {
            control.Set(PAINPUTSWITCH_AUDIO, 1);
        } else if ((ACT_Yaoxi == m_AudioCardType)
                   || (ACT_Luyuan169 == m_AudioCardType)) {
            control.Set(PAINPUTSWITCH_AUDIO, 0);
        } else {
            controlMute(0);
            control.Set(STRAM_AUDIO, 2);
        }
        break;
    }
    case AS_CarlifePhone:
    case AS_AutoPhone:
    case AS_ECLinkBluetoothMusic:
    case AS_HiCarBluetoothMusic:
    case AS_BluetoothMusic:
    case AS_BluetoothPhone: {
        if (ACT_Ark169 == m_AudioCardType) {
            control.Set(PAINPUTSWITCH_AUDIO, 1);
        } else if ((ACT_Yaoxi == m_AudioCardType)
                   || (ACT_Luyuan169 == m_AudioCardType)) {
            control.Set(PAINPUTSWITCH_AUDIO, 2);
        } else {
            controlMute(0);
            control.Set(STRAM_AUDIO, 3);
        }
        break;
    }
    default: {
        break;
    }
    }
    if (ACT_Ark169 == m_AudioCardType) {
        if (AS_Radio == source) {
            if(m_CurrentVolume < 16)
            {
                controlGain(Radio_VOL_Gain[m_CurrentVolume]);
            }
            else
            {
                controlGain(7);
            }
        } else if(AS_BluetoothMusic == source) {

            if(m_CurrentVolume < 4)
            {
                 controlGain(BT_VOL_Gain[m_CurrentVolume]);
            }
            else
            {
                 controlGain(5);
            }

        }else if(AS_BluetoothPhone == source  || AS_CarplayPhone == source
                 || AS_AutoPhone == source || AS_CarlifePhone == source)
        {
            controlGain(10);
        } else if(AS_Music == source || AS_Video == source){
            controlGain(0);
            //controlGain(Navi_VOL_Gain[m_CurrentVolume]);
        } else if (AS_Aux == source){
            if (QString(qgetenv("PROTOCOL_ID")).contains(QString("mingshang")))
            {
                controlGain(16);
            }
        }
        else {
            controlGain(0);
        }
    }
}

void AudioServicePrivate::SetOutputAudio(int volume)
{
    if (ACT_Ark169 == m_AudioCardType) {
    } else {
        AudioControl control;
        control.Set(FM_AUDIO, 0);
        control.Set(IRSTATUS_AUDIO, 0);
        control.Set(OUTPUT_AUDIO, 1);
        SetOutputVolume(volume);
    }
}

void AudioServicePrivate::SetOutputVolume(int volume)
{
#if 0
    AudioControl control;
    int nValue = pos2volume(volume);
    control.Set(AMP_AUDIO,0);
    usleep(10);
    control.Set(AMP_AUDIO,nValue/2);
    usleep(10);
    control.Set(AMP_AUDIO,nValue);
#else
    AudioControl control;
    int nValue = pos2volume(volume);
    switch (m_AudioCardType) {
    case ACT_Ark169:
    case ACT_Yaoxi:
    case ACT_Luyuan169: {
        control.Set(PAOUTPUTVOLUME_AUDIO, nValue);
        break;
    }
    default: {
        control.Set(LEFT_AUDIO,0);
        control.Set(RIGHT_AUDIO,0);
        usleep(10);
        control.Set(LEFT_AUDIO,nValue/2);
        control.Set(RIGHT_AUDIO,nValue/2);
        usleep(10);
        control.Set(LEFT_AUDIO,nValue);
        control.Set(RIGHT_AUDIO,nValue);
        break;
    }
    }
    //    if (ACT_Ark169 == m_AudioCardType) {
    //        //        control.Set(PAOUTPUTVOLUME_AUDIO, 0);
    //    } else {
    //        control.Set(LEFT_AUDIO,0);
    //        control.Set(RIGHT_AUDIO,0);
    //        usleep(10);
    //    }
    //    if (ACT_Ark169 == m_AudioCardType) {
    //        //        control.Set(PAOUTPUTVOLUME_AUDIO, nValue/2);
    //    } else {
    //        control.Set(LEFT_AUDIO,nValue/2);
    //        control.Set(RIGHT_AUDIO,nValue/2);
    //        usleep(10);
    //    }
    //    if (ACT_Ark169 == m_AudioCardType) {
    //        control.Set(PAOUTPUTVOLUME_AUDIO, nValue);
    //    } else {
    //        control.Set(LEFT_AUDIO,nValue);
    //        control.Set(RIGHT_AUDIO,nValue);
    //    }
#endif
}

int AudioServicePrivate::pos2volume(int pos)
{
    if (pos >= VOL_MAX_LEVEL) {
        pos = VOL_MAX_LEVEL;
        switch (m_AudioCardType) {
        case ACT_Yaoxi: {
            return pos;
            break;
        }
        case ACT_Luyuan169: {
            return 9 + pos;
            break;
        }
        case ACT_Ark169: {
            return VOL_MAX_LEVEL + 45;
            //return VOL_MAX_LEVEL + 45;
            break;
        }
        default: {
            return m_MaxVol;
            break;
        }
        }
    } else {
        switch (m_AudioCardType) {
        case ACT_Luyuan169: {
            if (0 == pos) {
                return 0;
            }
            return 9 + pos;
            break;
        }
        case ACT_Yaoxi: {
            return pos;
            break;
        }
        default: {
            if (0 == pos) {
                return 0;
            } else {
                //return pos*2 + 14;//modify by wandz 20190415 , for set default, min volume
                return pos + 45;
            }
            break;
        }
        }
    }
}

void AudioServicePrivate::setSoundItem(const int item, const int left, const int right, const bool write)
{
    if (write) {
        AudioPersistent::setSoundItem(item);
    }
    int leftValue = left;
    int rightValue = right;
    switch (item) {
    case SI_Master: {
        leftValue = -3;
        rightValue = -3;
        break;
    }
    case SI_Slave: {
        leftValue = 3;
        rightValue = -3;
        break;
    }
    case SI_RearLeft: {
        leftValue = -3;
        rightValue = 3;
        break;
    }
    case SI_RearRight: {
        leftValue = 3;
        rightValue = 3;
        break;
    }
    default: {
        if (-8 == leftValue) {
            leftValue = m_CustomSoundLeft;
        } else {
            m_CustomSoundLeft = leftValue;
        }
        if (-8 == rightValue) {
            rightValue = m_CustomSoundRight;
        } else {
            m_CustomSoundRight = rightValue;
        }
        if (write) {
            AudioPersistent::setLeftSound(leftValue);
            AudioPersistent::setRightSound(rightValue);
        }
        break;
    }
    }

    bool change = false;
    if (m_CurrentSoundLeft != leftValue) {
        m_CurrentSoundLeft = leftValue;
        change = true;
    }
    if (m_CurrentSoundRight != rightValue) {
        m_CurrentSoundRight = rightValue;
        change = true;
    }
    if (!write) {
        change = true;
    }
    if (change) {
        AudioControl control;
        int fl = 0;
        int fr = 0;
        int rl = 0;
        int rr = 0;
        switch (m_AudioCardType) {
        case ACT_Ark169: {
            qDebug()<<__PRETTY_FUNCTION__<<__LINE__<<"ark169";
            fl = 60 + round(sqrt(pow((leftValue - 7), 2) + pow((rightValue - 7), 2)));
            fr = 60 + round(sqrt(pow((leftValue + 7), 2) + pow((rightValue - 7), 2)));
            rl = 60 + round(sqrt(pow((leftValue - 7), 2) + pow((rightValue + 7), 2)));
            rr = 60 + round(sqrt(pow((leftValue + 7), 2) + pow((rightValue + 7), 2)));
            qDebug("last Audio:fl=%d fr=%d rl=%d rr=%d \n",fl,fr,rl,rr);
            switch (item) {
            case SI_Master:
                fl = 75;
                fr = 20;
                rl = 20;
                rr = 20;
                break;
            case SI_Slave:
                fl = 20;
                fr = 75;
                rl = 20;
                rr = 20;
                break;
            case SI_RearLeft:
                fl = 20;
                fr = 20;
                rl = 75;
                rr = 20;
                break;
            case SI_RearRight:
                fl = 20;
                fr = 20;
                rl = 20;
                rr = 75;
                break;
            default:
                if((left == 0) &&(right == 0))
                {
                    fl = 70;
                    fr = 70;
                    rl = 70;
                    rr = 70;
                }else if((left <= 0) && (right <= 0))
                {
                    fl = 75;
                    fr = fr +left*9;
                    rl = rl + left*9;
                    rr = rr + left*9;
                }else if ((left >= 0) &&(right <= 0))
                {
                    fl = fl + right*9;
                    fr = 75;
                    rl = rl + right*9;
                    rr = rr + right*9;
                }else if ((left <= 0) && (right >= 0))
                {
                    fl = fl + left*9;
                    fr = fr + left*9;
                    rl = 75;
                    rr = rr + left*9;
                }else if((left >= 0) && (right >= 0))
                {
                    fl = fl - left*9;
                    fr = fr - left*9;
                    rl = rl - left*9;
                    rr = 75;
                }
                break;
            }
            break;
        }
        case ACT_Yaoxi:
        case ACT_Luyuan169: {
#if 0
            fl = 11 + round(sqrt(pow((leftValue - 7), 2) + pow((rightValue - 7), 2)));
            fr = 11 + round(sqrt(pow((leftValue + 7), 2) + pow((rightValue - 7), 2)));
            rl = 11 + round(sqrt(pow((leftValue - 7), 2) + pow((rightValue + 7), 2)));
            rr = 11 + round(sqrt(pow((leftValue + 7), 2) + pow((rightValue + 7), 2)));
#else
            if((leftValue <= 0) && (rightValue <= 0))
            {
                fl = 31;
                fr = 31 + round(31*leftValue/7);
                rl = 31 + round(31*rightValue/7);
                rr = 31 - 31*round(sqrt(pow(leftValue, 2) + pow(rightValue, 2)))/round(sqrt(pow(7, 2) + pow(7, 2)));
            }
            else if((leftValue >= 0) && (rightValue <= 0))
            {
                fl = 31 - round(31*leftValue/7);
                fr = 31;
                rl = 31 - 31*round(sqrt(pow(leftValue, 2) + pow(rightValue, 2)))/round(sqrt(pow(7, 2) + pow(7, 2)));
                rr = 31 + round(31*rightValue/7);
            }
            else if((leftValue <= 0) && (rightValue >= 0))
            {
                fl = 31 - round(31*rightValue/7);
                fr = 31 - 31*round(sqrt(pow(leftValue, 2) + pow(rightValue, 2)))/round(sqrt(pow(7, 2) + pow(7, 2)));
                rl = 31;
                rr = 31 + round(31*leftValue/7);
            }
            else if((leftValue >= 0) && (rightValue >= 0))
            {
                fl = 31 - 31*round(sqrt(pow(leftValue, 2) + pow(rightValue, 2)))/round(sqrt(pow(7, 2) + pow(7, 2)));
                fr = 31 - round(31*rightValue/7);
                rl = 31 - round(31*leftValue/7);
                rr = 31;
            }
#endif
            break;
        }
        default: {
            break;
        }
        }
        qDebug()<<__PRETTY_FUNCTION__<<__LINE__<<"fl fr rl rr: "<<fl<<fr<<rl<<rr;
       // qDebug()<<"+++++++xxx0000lfp+++++++";
        control.Set(PAOUTPUTFL_AUDIO, fl);
        control.Set(PAOUTPUTFR_AUDIO, fr);
        control.Set(PAOUTPUTRL_AUDIO, rl);
        control.Set(PAOUTPUTRR_AUDIO, rr);
    }
    if (write) {
        Q_Q(AudioService);
        q->onSoundItemChange(item, leftValue, rightValue);
    }
}

void AudioServicePrivate::initializeFaderTimer()
{
    if (NULL == m_FaderTimer) {
        Q_Q(AudioService);
        m_FaderTimer = new QTimer(q);
        m_FaderTimer->setSingleShot(true);
        QObject::connect(m_FaderTimer, SIGNAL(timeout()),
                         q,            SLOT(onTimeout()));
        m_FaderTimer->setInterval(75);
    }
}

void AudioServicePrivate::faderOut()
{
    initializeFaderTimer();
    m_FaderStep = 0.0f;
    if (m_FaderTimer->isActive()) {
        m_FaderTimer->stop();
    }
    m_FaderTimer->start();
}
