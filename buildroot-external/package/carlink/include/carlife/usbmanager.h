#ifndef USBMANAGER_H
#define USBMANAGER_H

#include <stdint.h>
#define UEVENT_BUFFER_SIZE 2048

#define PATH_MAX        4096
#define KEYVALLEN 256

typedef struct __usb_info
{
    uint16_t idVendor;
    uint16_t idProduct;
    uint8_t  path[1024];
    uint8_t  inserted;
}usb_info;

class UsbManager
{   
public:
    explicit UsbManager();

public:
    //initialize thread
    bool usb_reader_thread();
    void SetInsertPath(char *pBuffer);
    void setCallback(void (*callback)(int,int,int, void*), void *parameter);

    void change_otg();
    void change_device();
private:
    void usb_monitor();
    void parse_usb_info(char *buf);

    void inserted_info(int type, int vid);
    static void* connect_thread_func(void *me);

private:
    void*  m_parameter;
    bool   m_binserted; //检测到设备
    void (*m_callback)(int,int, int, void *);
    int    m_carplay_usb_idx;
    usb_info m_usb_info;

    char   m_szUsbPath[PATH_MAX];
    char   m_szRebootInsert[PATH_MAX];
};

#endif // USBMANAGER_H
