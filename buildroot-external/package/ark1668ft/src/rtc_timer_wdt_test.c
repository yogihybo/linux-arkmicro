#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <linux/rtc.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

int g_exit = 0;
int fd_rtc = -1;
int g_result = 0;
unsigned int *wdtbase = NULL;
unsigned int wdtval1, wdtval2;
struct rtc_time rt1, rt2;

void timer_handler(int sig)
{
    int rtdiff, wdtdiff;

    if (ioctl(fd_rtc, RTC_RD_TIME, &rt2) < 0) {
        printf("read rtc time err.\n");
        g_exit = 1;
        return;
    }
    if (rt2.tm_sec >= rt1.tm_sec)
        rtdiff = rt2.tm_sec - rt1.tm_sec;
    else
        rtdiff = rt2.tm_sec + 60 - rt1.tm_sec;

    wdtval2 = *(wdtbase + 3);
    if (wdtval1 >= wdtval2)
        wdtdiff = wdtval1 - wdtval2;
    else
        wdtdiff = wdtval1 + *(wdtbase + 2) - wdtval2;

    printf("rtdiff = %d, wdtdiff=%d,\n", rtdiff, wdtdiff);

    if (rtdiff > 0 && rtdiff < 4 && wdtdiff > 8500 && wdtdiff < 9000)
        g_result = 1;

    g_exit = 1;
}

void *rtc_timer_wdt_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    struct rtc_time dt;
    signal(SIGALRM, timer_handler);
    alarm(2);
    fd_rtc = open("/dev/rtc0", O_RDWR | O_SYNC);
    if (fd_rtc < 0) {
        printf("open rtc device fail.\n");
        goto err;
    }
    if (ioctl(fd_rtc, RTC_RD_TIME, &rt1) < 0) {
        printf("read rtc tim err.\n");
        goto err;
    }

    wdtbase = map_phy_memory(0xe4b00000, 0x100, 1);
    if (wdtbase == NULL) {
        printf("wdt reg map fail.\n");
        goto err;
    }
    wdtval1 = *(wdtbase + 3);

    while(!g_exit) {
        usleep(100000);
    }

    if (!g_result)
        goto err;

    unmap_phy_memory(wdtbase, 0x100);
    close(fd_rtc);
    rt->finish = 1;
    rt->pass = 1;
    return (void*)-1;
err:
    if (wdtbase)
        unmap_phy_memory(wdtbase, 0x100);
    if (fd_rtc > 0)
        close(fd_rtc);
    rt->finish = 1;
    return (void*)-1;
}