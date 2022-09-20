#ifndef VIDEODECODER_H
#define VIDEODECODER_H


#include <stdint.h>
#include <semaphore.h>

#include "ark_api.h"
#include "mfcapi.h"
#include "dwl.h"

#define MAX_STREAM_BUFFER_SIZE	(1024*1024)

#ifdef __cplusplus

struct VideoFrame{
    int src_offset_x;
    int src_offset_y;
    int src_width;
    int src_height;
    int dst_offset_x;
    int dst_offset_y;
    int dst_width;
    int dst_height;
    int rotate;
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
#ifndef BPS_DECODE
    int DrawLayer(display_info *pinfo);

    int MallocMemory();

    int FreeMemory();

    display_info * disp_layer_init(enum ark_disp_layer layer, int format, int width, int height, int buf_cnt);
#endif

private:
    static VideoDecoder *mInstance;
    VideoFrame *mpVideoFrame;
    bool    m_bReady;
#ifdef BPS_DECODE
    video_handle *mHandle;
#else
    int         m_mapSize;
    //save bus add y address
    unsigned int myBusAddress;

    OutFrameBuffer mOutBuffer;
    DWLLinearMem_t mInBuffer;

    MFCHandle  *mMFCHanle;
    memalloc_handle * mMemHandle;
    reqbuf_info* mInreqbuf;
    //save bus add y address
    int mFdMemalloc;

   // display_info *overlay;
    display_info *overlay;
#endif

};
#endif

#endif // VIDEODECODER_H
