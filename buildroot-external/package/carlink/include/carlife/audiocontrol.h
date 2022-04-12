#ifndef AUDIOCONTROL_H
#define AUDIOCONTROL_H

#include <alsa/asoundlib.h>
//#include "alsadbus.h"



#define DEFAULT     "default"

#define LEFT_ID     "numid=1,iface=MIXER,name='Left Playback Volume'"
#define RIGHT_ID    "numid=2,iface=MIXER,name='Right Playback Volume'"
#define STRAM_ID    "numid=3,iface=MIXER,name='Stream Select'"
#define OUTPUT_ID   "numid=4,iface=MIXER,name='Output Select'"
#define AMP_ID      "numid=5,iface=MIXER,name='AMP Volume'"
#define FM_ID       "numid=6,iface=MIXER,name='FM Freq'"
#define IRSTATUS_ID "numid=7,iface=MIXER,name='IR Status'"
#define IRCHANNEL_ID "numid=8,iface=MIXER,name='IR Channel'"
#define PAMUTE_ID   "numid=9,iface=MIXER,name='PA Mute'"

#define SOFTMASTER_ID  "numid=34,iface=MIXER,name='softmaster'"

typedef enum
{
    LEFT_AUDIO = 0x01,
    RIGHT_AUDIO,
    STRAM_AUDIO,
    OUTPUT_AUDIO,
    AMP_AUDIO,
    FM_AUDIO,
    IRSTATUS_AUDIO,
    IRCHANNEL_AUDIO,
    PAMUTE_AUDIO,
    SOFTMASTER_AUDIO,
}TYPE_AUDIO;

typedef struct Packet
{
    int count;
    int type;
} Packet_t;

typedef struct Data
{
    int max;
    int min;
    int cur;
    int step;
}Data_t;


class AudioControl
{
public:
    AudioControl();

public:

    //get control value
    virtual int Get(int Audio_Type ,int *nValue);

    //set control value
    virtual int Set(int Audio_Type, int nValue);

    //get control range
    virtual int GetRange(int Audio_Type, int *pMin ,int *pMax);

protected:
    //get string from enum's
    char* getstring(int type);

    //get id from audio's string
    int getpid(int audio_type, snd_ctl_elem_id_t **ppid);

    //get info from id
    int getpinfo(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t **ppinfo);

    //get elem from id
    int getpelem(snd_ctl_elem_id_t *pid, snd_hctl_elem_t **ppelem);

    //get control from id and info
    int getpcontrol(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo ,snd_ctl_elem_value_t **ppcontrol);

    //get type
    int get_type(snd_ctl_elem_info_t *pinfo, int *ptype);

    //get size
    int get_size(snd_ctl_elem_info_t *pinfo, int *psize);

    //get range from elem and info
    int get_range(int type, snd_hctl_elem_t *pelem, snd_ctl_elem_info_t *pinfo, Data *pdata);

    //get value
    int get_value(int type, int count, snd_ctl_elem_value_t *pcontrol, int *pvalue);

    //set value
    int set_value(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo, snd_ctl_elem_value_t *pcontrol, int value);

public:
    static int ALSAMixerOpen(char* pMode, char* pCtrlname);
    static void ALSAMixerClose(void);
    static int ArkALSADuckMixerGetVolume(void);
    static void ArkALSADuckMixerSetVolume(int inVolume);

};


extern int ALSAMixerOpen(void);
extern void ALSAMixerClose(void);
extern int ArkALSADuckMixerGetVolume(void);
extern void ArkALSADuckMixerSetVolume(int inVolume);
#endif // AUDIOCONTROL_H
