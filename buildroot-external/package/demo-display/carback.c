#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <linux/fb.h>
#include <string.h>
#include <pthread.h>
#include "carback.h"
#include "include/ark_api.h"

static pthread_t carback_work_id;
static int fd_carback;
int carback_detect;


static int carback_open(void)
{
        int fd, ret;

        fd = open("/dev/carback", O_RDWR);
        if (fd < 0) {
                printf("open /dev/carback failed.\n");
                return -ENOENT;
        } 

        ret = ioctl(fd, CARBACK_IOCTL_SET_APP_READY, NULL);
        if (ret != 0) {
                printf("%s: ioctl error.\n",__func__);
        }

        return fd;
}

static int carback_app_enter_done(int fd)
{
        int ret;

        ret = ioctl(fd, CARBACK_IOCTL_APP_ENTER_DONE, NULL);
        if (ret != 0) {
                printf("%s: ioctl error.\n",__func__);
        }

        return ret;
}

static int carback_app_exit_done(int fd)
{
        int ret;

        ret = ioctl(fd, CARBACK_IOCTL_APP_EXIT_DONE, NULL);
        if (ret != 0) {
                printf("%s: ioctl error.\n",__func__);
        }

        return ret;
}

static int carback_get_status(int fd)
{
        int ret;
        int carback_status = 0;

        ret = ioctl(fd, CARBACK_IOCTL_GET_STATUS, &carback_status);
        if (ret != 0) {
                printf("%s: ioctl error.\n",__func__);
        }

        return ret;
}


static int carback_detect_signal(int fd)
{
        int ret;
        int signal = 0;

        ret = ioctl(fd, CARBACK_IOCTL_APP_ENTER_DONE, &signal);
        if (ret != 0) {
                printf("%s: ioctl error.\n",__func__);
        }

        return ret;
}



static void* carback_work_thread(void * param)
{
        fd_set rdfdset;
        unsigned char carback;
        int ret;

        printf("%s-->\n", __func__);

        while (carback_detect) {
                carback = 0;
                FD_ZERO(&rdfdset);
                FD_SET(fd_carback, &rdfdset);

                ret = select(fd_carback + 1, &rdfdset, NULL, NULL, NULL);
                if (ret < 0) {
                        printf("Failed to select.\n");
                        continue;		
                }
                if (FD_ISSET(fd_carback, &rdfdset)) {
                        ret = read(fd_carback, &carback, 1);
                        printf("%s: ret=%d ,carback=%d.\n",__func__, ret, carback);

                        //处理倒车与app 交互相关事项处理，包括显示
                        if(carback){
                                arkapi_display_force_hide_layer(PRIMARY_LAYER);
                                arkapi_display_force_hide_layer(OVER_LAYER);
                                carback_app_enter_done(fd_carback);
                        }else{
                                arkapi_display_force_show_layer(PRIMARY_LAYER);
                                arkapi_display_force_show_layer(OVER_LAYER);
                                carback_app_exit_done(fd_carback);
                        }

                }
        }
	
}


void carback_work_start(void)
{
	
        fd_carback = carback_open();
        if(fd_carback < 0){
                printf("%s: carback open fail.\n", __func__);
                return;
        }
        printf("%s: carback open success.\n", __func__);

        carback_detect = 1;

        pthread_create(&carback_work_id, NULL, carback_work_thread, NULL);
    
 }

 void carback_work_exit(void)
 {
        printf("%s.\n", __func__);
        carback_detect = 0;
        sleep(1);

        if(carback_work_id)
                pthread_join(carback_work_id, NULL);
        if(fd_carback)
                close(fd_carback);
 }

