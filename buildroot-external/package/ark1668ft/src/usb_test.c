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

static int is_usb_exist(void)
{
    DIR *dir;
    struct dirent *ptr;
	int usb0_exist = 0, usb1_exist = 0;

    if ((dir = opendir("/dev")) == NULL) {
        printf("Open dev dir error...");
        return 0;
    }

    while ((ptr= readdir(dir)) != NULL) {
        if (usb0_exist && usb1_exist)
            break;

        /* current dir OR parrent dir */
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
            continue;
        else if(ptr->d_type == 6)    //block device file
        {
        	if (strncmp(ptr->d_name, "sda", 3) == 0) {
				usb0_exist = 1;
			} else if (strncmp(ptr->d_name, "sdb", 3) == 0) {
                usb1_exist = 1;
            }
	    } else continue;
    }

    closedir(dir);
    return usb0_exist && usb1_exist;
}

void *usb_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
	int timeout = TEST_TIMEOUT / 2 / 100;
    char command[128];
    char filename0[32] = "/media/usb0/";
    char filename1[32] = "/media/usb1/";
    int fd0 = -1, fd1 = -1;
    char *buf0 = NULL, *buf1 = NULL;
    int ret;

    printf("usb test start.\n");

    do {
        if (is_usb_exist())
            break;
        usleep(100000);
    } while(timeout--);

	if (!is_dir_exist("/media/usb0")) {
		ret = system("mkdir -p /media/usb0");
		if (ret != 0) {
			printf("mkdir /media/usb0 fail.\n");
			goto err;
		}
	}

    if (!is_dir_exist("/media/usb1")) {
		ret = system("mkdir -p /media/usb1");
		if (ret != 0) {
			printf("mkdir /media/usb1 fail.\n");
			goto err;
		}
	}

    if (mount_device("/media/usb0", "/dev/sda1") != 0) {
        if (mount_device("/media/usb0", "/dev/sda") != 0) {
            printf("can't mount usb0.\n");
            goto err;
        }
    }

    if (mount_device("/media/usb1", "/dev/sdb1") != 0) {
        if (mount_device("/media/usb1", "/dev/sdb") != 0) {
            printf("can't mount usb1.\n");
            goto err;
        }
    }

    strncat(filename0, USB_TESTFILE_NAME, 16);
    fd0 = open(filename0, O_RDONLY);
    if (fd0 < 0) {
        printf("open usb0 test file fail.\n");
        goto err;
    }

    strncat(filename1, USB_TESTFILE_NAME, 16);
    fd1 = open(filename1, O_RDONLY);
    if (fd1 < 0) {
        printf("open usb1 test file fail.\n");
        goto err;
    }

    buf0 = malloc(USB_TESTFILE_SIZE);
    buf1 = malloc(USB_TESTFILE_SIZE);
    if (!buf0 || !buf1) {
        printf("usb test malloc fail.\n");
        goto err;
    }

    if (read(fd0, buf0, USB_TESTFILE_SIZE) != USB_TESTFILE_SIZE) {
        printf("read usb0 data err.\n");
        goto err;
    }

    if (read(fd1, buf1, USB_TESTFILE_SIZE) != USB_TESTFILE_SIZE) {
        printf("read usb1 data err.\n");
        goto err;
    }

    if (memcmp(buf0, buf1, USB_TESTFILE_SIZE) != 0) {
        printf("compare usb data fail.\n");
        goto err;
    }

    free(buf0);
    free(buf1);
    close(fd0);
    close(fd1);
    rt->finish = 1;
    rt->pass = 1;

    printf("usb test ok.\n");
	return (void*)0;

err:
    if (buf0) free(buf0);
    if (buf1) free(buf1);
    if (fd0 > 0) close(fd0);
    if (fd1 > 0) close(fd1);
    rt->finish = 1;
    printf("usb test fail.\n");
    return (void*)-1;
}