#include "DeviceWatcher.h"
#include "DiskScanner/DiskScanner.h"
#include "DiskDetach/DiskDetach.h"
#include "RunnableThread.h"
#include <QDebug>
//#include "Detach.h"
#include <QDir>
#include <QRegExp>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <sys/mount.h>
#include <pthread.h>
#include <stdbool.h>
#include<unistd.h>
#include<fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <QTimer>
#ifdef ENABLE_ARKN141_CAMERA
#include "libusb.h"
#endif
#define USB_MOUNT_POINT         "/media/udisk"
#define USB_MOUNT_DATA          "fmask=0133,utf8,errors=continue"
#define USB_MOUNT_FLAG          (MS_NODEV | MS_NOSUID | MS_DIRSYNC)
#define USB_MOUNT_FORMAT        "vfat"
#define USB_MOUNT_FORMAT_EXFAT   "-t exfat"

#define SD_MOUNT_POINT          "/media/sdisk"
#define SD_MOUNT_FROMAT         "vfat"
#define SD_MOUNT_FROMAT_EXFAT   "-t exfat"
#define SD_MOUNT_FLAG           (MS_NODEV | MS_NOSUID | MS_DIRSYNC)
#define SD_MOUNT_DATA           "fmask=0133,utf8,errors=continue"
static const QString mountUSBPath("/media/udisk");
static const QString mountSDPath("/media/sdisk");

class DeviceWatcherPrivate
{
    Q_DISABLE_COPY(DeviceWatcherPrivate)
public:
    explicit DeviceWatcherPrivate(DeviceWatcher *parent);
    ~DeviceWatcherPrivate();
    void initialize();
    void connectAllSlots();
    void diskDeviceAdd(const QString &path);
    void diskDeviceRemove(const QString &path);
    void usbDiskScan(const QString &path);
    void usbDiskCancelScan(const QString &path);
    void sdDiskScan(const QString &path);
    void sdDiskCancelScan(const QString &path);
    bool regExpUSBPath(const QString& path);
    bool regExpSDPath(const QString& path);
    bool usbSpecialDeviceDetach(const QString &device, const DiskDeviceWatcher::Action action);
#ifdef ENABLE_ARKN141_CAMERA
    int usbArkn141CameraDetach(const QString &path);
#endif
    bool m_UsbSpecialDeviceDetach;
    DiskScanner* m_USBDiskScanner;
    DiskScanner* m_SDDiskScanner;
    DiskDetach* m_DiskDetach;
    QString m_USBDiskPath;
    QString m_SDDiskPath;
    DeviceWatcherStatus m_USBDiskStatus;
    DeviceWatcherStatus m_SDDiskStatus;
    bool m_InitDetach;
private:
    DeviceWatcher* m_Parent;
};

DeviceWatcher::DeviceWatcher(QObject *parent)
    : QObject(parent)
    , m_Private(new DeviceWatcherPrivate(this))
{
}

DeviceWatcher::~DeviceWatcher()
{
}

void DeviceWatcher::onDiskDeviceChangeHandler(const QString &device, const DiskDeviceWatcher::Action action)
{
    m_Private->m_DiskDetach->onDiskDeviceChange(device, action);
}

void DeviceWatcher::synchronize()
{
    deviceWatcherCheckStatus(DWT_SDDisk);
    deviceWatcherCheckStatus(DWT_USBDisk);
}

void DeviceWatcher::deviceWatcherCheckStatus(const int type)
{
    switch (type) {
    case DWT_SDDisk: {
        emit onDeviceWatcherStatus(DWT_SDDisk, m_Private->m_SDDiskStatus);
        break;
    }
    case DWT_USBDisk: {
        emit onDeviceWatcherStatus(DWT_USBDisk, m_Private->m_USBDiskStatus);
        break;
    }
    default: {
        break;
    }
    }
}
void DeviceWatcher::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    QDir dir("/dev/");
    dir.setFilter(QDir::System | QDir::NoDotAndDotDot);
    QFileInfoList fileInfoList = dir.entryInfoList();
    bool sdExsits(false);
    bool usbExists(false);
    for (QFileInfoList::iterator fileIter = fileInfoList.begin(); fileIter != fileInfoList.end(); ++fileIter) {
        if ((!sdExsits) && (m_Private->regExpSDPath(fileIter->filePath()))) {
            sdExsits = true;
            onDiskDeviceChangeHandler(fileIter->filePath(), DiskDeviceWatcher::A_Add);
        } else if ((!usbExists) && (m_Private->regExpUSBPath(fileIter->filePath()))) {
            usbExists = true;
            if (0 == QString(qgetenv("PROTOCOL_ID")).compare(QString("yaoxi"))) {
                system("echo otg > /sys/devices/platform/musb-ark1680.1/musb-hdrc.1/mode");
            } else {
                onDiskDeviceChangeHandler(fileIter->filePath(), DiskDeviceWatcher::A_Add);
            }
        } else if (sdExsits && usbExists) {
            break;
        }
    }
    //}
    QObject::timerEvent(event);
}

DeviceWatcherPrivate::DeviceWatcherPrivate(DeviceWatcher *parent)
    : m_Parent(parent)
{
    m_USBDiskScanner = NULL;
    m_SDDiskScanner = NULL;
    m_DiskDetach = NULL;
    m_USBDiskStatus = DWS_Empty;
    m_SDDiskStatus = DWS_Empty;
    //m_InitDetach = false;
    m_UsbSpecialDeviceDetach = false;
    initialize();
    connectAllSlots();
    m_Parent->startTimer(0);
}

DeviceWatcherPrivate::~DeviceWatcherPrivate()
{
//    if (m_InitDetach) {
//        uninitDetach();
//        m_InitDetach = false;
//    }
}

void DeviceWatcherPrivate::initialize()
{
    QStringList m_MusicSuffix;
    m_MusicSuffix.clear();
    m_MusicSuffix.append(QString(".MP2"));
    m_MusicSuffix.append(QString(".MP3"));
    m_MusicSuffix.append(QString(".WMA"));
    m_MusicSuffix.append(QString(".M4A"));
    m_MusicSuffix.append(QString(".M4R"));
    m_MusicSuffix.append(QString(".MMF"));
    m_MusicSuffix.append(QString(".FLAC"));
    m_MusicSuffix.append(QString(".APE"));
    m_MusicSuffix.append(QString(".OGG"));
    m_MusicSuffix.append(QString(".AC3"));
    m_MusicSuffix.append(QString(".AAC"));
    m_MusicSuffix.append(QString(".WAV"));
    m_MusicSuffix.append(QString(".AMR"));
    m_MusicSuffix.append(QString(".RA"));
    m_MusicSuffix.append(QString(".AU"));
    m_MusicSuffix.append(QString(".MMF"));
//    m_MusicSuffix.append(QString(".AIFF"));

    QStringList m_ImageSuffix;
    m_ImageSuffix.clear();
    m_ImageSuffix.append(QString(".JPG"));
    m_ImageSuffix.append(QString(".BMP"));
    m_ImageSuffix.append(QString(".JPEG"));
    m_ImageSuffix.append(QString(".PNG"));
    m_ImageSuffix.append(QString(".GIF"));
    m_ImageSuffix.append(QString(".TIF"));
    m_ImageSuffix.append(QString(".TIFF"));
    m_ImageSuffix.append(QString(".PCX"));
    m_ImageSuffix.append(QString(".ICO"));
    QStringList m_VideoSuffix;
    m_VideoSuffix.clear();
    m_VideoSuffix.append(QString(".AVI"));
    m_VideoSuffix.append(QString(".MP4"));
    m_VideoSuffix.append(QString(".MPG"));
    m_VideoSuffix.append(QString(".M4V"));
    m_VideoSuffix.append(QString(".MKV"));
    m_VideoSuffix.append(QString(".3GP"));
    m_VideoSuffix.append(QString(".ASF"));
    m_VideoSuffix.append(QString(".MOV"));
    m_VideoSuffix.append(QString(".MPEG"));
    m_VideoSuffix.append(QString(".VOB"));
    m_VideoSuffix.append(QString(".TS"));
    m_VideoSuffix.append(QString(".WMV"));
    m_VideoSuffix.append(QString(".RM"));
    m_VideoSuffix.append(QString(".RMVB"));
    m_VideoSuffix.append(QString(".DIVX"));
    m_VideoSuffix.append(QString(".FLV"));
    m_VideoSuffix.append(QString(".SWF"));
    m_VideoSuffix.append(QString(".OGM"));
    //m_VideoSuffix.append(QString(".DAT"));

    m_USBDiskScanner = new DiskScanner(m_Parent);
    m_USBDiskScanner->setMinimumScanTime(1500);
    QMap<int, QStringList> map;
    map.insert(0, m_MusicSuffix);
    map.insert(1, m_ImageSuffix);
    map.insert(2, m_VideoSuffix);
    m_USBDiskScanner->setScannerSuffixMap(map);

    m_SDDiskScanner = new DiskScanner(m_Parent);
    m_SDDiskScanner->setMinimumScanTime(1500);
    map.insert(0, m_MusicSuffix);
    map.insert(1, m_ImageSuffix);
    map.insert(2, m_VideoSuffix);
    m_SDDiskScanner->setScannerSuffixMap(map);

    m_USBDiskPath.clear();
    m_SDDiskPath.clear();

    m_DiskDetach = new DiskDetach(m_Parent);
}

void DeviceWatcherPrivate::connectAllSlots()
{
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(m_USBDiskScanner, SIGNAL(filePath(const QString &, const int)),
                     m_Parent,         SLOT(onUSBFilePath(const QString &, const int)),
                     type);
    QObject::connect(m_USBDiskScanner, SIGNAL(finish()),
                     m_Parent,         SLOT(onFinish()),
                     type);
    QObject::connect(m_SDDiskScanner, SIGNAL(filePath(const QString &, const int)),
                     m_Parent,        SLOT(onSDFilePath(const QString &, const int)),
                     type);
    QObject::connect(m_SDDiskScanner, SIGNAL(finish()),
                     m_Parent,        SLOT(onFinish()),
                     type);
    QObject::connect(m_DiskDetach, SIGNAL(diskPartition(const QString &, const DiskDeviceWatcher::Action)),
                     m_Parent,     SLOT(onDiskDeviceChange(const QString &, const DiskDeviceWatcher::Action)),
                     type);
}

void DeviceWatcherPrivate::diskDeviceAdd(const QString &path)
{
    if (regExpUSBPath(path)) {
        if (m_USBDiskPath.isEmpty()
                && QFile::exists(path)) {
            if (!QFile::exists(mountUSBPath)) {
                system("mkdir -p /media/udisk");
            }
            umount(USB_MOUNT_POINT);
            int ret = mount(path.toLocal8Bit().data(), USB_MOUNT_POINT, USB_MOUNT_FORMAT, USB_MOUNT_FLAG, USB_MOUNT_DATA);
            if (0 == ret) {
                m_USBDiskPath = path;
                usbDiskScan(mountUSBPath);
            } else {
                umount(USB_MOUNT_POINT);
                int ret = mount(path.toLocal8Bit().data(), USB_MOUNT_POINT, USB_MOUNT_FORMAT_EXFAT, USB_MOUNT_FLAG, USB_MOUNT_DATA);
                if (0 == ret) {
                    m_USBDiskPath = path;
                    usbDiskScan(mountUSBPath);
                }
                else
                {
                    qDebug() << "mount usb failed!!!!!1";
                }
            }

        }
    } else if (regExpSDPath(path)) {
        if (m_SDDiskPath.isEmpty()
                && QFile::exists(path)) {
            if (!QFile::exists(mountSDPath)) {
                system("mkdir -p /media/sdisk");
            }

            umount(SD_MOUNT_POINT);
            int ret = mount(path.toLocal8Bit().data(), SD_MOUNT_POINT, SD_MOUNT_FROMAT, SD_MOUNT_FLAG, SD_MOUNT_DATA);
            if (0 == ret) {
                m_SDDiskPath = path;
                sdDiskScan(mountSDPath);
            } else {
                umount(SD_MOUNT_POINT);
                int ret = mount(path.toLocal8Bit().data(), SD_MOUNT_POINT, SD_MOUNT_FROMAT_EXFAT, SD_MOUNT_FLAG, SD_MOUNT_DATA);
                if (0 == ret) {
                    m_USBDiskPath = path;
                    sdDiskScan(mountSDPath);
                }
                else
                {
                    qDebug() << "mount usb failed!!!!!1";
                }
            }

        }
    } else {
        qDebug() << "add other device!";
    }
}

void DeviceWatcherPrivate::diskDeviceRemove(const QString &path)
{
    if (m_USBDiskPath == path) {
        if (DWS_Unsupport == m_USBDiskStatus) {
            m_USBDiskStatus = DWS_Remove;
            m_USBDiskPath.clear();
            emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk, m_USBDiskStatus);
            m_USBDiskStatus = DWS_Empty;
            return ;
        }
        usbDiskCancelScan(mountUSBPath);
        QString command = QString(" umount -l ") + mountUSBPath;
        int ret = system(command.toLocal8Bit().constData());
        if (0 == ret) {
        } else {
            qDebug() << "umount usb failed!!!!";
        }
    } else if (m_SDDiskPath == path) {
        if (DWS_Unsupport == m_SDDiskStatus) {
            m_SDDiskStatus = DWS_Remove;
            m_SDDiskPath.clear();
            emit m_Parent->onDeviceWatcherStatus(DWT_SDDisk, m_SDDiskStatus);
            m_SDDiskStatus = DWS_Empty;
            return ;
        }
        sdDiskCancelScan(mountSDPath);
        QString command = QString(" umount -l ") + mountSDPath;
        int ret = system(command.toLocal8Bit().constData());
        if (0 == ret) {
        } else {
            qDebug() << "umount sd failed!!!!";
        }
    }  else {
        //modify by wandz at20190506
        qDebug() << "remove other device!" << path << m_SDDiskPath << m_USBDiskPath;
        if (DWS_Unsupport == m_SDDiskStatus) {
            m_SDDiskStatus = DWS_Remove;
            m_SDDiskPath.clear();
            emit m_Parent->onDeviceWatcherStatus(DWT_SDDisk, m_SDDiskStatus);
            m_SDDiskStatus = DWS_Empty;
            return ;
        }
        if (DWS_Unsupport == m_USBDiskStatus) {
            m_USBDiskStatus = DWS_Remove;
            m_USBDiskPath.clear();
            emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk, m_USBDiskStatus);
            m_USBDiskStatus = DWS_Empty;
            return ;
        }
    }
}

void DeviceWatcherPrivate::usbDiskScan(const QString &path)
{
    m_USBDiskStatus = DWS_Busy;
    m_USBDiskScanner->startScanner(path);
    emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk, m_USBDiskStatus);
}

void DeviceWatcherPrivate::usbDiskCancelScan(const QString &path)
{
    m_USBDiskStatus = DWS_Remove;
    m_USBDiskPath.clear();
    m_USBDiskScanner->stopScanner();
    emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk, m_USBDiskStatus);
    m_USBDiskStatus = DWS_Empty;
}

void DeviceWatcherPrivate::sdDiskScan(const QString &path)
{
    m_SDDiskStatus = DWS_Busy;
    m_SDDiskScanner->startScanner(path);
    emit m_Parent->onDeviceWatcherStatus(DWT_SDDisk, m_SDDiskStatus);
}

void DeviceWatcherPrivate::sdDiskCancelScan(const QString &path)
{
    m_SDDiskStatus = DWS_Remove;
    m_SDDiskPath.clear();
    m_SDDiskScanner->stopScanner();
    emit m_Parent->onDeviceWatcherStatus(DWT_SDDisk, m_SDDiskStatus);
    m_SDDiskStatus = DWS_Empty;
}

bool DeviceWatcherPrivate::regExpUSBPath(const QString &path)
{
    return path.startsWith(QString("/dev/sd"));
}


bool DeviceWatcherPrivate::regExpSDPath(const QString &path)
{
    return (path.startsWith(QString("/dev/mmcblk1")));
}

#ifdef ENABLE_ARKN141_CAMERA
int DeviceWatcherPrivate::usbArkn141CameraDetach(const QString &path)
{
    libusb_context *usb_video_context = NULL;
    libusb_device_handle *usb_video_handle = NULL;
    unsigned int vid = 0x18EC;
    unsigned int pid = 0x0141;
    int r = -1;

    if(regExpUSBPath(path)) {
        r = libusb_init(&usb_video_context);
        if (r < 0) {
            qDebug()<<"### usbArkn141CameraDetach failed, libusb_init NG";
            return r;
        }

        usb_video_handle = libusb_open_device_with_vid_pid(usb_video_context, vid, pid);
        if(usb_video_handle) {
            libusb_close(usb_video_handle);
            usb_video_handle = NULL;
        } else {
            r = -1;
            qDebug()<<"### usbArkn141CameraDetach failed, open_device_with_vid_pid NG";
        }

        if(usb_video_context) {
            libusb_exit(usb_video_context);
            usb_video_context = NULL;
        }
    }

    return r;
}
#endif

bool DeviceWatcherPrivate::usbSpecialDeviceDetach(const QString &device, const DiskDeviceWatcher::Action action)
{
    bool ret;

    switch (action) {
    case DiskDeviceWatcher::A_Add: {
        if (0) { }
#ifdef ENABLE_ARKN141_CAMERA
        else if(usbArkn141CameraDetach(device) == 0) {
            m_UsbSpecialDeviceDetach = true;
            //Handler(device, DiskDeviceWatcher::A_Remove);
            emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk+10, DWS_Ready);
        }
#endif
        else {
            m_UsbSpecialDeviceDetach = false;
        }
        break;
    }
    case DiskDeviceWatcher::A_Remove: {
        if(m_UsbSpecialDeviceDetach) {
            m_UsbSpecialDeviceDetach = false;
#ifdef ENABLE_ARKN141_CAMERA
            emit m_Parent->onDeviceWatcherStatus(DWT_USBDisk+10, DWS_Remove);
#endif
            return true;
        }
        break;
    }
    default: {
        m_UsbSpecialDeviceDetach = false;
        break;
    }
    }

    return m_UsbSpecialDeviceDetach;
}

void DeviceWatcher::onDiskDeviceChange(const QString &device, const DiskDeviceWatcher::Action action)
{
    switch (action) {
    case DiskDeviceWatcher::A_Add: {

        if(m_Private->usbSpecialDeviceDetach(device, action) == false) {
            m_Private->diskDeviceAdd(device);
        }
        break;
    }
    case DiskDeviceWatcher::A_Remove: {
        if(m_Private->usbSpecialDeviceDetach(device, action) == false) {
            m_Private->diskDeviceRemove(device);
        }
        break;
    }
    default: {
        break;
    }
    }
}

void DeviceWatcher::onUSBFilePath(const QString &path, const int index)
{
    switch(index) {
    case 0: {
        emit onMusicFilePath(path, DWT_USBDisk);
        break;
    }
    case 1: {
        emit onImageFilePath(path, DWT_USBDisk);
        break;
    }
    case 2: {
        emit onVideoFilePath(path, DWT_USBDisk);
        break;
    }
    default: {
        break;
    }
    }
}

void DeviceWatcher::onSDFilePath(const QString &path, const int index)
{
    switch(index) {
    case 0: {
        emit onMusicFilePath(path, DWT_SDDisk);
        break;
    }
    case 1: {
        emit onImageFilePath(path, DWT_SDDisk);
        break;
    }
    case 2: {
        emit onVideoFilePath(path, DWT_SDDisk);
        break;
    }
    default: {
        break;
    }
    }
}

void DeviceWatcher::onFinish()
{
    QObject* ptr = sender();
    if (ptr == m_Private->m_USBDiskScanner) {
        qDebug() << "USB::onFinish";
        m_Private->m_USBDiskStatus = DWS_Ready;
        emit onDeviceWatcherStatus(DWT_USBDisk, m_Private->m_USBDiskStatus);
    } else if (ptr == m_Private->m_SDDiskScanner) {
        qDebug() << "SD::onFinish";
        m_Private->m_SDDiskStatus = DWS_Ready;
        emit onDeviceWatcherStatus(DWT_SDDisk, m_Private->m_SDDiskStatus);
    }
}
