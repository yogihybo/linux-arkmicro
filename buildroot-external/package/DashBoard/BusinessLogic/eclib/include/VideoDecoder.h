#ifndef VIDEODECODER_H
#define VIDEODECODER_H


#include <stdint.h>
#include <semaphore.h>

#include "ark_api.h"
//#include "display.h"
#include "mfcapi.h"
#include "dwl.h"
#define DISPLAY_LAYER	1
//#define TVOUT_LAYER		3
#define TVOUT_WIDTH		720
#define TVOUT_HEIGHT	480
#define MEMALLOC_MODULE_PATH	"/dev/memalloc"
#define MAX_STREAM_BUFFER_SIZE	(1024*1024)
#define SOFT_DECODEC  0


struct VideoFrame{
    int handle;
    int offset_x;
    int offset_y;
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
};

struct display_info{
    disp_handle *display_handle;
    int lay_id;

    unsigned int disp_posx;
    unsigned int disp_posy;
    unsigned int disp_width;
    unsigned int disp_height;
    struct ark_reqbuf reqbuf;
};

struct test_info{
    int platform;
    int pic_format;
    char filepath[64];
};


class VideoDecoder
{
public:
    VideoDecoder();

    static VideoDecoder *instance();

//init decoder
    bool Init(VideoFrame* pVideoFrame);

    void Uninit();
//open video layer
    bool Open(VideoFrame* pVideoFrame);

    bool Close();

    bool Show(bool bVisible);

    int InputDecoder(const void *data, int length);
private:

    int DrawLayer(display_info *pinfo);

    int MallocMemory();

    int FreeMemory();

    display_info * disp_layer_init(enum ark_disp_layer layer, int format, int width, int height, int buf_cnt);
private:
    static VideoDecoder *mInstance;
    int m_nDecodeFrame;
    VideoFrame *mpVideoFrame;
    MFCHandle  *mMFCHanle;
    memalloc_handle * mMemHandle;
    int         m_mapSize;
    //save bus add y address
    unsigned int myBusAddress;
    int mFdMemalloc;

   // display_info *overlay;
    display_info *overlay;

    OutFrameBuffer mOutBuffer;
    DWLLinearMem_t mInBuffer;
    bool    m_bReady;
};

#endif // VIDEODECODER_H
