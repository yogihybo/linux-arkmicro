#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

void *spi_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
	int timeout = TEST_TIMEOUT / 2 / 100;
    char command[128];
    char filename0[32] = "/media/spinor/";
    char filename1[32] = "/media/spinor/";
    int fddata = -1, fdspi = -1;
    char *buf0 = NULL, *buf1 = NULL;
    int ret;

    printf("spi test start.\n");

    fddata = open(SPINOR_TESTFILE_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open spidata test file fail.\n");
        goto err;
    }

    fdspi = open("/dev/mtd9", O_RDONLY | O_SYNC);
    if (fddata < 0) {
        printf("open spi device file fail.\n");
        goto err;
    }

    buf0 = malloc(SPINOR_TESTFILE_SIZE);
    buf1 = malloc(SPINOR_TESTFILE_SIZE);
    if (!buf0 || !buf1) {
        printf("spi test malloc fail.\n");
        goto err;
    }

    if (read(fddata, buf0, SPINOR_TESTFILE_SIZE) != SPINOR_TESTFILE_SIZE) {
        printf("read spi data file err.\n");
        goto err;
    }

    if (read(fdspi, buf1, SPINOR_TESTFILE_SIZE) != SPINOR_TESTFILE_SIZE) {
        printf("read spiflash data err.\n");
        goto err;
    }

    if (memcmp(buf0, buf1, SPINOR_TESTFILE_SIZE) != 0) {
        printf("compare spinor data fail.\n");
        goto err;
    }

    free(buf0);
    free(buf1);
    close(fddata);
    close(fdspi);
    rt->finish = 1;
    rt->pass = 1;

    printf("spi test ok.\n");
	return (void*)0;

err:
    if (buf0) free(buf0);
    if (buf1) free(buf1);
    if (fddata > 0) close(fddata);

    rt->finish = 1;
    printf("spi test fail.\n");
    return (void*)-1;
}