#include "MusicPlayer.h"
#include "DeviceWatcher/DeviceWatcher.h"
#include "AutoConnect.h"
#include "Utility.h"
#include "UserInterfaceUtility.h"
#include "MusicInformation/MusicInformation.h"
#include <QTimer>
#include <QDBusConnection>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDomDocument>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
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
#include "configUtils.h"
using namespace std;
static const string SDMultimedia("/data/MultiMediaFile/SDMultimedia");
static const string SDMusicMillesmial("/data/MultiMediaFile/SDMusicMillesmial");
static const string SDMusicPathInfo("/data/MultiMediaFile/SDMusicPathInfo");
static const string USBMultimedia("/data/MultiMediaFile/USBMultimedia");
static const string USBMusicMillesmial("/data/MultiMediaFile/USBMusicMillesmial");
static const string USBMusicPathInfo("/data/MultiMediaFile/USBMusicPathInfo");

int getCrrentMusicPlayMode()
{
    QFile file("/etc/playmode");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<Q_FUNC_INFO<<"file :/etc/playmode is open failed";
        return 0;
    }
    QTextStream in(&file);
    QString line = in.readLine();
    qDebug() << __PRETTY_FUNCTION__<<line.toInt();
    file.flush();
    file.close();

    if(line.isEmpty()){
        return 0;
    }

    return line.toInt();
}

class MusicPlayerPrivate
{
    Q_DISABLE_COPY(MusicPlayerPrivate)
public:
    explicit MusicPlayerPrivate(MusicPlayer *parent);
    ~MusicPlayerPrivate();
    void connectAllSlots();
    QList<QString>& getPathList(const DeviceWatcherType type);
    void seekToMillesimal(const int millesimal);
    void setPlayStatus(const MusicPlayerPlayStatus status);
    void setPlayMode(const MusicPlayerPlayMode mode);
    void pauseToggleHandler(const QString& output);
    void timeLengthHandler(const QString &output);
    void millesimalHandler(const QString& output);
    void timePositionHandler(const QString &output);
    void quitHandler();
    void stopHandler();
    void endOfFileHandler();
    void unsupportHandler();
    void exitMusicPlayer();
    void createUSBFileNamesXml();
    void createSDFileNamesXml();
    void musicPlayerSetPlay();
    void musicPlayerSetPause();
    void musicPlayerSetStop();
    void musicPlayerSetExit(const DeviceWatcherType type);
    void musicPlayerPlayStatus(int type,int status);
    void writeCmdToMplayer(char* cmd);
    void quitMpalyer();
    QString chgform(QString name);
    ssize_t readLine(int fd, char *buffer, size_t n);
    MusicPlayerPlayMode m_PlayMode;
    MusicPlayerPlayStatus m_PlayStatus;
    QList<QString> m_USBPathList;
    QList<QString> m_SDPathList;
    QString m_USBFileNamesXml;
    QString m_SDFileNamesXml;
    QStringList m_USBFileArtistList;
    QStringList m_SdFileArtistList;
    DeviceWatcherType m_DiskType;
    QString m_CurrentFilePath;
    MusicInformation m_MusicInformation;
    int m_PlayIndex;
    int m_ElapsedTime;
    int m_EndTime;
    int m_Millesimal;
    int m_ReadpThread;
    int fd[2];
    int m_Fd_w;
    bool m_IsRunning;
    pid_t m_pid1;
    pid_t m_pid2;
private:
    Q_DECLARE_PUBLIC(MusicPlayer)
    MusicPlayer* const q_ptr;
};

static void* readyReadStandardOutput(void* arg)
{
    MusicPlayerPrivate* pThis = (MusicPlayerPrivate*)arg;
    char info[512] = {0};
    int  ret = 0;
    while (1) {
        if(pThis->m_ReadpThread == 0)
        {
            qDebug()<<"+++[MusicPlayer::readyReadStandardOutput:pThis->m_ReadpThread]+++"<<pThis->m_ReadpThread;
            break;//退出线程
        }
        bzero(info, 512);
        ret = pThis->readLine(pThis->fd[0], info, 512);
        while(ret > 0)
        {
            string str(info);
            QString output = QString::fromStdString(str);
            //qDebug()<<"mplayer music:"<<output;
            //音频的长度,时间长度
            if (output.contains(QString("ANS_LENGTH="))) {
                pThis->timeLengthHandler(output);
            }
            //音频播放位置的百分比
            else if (output.contains(QString("ANS_PERCENT_POSITION="))) {
                pThis->millesimalHandler(output);
            }
            //音频播放的时间点
            else if (output.contains(QString("ANS_TIME_POSITION="))) {
                pThis->timePositionHandler(output);
            }
            //音频暂停与播放切换成功与否
            else if (output.contains(QString("ANS_pause"))) {
               pThis->pauseToggleHandler(output);
            }
            //音频播放完成
            else if (output.contains(QString("EOF code: 1"))) {
                sleep(2);
                pThis->setPlayStatus(MPPS_Stop);
                pThis->endOfFileHandler();
            }
            else if (output.contains(QString("EOF code: 4"))
                     || (output.contains(QString("EOF code: 2")))) {
                pThis->stopHandler();
            }
            //不支持的音频
            else if ((output.contains(QString("incorrect streams")))
                || (output.contains(QString("No stream found")))){
                sleep(2);
                pThis->unsupportHandler();
            }/* else if (output.contains(QString("seek-begin"))) {
            } *///进度条改变播放完成
            else if ((output.contains(QString("seek-finish"))
                        || (output.contains(QString("seek-error"))))) {
              pThis->musicPlayerPlayStatus(pThis->m_DiskType, MPPS_SeekFinish);
            }
            else{
                if(output != "\n")
                    qDebug()<<"output:"<<output;
            }
            bzero(info, 512);
            ret = pThis->readLine(pThis->fd[0], info, 512);
        }
        if(ret <= 0){
            sleep(1);
        }
    }
    close(pThis->fd[0]);
    pThis->m_IsRunning   = false;
    if(pThis->m_Fd_w > 0)
    {
        close(pThis->m_Fd_w);
        pThis->m_Fd_w  = -1;
    }
    pThis->m_pid2 = wait(NULL);//回收mpalyer的进程号，避免mpalyer成为僵尸进程
    printf("*********MusicPlayer_pid2= %d \n",pThis->m_pid2);//wait的返回值为子进程的pid
    return NULL;

}

void MusicPlayer::musicPlayerRequestFileNames(const DeviceWatcherType type)
{
    Q_D(MusicPlayer);
    switch (type) {
    case DWT_SDDisk: {
        emit onMusicPlayerFileNames(DWT_SDDisk, d->m_SDFileNamesXml);
        break;
    }
    case DWT_USBDisk: {
        emit onMusicPlayerFileNames(DWT_USBDisk, d->m_USBFileNamesXml);
        break;
    }
    default: {
        break;
    }
    }
}

void MusicPlayer::musicPlayerSetPlayModeToggle()
{
    Q_D(MusicPlayer);
    switch (d->m_PlayMode) {
    case MPPM_AllRepeat: {
        musicPlayerSetPlayMode(MPPM_Shuffle);
        break;
    }
    case MPPM_Shuffle: {
        musicPlayerSetPlayMode(MPPM_RepeatOnce);
        break;
    }
    case MPPM_RepeatOnce:
    default: {
        musicPlayerSetPlayMode(MPPM_AllRepeat);
        break;
    }
    }
}

void MusicPlayer::musicPlayerSetPlayMode(const MusicPlayerPlayMode mode)
{
    Q_D(MusicPlayer);
    d->setPlayMode(mode);
}

void MusicPlayer::musicPlayerSetPlayStatusToggle()
{
    Q_D(MusicPlayer);
    qDebug()<<"++++++++d->m_PlayStatus+++++++"<<d->m_PlayStatus;
    switch (d->m_PlayStatus) {
    case MPPS_Play: {
        musicPlayerSetPlayStatus(MPPS_Pause);
        break;
    }
    case MPPS_Unsupport: {
        playMusicIndex(d->m_DiskType, d->m_PlayIndex, 0);
        break;
    }
    case MPPS_Pause:
    default: {
        musicPlayerSetPlayStatus(MPPS_Play);
        break;
    }
    }
}

void MusicPlayer::musicPlayerSetPlayStatus(const MusicPlayerPlayStatus status)
{
    Q_D(MusicPlayer);
    switch (status) {
    case MPPS_Play: {
        d->musicPlayerSetPlay();
        break;
    }
    case MPPS_Pause: {
        d->musicPlayerSetPause();
        break;
    }
    case MPPS_Stop: {
        d->musicPlayerSetStop();
        break;
    }
    case MPPS_Exit: {
        break;
    }
    default : {
        break;
    }
    }
}

void MusicPlayer::musicPlayerPlayListViewIndex(const DeviceWatcherType type, const int index, const int millesimal)
{
    Q_D(MusicPlayer);
    playMusicIndex(type, index, millesimal);
}

void MusicPlayer::musicPlayerPlayPreviousListViewIndex()
{
    Q_D(MusicPlayer);
    int lastIndex;
    if (MPPM_Shuffle ==  d->m_PlayMode) {
        lastIndex = qrand() %  d->getPathList( d->m_DiskType).size();
    } else {
        lastIndex =  d->m_PlayIndex;
        if ((lastIndex > 0)
                && (lastIndex <=  d->getPathList( d->m_DiskType).size() - 1)) {
            --lastIndex;
        } else {
            lastIndex =  d->getPathList( d->m_DiskType).size() - 1;
        }
    }
    musicPlayerPlayListViewIndex( d->m_DiskType, lastIndex, 0);
}

void MusicPlayer::musicPlayerPlayNextListViewIndex()
{
    Q_D(MusicPlayer);
    int lastIndex;
    if (MPPM_Shuffle == d->m_PlayMode) {
        lastIndex = qrand() % d->getPathList(d->m_DiskType).size();
    } else {
        lastIndex = d->m_PlayIndex;
        if ((lastIndex < (d->getPathList(d->m_DiskType).size() - 1))
                && (lastIndex >= 0)) {
            ++lastIndex;
        } else {
            lastIndex = 0;
        }
    }
    musicPlayerPlayListViewIndex(d->m_DiskType, lastIndex, 0);
}

void MusicPlayer::musicPlayerSeekToMillesimal(const int millesimal)
{
    Q_D(MusicPlayer);
    if (d->m_IsRunning) {
        d->seekToMillesimal(millesimal);
    }
}

void MusicPlayer::musicPlayerExit()
{
    Q_D(MusicPlayer);
    d->musicPlayerSetExit(d->m_DiskType);
}

void MusicPlayer::onDeviceWatcherStatus(const DeviceWatcherType type, const DeviceWatcherStatus status)
{
    Q_D(MusicPlayer);
    if (DWT_SDDisk == type) {
        switch (status) {
        case DWS_Empty: {
            d->m_SDPathList.clear();
            d->m_SdFileArtistList.clear();
            break;
        }
        case DWS_Busy: {
            d->m_SDPathList.clear();
            d->m_SdFileArtistList.clear();
            break;
        }
        case DWS_Ready: {
            d->createSDFileNamesXml();
            musicPlayerRequestFileNames(DWT_SDDisk);
            emit onMusicPlayerFileArtist(DWT_SDDisk,d->m_SdFileArtistList);
            break;
        }
        case DWS_Remove: {
            d->m_SDPathList.clear();
            d->m_SdFileArtistList.clear();
            if (DWT_SDDisk == d->m_DiskType) {
                if (d->m_IsRunning) {
                    d->quitMpalyer();
                }
                d->setPlayStatus(MPPS_Exit);
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
            d->m_USBFileArtistList.clear();
            break;
        }
        case DWS_Busy: {
            d->m_USBPathList.clear();
            d->m_USBFileArtistList.clear();
            break;
        }
        case DWS_Ready: {
            d->createUSBFileNamesXml();
            musicPlayerRequestFileNames(DWT_USBDisk);
            emit onMusicPlayerFileArtist(DWT_USBDisk,d->m_USBFileArtistList);
            break;
        }
        case DWS_Remove: {
            d->m_USBPathList.clear();
            d->m_USBFileArtistList.clear();
            if (DWT_USBDisk == d->m_DiskType) {
                if (d->m_IsRunning) {
                    d->quitMpalyer();
                }
                d->setPlayStatus(MPPS_Exit);
            }
            break;
        }
        default : {
            break;
        }
        }
    }
}

void MusicPlayer::onMusicFilePath(const QString &path, const DeviceWatcherType type)
{
    Q_D(MusicPlayer);
    if (DWT_USBDisk == type) {
        d->m_USBPathList.append(path);
    } else if (DWT_SDDisk == type) {
        d->m_SDPathList.append(path);
    }
}

void MusicPlayer::onTimeout()
{
    Q_D(MusicPlayer);
    d->endOfFileHandler();
}

void MusicPlayer::playMusicIndex(const DeviceWatcherType type, const int index, const int millesimal)
{
    Q_D(MusicPlayer);
    QList<QString> temp = d->getPathList(type);
    qDebug()<<"+++[MusicPlayer::playMusicIndex]+++";
    if ((temp.size() > index)
            && (QFile(temp.at(index))).exists()) {
        d->m_CurrentFilePath = temp.at(index);
        d->m_DiskType = type;
        d->m_ElapsedTime = 0;
        d->m_EndTime = 0;
        d->m_PlayIndex = index;
        d->m_Millesimal = 0;
        //qDebug()<<
        //启动一个上一次播放过的音频，如果mplayer已经启动，则先退出mplayer
        if (0 != millesimal) {
            if(d->m_IsRunning)
            {
                d->quitMpalyer();
                sleep(2);//等待mplayer完全退出，m_IsRunning置为false
            }
        }
        d->m_DiskType = type;
        if (d->m_IsRunning == false){
            //创建一个有名管道/data/pipe，如果不存在直接创建，存在则先删除再创建
            if(access("/data/pipe",F_OK) != 0)
            {
                printf("pipe not exist\n");
                mkfifo("/data/pipe",0777);
            }
            else{
                unlink("/data/pipe");
                mkfifo("/data/pipe", 0777);
            }
            //初始化无名管道，这步不能少
            if(pipe(d->fd) < 0){
                perror("pipe error\n");
                return;
            }
            fcntl(d->fd[0], F_SETFL, O_NONBLOCK); //无名管道设为非阻塞
            d->m_pid1 = vfork();//创建一个子进程，用来启动mplayer进程
            printf("+++++++++00000Music_pid++++++++\n");
            if(d->m_pid1 < 0){
                perror("fork error\n");
            }
            else if(d->m_pid1 == 0)
            {
                printf("child:video0000%d\n",getpid());
                close(d->fd[0]);
                dup2(d->fd[1], STDERR_FILENO);//将该子进程的标准输出重定向到无名管道,也就是把mplayer的标准输出重定向到无名管道
                QByteArray _NameByte = d->m_CurrentFilePath.toLocal8Bit();
                const char* _Name    = _NameByte.data();
                if(millesimal == 0){
                    //启动一个从头开始播放音频的mplayer进程　"-input", "file=/data/pipe"是通知mplayer使用该有名管道通信
                   execlp("/usr/bin/mplayer", "mplayer", "-slave", "-quiet", "-idle","-novideo","-input","file=/data/pipe", _Name,NULL);	//命名管道pipe有命令数据，mplayer会自动读取
                }
                else{
                    //启动一个从固定位置播放音频的mplayer进程
                   const char* _ChMillesimal = std::to_string(millesimal).c_str();
                   execlp("/usr/bin/mplayer", "mplayer", "-slave", "-quiet", "-idle","-novideo","-input","file=/data/pipe","-ss",_ChMillesimal,_Name,NULL);
                }
            }
            else{
                emit onMusicPlayerPlayMode(d->m_PlayMode);
                d->m_PlayStatus = MPPS_Start;
                emit onMusicPlayerPlayStatus(d->m_DiskType, d->m_PlayStatus);
                QFileInfo fileInfo(d->m_CurrentFilePath);
                d->m_MusicInformation.parserFilePath(d->m_CurrentFilePath);
                emit onMusicPlayerElapsedInformation(d->m_ElapsedTime, d->m_Millesimal);
                emit onMusicPlayerID3TagChange(d->m_DiskType,
                                                         d->m_PlayIndex,
                                                         fileInfo.filePath(),
                                                         d->m_MusicInformation.getTitle(),
                                                         d->m_MusicInformation.getArtist(),
                                                         d->m_MusicInformation.getAlbum(),
                                                         d->m_EndTime);
                QString filePath = fileInfo.filePath() + QString("/") +
                        fileInfo.created().toString(QString("yyyyMMddhhmmss")) +
                        fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) +
                        QString::number(fileInfo.size());
                int value = MT_Music;
                if(d->m_DiskType == DWT_USBDisk){
                    SaveConfigString(filePath.toStdString(),USBMusicPathInfo);
                    system("sync");
                    SaveConfigString(value,USBMultimedia);
                    system("sync");
                }
                else if(d->m_DiskType == DWT_SDDisk){
                    SaveConfigString(filePath.toStdString(),SDMusicPathInfo);
                    system("sync");
                    SaveConfigString(value,SDMultimedia);
                    system("sync");
                }
                d->m_ReadpThread = 1;//线程退出标志
                close(d->fd[1]);
                usleep(200*1000);
                d->m_IsRunning = true;
                pthread_t pthead;
                int ret = pthread_create(&pthead, NULL,readyReadStandardOutput,d);//创建一个线程用来读取mplayer的标准输出
                if(ret != 0) {
                    printf("pthread_create failed!\n");
                }
            }
        }
        else{
            //加载音频项到mplayer
            char pathname[512] = {0};
            strcat(pathname, "loadfile ");
            QString _FilePath = d->chgform(d->m_CurrentFilePath);
            //qDebug()<<"++++filePath000+++++"<< _FilePath;
            QByteArray _NameByte = _FilePath.toLocal8Bit();
            char* _Name   = _NameByte.data();
            strcat(pathname, _Name);
            strcat(pathname, "\n");
            printf("+++++++++++++m_Fd_w_pathname:%s", pathname);
            //使用有名管道/data/pipe向mplayer发送命令
            if(d->m_Fd_w < 0)
            {
                 d->m_Fd_w = open("/data/pipe",O_WRONLY| O_NONBLOCK | O_TRUNC);
                 if(d->m_Fd_w < 0)
                 {
                     perror("open file error");
                     d->m_ReadpThread = 0;
                     return;
                 }
            }
            ftruncate(d->m_Fd_w,0);
            lseek(d->m_Fd_w,0,SEEK_SET);
            write(d->m_Fd_w, pathname, strlen(pathname));
            //这些是与UI界面通信的代码
            emit onMusicPlayerPlayMode(d->m_PlayMode);
            d->m_PlayStatus = MPPS_Start;
            emit onMusicPlayerPlayStatus(d->m_DiskType, d->m_PlayStatus);
            QFileInfo fileInfo(d->m_CurrentFilePath);
            d->m_MusicInformation.parserFilePath(d->m_CurrentFilePath);
            emit onMusicPlayerElapsedInformation(d->m_ElapsedTime, d->m_Millesimal);
            emit onMusicPlayerID3TagChange(d->m_DiskType,
                                                     d->m_PlayIndex,
                                                     fileInfo.filePath(),
                                                     d->m_MusicInformation.getTitle(),
                                                     d->m_MusicInformation.getArtist(),
                                                     d->m_MusicInformation.getAlbum(),
                                                     d->m_EndTime);
            QString filePath = fileInfo.filePath() + QString("/") +
                    fileInfo.created().toString(QString("yyyyMMddhhmmss")) +
                    fileInfo.lastModified().toString(QString("yyyyMMddhhmmss")) +
                    QString::number(fileInfo.size());
            int value = MT_Music;
            if(d->m_DiskType == DWT_USBDisk){
                SaveConfigString(filePath.toStdString(),USBMusicPathInfo);
                system("sync");
                SaveConfigString(value,USBMultimedia);
                system("sync");
            }
            else if(d->m_DiskType == DWT_SDDisk){
                SaveConfigString(filePath.toStdString(),SDMusicPathInfo);
                system("sync");
                SaveConfigString(value,SDMultimedia);
                system("sync");
            }
        }
    }
}

MusicPlayer::MusicPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new MusicPlayerPrivate(this))
{
}

MusicPlayer::~MusicPlayer()
{
}
MusicPlayerPrivate::MusicPlayerPrivate(MusicPlayer *parent)
    : q_ptr(parent)
{
    m_ReadpThread = 1;
    m_IsRunning = false;
    m_Fd_w      = -1;
    connectAllSlots();
    m_PlayMode = getCrrentMusicPlayMode();
}

MusicPlayerPrivate::~MusicPlayerPrivate()
{
    if (m_IsRunning) {
        quitMpalyer();
    }
}

void MusicPlayerPrivate::connectAllSlots()
{
    Q_Q(MusicPlayer);
    connectSignalAndSlotByNamesake(g_DeviceWatcher, q, SLOT(onDeviceWatcherStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_DeviceWatcher, q, SLOT(onMusicFilePath(const QString &, const int)));
}



QList<QString> &MusicPlayerPrivate::getPathList(const int type)
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

void MusicPlayerPrivate::seekToMillesimal(const int millesimal)
{
    Q_Q(MusicPlayer);
    if ((MPPS_Play == m_PlayStatus)
            || (MPPS_Pause == m_PlayStatus)) {
        QString cmd("seek %1.%2 1\n");
        cmd = cmd.arg(millesimal / 10, 3, 10, QChar(' ')).arg(millesimal % 10, 1, 10, QChar('0'));
        QByteArray ba = cmd.toLatin1(); // must
        char* ch;
        ch=ba.data();
        writeCmdToMplayer(ch);//改变播放时间点，也可以用来实现快进和快退
        setPlayStatus(MPPS_Play);
    } else if (MPPS_Stop == m_PlayStatus) {
        q->playMusicIndex(m_DiskType, m_PlayIndex, 0);
    }
}

void MusicPlayerPrivate::setPlayStatus(const MusicPlayerPlayStatus status)
{
    Q_Q(MusicPlayer);
    if (status != m_PlayStatus) {
        m_PlayStatus = status;
    }
    emit q->onMusicPlayerPlayStatus(m_DiskType, m_PlayStatus);
}

void MusicPlayerPrivate::setPlayMode(const MusicPlayerPlayMode mode)
{
    Q_Q(MusicPlayer);
    if (mode != m_PlayMode) {
        m_PlayMode = mode;
    }
    emit q->onMusicPlayerPlayMode(m_PlayMode);
}

void MusicPlayerPrivate::pauseToggleHandler(const QString &output)
{
    if (output.contains(QString("yes"))) {
        setPlayStatus(MPPS_Pause);
    } else if (output.contains(QString("no"))) {
        setPlayStatus(MPPS_Play);
    }
}

void MusicPlayerPrivate::timeLengthHandler(const QString &output)
{
    Q_Q(MusicPlayer);
    int start = output.indexOf(QChar('=')) + 1;
    int end = output.indexOf(QChar('\n'));
    if ((-1 != start)
            && (-1 != end)) {
        m_EndTime = static_cast<int>(output.mid(start, end - start).toFloat());
    } else {
        m_EndTime = 0;
    }
    QFileInfo fileInfo(m_CurrentFilePath);
    m_MusicInformation.parserFilePath(m_CurrentFilePath);
    //qDebug()<<"++++++++++timeLengthHandler++++++++++++";
    emit q->onMusicPlayerID3TagChange(m_DiskType,
                                             m_PlayIndex,
                                             fileInfo.filePath(),
                                             m_MusicInformation.getTitle(),
                                             m_MusicInformation.getArtist(),
                                             m_MusicInformation.getAlbum(),
                                             m_EndTime);
    m_PlayStatus = MPPS_Play;
    emit q->onMusicPlayerPlayStatus(m_DiskType, m_PlayStatus);
}

void MusicPlayerPrivate::millesimalHandler(const QString &output)
{
    int start = output.indexOf(QChar('=')) + 1;
    int end = output.indexOf(QChar('\n'));
    if ((-1 != start)
            && (-1 != end)) {
        bool ok(false);
        int millesimal   = static_cast<int>(output.mid(start, end - start).toFloat(&ok) * 1000);
        if (ok) {
            m_Millesimal = millesimal;
        }
    }
}

void MusicPlayerPrivate::timePositionHandler(const QString &output)
{
    Q_Q(MusicPlayer);
    if (output.startsWith(QString("ANS_TIME_POSITION="), Qt::CaseSensitive)) {
        int start = output.indexOf(QChar('=')) + 1;
        int end = output.indexOf(QChar('\n'));
        if ((-1 != start)
                && (-1 != end)) {
            bool ok(false);
            int elapsed = output.mid(start, end - start).toInt(&ok);
            if (ok) {
                if (((qAbs(m_ElapsedTime - elapsed) > 5))
                        || (elapsed % 5)) {
                    if(m_DiskType == DWT_USBDisk){
                        SaveConfigString(elapsed,USBMusicMillesmial);
                        system("sync");
                    }
                    else if(m_DiskType == DWT_SDDisk){
                        SaveConfigString(elapsed,SDMusicMillesmial);
                        system("sync");
                    }
                }
                m_ElapsedTime = elapsed;
            }
        }
        emit q->onMusicPlayerElapsedInformation(m_ElapsedTime, m_Millesimal);
    }
}

void MusicPlayerPrivate::quitHandler()
{
    setPlayStatus(MPPS_Exit);
}

void MusicPlayerPrivate::stopHandler()
{
    Q_Q(MusicPlayer);
    m_ElapsedTime = 0;
    m_Millesimal = 0;
    setPlayStatus(MPPS_Stop);
    emit q->onMusicPlayerElapsedInformation(m_ElapsedTime, m_Millesimal);
}

void MusicPlayerPrivate::endOfFileHandler()
{
    //qDebug()<<"+++++++endOfFileHandler+++++++";
    Q_Q(MusicPlayer);
    switch (m_PlayStatus) {
    case MPPS_Start:
    case MPPS_Unsupport:
    case MPPS_Pause:
    case MPPS_Stop:
    case MPPS_Play: {
        switch (m_PlayMode) {
        case MPPM_AllRepeat: {
            if ((getPathList(m_DiskType).size() - 1) > m_PlayIndex) {
                ++m_PlayIndex;
            } else {
                m_PlayIndex = 0;
            }
            break;
        }
        case MPPM_RepeatOnce: {
            break;
        }
        case MPPM_Shuffle:
        default : {
            m_PlayIndex = qrand() % getPathList(m_DiskType).size();
            break;
        }
        }
        q->playMusicIndex(m_DiskType, m_PlayIndex, 0);
        break;
    }
    default: {
        break;
    }
    }
}

void MusicPlayerPrivate::unsupportHandler()
{
    Q_Q(MusicPlayer);
    m_EndTime = 0;
    QFileInfo fileInfo(m_CurrentFilePath);
    emit q->onMusicPlayerID3TagChange(m_DiskType,
                                             m_PlayIndex,
                                             fileInfo.filePath(),
                                             QString(),
                                             QString(),
                                             QString(),
                                             m_EndTime);
    m_ElapsedTime = 0;
    m_Millesimal = 0;
    emit q->onMusicPlayerElapsedInformation(m_ElapsedTime, m_Millesimal);
    setPlayStatus(MPPS_Unsupport);
    endOfFileHandler();
}

void MusicPlayerPrivate::exitMusicPlayer()
{
    if (m_IsRunning) {
        quitMpalyer();
        setPlayStatus(MPPS_Exit);
    }
}
void MusicPlayerPrivate::createUSBFileNamesXml()
{
    QDomDocument domDocument;
    domDocument.clear();
    domDocument.appendChild(domDocument.createElement(QString("MusicPlayer")));
    QDomElement root = domDocument.firstChildElement(QString("MusicPlayer"));
    QDomElement fileNames;
    QDomElement info;
    fileNames = domDocument.createElement(QString("USBFileNames"));
    root.appendChild(fileNames);
    m_USBFileNamesXml.clear();
    string pathInfo;
    LoadConfigString(pathInfo,USBMusicPathInfo);
    QString usbPersistantPathInfo = QString::fromStdString(pathInfo);
    QStringList stringList = usbPersistantPathInfo.split(QChar('/'));
    QString usbPersistantPath = usbPersistantPathInfo.left(usbPersistantPathInfo.length() - stringList.last().length() - 1);
    QString persistantIndex("");
    QFileInfo fileInfo;
    int i;
    for (i = 0; i < m_USBPathList.size(); ++i) {
        info = domDocument.createElement(QString("Index:" + QString::number(i)));
        fileNames.appendChild(info);
        m_MusicInformation.parserFilePath(m_USBPathList.at(i));
        m_USBFileArtistList.append(m_MusicInformation.getArtist());
        fileInfo.setFile(m_USBPathList.at(i));
        info.appendChild(domDocument.createTextNode(fileInfo.filePath()));
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
    int musicMillesmial = 0;
    if (persistantIndex.isEmpty()) {
        if (0 == i) {
            persistantIndex = QString("-1");
            multimediaType = MT_Idle;
            int value = MT_Image;
            SaveConfigString(value,USBMultimedia);
            system("sync");
        } else {
            if (0 == multimediaType) {
                multimediaType = MT_Music;
                SaveConfigString(multimediaType,USBMultimedia);
                system("sync");
            }
            persistantIndex = QString("0");
        }
    } else {
        LoadConfigString(musicMillesmial,USBMusicMillesmial);
    }
    QString data = persistantIndex + QString("-") + QString::number(musicMillesmial);
    persistant.appendChild(domDocument.createTextNode(data));
    QDomElement type = domDocument.createElement(QString("USBType"));
    root.appendChild(type);
    type.appendChild(domDocument.createTextNode(QString::number(multimediaType)));
    m_USBFileNamesXml = domDocument.toString();
}

void MusicPlayerPrivate::createSDFileNamesXml()
{
    QDomDocument domDocument;
    domDocument.clear();
    domDocument.appendChild(domDocument.createElement(QString("MusicPlayer")));
    QDomElement root = domDocument.firstChildElement(QString("MusicPlayer"));
    QDomElement fileNames;
    QDomElement info;
    fileNames = domDocument.createElement(QString("SDFileNames"));
    root.appendChild(fileNames);
    m_SDFileNamesXml.clear();
    string pathInfo;
    LoadConfigString(pathInfo,SDMusicPathInfo);
    QString sdPersistantPathInfo = QString::fromStdString(pathInfo);
    QStringList stringList = sdPersistantPathInfo.split(QChar('/'));
    QString sdPersistantPath = sdPersistantPathInfo.left(sdPersistantPathInfo.length() - stringList.last().length() - 1);
    QString persistantIndex("");
    QFileInfo fileInfo;
    int i;
    for (i = 0; i < m_SDPathList.size(); ++i) {
        info = domDocument.createElement(QString("Index:" + QString::number(i)));
        fileNames.appendChild(info);
        m_MusicInformation.parserFilePath(m_SDPathList.at(i));
        m_SdFileArtistList.append(m_MusicInformation.getArtist());
        fileInfo.setFile(m_SDPathList.at(i));
        info.appendChild(domDocument.createTextNode(fileInfo.filePath()));
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
    int musicMillesmial = 0;
    if (persistantIndex.isEmpty()) {
        if (0 == i) {
            persistantIndex = QString("-1");
            multimediaType = MT_Idle;
            int value = MT_Image;
            SaveConfigString(value,SDMultimedia);
            system("sync");
        }else{
            if (0 == multimediaType) {
                multimediaType = MT_Music;
                SaveConfigString(multimediaType,SDMultimedia);
                system("sync");
            }
            persistantIndex = QString("0");
        }
    } else {
        LoadConfigString(musicMillesmial,SDMusicMillesmial);
    }
    QString data = persistantIndex + QString("-") + QString::number(musicMillesmial);
    persistant.appendChild(domDocument.createTextNode(data));
    QDomElement type = domDocument.createElement(QString("SDType"));
    root.appendChild(type);
    type.appendChild(domDocument.createTextNode(QString::number(multimediaType)));
    m_SDFileNamesXml = domDocument.toString();
}

void MusicPlayerPrivate::musicPlayerSetPlay()
{
    switch (m_PlayStatus) {
        case MPPS_Pause: {
            char* cmd = "pause\n";
            writeCmdToMplayer(cmd);//暂停后播放
            break;
        }
        default: {
            break;
        }
    }
}

void MusicPlayerPrivate::musicPlayerSetPause()
{
    switch (m_PlayStatus) {
    case MPPS_Start:
    case MPPS_Play: {
        char* cmd = "pause\n";
        writeCmdToMplayer(cmd);//暂停
    }
    case MPPS_Pause: {
        break;
    }
    default: {
        return ;
        break;
    }
    }
}

void MusicPlayerPrivate::musicPlayerSetStop()
{
    if ((MPPS_Play == m_PlayStatus)
            || (MPPS_Pause == m_PlayStatus)) {
        char* cmd = "stop\n";
        writeCmdToMplayer(cmd);//停止播放
    } else {
        stopHandler();
    }
}

void MusicPlayerPrivate::musicPlayerSetExit(const DeviceWatcherType type)
{
    if (type == m_DiskType) {
        exitMusicPlayer();
    }
}

void MusicPlayerPrivate::musicPlayerPlayStatus(int type,int status){
    Q_Q(MusicPlayer);
    emit q->onMusicPlayerPlayStatus(type, status);
}

void MusicPlayerPrivate::writeCmdToMplayer(char* cmd)
{
    Q_Q(MusicPlayer);
    printf("+++[MusicPlayerPrivate::writeCmdToMplayer:%s]+++\n",cmd);
    if(m_Fd_w < 0)
    {
        m_Fd_w = open("/data/pipe",O_WRONLY | O_NONBLOCK | O_TRUNC);
        if(m_Fd_w < 0)
        {
            qDebug()<<"+++[MusicPlayerPrivate::writeCmdToMplayer:]+++";
            perror("open fifo error\n");
            m_ReadpThread = 0;
            return;
        }
    }
    ftruncate(m_Fd_w,0);
    lseek(m_Fd_w,0,SEEK_SET);
    write(m_Fd_w,cmd,strlen(cmd));
}
//退出mplayer
void MusicPlayerPrivate::quitMpalyer(){
    Q_Q(MusicPlayer);
    qDebug()<<"+++[MusicPlayerPrivate::quitMpalyer()]+++";
    char* cmd = "quit\n";
    writeCmdToMplayer(cmd);
    m_ReadpThread = 0;
    usleep(500*1000);//等待线程退出
    emit q->onMusicPlayerExit();
}

QString MusicPlayerPrivate::chgform(QString name)	//歌名有空格的在空格前加/  （其实加上双引号“”就行）
{
    qDebug()<<"+++[MusicPlayerPrivate::chgform]+++"<< name;
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

ssize_t MusicPlayerPrivate::readLine(int fd, char *buffer, size_t n)
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



