#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>

#include "ftcfg.h"
#include "utils.h"
#include "ark1668ft.h"

struct ft_runtime g_runtime[TEST_ITEMS_NUM];

#define TEST_THREAD_NAME(x) x##_test_thread

TestThreadFun test_thread[TEST_ITEMS_NUM] = {
	TEST_THREAD_NAME(usb),
	TEST_THREAD_NAME(sdmmc),
	TEST_THREAD_NAME(spi),
	TEST_THREAD_NAME(vdec),
	TEST_THREAD_NAME(gpu),
	TEST_THREAD_NAME(jpg_ps),
	TEST_THREAD_NAME(lcd_itu_i2c),
	TEST_THREAD_NAME(deinterlace),
	TEST_THREAD_NAME(rtc_timer_wdt),
	TEST_THREAD_NAME(serial),
	TEST_THREAD_NAME(sound),
	TEST_THREAD_NAME(pwm),
	TEST_THREAD_NAME(adc),
	/* run_gpio_test(); */
};

static void initialize(void) {
	int i;

	for (i = 0; i < TEST_ITEMS_NUM; i++) {
		g_runtime[i].test_thread = test_thread[i];
		g_runtime[i].finish = 0;
		g_runtime[i].pass = 0;
	}
}

int main(int argc, char *argv[])
{
	int timeout = TEST_TIMEOUT / 100;
	int i;

	printf("ark1668 final test start.\n");

	initialize();

	for (i = 0; i < TEST_ITEMS_NUM; i++) {
		pthread_create(&g_runtime[i].tid, NULL,
			g_runtime[i].test_thread, &g_runtime[i]);
		pthread_detach(g_runtime[i].tid);
	}

	do {
		for (i = 0; i < TEST_ITEMS_NUM; i++) {
			if (!g_runtime[i].finish)
				break;
		}

		if (i == TEST_ITEMS_NUM)
			break;

		usleep(100000);
	} while(timeout--);

	for (i = 0; i < TEST_ITEMS_NUM; i++) {
		if (g_runtime[i].pass)
			printf("item %d test ok.\n", i);
		else
			printf("item %d test fail.\n", i);
	}

	gpio_set_output_value(RET0_GPIO,
		!(g_runtime[0].pass & g_runtime[1].pass & g_runtime[2].pass));
	gpio_set_output_value(RET1_GPIO,
		!(g_runtime[3].pass & g_runtime[4].pass & g_runtime[5].pass));
	gpio_set_output_value(RET2_GPIO,
		!(g_runtime[6].pass & g_runtime[7].pass));
	gpio_set_output_value(RET3_GPIO, !g_runtime[8].pass);
	gpio_set_output_value(RET4_GPIO, !g_runtime[9].pass);
	gpio_set_output_value(RET5_GPIO, !g_runtime[10].pass);
	gpio_set_output_value(RET6_GPIO, !g_runtime[11].pass);
	gpio_set_output_value(RET7_GPIO, !g_runtime[12].pass);

	usleep(100);
	gpio_set_output_value(TEST_DONE_GPIO, 0);

	printf("ark1668 final test finished.\n");
}
