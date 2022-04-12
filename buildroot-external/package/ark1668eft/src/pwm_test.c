#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668eft.h"

#define PWM_MAX_NUM		4

#define PWMTEST_IOCTL_BASE							0x9F
#define PWMTEST_IOCTL_GET_PWM0_COUNT				_IOR(PWMTEST_IOCTL_BASE, 0, int)
#define PWMTEST_IOCTL_GET_PWM1_COUNT				_IOR(PWMTEST_IOCTL_BASE, 1, int)
#define PWMTEST_IOCTL_GET_PWM2_COUNT				_IOR(PWMTEST_IOCTL_BASE, 2, int)
#define PWMTEST_IOCTL_GET_PWM3_COUNT				_IOR(PWMTEST_IOCTL_BASE, 3, int)

void *pwm_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    int count1 = 0, count2 = 0, count3 = 0;
    int fdpwm = -1;

    /* pwm1 10Hz */
    system("echo 1 > /sys/class/pwm/pwmchip0/export");
    system("echo 100000000 > /sys/class/pwm/pwmchip0/pwm1/period");
    system("echo 50000000 > /sys/class/pwm/pwmchip0/pwm1/duty_cycle");
    system("echo 1 > /sys/class/pwm/pwmchip0/pwm1/enable");

    /* pwm2 20Hz */
    system("echo 2 > /sys/class/pwm/pwmchip0/export");
    system("echo 50000000 > /sys/class/pwm/pwmchip0/pwm2/period");
    system("echo 25000000 > /sys/class/pwm/pwmchip0/pwm2/duty_cycle");
    system("echo 1 > /sys/class/pwm/pwmchip0/pwm2/enable");

    /* pwm3 50Hz */
    system("echo 3 > /sys/class/pwm/pwmchip0/export");
    system("echo 20000000 > /sys/class/pwm/pwmchip0/pwm3/period");
    system("echo 10000000 > /sys/class/pwm/pwmchip0/pwm3/duty_cycle");
    system("echo 1 > /sys/class/pwm/pwmchip0/pwm3/enable");

    sleep(2);

    fdpwm = open("/dev/pwmtest", O_RDWR | O_SYNC);
    if (fdpwm < 0) {
        printf("open pwmtest device fail.\n");
        goto err;
    }

    ioctl(fdpwm, PWMTEST_IOCTL_GET_PWM1_COUNT, &count1);
    ioctl(fdpwm, PWMTEST_IOCTL_GET_PWM2_COUNT, &count2);
    ioctl(fdpwm, PWMTEST_IOCTL_GET_PWM3_COUNT, &count3);

    if (count1 < 40 || count1 > 60)
        goto err;

    if (count2 < 80 || count2 > 120)
        goto err;

    if (count3 < 180 || count3 > 220)
        goto err;

    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:

    rt->finish = 1;
    return (void*)-1;
}