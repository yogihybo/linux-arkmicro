#ifndef VIDEODECODE_H
#define VIDEODECODE_H


#include <stdint.h>
#include <semaphore.h>



#define DISPLAY_LAYER	1
//#define TVOUT_LAYER		3
#define TVOUT_WIDTH		720
#define TVOUT_HEIGHT	480
#define MEMALLOC_MODULE_PATH	"/tmp/dev/memalloc"
#define MAX_STREAM_BUFFER_SIZE	(1024*1024)
#define SOFT_DECODEC  0



class VideoDecode
{
public:
    VideoDecode();
    virtual ~VideoDecode();

public:
    void SetVideoCallback(void (*callback)(int, void*), void *parameter);

    int InitVideoDecode(int x, int y, int src_width, int src_height, int dst_width, int dst_height);  //初始化视频解码

    void UninitVideoDecode(); //释放视频解码

    void SetDecoderExit(bool bExit); //解码释放

    bool GetDecoderExit()  const {return m_bDoExit;}

    void SetDecoderEntry(bool bEntry); //连接上后释放解码第二次及以上进入

    int OpenVideoLayer(int x, int y, int src_width, int src_height,int dst_width, int dst_height); //打开FB视频层

    void CloseVideoLayer(); //关闭FB视频层

    int MallocMemory();  //分配内存

    int FreeMemory();   //释放内存

    bool ShowPlayer(bool bVisible); //显示还是隐藏视频

    bool IsPlayerVisible() const
    {
        return m_bVisible;
    }

    int Decode(const void *data, const int length); //解码调用一桢数据

    int Draw();

    void FindNALUHead(char *data , int length);

    bool IsIDRFrame(char *data, int size);


private:
    int m_VideoFD;
    int m_TVOutFD;
    int m_nSrcWidth;
    int m_nSrcHeight;
    int m_x;
    int m_y;
    int m_nDecodeFrame;
    int m_HeadSize;
    char m_HeadData[100];
    uint8_t *m_player;
    void (*m_video_callback)(int, void *);
    void *m_parameter;
    //解码完成准备显示图片
    bool  m_bReady;

    //连接中去退出
    bool  m_bDoExit;
    //连接中再进入
    bool  m_bDoEntry;
    bool  m_bVisible;

    bool  m_bMemory;

    bool  m_bSecondVisible;
    bool  m_bHead;

    int   m_mapSize;
    unsigned int mOldyBusAddress;
    //pthread_mutex_t m_VideoMutex;
};


#endif // VIDEODECODE_H
