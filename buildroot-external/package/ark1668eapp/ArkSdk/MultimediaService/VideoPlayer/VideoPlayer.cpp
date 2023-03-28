#include "VideoPlayer.h"
#include "AutoConnect.h"
#include "Utility.h"
#include "UserInterfaceUtility.h"
#include "ark_api.h"
#include <QTimer>
#include <QList>
#include <QFileInfo>
#include <QDomDocument>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <QDebug>
#include <arpa/inet.h>  //for in_addr
#include <linux/rtnetlink.h>    //for rtnetlink
#include <net/if.h> //for IF_NAMESIZ, route_info
#include <QFile>
#include <pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<semaphore.h>
#include <errno.h>
#include <iostream>
#include "configUtils.h"
using namespace std;
static const string SDMultimedia("/data/MultiMediaFile/SDMultimedia");
static const string SDVideoMillesmial("/data/MultiMediaFile/SDVideoMillesmial");
static const string SDVideoPathInfo("/data/MultiMediaFile/SDVideoPathInfo");
static const string USBMultimedia("/data/MultiMediaFile/USBMultimedia");
static const string USBVideoMillesmial("/data/MultiMediaFile/USBVideoMillesmial");
static const string USBVideoPathInfo("/data/MultiMediaFile/USBVideoPathInfo");
/*
*功能:获取当前的播放模式
*/
static char* getPidFromStr(const char *str)
{
    static char sPID[8] = {0};
    int tmp = 0;
    int pos1 = 0;
    int pos2 = 0;
    int i = 0;
    int j = 0;

    for (i=0; i<strlen(str); i++) {
        if ( (tmp==0) && (str[i]>='0' && str[i]<='9') ) {
            tmp = 1;
            pos1 = i;
        }
        if ( (tmp==1) && (str[i]<'0' || str[i]>'9') ) {
            pos2 = i;
            break;
        }
    }
    for (j=0,i=pos1; i<pos2; i++,j++) {
        sPID[j] = str[i];
    }
    return sPID;
}
int getCrrentvideoPlayMode()
{
    QFile file("/etc/videoplaymode");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<Q_FUNC_INFO<<"file :/etc/videoplaymode is open failed";
    }

    QTextStream in(&file);
    QString line = in.readLine();
    qDebug() << Q_FUNC_INFO<<"current video mode ="<<line.toInt();
    file.flush();
    file.close();

    if(line.isEmpty()){
        return 0;
    }
    return line.toInt();
}

class VideoPlayerPrivate
{
    Q_DISABLE_COPY(VideoPlayerPrivate)
public:
    enum CommandType {
        CT_Undefine = -1,
        CT_PlayIndex = 0,
        CT_PlayNext = 1,
        CT_PlayPrevious = 2,
        CT_Pause = 3,
        CT_Seek = 4,
        CT_Toggle = 5,
    };
    explicit VideoPlayerPrivate(VideoPlayer *parent);
    ~VideoPlayerPrivate();
    void initialize();
    void connectAllSlots();
    void setPlayMode(const VideoPlayerPlayMode mode);
    void setPlayStatus(const VideoPlayerPlayStatus status);
    void pauseToggleHandler(const QString& output);
    void startHandler(const QString& output);
    void endTimePositionHandler(const QString &output);
    void millesimalHandler(const QString& output);
    void timePositionHandler(const QString &output);
    QList<QString>& getPathList(const DeviceWatcherType type);
    void endOfFileHandler();
    void unsupportHandler();
    void undragAndDropHandler();//add by wandz 20190425
    void seekToMillesimal(const int millesimal);
    void createSDFileNamesXml();
    void createUSBFileNamesXml();
    void videoPlayerSetPlay();
    void videoPlayerSetSuspend();
    void videoPlayerSetPause();
    void videoPlayerSetExit(const DeviceWatcherType type);
    void exitVideoPlayer();
    void videoPlayerVisible(const bool flag);
    int  isRunning(char *process);
    QString chgform(QString name);
    void writeCmdToMplayer(char* cmd);
    void quitMpalyer();
    void onVideoPlayerVisible(bool visible);
    void sliderTouchEnable(bool enable);
    void videoPlayerPlayStatus(const int type, const int status);
    ssize_t readLine(int fd, char *buffer, size_t n);
    VideoPlayerPlayMode m_PlayMode;
    VideoPlayerPlayStatus m_PlayStatus;
    QString m_SDFileNamesXml;
    QString m_USBFileNamesXml;
    QList<QString> m_SDPathList;
    QList<QString> m_USBPathList;
    QProcess* m_VideoPlayer;
    DeviceWatcherType m_DiskType;
    int m_PlayIndex;
    int m_ElapsedTime;
    int m_Millesimal;
    int m_EndTime;
    int m_X;
    int m_Y;
    int m_Width;
    int m_Height;
    bool m_Suspend;
    VideoPlayerPrivate::CommandType m_LastCommand;
    int m_ReadpThread;
    int fd[2];
    int m_Fd_w;
    pid_t m_pid1;
    pid_t m_pid2;
    QString m_Process;
private:
    Q_DECLARE_PUBLIC(VideoPlayer)
    VideoPlayer* const q_ptr;
};

static void* quitHanlder(void* arg)
{
    VideoPlayerPrivate* pThis = (VideoPlayerPrivate*)arg;
    char info[512] = {0};
    int  ret = 0;
    while (1) {
        if(pThis->m_ReadpThread == 0)
        {
            qDebug()<<"+++[VideoPlayer::quitHanlder:pThis->m_ReadpThread]+++"<<pThis->m_ReadpThread;
            break;//退出线程
        }
        bzero(info, 512);
        ret = pThis->readLine(pThis->fd[0], info, 512);
        while(ret > 0)
        {
            string str(info);
            QString output = QString::fromStdString(str);
            //qDebug()<<"=== mplayer video:"<<output;
            if (output.contains(QString("ANS_LENGTH="))) {
                pThis->endTimePositionHandler(output);
            } else if (output.contains(QString("ANS_PERCENT_POSITION="))) {
                pThis->millesimalHandler(output);
            } else if (output.contains(QString("ANS_TIME_POSITION="))) {
                pThis->timePositionHandler(output);
            } else if (output.contains(QString("ANS_pause"))) {
                pThis->pauseToggleHandler(output);
            } else if (output.contains(QString("EOF code: 1"))) {
                pThis->setPlayStatus(VPPS_Stop);
                sleep(2);
                pThis->endOfFileHandler();
            }
            else if (output.contains(QString("give up video"))) {
                pThis->onVideoPlayerVisible(false);
            } else if (output.contains(QString("take video"))) {
                pThis->onVideoPlayerVisible(true);
            } else if(output.contains(QString("Cannot seek in this file"))){
                qDebug()<<__PRETTY_FUNCTION__<<__LINE__<<"seek failed";
                pThis->sliderTouchEnable(false);//dll add 不可拖动的视频禁止拖动
                pThis->undragAndDropHandler();
            }else if ((output.contains(QString("incorrect streams")))//wandz video
                       || (output.contains(QString("No stream found")))
                       || (output.contains(QString("Cannot find codec matching selected -vo and video format")))
                        || (output.contains(QString("Maybe you are playing a non-interleaved stream/file or the codec failed")))
                      ) {
                sleep(2);
                pThis->unsupportHandler();
            }/* else if (output.contains(QString("seek-begin"))) {
            } */else if ((output.contains(QString("seek-finish"))
                          || (output.contains(QString("seek-error"))))) {
                pThis->videoPlayerPlayStatus(pThis->m_DiskType, VPPS_SeekFinish);
            }
            else if((output.contains(QString("Exiting... (Fatal error)"))))
            {
                pThis->videoPlayerPlayStatus(pThis->m_DiskType, VPPS_PlayError);
            }
            else {
                if (output != QString("\n")) {
                    qDebug() << "output" << output;
                }
            }
            bzero(info, 512);
            ret = pThis->readLine(pThis->fd[0], info, 512);
        }
        if(ret <= 0){
            sleep(1);
        }
    }
    if(pThis->m_Fd_w > 0)
    {
        close(pThis->m_Fd_w);
        pThis->m_Fd_w = -1 ;
    }
    close(pThis->fd[0]);
    pThis->m_pid2 = wait(NULL);//回收mpalyer的进程号，避免mpalyer成为僵尸进程
    printf("*********VideoPlayer_pid2= %d \n",pThis->m_pid2);//wait的返回值为子进程的pid
    pThis->m_Process.clear();
    return NULL;
}

void VideoPlayer::videoPlayerRequestFileNames(const DeviceWatcherType type)
{
    Q_D(VideoPlayer);
    switch (type) {
    case DWT_SDDisk: {
        emit onVideoPlayerFileNames(DWT_SDDisk, d->m_SDFileNamesXml);
        break;
    }
    case DWT_USBDisk: {
        emit onVideoPlayerFileNames(DWT_USBDisk, d->m_USBFileNamesXml);
        break;
    }
    default: {
        break;
    }
    }
}

void VideoPlayer::videoPlayerSetPlayModeToggle()
{
    Q_D(VideoPlayer);
    switch (d->m_PlayMode) {
    case VPPM_AllRepeat: {
        videoPlayerSetPlayMode(VPPM_Shuffle);
        break;
    }
    case VPPM_Shuffle: {
        videoPlayerSetPlayMode(VPPM_RepeatOnce);
        break;
    }
    case VPPM_RepeatOnce:
    default: {
        videoPlayerSetPlayMode(VPPM_AllRepeat);
        break;
    }
    }
}

void VideoPlayer::videoPlayerSetPlayMode(const VideoPlayerPlayMode mode)
{
    Q_D(VideoPlayer);
    d->setPlayMode(mode);
}

void VideoPlayer::videoPlayerSetPlayStatusToggle()
{
    Q_D(VideoPlayer);
    switch (d->m_PlayStatus) {
    case VPPS_Play: {
        videoPlayerSetPlayStatus(VPPS_Pause);
        break;
    }
    case VPPS_Pause: {
        videoPlayerSetPlayStatus(VPPS_Play);
        break;
    }
    case VPPS_Stop: {
        videoPlayerSetPlayStatus(VPPS_Stop);
    }
    case VPPS_Unsupport: {
        playVideoIndex(d->m_DiskType, d->m_PlayIndex, d->m_X, d->m_Y, d->m_Width, d->m_Height, 0);
        break;
    }
    default: {
        break;
    }
    }
}

void VideoPlayer::videoPlayerSetPlayStatus(const VideoPlayerPlayStatus status)
{
    Q_D(VideoPlayer);
    if(d->m_Process.size() != 0){
        QByteArray processArray = d->m_Process.toLatin1();
        char *process = processArray.data();
        if(d->isRunning(process)){
            if (VideoPlayerPrivate::CT_Undefine == d->m_LastCommand) {
                switch (status) {
                case VPPS_Play: {
                    d->videoPlayerSetPlay();
                    break;
                }
                case VPPS_Pause: {
                    d->videoPlayerSetPause();
                    break;
                }
                case VPPS_SuspendToggle: {
                    d->videoPlayerSetSuspend();
                    break;
                }
                case VPPS_Stop: {
                    videoPlayerPlayListViewIndex(d->m_DiskType, d->m_PlayIndex, d->m_X, d->m_Y, d->m_Width, d->m_Height, 0);
                    break;
                }
                default : {
                    break;
                }
                }
            }
        }
    }

}

void VideoPlayer::videoPlayerPlayListViewIndex(const DeviceWatcherType type, const int index, const int x, const int y, const int width, const int height, const int millesimal)
{
    Q_D(VideoPlayer);
    if (VideoPlayerPrivate::CT_Undefine == d->m_LastCommand) {
        playVideoIndex(type, index, x, y, width, height, millesimal);
    }
}

void VideoPlayer::videoPlayerPlayPreviousListViewIndex()
{
    Q_D(VideoPlayer);
    if (VideoPlayerPrivate::CT_Undefine == d->m_LastCommand) {
        int lastIndex;
        if (MPPM_Shuffle == d->m_PlayMode) {
            lastIndex = qrand() % d->getPathList(d->m_DiskType).size();
        } else {
            lastIndex = d->m_PlayIndex;
            QList<QString> temp = d->getPathList(d->m_DiskType);
            if ((lastIndex > 0)
                    && (lastIndex <= temp.size() - 1)) {
                --lastIndex;
            } else {
                lastIndex = temp.size() - 1;
            }
        }
        videoPlayerPlayListViewIndex(d->m_DiskType, lastIndex, d->m_X, d->m_Y, d->m_Width, d->m_Height, 0);
    }
}

void VideoPlayer::videoPlayerPlayNextListViewIndex()
{
    Q_D(VideoPlayer);
    if (VideoPlayerPrivate::CT_Undefine == d->m_LastCommand) {
        int lastIndex;
        if (MPPM_Shuffle == d->m_PlayMode) {
            lastIndex = qrand() % d->getPathList(d->m_DiskType).size();
        } else {
            lastIndex = d->m_PlayIndex;
            QList<QString> temp = d->getPathList(d->m_DiskType);
            if (((lastIndex < (temp.size() - 1)))
                    && (lastIndex >= 0)) {
                ++lastIndex;
            } else {
                lastIndex = 0;
            }
        }
        videoPlayerPlayListViewIndex(d->m_DiskType, lastIndex, d->m_X, d->m_Y, d->m_Width, d->m_Height, 0);
    }
}

void VideoPlayer::videoPlayerSeekToMillesimal(const int millesimal)
{
    Q_D(VideoPlayer);
    if(d->m_Process.size() != 0){
        QByteArray processArray = d->m_Process.toLatin1();
        char *process = processArray.data();
        if(d->isRunning(process)){
            if (VideoPlayerPrivate::CT_Undefine == d->m_LastCommand) {
                d->seekToMillesimal(millesimal);
            }
        }
    }
}

void VideoPlayer::videoPlayerExit()
{
    Q_D(VideoPlayer);
    d->videoPlayerSetExit(d->m_DiskType);
}

void VideoPlayer::videoPlayerVisible(const bool flag)
{
    Q_D(VideoPlayer);
    if(d->m_Process.size() != 0){
        QByteArray processArray = d->m_Process.toLatin1();
        char *process = processArray.data();
        if(d->isRunning(process)){
            d->videoPlayerVisible(flag);
        }
    }
}

void VideoPlayer::onDeviceWatcherStatus(const DeviceWatcherType type, const DeviceWatcherStatus status)
{
    Q_D(VideoPlayer);
    if (DWT_SDDisk == type) {
        switch (status) {
        case DWS_Empty: {
            d->m_SDPathList.clear();
            break;
        }
        case DWS_Busy: {
            d->m_SDPathList.clear();
            break;
        }
        case DWS_Ready: {
            d->createSDFileNamesXml();
            videoPlayerRequestFileNames(DWT_SDDisk);
            break;
        }
        case DWS_Remove: {
            d->m_SDPathList.clear();
            if (DWT_SDDisk == d->m_DiskType) {
                if(d->m_Process.size() != 0){
                    QByteArray processArray = d->m_Process.toLatin1();
                    char *process = processArray.data();
                    if(d->isRunning(process)){
                        d->quitMpalyer();
                    }
                }
                d->setPlayStatus(VPPS_Exit);
            }
            break;
        }
        default : {
            break;
        }
        }
    } else if (DWT_USBDisk == type) {
        switch (status) {
        case DWS_Empty: {
            d->m_USBPathList.clear();
            break;
        }
        case DWS_Busy: {
            d->m_USBPathList.clear();
            break;
        }
        case DWS_Ready: {
            d->createUSBFileNamesXml();
            videoPlayerRequestFileNames(DWT_USBDisk);
            break;
        }
        case DWS_Remove: {
            d->m_USBPathList.clear();
            if (DWT_USBDisk == d->m_DiskType) {
                if(d->m_Process.size() != 0){
                    QByteArray processArray = d->m_Process.toLatin1();
                    char *process = processArray.data();
                    if(d->isRunning(process)){
                        d->quitMpalyer();
                    }
                }
                d->setPlayStatus(VPPS_Exit);
            }
        }
        default : {
            break;
        }
        }
    }
}

void VideoPlayer::onVideoFilePath(const QString &path, const DeviceWatcherType type)
{
    Q_D(VideoPlayer);
    if (type == DWT_USBDisk) {
        d->m_USBPathList.append(path);
    } else if (type == DWT_SDDisk) {
        d->m_SDPathList.append(path);
    }
    //    emit onVideoPlayerFilePath(type, path);
}

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoPlayerPrivate(this))
{
}

VideoPlayer::~VideoPlayer()
{
}



VideoPlayerPrivate::VideoPlayerPrivate(VideoPlayer *parent)
    : q_ptr(parent)
{
//    m_PlayMode = VPPM_AllRepeat;
    m_PlayMode = getCrrentvideoPlayMode();
    m_PlayStatus = VPPS_Exit;
    m_VideoPlayer = NULL;
    m_DiskType = DWT_Undefine;
    m_PlayIndex = -1;
    m_ElapsedTime = -1;
    m_Millesimal = 0;
    m_EndTime = -1;
    m_X = 0;
    m_Y = 0;
    m_Width = 800;
    m_Height = 480;
    m_Suspend = false;
    m_LastCommand = VideoPlayerPrivate::CT_Undefine;
    m_ReadpThread = 1;
    m_Fd_w = -1;
    m_Process.clear();
    initialize();
    connectAllSlots();
}

VideoPlayerPrivate::~VideoPlayerPrivate()
{
    Q_Q(VideoPlayer);
    if(m_Process.size() != 0){
        QByteArray processArray = m_Process.toLatin1();
        char *process = processArray.data();
        if(isRunning(process)){
            quitMpalyer();
        }
    }

}

void VideoPlayerPrivate::initialize()
{
    Q_Q(VideoPlayer);
    m_VideoPlayer = new QProcess(q);
    m_VideoPlayer->setProcessChannelMode(QProcess::MergedChannels);
}

void VideoPlayerPrivate::connectAllSlots()
{
    Q_Q(VideoPlayer);
    connectSignalAndSlotByNamesake(g_DeviceWatcher, q, SLOT(onDeviceWatcherStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_DeviceWatcher, q, SLOT(onVideoFilePath(const QString &, const int)));
}
void VideoPlayer::videoPlayerSetGeometry(int x,int y,int width,int height)
{
    //不行　花屏
//    Q_D(VideoPlayer);
//    d->m_Width = width;
//    d->m_Height = height;
//    d->m_X = x ;
//    d->m_Y = y;
//    QString _GeometryStr = QString(QString::number(d->m_Width)
//                                     + QString("x")
//                                     + QString::number(d->m_Height)
//                                     + QString("+")
//                                     + QString::number(d->m_X)
//                                     + QString("+")
//                                     + QString::number(d->m_Y));
//    QByteArray _GeometryByte = _GeometryStr.toLocal8Bit();
//    const char* _Geometry    = _GeometryByte.data();
//    char _GeometryCmd[512]   = {0};
//    strcat(_GeometryCmd, "change_geometry ");
//    strcat(_GeometryCmd, _Geometry);
//    strcat(_GeometryCmd, "\n");
//    d->writeCmdToMplayer(_GeometryCmd);
}
void VideoPlayer::playVideoIndex(const DeviceWatcherType type, const int index, const int x, const int y, const int width, const int height, const int millesimal)
{
    Q_D(VideoPlayer);
    QList<QString> temp = d->getPathList(type);
    if ((VideoPlayerPrivate::CT_Undefine == d->m_LastCommand)
            && (temp.size() > index)
            && (QFile(temp.at(index))).exists()) {
        d->m_DiskType = type;
        d->m_ElapsedTime = 0;
        d->m_EndTime = 0;
        d->m_PlayIndex = index;
        d->m_Suspend = false;
        d->m_Millesimal = millesimal;
        d->m_X = x;
        d->m_Y = y;
        d->m_Width = width;
        d->m_Height = height;
        bool _ProcessIsRunning(false);
        if (0 != millesimal) {
            if(d->m_Process.size() != 0){
                QByteArray processArray = d->m_Process.toLatin1();
                char *process = processArray.data();
                if(d->isRunning(process)){
                    d->quitMpalyer();
                    usleep(500*1000);
                }
            }
        }
        d->m_DiskType = type;
        if(d->m_Process.size() != 0){
            QByteArray processArray = d->m_Process.toLatin1();
            char *process = processArray.data();
            sleep(1);
            if(!d->isRunning(process)){
                _ProcessIsRunning = true;
            }
        }
        if(d->m_Process.size() == 0)
        {
            _ProcessIsRunning = true;
        }

        if (_ProcessIsRunning){

            if(access("/data/VideoPipe",F_OK) != 0)
            {
                printf("VideoPipe exist\n");
                mkfifo("/data/VideoPipe",0777);
            }
            else{
                unlink("/data/VideoPipe");
                mkfifo("/data/VideoPipe", 0777);
            }
            if(pipe(d->fd) < 0){
                perror("pipe error\n");
                return;
            }
            fcntl(d->fd[0], F_SETFL, O_NONBLOCK); //无名管道设为非阻塞
            d->m_pid1 = vfork();
            printf("+++++++++0000video_pid++++++++\n");
            if(d->m_pid1 < 0){
                perror("fork error\n");
            }
            else if(d->m_pid1 == 0)
            {
                printf("child:video0000%d\n",getpid());
                close(d->fd[0]);
                dup2(d->fd[1], STDERR_FILENO);
                QByteArray _NameByte = temp.at(d->m_PlayIndex).toLocal8Bit();
                const char* _Name    = _NameByte.data();
                QString _GeometryStr = QString(QString::number(d->m_Width)
                                                 + QString("x")
                                                 + QString::number(d->m_Height)
                                                 + QString("+")
                                                 + QString::number(d->m_X)
                                                 + QString("+")
                                                 + QString::number(d->m_Y));
                QByteArray _GeometryByte = _GeometryStr.toLocal8Bit();
                const char* _Geometry = _GeometryByte.data();
                if(millesimal == 0){
                   execlp("/usr/bin/mplayer", "mplayer", "-slave", "-quiet", "-idle","-geometry",_Geometry,
                          "-vo", "customfb","-ao","alsa","-input", "file=/data/VideoPipe", _Name,NULL);	//命名管道pipe有命令数据，mplayer会自动读取
                }
                else{
                   const char* _ChMillesimal = std::to_string(millesimal).c_str();
                   execlp("/usr/bin/mplayer", "mplayer", "-slave", "-quiet", "-idle","-geometry",_Geometry,
                          "-vo", "customfb","-ao","alsa","-input", "file=/data/VideoPipe","-ss",_ChMillesimal, _Name,NULL);	//命名管道pipe有命令数据，mplayer会自动读取
                }
            }
            else{
                //切換视频时解开进度条拖动..防止不可拖动的视频也能拖动
                emit onSliderTouchEnable(true);
                emit onVideoPlayerElapsedInformation(d->m_ElapsedTime, d->m_Millesimal);
                emit onVideoPlayerInformation(d->m_DiskType,d->m_PlayIndex, QFileInfo(temp.at(d->m_PlayIndex)).fileName(), d->m_EndTime);
                emit onVideoPlayerPlayMode(d->m_PlayMode);
                d->setPlayStatus(VPPS_Start);
                QFileInfo fileInfo(temp.at(d->m_PlayIndex));
                QString filePath = fileInfo.filePath() + QString("/") +
                        fileInfo.created().toString(QString("yyyyMMddhhmmss")) +
                        fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) +
                        QString::number(fileInfo.size());
                int value = MT_Video;
                if(d->m_DiskType == DWT_USBDisk){
                    SaveConfigString(filePath.toStdString(),USBVideoPathInfo);
                    system("sync");
                    SaveConfigString(value,USBMultimedia);
                    system("sync");
                }
                else if(d->m_DiskType == DWT_SDDisk){
                    SaveConfigString(filePath.toStdString(),SDVideoPathInfo);
                    system("sync");
                    SaveConfigString(value,SDMultimedia);
                    system("sync");
                }
                d->m_Process = QString("mplayer");
                d->m_ReadpThread = 1;
                close(d->fd[1]);
                usleep(200*1000);
                pthread_t pthead;
                int ret = pthread_create(&pthead, NULL,quitHanlder,d);
                if(ret != 0) {
                    printf("pthread_create failed!\n");
                }
            }
        } 
        else{
            char pathname[512] = {0};
            strcat(pathname, "loadfile ");
            QString _FilePath = d->chgform(temp.at(d->m_PlayIndex));
            QByteArray _NameByte = _FilePath.toLocal8Bit();
            char* _Name   = _NameByte.data();
            strcat(pathname, _Name);
            strcat(pathname, "\n");
            printf("+++++++++++++pathname:%s", pathname);
            if(d->m_Fd_w < 0)
            {
               d->m_Fd_w = open("/data/VideoPipe", O_WRONLY | O_NONBLOCK | O_TRUNC);
               if(d->m_Fd_w < 0){
                   perror("open file error");
                   d->m_ReadpThread = 0;
                   d->m_Process.clear();
                   return;
               }
            }
            ftruncate(d->m_Fd_w,0);
            lseek(d->m_Fd_w,0,SEEK_SET);
            write(d->m_Fd_w, pathname, strlen(pathname));
            //切換视频时解开进度条拖动..防止不可拖动的视频也能拖动
            emit onSliderTouchEnable(true);
            d->setPlayStatus(VPPS_Start);
            emit onVideoPlayerElapsedInformation(d->m_ElapsedTime, d->m_Millesimal);
            emit onVideoPlayerInformation(d->m_DiskType,d->m_PlayIndex, QFileInfo(temp.at(d->m_PlayIndex)).fileName(), d->m_EndTime);
            emit onVideoPlayerPlayMode(d->m_PlayMode);
            QFileInfo fileInfo(temp.at(d->m_PlayIndex));
            QString filePath = fileInfo.filePath() + QString("/") +
                    fileInfo.created().toString(QString("yyyyMMddhhmmss")) +
                    fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) +
                    QString::number(fileInfo.size());
            int value = MT_Video;
            if(d->m_DiskType == DWT_USBDisk){
                SaveConfigString(filePath.toStdString(),USBVideoPathInfo);
                system("sync");
                SaveConfigString(value,USBMultimedia);
                system("sync");
            }
            else if(d->m_DiskType == DWT_SDDisk){
                SaveConfigString(filePath.toStdString(),SDVideoPathInfo);
                system("sync");
                SaveConfigString(value,SDMultimedia);
                system("sync");
            }
        }
    }
}

void VideoPlayerPrivate::setPlayMode(const VideoPlayerPlayMode mode)
{
    Q_Q(VideoPlayer);
    if (mode != m_PlayMode) {
        m_PlayMode = mode;
    }
    emit q->onVideoPlayerPlayMode(m_PlayMode);
}

void VideoPlayerPrivate::setPlayStatus(const VideoPlayerPlayStatus status)
{
    Q_Q(VideoPlayer);
    if (status != m_PlayStatus) {
        m_PlayStatus = status;
        emit q->onVideoPlayerPlayStatus(m_DiskType, status);
    }

}

void VideoPlayerPrivate::pauseToggleHandler(const QString &output)
{
    if (output.contains(QString("yes"))) {
        setPlayStatus(VPPS_Pause);
    } else if (output.contains(QString("no"))) {
        setPlayStatus(VPPS_Play);
        if (VideoPlayerPrivate::CT_Seek == m_LastCommand) {
        }
    }
}

void VideoPlayerPrivate::startHandler(const QString &output)
{
}

void VideoPlayerPrivate::endTimePositionHandler(const QString &output)
{
    Q_Q(VideoPlayer);
    QString keyWord("ANS_LENGTH=");
    int startPos = output.indexOf(keyWord);
    int length = keyWord.length();
    QString endTime;
    endTime.clear();
    QList<QString> temp = getPathList(m_DiskType);
    if (temp.size() > m_PlayIndex) {
        m_EndTime = 0;
        if (-1 != startPos) {
            for (int i = 0; i < (output.length() - startPos); ++i) {
                if (QChar('\n') == output.at(startPos + length + i)) {
                    m_EndTime = static_cast<int>(endTime.toFloat());
                    break;
                }
                endTime += output.at(startPos + length + i);
            }
        }
        //qDebug()<<"+++++++endTimePositionHandler+++++++++++";
        emit q->onVideoPlayerInformation(m_DiskType, m_PlayIndex, QFileInfo(temp.at(m_PlayIndex)).fileName(), m_EndTime);
        setPlayStatus(VPPS_Play);
    } else {
        qCritical() << temp.size() << m_PlayIndex;
    }
}

void VideoPlayerPrivate::millesimalHandler(const QString &output)
{
    int start = output.indexOf(QChar('=')) + 1;
    int end = output.indexOf(QChar('\n'));
    if ((-1 != start)
            && (-1 != end)) {
        bool ok(false);
        int millesimal = static_cast<int>(output.mid(start, end - start).toFloat(&ok) * 1000);
        if (ok) {
            m_Millesimal = millesimal;
        }
    }
}

void VideoPlayerPrivate::timePositionHandler(const QString &output)
{
    Q_Q(VideoPlayer);
    if (output.startsWith(QString("ANS_TIME_POSITION="), Qt::CaseSensitive)) {
        int start = output.indexOf(QChar('=')) + 1;
        int end = output.indexOf(QChar('\n'));
        if ((-1 != start)
                && (-1 != end)) {
            bool ok = true;
            int elapsed = output.mid(start, end - start).toInt(&ok);
            if (ok) {
                if (((qAbs(m_ElapsedTime - elapsed) > 5))
                        || (elapsed % 5)) {
                    if(m_DiskType == DWT_USBDisk){
                        SaveConfigString(elapsed,USBVideoMillesmial);
                        system("sync");
                    }
                    else if(m_DiskType == DWT_SDDisk){
                        SaveConfigString(elapsed,SDVideoMillesmial);
                        system("sync");
                    }
                }
                m_ElapsedTime = elapsed;
                emit q->onVideoPlayerElapsedInformation(m_ElapsedTime, m_Millesimal);
            }
        }
    }
}

QList<QString> &VideoPlayerPrivate::getPathList(const int type)
{
    switch (type) {
    case DWT_SDDisk: {
        return m_SDPathList;
        break;
    }
    case DWT_USBDisk:
    default: {
        return m_USBPathList;
        break;
    }
    }
}

void VideoPlayerPrivate::endOfFileHandler()
{
    Q_Q(VideoPlayer);
    switch (m_PlayStatus) {
    case VPPS_Start:
    case VPPS_Unsupport:
    case VPPS_Pause:
    case VPPS_Stop:
    case VPPS_Play: {
        switch (m_PlayMode) {
        case VPPM_AllRepeat: {
            if ((getPathList(m_DiskType).size() - 1) > m_PlayIndex) {
                ++m_PlayIndex;
            } else {
                m_PlayIndex = 0;
            }
            break;
        }
        case VPPM_RepeatOnce: {
            break;
        }
        case VPPM_Shuffle:
        default : {
            m_PlayIndex = qrand() % getPathList(m_DiskType).size();
            break;
        }
        }
        q->playVideoIndex(m_DiskType, m_PlayIndex, m_X, m_Y, m_Width, m_Height, 0);
        break;
    }
    default: {
        break;
    }
    }
}

void VideoPlayerPrivate::unsupportHandler()
{
    Q_Q(VideoPlayer);
    QList<QString> temp = getPathList(m_DiskType);
    if (m_PlayIndex < temp.size()) {
        m_ElapsedTime = 0;
        m_Millesimal = 0;
        emit q->onVideoPlayerElapsedInformation(m_ElapsedTime, m_Millesimal);
        m_EndTime = 0;
        emit q->onVideoPlayerInformation(m_DiskType, m_PlayIndex, QFileInfo(temp.at(m_PlayIndex)).fileName(), m_EndTime);
        setPlayStatus(VPPS_Unsupport);
        endOfFileHandler();
    }
}

//handler for can't drag and drop for video,from wandz at 20190425
void VideoPlayerPrivate::undragAndDropHandler()
{
    setPlayStatus(VPPS_UndragAndDrop);
}

void VideoPlayerPrivate::seekToMillesimal(const int millesimal)
{
    Q_Q(VideoPlayer);
    if ((VPPS_Pause == m_PlayStatus)
            || (VPPS_Play == m_PlayStatus)) {
        QString cmd("seek %1.%2 1\n");
        cmd = cmd.arg(millesimal / 10, 3, 10, QChar(' ')).arg(millesimal % 10, 1, 10, QChar('0'));
        writeCmdToMplayer(cmd.toLocal8Bit().data());
    } else {
        q->playVideoIndex(m_DiskType, m_PlayIndex, m_X, m_Y, m_Width, m_Height, 0);
    }
}

void VideoPlayerPrivate::createSDFileNamesXml()
{
    QDomDocument domDocument;
    domDocument.clear();
    domDocument.appendChild(domDocument.createElement(QString("VideoPlayer")));
    QDomElement root = domDocument.firstChildElement(QString("VideoPlayer"));
    QDomElement fileNames;
    QDomElement info;
    fileNames = domDocument.createElement(QString("SDFileNames"));
    root.appendChild(fileNames);
    m_SDFileNamesXml.clear();
    string pathInfo;
    LoadConfigString(pathInfo,SDVideoPathInfo);
    QString sdPersistantPathInfo = QString::fromStdString(pathInfo);
    QStringList stringList = sdPersistantPathInfo.split(QChar('/'));
    QString sdPersistantPath = sdPersistantPathInfo.left(sdPersistantPathInfo.length() - stringList.last().length() - 1);
    QString persistantIndex("");
    QFileInfo fileInfo;
    int i;
    for (i = 0; i < m_SDPathList.size(); ++i) {
        info = domDocument.createElement(QString("Index:" + QString::number(i)));
        fileNames.appendChild(info);
        fileInfo.setFile(m_SDPathList.at(i));
        info.appendChild(domDocument.createTextNode(fileInfo.fileName()));
        if (persistantIndex.isEmpty()) {
            if (fileInfo.filePath() == sdPersistantPath) {
                if (sdPersistantPathInfo == (fileInfo.filePath() + QString("/") + fileInfo.created().toString(QString("yyyyMMddhhmmss")) + fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) + QString::number(fileInfo.size()))) {
                    persistantIndex = QString::number(i);
                }
            }
        }
    }
    QDomElement persistant = domDocument.createElement(QString("SDPersistant"));
    root.appendChild(persistant);
    int mediaType = 0;
    LoadConfigString(mediaType,SDMultimedia);
    unsigned char multimediaType = (unsigned char)mediaType;
    int videoMillesmial = 0;
    if (persistantIndex.isEmpty()) {
        if (0 == i) {
            persistantIndex = QString("-1");
            multimediaType = MT_Idle;
        } else {
            if (0 == multimediaType) {
                multimediaType = MT_Video;
                SaveConfigString(multimediaType,SDMultimedia);
                system("sync");
            }
            persistantIndex = QString("0");
        }
    } else {
        LoadConfigString(videoMillesmial,SDVideoMillesmial);
    }
    QString data = persistantIndex + QString("-") + QString::number(videoMillesmial);
    //qDebug()<<__PRETTY_FUNCTION__<<"line:"<<__LINE__<<"sd videoMillesmia"<<data;
    persistant.appendChild(domDocument.createTextNode(data));
    QDomElement type = domDocument.createElement(QString("SDType"));
    root.appendChild(type);
    type.appendChild(domDocument.createTextNode(QString::number(multimediaType)));
    m_SDFileNamesXml = domDocument.toString();
}

void VideoPlayerPrivate::createUSBFileNamesXml()
{
    QDomDocument domDocument;
    domDocument.clear();
    domDocument.appendChild(domDocument.createElement(QString("VideoPlayer")));
    QDomElement root = domDocument.firstChildElement(QString("VideoPlayer"));
    QDomElement fileNames;
    QDomElement info;
    fileNames = domDocument.createElement(QString("USBFileNames"));
    root.appendChild(fileNames);
    m_USBFileNamesXml.clear();
    string pathInfo;
    LoadConfigString(pathInfo,USBVideoPathInfo);
    QString usbPersistantPathInfo = QString::fromStdString(pathInfo);
    QStringList stringList = usbPersistantPathInfo.split(QChar('/'));
    QString usbPersistantPath = usbPersistantPathInfo.left(usbPersistantPathInfo.length() - stringList.last().length() - 1);
    QString persistantIndex("");
    QFileInfo fileInfo;
    int i;
    for (i = 0; i < m_USBPathList.size(); ++i) {
        info = domDocument.createElement(QString("Index:" + QString::number(i)));
        fileNames.appendChild(info);
        fileInfo.setFile(m_USBPathList.at(i));
        info.appendChild(domDocument.createTextNode(fileInfo.fileName()));
        if (persistantIndex.isEmpty()) {
            if (fileInfo.filePath() == usbPersistantPath) {
                if (usbPersistantPathInfo == (fileInfo.filePath() + QString("/") + fileInfo.created().toString(QString("yyyyMMddhhmmss")) + fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) + QString::number(fileInfo.size()))) {
                    persistantIndex = QString::number(i);
                }
            }
        }
    }
    QDomElement persistant = domDocument.createElement(QString("USBPersistant"));
    root.appendChild(persistant);
    int mediaType = 0;
    LoadConfigString(mediaType,USBMultimedia);
    unsigned char multimediaType = (unsigned char)mediaType;
    int videoMillesmial = 0;
    if (persistantIndex.isEmpty()) {
        if (0 == i) {
            persistantIndex = QString("-1");
            multimediaType = MT_Idle;
        } else {
            if (0 == multimediaType) {
                multimediaType = MT_Video;
                SaveConfigString(multimediaType,USBMultimedia);
                system("sync");
            }
            persistantIndex = QString("0");
        }
    } else {
        LoadConfigString(videoMillesmial,USBVideoMillesmial);
    }
    QString data = persistantIndex + QString("-") + QString::number(videoMillesmial);
    //qDebug()<<__PRETTY_FUNCTION__<<"line:"<<__LINE__<<"usb videoMillesmia"<<data;
    persistant.appendChild(domDocument.createTextNode(data));
    QDomElement type = domDocument.createElement(QString("USBType"));
    root.appendChild(type);
    type.appendChild(domDocument.createTextNode(QString::number(multimediaType)));
    m_USBFileNamesXml = domDocument.toString();
}

void VideoPlayerPrivate::videoPlayerSetPlay()
{
    switch (m_PlayStatus) {
    case VPPS_Play: {
        break;
    }
    case VPPS_Pause: {
        char* cmd = "pause\n";
        writeCmdToMplayer(cmd);
        break;
    }
    default: {
        return;
        break;
    }
    }
}

void VideoPlayerPrivate::videoPlayerSetSuspend()
{
    if (m_Suspend) {
        m_Suspend = false;
        videoPlayerSetPlay();
    } else {
        if ((VPPS_Play == m_PlayStatus)
                || (VPPS_Start == m_PlayStatus)) {
            m_Suspend = true;
            videoPlayerSetPause();
        } else if (VPPS_Pause == m_PlayStatus) {
            char* cmd = "pause\n";
            writeCmdToMplayer(cmd);
        }
    }
}

void VideoPlayerPrivate::videoPlayerSetPause()
{
    switch (m_PlayStatus) {
    case VPPS_Start:
    case VPPS_Play: {
        char* cmd = "pause\n";
        writeCmdToMplayer(cmd);
        break;
    }
    case VPPS_Pause: {
        break;
    }
    default: {
        return;
        break;
    }
    }
}

void VideoPlayerPrivate::videoPlayerSetExit(const DeviceWatcherType type)
{
    if (type == m_DiskType) {
        exitVideoPlayer();
    }
}

void VideoPlayerPrivate::exitVideoPlayer()
{
    Q_Q(VideoPlayer);
    if(m_Process.size() != 0){
        QByteArray processArray = m_Process.toLatin1();
        char *process = processArray.data();
        if(isRunning(process)){
            quitMpalyer();
            setPlayStatus(VPPS_Exit);
        }
    }
}

void VideoPlayerPrivate::videoPlayerVisible(const bool flag)
{
    if(m_Process.size() != 0){
        QByteArray processArray = m_Process.toLatin1();
        char *process = processArray.data();
        if(isRunning(process)){
            if (flag) {
                char* cmd = "takevideo\n";
                writeCmdToMplayer(cmd);
            } else {
                char* cmd = "giveupvideo\n";
                writeCmdToMplayer(cmd);
            }
        }
    }
}
int VideoPlayerPrivate::isRunning(char *process)
{
    int ret = 0;
    char sCurrPid[16] = {0};
    sprintf(sCurrPid, "%d", getpid());

    FILE *fstream=NULL;
    char buff[1024] = {0};

    char cmd[128]="";
    strcat(cmd,"ps -e -o pid,comm | grep ");
    strcat(cmd, process);
    strcat(cmd, " | grep -v PID | grep -v grep");
    //printf("=== isRunning cmd: %s\n",cmd);
    if(NULL==(fstream=popen(cmd, "r")))
    {
        printf("execute command failed:");
        return -1;
    }
    while(NULL!= fgets(buff, sizeof(buff), fstream)) {
        char *oldPID = getPidFromStr(buff);
        if ( strcmp(sCurrPid, oldPID) != 0 ) {
            printf("Runing，PID=%s\n", oldPID);
            ret = 1;
        }
    }
    pclose(fstream);

    return ret;
}

QString VideoPlayerPrivate::chgform(QString name)
{
    //printf("+++++++chgform_name:%s\n",name);
   // qDebug()<<"+++[MusicPlayerPrivate::chgform]+++"<< name;
    QString leftStr;
    QString str;
    str.clear();
    while(name.size())
    {
        leftStr= name.left(1);
        if(name.size()>1)
        {
            name = name.right(name.size()-1);
        }
        else{
            name.clear();
        }
        if(leftStr == " ")
        {
            leftStr = "\\ ";
        }
        str += leftStr;
    }
    return str;
//    char buf[512] = {0};
//    qDebug()<<"++1111++";
//    char *p       = strtok(name, " ");
//    printf( "%s\n", p );
//    if(p != NULL){
//         strcat(buf, p);
//         strcat(buf, "\\ ");
//    }
//    p = strtok(NULL, " ");
//    while(p != NULL)
//    {
//        if(strcmp(p,"\0")){
//            strcat(buf, "\\ ");
//        }
//        else{
//            strcat(buf, p);
//            strcat(buf, "\\ ");
//        }
//        p = strtok(NULL, " ");
//    }
}
void VideoPlayerPrivate::writeCmdToMplayer(char* cmd)
{
    Q_Q(VideoPlayer);
    printf("+++[MusicPlayerPrivate::writeCmdToMplayer:%s]+++\n",cmd);
    if(m_Fd_w < 0)
    {
        //qDebug()<<"+++++++ffff0000++++++++";
        m_Fd_w = open("/data/VideoPipe",O_WRONLY | O_NONBLOCK | O_TRUNC);
        if(m_Fd_w < 0)
        {
            qDebug()<<"+++[MusicPlayerPrivate::writeCmdToMplayer:]+++";
            perror("open fifo error\n");
            m_ReadpThread = 0;
            m_Process.clear();
            return;
        }
    }
    ftruncate(m_Fd_w,0);
    lseek(m_Fd_w,0,SEEK_SET);
    write(m_Fd_w,cmd,strlen(cmd));
}
void VideoPlayerPrivate::quitMpalyer(){
    Q_Q(VideoPlayer);
    qDebug()<<"+++[VideoPlayerPrivate::quitMpalyer()]+++";
    char* cmd = "quit\n";
    writeCmdToMplayer(cmd);
    m_ReadpThread = 0;
    usleep(500*1000);
    emit q->onVideoPlayerExit();
}
ssize_t VideoPlayerPrivate::readLine(int fd, char *buffer, size_t n)
{
    ssize_t numRead;
    size_t totRead;
    char *buf;
    char ch;
    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }
    buf = buffer;
    totRead = 0;
    for (;;) {
        int flags = fcntl(fd,F_GETFL,0);
        flags |= O_NONBLOCK;
        fcntl(fd,F_SETFL,flags);
        numRead = read(fd, &ch, 1);
        if (-1 == numRead) {
            if (errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        } else if (numRead == 0) {
            if (totRead == 0) {
                return 0;
            } else {
                break;
            }
        } else {
            if (totRead < n - 1) {
                totRead++;
                *buf++ = ch;
            }
            if (ch == '\n') {
                break;
            }
        }
    }

    *buf = '\0';
    return totRead;
}
void VideoPlayerPrivate::onVideoPlayerVisible(bool visible)
{
    Q_Q(VideoPlayer);
    emit q->onVideoPlayerVisible(visible);
}

void VideoPlayerPrivate::sliderTouchEnable(bool enable){
    Q_Q(VideoPlayer);
    emit q->onSliderTouchEnable(enable);
}
void VideoPlayerPrivate::videoPlayerPlayStatus(const int type, const int status){
    Q_Q(VideoPlayer);
    emit q->onVideoPlayerPlayStatus(type, status);
}
