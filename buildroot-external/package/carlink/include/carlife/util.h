#ifndef UTIL_H
#define UTIL_H

#define MAXBUFSIZE 1024
#define MAXPACKETAGE 32
//#include "arkconfig.h"
//#include "carplay.h"

#include <string>
using namespace std;

typedef struct {
    int    x;
    int    y;
    int    width;
    int    height;
} screen_pos;


enum MSG_TYPE
{
    GO_START_LINK = 1,
    GO_BACKGROUND,
    GO_FOREGROUND,
    GO_EXIT,
    GO_RESTART_LINK,
    GO_MUTE,
    GO_UNMUTE,
    GO_EXIT_PROCESS,
};


enum MSG_RECV
{
    CMD_BACKGROUND = 10,
    CMD_FOREGROUND = 11,
    CMD_VIDEOREADY = 12,
    CMD_VIDEOSECOND= 13,
    CMD_REMOVED    = 14,
    CMD_SUCCESS    = 15,
    CMD_FAILED     = 16,
    CMD_RECORD_START = 17,
    CMD_RECORD_END = 18,
    CMD_INSERTED   = 19,
    CMD_NOTINSERTED= 20,
    CMD_BT_DISCONNECT = 21,
    CMD_CALL_PHONE = 22,
    CMD_CALL_PHONE_EXIT = 23,
    CMD_FAILED_EAP = 24,
    CMD_FAILED_UNSTART = 25,
    CMD_SOCKET_TRUST = 26,
    CMD_OPEN_CARLIFE = 27,
    CMD_VOLUME_START = 28,
    CMD_VOLUME_STOP = 29,
    CMD_SIRI_START = 24,
    CMD_SIRI_STOP  = 25,
    CMD_ASSIST_AUDIO_START = 26,
    CMD_ASSIST_AUDIO_STOP = 27,
    CMD_CARLIFE_EXITED_INVOKED = 30,
    CMD_CARLIFE_MUSIC_STATUS = 31,
    CMD_MUSIC_START = 32,
    CMD_MUSIC_STOP = 33,
    CMD_NODATA     = 34,
    CMD_VIDEOINIT  = 35,
    CMD_RECORDER_INIT = 36,
    CMD_RECORDER_UNINIT = 37,
    CMD_START_LINK  =40,
    CMD_BT_CALL_INCOMMING = 100,
    CMD_BT_CALL_OUT = 101,     //
    CMD_BT_CALL_TERMINAL = 102,
    CMD_BT_CALL_ANSWER = 103,  //BT来电
    CMD_BT_CALL_REJECT = 104,  //BT拒绝接听
    CMD_BT_CALL_MUTE   = 105, //BT挂断
    CMD_BT_CALL_UNMUTE   = 106, //BT挂断
    CMD_BT_CALL_DTMF_0  = 107, //0
    CMD_BT_CALL_DTMF_1  = 108, //1
    CMD_BT_CALL_DTMF_2  = 109, //2
    CMD_BT_CALL_DTMF_3  = 110, //3
    CMD_BT_CALL_DTMF_4  = 111, //4
    CMD_BT_CALL_DTMF_5  = 112, //5
    CMD_BT_CALL_DTMF_6  = 113, //6
    CMD_BT_CALL_DTMF_7  = 114, //7
    CMD_BT_CALL_DTMF_8  = 115, //8
    CMD_BT_CALL_DTMF_9  = 116, //9
    CMD_BT_CALL_DTMF_STAR  = 117, //*
    CMD_BT_CALL_DTMF_SHARP  = 118, //#

};

typedef struct
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned max_size;            //! Nal Unit Buffer size
  int forbidden_bit;            //! should be always FALSE
  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
  int nal_unit_type;            //! NALU_TYPE_xxxx
  char *buf;                    //! contains the first byte followed by the EBSP
  unsigned short lost_packets;  //! true, if packet loss is detected
  int data_offset;
} NALU_t;

class Util
{
public:
    Util();

    static void get_app(char *path);

    static void get_self_path_name(char *path);

    static bool get_deivce_type(int *device, int *vid, char *pBuffer);

    static void getFrameBufferFixedSize(int &width, int &height, const char* fbPath);

    static NALU_t * AllocNALU(int buffersize);

    static void FreeNALU(NALU_t *n);

    static int GetAnnexbNALU (NALU_t *nalu,  unsigned char * Buf);

    static void init_var();

    static int open_local_socket();

    static int send_local_message( int inSock, const void *inMsg, size_t inLen);

    static void ReadBTInfo(string& addr, string& pin);

    static void changeOTG();

};

#endif // UTIL_H
