#include "MirrorLink.h"
#include "mirrorplayer.h"

MirrorLink::MirrorLink()
{
 #ifdef  USE_MIRROR
    m_pMirrorPlayer = new MirrorPlayer();
#endif
}
MirrorLink::~MirrorLink()
{
#ifdef  USE_MIRROR
    if(m_pMirrorPlayer)
        delete m_pMirrorPlayer;
#endif
}

#ifdef  USE_MIRROR
bool MirrorLink::init(LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    m_pMirrorPlayer->Initialize();
    m_pMirrorPlayer->SetLinkstatusCallback(linkstatus_callback_func,this);
    m_pMirrorPlayer->SetVideoStartCallback(std::bind(&MirrorLink::onVideoStartCallback,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    m_pMirrorPlayer->SetVideoInfoCallback(std::bind(&MirrorLink::onVideoInfoCallback,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    return true;
}

bool MirrorLink::release()
{

    if(m_pMirrorPlayer){
        m_pMirrorPlayer->ExitLink();
        delete m_pMirrorPlayer;
        m_pMirrorPlayer = nullptr;
    }

    return true;
}

bool MirrorLink::start()
{

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(m_pMirrorPlayer){
        onSdkConnectStatus(CONNECT_STATUS_CONNECTING, Phone_Android);
        m_pMirrorPlayer->startServer();
    }
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    return true;
}

bool MirrorLink::stop()
{

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if(m_pMirrorPlayer){
        m_pMirrorPlayer->stopServer();
    }
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    return true;
}

bool MirrorLink::start_mirror()
{
    return true;
}

bool MirrorLink::stop_mirror()
{
    return true;
}

bool MirrorLink::set_background()
{
}

bool MirrorLink::set_foreground()
{

}

bool MirrorLink::get_audio_focus()
{

}

bool MirrorLink::release_audio_focus()
{

}

void MirrorLink::set_inserted(bool inserted, PhoneType phoneType)
{

}

void MirrorLink::send_screen_size(int width, int height)
{

}

#define DUMP_REC_FILE 0
#if DUMP_REC_FILE
static FILE *pfile = NULL;
#endif

void MirrorLink::record_audio_callback(unsigned char *data, int len)
{

}

void MirrorLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{

}

void MirrorLink::send_phone_bluetooth(const string& address)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void MirrorLink::send_touch(int x, int y, TouchCode touchCode)
{
    printf("carplay x:%d, y:%d, press:%d\r\n",x, y, touchCode);

}

bool MirrorLink::send_key(KeyCode keyCode)
{

}

bool MirrorLink::send_wheel(WheelCode wheel, bool foucs)
{

}

bool MirrorLink::open_page(AppPage appPage)
{

}

void MirrorLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{

}


void MirrorLink::linkstatus_callback_func(int status, void* parameter)
{
    MirrorLink *pthis = (MirrorLink*)parameter;

    printf("status = %d\r\n", status);

}

void MirrorLink::onVideoStartCallback(bool start, int width, int height)
{

    int offx = 0 ;//+ (mLinkConfig.screen_width - width) / 2;
    int offy = 0 ;//+ (mLinkConfig.screen_height - height) / 2;

    if(start)
        video_start(offx, offy, width, height);
    else
        video_stop();

}

void MirrorLink::onVideoInfoCallback(int width, int height, unsigned char* data, int length)
{
    video_play(data, length);
}

#endif
