#ifndef AUDIO_H
#define AUDIO_H

#include "AudioService.h"
#include "DbusService.h"
#include <QObject>
#include <QString>
#include <QGuiApplication>
#include <QProcess>
#include <QScopedPointer>

class AudioPrivate;
class Audio
        : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Audio)
#ifdef g_Audio
#undef g_Audio
#endif
#define g_Audio (Audio::instance())
public:
    inline static Audio* instance() {
        static Audio* audio(new Audio(qApp));
        return audio;
    }
    bool requestAudioSource(const int source);
    void releaseAudioSource(const int source);
    void releaseVolume();
    int getcurrentvolumevalue();
    void reset();
    void setvolumebalance();
    void requestMuteToggole();
    void requestSpeaker(const int item);
    void requestMute(const int item);
    void requestIncreaseVolume();
    void requestDecreaseVolume();
    void requestSetVolume(const int volume);
    void setEqualizerItem(const int item, const int bass, const int middle, const int treble);
    void EqualizerItemTogglePrevious();
    void EqualizerItemToggleNext();
    void setSoundItem(const int item, const int left, const int right);
    AudioSource getPhoneSource();
    AudioSource getMultimediaSource();
    AudioSource getAudioSource();
    MuteItem getMute();
    void faderOut();
    void AudioSwitchNone();
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
    void onHolderChange(const int oldHolder, const int newHolder);
private slots:
    void onServiceUnregistered(const QString& service);
    void onMuteChangeHandler(const int mute);
    void onEqualizerItemChangeHandler(const int item, const int bass, const int middle, const int treble);
private:
    explicit Audio(QObject* parent = NULL);
    ~Audio();
    void initializePrivate();
    friend class AudioPrivate;
    QScopedPointer<AudioPrivate> m_Private;
};

#endif // AUDIO_H
