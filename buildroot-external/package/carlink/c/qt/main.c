#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include<stdbool.h>
#include "../../cmd/CarLinkWrapper.h"
bool gRunning = false;
void *gHandle = NULL;

static void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            gRunning = false;
            break;
    }
}

static void init_signals(void) {
    struct sigaction sigact;

    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
}

void usbstate(int connect_status, int phone_type)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(connect_status == 0 && phone_type == 2)
	    Start(gHandle, 0, 0);
    printf("connect_status = %d, phone_type = %d\r\n", connect_status, phone_type);
}

void appstatus(int appStatus, void *reserved)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("appStatus = %d\r\n", appStatus);
}

int main(void)
{
    init_signals();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);


    gHandle = GetInstance();
    RegisterUsbState(gHandle, usbstate);
    RegisterAppStatus(gHandle, appstatus);
    InitCarLinkPlayer(gHandle);
    

    gRunning = true;
    while (gRunning) {
        sleep(1);
    }

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);


    printf("%s:%s:%d process exited\r\n",__FILE__,__func__,__LINE__);

    return 0;
}
