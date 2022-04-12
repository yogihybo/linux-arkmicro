#ifndef ADBCOMMAND_H
#define ADBCOMMAND_H

#ifdef __cplusplus
extern "C" {
#endif
#include "command.h"

#ifdef __cplusplus
}
#endif

class AdbCommand
{
public:
    AdbCommand();

public:
    static process_t adb_execute(const char *serial, const char *const adb_cmd[], size_t len);

    static process_t adb_forward(const char *serial, uint16_t local_port, const char *device_socket_name);

    static process_t adb_forward_remove(const char *serial, uint16_t local_port);

    static process_t adb_reverse(const char *serial, const char *device_socket_name, uint16_t local_port);

    static process_t adb_reverse_remove(const char *serial, const char *device_socket_name);

    static process_t adb_push(const char *serial, const char *local, const char *remote);

    static process_t adb_install(const char *serial, const char *local);

    static bool process_check_success(process_t proc, const char *name);

    static bool is_regular_file(const char *path);

    static int isDeviceConnected(char *pDevice);

    static int isWirelessConnected(char *pDevice);

    static bool adbConnect(const char *ip);

    static bool adbDisConnect(const char *ip);

    static bool adbReset();

    static bool pushFile(const char *serail, const char *src, const char *dest, int timeout);

    static bool adbForward(const char *serial, const char *device_socket_name, uint16_t local_port);

    static bool adbReverse(const char *serial, const char *device_socket_name, uint16_t local_port);

    static bool adbForwardRemove(const char *serial, uint16_t local_port);

    static bool adbReverseRemove(const char *serial, const char *device_socket_name);

    static bool adbExecute(const char *serial, const char  *adb_cmd);

    static long getFileSize(const char *filePath);
};

#endif // ADBCOMMAND_H
