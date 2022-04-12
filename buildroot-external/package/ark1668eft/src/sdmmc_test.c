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
#include "ark1668eft.h"

static int is_sdmmc_exist(void)
{
    DIR *dir;
    struct dirent *ptr;
	int sdmmc0_exist = 0, sdmmc1_exist = 0;

    if ((dir = opendir("/dev")) == NULL) {
        printf("Open dev dir error...");
        return 0;
    }

    while ((ptr= readdir(dir)) != NULL) {
        if (sdmmc0_exist && sdmmc1_exist)
            break;

        /* current dir OR parrent dir */
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
            continue;
        else if(ptr->d_type == 6)    //block device file
        {
        	if (strncmp(ptr->d_name, "mmcblk0", 7) == 0) {
				sdmmc0_exist = 1;
			} else if (strncmp(ptr->d_name, "mmcblk1", 7) == 0) {
                sdmmc1_exist = 1;
            }
	    } else continue;
    }

    closedir(dir);
    return sdmmc0_exist && sdmmc1_exist;
}

void *sdmmc_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
	int timeout = TEST_TIMEOUT / 2 / 100;
    char command[128];
    char filename0[32] = "/media/sdmmc0/";
    char filename1[32] = "/media/sdmmc1/";
    int fd0 = -1, fd1 = -1;
    char *buf0 = NULL, *buf1 = NULL;
    int ret;

    printf("sdmmc test start.\n");

    do {
        if (is_sdmmc_exist())
            break;
        usleep(100000);
    } while(timeout--);

	if (!is_dir_exist("/media/sdmmc0")) {
		ret = system("mkdir -p /media/sdmmc0");
		if (ret != 0) {
			printf("mkdir /media/sdmmc0 fail.\n");
			goto err;
		}
	}

    if (!is_dir_exist("/media/sdmmc1")) {
		ret = system("mkdir -p /media/sdmmc1");
		if (ret != 0) {
			printf("mkdir /media/sdmmc1 fail.\n");
			goto err;
		}
	}

    if (mount_device("/media/sdmmc0", "/dev/mmcblk0p1") != 0) {
        if (mount_device("/media/sdmmc0", "/dev/mmcblk0") != 0) {
            printf("can't mount sdmmc0.\n");
            goto err;
        }
    }

    if (mount_device("/media/sdmmc1", "/dev/mmcblk1p1") != 0) {
        if (mount_device("/media/sdmmc1", "/dev/mmcblk1") != 0) {
            printf("can't mount sdmmc1.\n");
            goto err;
        }
    }

    strncat(filename0, SDMMC_TESTFILE_NAME, 16);
    fd0 = open(filename0, O_RDONLY);
    if (fd0 < 0) {
        printf("open sdmmc0 test file fail.\n");
        goto err;
    }

    strncat(filename1, SDMMC_TESTFILE_NAME, 16);
    fd1 = open(filename1, O_RDONLY);
    if (fd1 < 0) {
        printf("open sdmmc1 test file fail.\n");
        goto err;
    }

    buf0 = malloc(SDMMC_TESTFILE_SIZE);
    buf1 = malloc(SDMMC_TESTFILE_SIZE);
    if (!buf0 || !buf1) {
        printf("sdmmc test malloc fail.\n");
        goto err;
    }

    if (read(fd0, buf0, SDMMC_TESTFILE_SIZE) != SDMMC_TESTFILE_SIZE) {
        printf("read sdmmc0 data err.\n");
        goto err;
    }

    if (read(fd1, buf1, SDMMC_TESTFILE_SIZE) != SDMMC_TESTFILE_SIZE) {
        printf("read sdmmc1 data err.\n");
        goto err;
    }

    if (memcmp(buf0, buf1, SDMMC_TESTFILE_SIZE) != 0) {
        printf("compare sdmmc data fail.\n");
        goto err;
    }

    free(buf0);
    free(buf1);
    close(fd0);
    close(fd1);
    rt->finish = 1;
    rt->pass = 1;

    printf("sdmmc test ok.\n");

	return (void*)0;

err:
    if (buf0) free(buf0);
    if (buf1) free(buf1);
    if (fd0 > 0) close(fd0);
    if (fd1 > 0) close(fd1);
    rt->finish = 1;
    printf("sdmmc test fail.\n");
    return (void*)-1;
}