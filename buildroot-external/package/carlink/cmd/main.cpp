#include "CarLinkPlayer.h"
#include <signal.h>
#include <unistd.h>


bool gRunning = false;
CarLinkPlayer* gLink;

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

int main(void)
{
    init_signals();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    gLink = new CarLinkPlayer();
    gLink->initialize();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    gRunning = true;
    while (gRunning) {
        sleep(1);
    }

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    delete gLink;

    printf("%s:%s:%d process exited\r\n",__FILE__,__func__,__LINE__);

    return 0;
}

