#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668eft.h"
#include "display.h"

void *lcd_itu_i2c_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;

    if (display_init() < 0)
        goto err;

    if (lcd_rgb_pad_test() < 0)
        goto err;

    if (ark7116Init() < 0)
        goto err;

    ark_disp_init_tvenc_cvbs(CVBS_NTSC);
    ark_disp_init_itu601_out();

    itu656_init();
    sleep(1);
    itu_uninit();

    if (itu656_compare_data() < 0)
        goto err;

    itu601_init();
    sleep(1);
    itu_uninit();

    if (itu601_compare_data() < 0)
        goto err;

    display_uninit();

    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    display_uninit();
    rt->finish = 1;
    return (void*)-1;
}