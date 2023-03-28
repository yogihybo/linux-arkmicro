#ifndef AUDIOSERVICE_H
#define AUDIOSERVICE_H

#define AudioApplication             QString("-audio")
#define ArkMicroAudioService         QString("com.arkmicro.audio")
#define ArkMicroAudioPath            QString("/com/arkmicro/audio")
#define ArkMicroAudioInterface       QString("Local.DbusServer.Audio")
#define ArkMicroAudioRequest         QString("requestAudioSource")
#define ArkMicroAudioRelease         QString("releaseAudioSource")
//#define ArkMicroopen_amixer_mode     QString("open_amixer_mode")

#include "AudioServiceProxy.h"
#include "AudioPersistent.h"
#include <QObject>
#include <QScopedPointer>
typedef enum {
  UNKNOW_NAME = 0,
   AUDIO_PHONE_MUSIC = 1,
    AUDIO_PHONE_NAVI = 2,
    AUDIO_PHONE_TELL = 3,
    AUDIO_PHONE_TTS = 4,
}AUDIO_STREAM_NAME;
#define AUDIO_STREAM_NAME int

typedef enum {
  CTRL_UNKNOW_NAME = 0,
   CTRL_PHONE_MUSIC = 1,
    CTRL_PHONE_NAVI = 2,
    CTRL_PHONE_TELL = 3,
    CTRL_PHONE_TTS = 4,
}STREAM_CTRL_NAME;
#define STREAM_CTRL_NAME int

class AudioServicePrivate;
class AudioService : private QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioService)
    Q_CLASSINFO("D-Bus Interface", "Local.DbusServer.Audio")
public:
    explicit AudioService(QObject* parent = NULL);
    ~AudioService();
public slots:
    void requestAudioSource(const int source);
    void releaseAudioSource(const int source);
    void reqesetReset();
    void requestMuteToggole();
    void requestSpeaker(const int item);
    void requestMute(const int item);
    void requestIncreaseHalfVolume();
    void requestDecreaseHalfVolume();
    void requestIncreaseVolume();
    void requestDecreaseVolume();
    void requestSetVolume(const int volume);
    void setEqualizerItem(const int item, const int bass, const int middle, const int treble);
    void setSoundItem(const int item, const int left, const int right);
    void faderOut();
    //amixer alsa phone volume controls 20190828
    int open_amixer_mode(int stream_name, int ctrlname);
    void close_amixer_mode();
    int get_amixersoftmaster_volume();
    void set_amixersoftmaster_volume(int volume);
signals:
    void onMuteChange(const int mute);
    void onVolumeChange(const int volume);
    void onEqualizerItemChange(const int item, const int bass, const int middle, const int treble);
    void onSoundItemChange(const int item, const int left, const int right);
private slots:
    void onTimeout();
private:
    Q_DECLARE_PRIVATE(AudioService)
    AudioServicePrivate* const d_ptr;
};

#endif // AUDIOSERVICE_H
