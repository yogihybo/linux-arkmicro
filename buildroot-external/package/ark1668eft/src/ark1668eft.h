#ifndef __ARK1668EFT_H__
#define __ARK1668EFT_H__

#include <pthread.h>

typedef void* (*TestThreadFun)(void *arg);

struct ft_runtime {
	pthread_t tid;
	TestThreadFun test_thread;
    int finish;
	int pass;
};

void *usb_test_thread(void *arg);
void *sdmmc_test_thread(void *arg);
void *vdec_test_thread(void *arg);
void *gpu_test_thread(void *arg);
void *jpg_ps_test_thread(void *arg);
void *rtc_timer_wdt_test_thread(void *arg);
void *spi_test_thread(void *arg);
void *serial_test_thread(void *arg);
void *sound_test_thread(void *arg);
void *lcd_itu_i2c_test_thread(void *arg);
void *deinterlace_test_thread(void *arg);
void *pwm_test_thread(void *arg);
void *adc_test_thread(void *arg);

#endif