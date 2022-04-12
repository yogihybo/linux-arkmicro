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

static u32* adcbase = NULL;
static u32* sysbase = NULL;

#define rADC_CTL        *(volatile u32*)(adcbase+0x00/4)
#define rADC_AUX0       *(volatile u32*)(adcbase+0x14/4)
#define rADC_AUX1       *(volatile u32*)(adcbase+0x18/4)
#define rADC_AUX2       *(volatile u32*)(adcbase+0x1c/4)
#define rADC_AUX3       *(volatile u32*)(adcbase+0x20/4)
#define rADC_PANXZ1     *(volatile u32*)(adcbase+0x24/4)
#define rADC_PANYZ2     *(volatile u32*)(adcbase+0x28/4)

#define rSYS_ANALOG_REG0    *(volatile u32*)(sysbase+0x140/4)

void *adc_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    u32 aux0val, aux1val, aux2val, aux3val;
    u32 panxval, panyval;
    int retry = 1;

    adcbase = map_phy_memory(0xe4500000, 0x1000, 1);
    if (!adcbase)
        goto err;

    sysbase = map_phy_memory(0xe4900000, 0x1000, 1);
    if (!sysbase) {
        unmap_phy_memory(adcbase, 0x1000);
        goto err;
    }

    /* select vref 3.3v */
    rSYS_ANALOG_REG0 &= ~(1 << 22);

    rADC_CTL = (1 << 13) | (1 << 3);
    usleep(100000);
    aux0val = rADC_AUX0;

    rADC_CTL = (1 << 14) | (1 << 4);
    usleep(100000);
    aux1val = rADC_AUX1;

    rADC_CTL = (1 << 15) | (1 << 5);
    usleep(100000);
    aux2val = rADC_AUX2;

    rADC_CTL = (1 << 16) | (1 << 6);
    usleep(100000);
    aux3val = rADC_AUX3;

    printf("aux %d, %d, %d, %d.\n", aux0val, aux1val, aux2val, aux3val);
    if (!((aux0val > aux1val) && (aux1val > aux2val) && (aux2val > aux3val)))
        goto err;

    gpio_export(PANX_GPIO);
    gpio_set_dir(PANX_GPIO, "out");
    gpio_export(PANY_GPIO);
    gpio_set_dir(PANY_GPIO, "out");

    rADC_CTL = (1 << 12) | (1 << 2);

test:
    gpio_set_value(PANX_GPIO, 0);
    gpio_set_value(PANY_GPIO, 1);
    usleep(100000);
    if (retry)
        panyval = rADC_PANYZ2;
    else
        panxval = rADC_PANXZ1;

    gpio_set_value(PANX_GPIO, 1);
    gpio_set_value(PANY_GPIO, 0);
    usleep(100000);
    if (retry)
        panxval = rADC_PANXZ1;
    else
        panyval = rADC_PANYZ2;

    printf("pan %d, %d.\n", panxval, panyval);

    if (panxval > 0xd20 || panxval < 0xba0 || panyval > 0x990 || panyval < 0x810) {
        if (retry) {
            printf("switch gpio to test again.\n");
            retry = 0;
            goto test;
        } else {
            goto err;
        }
    }

    unmap_phy_memory(adcbase, 0x1000);

    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (!adcbase)
        unmap_phy_memory(adcbase, 0x1000);

    if (!sysbase)
        unmap_phy_memory(sysbase, 0x1000);

    rt->finish = 1;
    return (void*)-1;
}