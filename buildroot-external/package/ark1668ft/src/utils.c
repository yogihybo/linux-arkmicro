#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <alloca.h>
#include <linux/i2c-dev.h>

#include "ftypes.h"
#include "utils.h"
#include "memalloc.h"
#include "dwl.h"

#define ARK_I2C_RETRY_COUNT		3

struct i2c_msg {
    __u16 addr; /* slave address            */
    unsigned short flags;
#define I2C_M_TEN   0x10    /* we have a ten bit chip address   */
#define I2C_M_RD    0x01
#define I2C_M_NOSTART   0x4000
#define I2C_M_REV_DIR_ADDR  0x2000
#define I2C_M_IGNORE_NAK    0x1000
#define I2C_M_NO_RD_ACK     0x0800
    short len;      /* msg length               */
    char *buf;      /* pointer to msg data          */
};

struct cma_mem *alloc_cma_mem(int size)
{
    struct cma_mem *cma = NULL;
	u32 pgsize = getpagesize();
	MemallocParams params;

    cma = malloc(sizeof(*cma));
    if (!cma) {
        printf("No enough memory.\n");
        return NULL;
    }
    memset(cma, 0, sizeof(*cma));

	cma->fdmem = open(MEMALLOC_MODULE_PATH, O_RDWR | O_SYNC);
	if (cma->fdmem == -1) {
		printf("Failed to open: %s\n", MEMALLOC_MODULE_PATH);
        goto err;
	}

	params.size = (size + pgsize - 1) & ~(pgsize - 1);
	/* get memory linear memory buffers */
	ioctl(cma->fdmem, MEMALLOC_IOCXGETBUFFER, &params);
	if (params.busAddress == 0) {
		printf("Memalloc: get buffer failure\n");
		goto err;
	}

    cma->viraddr = map_phy_memory(params.busAddress, params.size, 1);
    if (!cma->viraddr) {
        printf("Fialed to map phyaddr\n");
        goto err;
    }
	cma->phyaddr = params.busAddress;
    cma->size = params.size;

	return cma;

err:
    free_cma_mem(cma);

    return NULL;
}

void free_cma_mem(struct cma_mem *cma)
{
    if (!cma) return;

    if (cma->phyaddr)
	    ioctl(cma->fdmem, MEMALLOC_IOCSFREEBUFFER, cma->phyaddr);
    if (cma->viraddr)
        unmap_phy_memory(cma->viraddr, cma->size);
	close(cma->fdmem);
    free(cma);
}

void *map_phy_memory(unsigned int phyaddr, int size, int nocache)
{
    int fd_mem;
    int oflags = O_RDWR;
    void *ptr;

    if (nocache) oflags |= O_SYNC;
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd_mem == -1) {
		printf("Failed to open: %s\n", "/dev/mem");
		return NULL;
	}

	ptr = (u32 *) mmap(0, size, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd_mem, phyaddr);
	if (ptr == MAP_FAILED) {
		close(fd_mem);
		return NULL;
	}

    close(fd_mem);
    return ptr;
}

void unmap_phy_memory(void * viraddr, int size)
{
    if (viraddr)
        munmap(viraddr, size);
}

int is_dir_exist(char *path)
{
	DIR *dir;

	if (path == NULL)
		return 0;

	if ((dir = opendir(path)) == NULL)
		return 0;

	closedir(dir);
	return 1;
}

int is_file_exist(char *path)
{
    if (access(path, F_OK) != -1)
		return 1;

	return 0;
}


int mount_device(char *mount_path, char *dev_path)
{
    char command[128];

    strcpy(command, "mount ");
	strncat(command, dev_path, 32);
    strcat(command, " ");
	strncat(command, mount_path, 64);
	return system(command);
}

void umount_device(char *mount_path)
{
    char command[64];

    strcpy(command, "umount ");
	strncat(command, mount_path, 32);
	system(command);
}

unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}

int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		 printf ("\nFailed export GPIO-%d\n", gpio);
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	//printf ("\nSucessfully export GPIO-%d\n", gpio);
	return 0;
}

int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		 printf ("\nFailed unexport GPIO-%d\n", gpio);
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	//printf ("\nSucessfully unexport GPIO-%d\n", gpio);
	return 0;
}

int gpio_set_dir(unsigned int gpio, const char *dir)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		 printf ("\nFailed set GPIO-%d direction\n", gpio);
		return fd;
	}

	write(fd, dir, strlen(dir)+1);
	close(fd);
	//printf ("\nSucessfully set GPIO-%d direction\n", gpio);
	return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		printf ("\nFailed set GPIO-%d value\n", gpio);
		return fd;
	}

	if (value!=0)
		{
		int i = write(fd, "1", 2);
	   // printf ("\nGPIO-%d value set high\n", gpio);
		}
	else
		{
		write(fd, "0", 2);
		//printf ("\nGPIO-%d value set low\n", gpio);
		}

	close(fd);
	//printf ("\nSucessfully set GPIO-%d value\n", gpio);
	return 0;
}

int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch;

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		printf ("\nFailed get GPIO-%d value\n", gpio);
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	//printf ("\nSucessfully get GPIO-%d value\n", gpio);
	return 0;
}

int gpio_set_output_value(unsigned int gpio, unsigned int value)
{
	gpio_export(gpio);
	gpio_set_dir(gpio, "out");
	gpio_set_value(gpio, value);

	return 0;
}

ssize_t ark_i2c_read(int fd, uint8_t addr, uint8_t reg, void *buf, size_t count)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg msg[2];
	int ret;
	int retry_count = 0;

	data.nmsgs = 2;
	data.msgs = (struct i2c_msg *)msg;

	data.msgs[0].addr = addr >> 1;
	data.msgs[0].flags = 0;
	data.msgs[0].buf = &reg;
	data.msgs[0].len = 1;

	data.msgs[1].addr = addr >> 1;
	data.msgs[1].flags = 1;
	data.msgs[1].buf = buf;
	data.msgs[1].len = count;
retry:
	ret = ioctl(fd, I2C_RDWR, (unsigned long) &data);
	if(ret < 0) {
		printf("Error during addr(0x%.2x) I2C_RDWR ioctl with error(%d): %s\n",
			addr, errno, strerror(errno));
		if (retry_count++ < ARK_I2C_RETRY_COUNT)
			goto retry;
		return -1;
	}

	return count;
}

ssize_t ark_i2c_write(int fd, uint8_t addr, uint8_t reg, void *buf, size_t count)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg msg;
	int ret;
	int retry_count = 0;

	uint8_t *tmp_buf = alloca(1 + count);//automatic free
    if (!tmp_buf)
        return -1;
    tmp_buf[0] = reg;
    memcpy(tmp_buf+1, buf, count);

	data.nmsgs = 1;
	data.msgs = (struct i2c_msg *)&msg;

	data.msgs[0].addr = addr >> 1;
	data.msgs[0].flags = 0;
	data.msgs[0].buf = tmp_buf;
	data.msgs[0].len = count + 1;
retry:
	ret = ioctl(fd, I2C_RDWR, (unsigned long) &data);
	if(ret < 0) {
		printf("Error during addr(0x%.2x) I2C_RDWR ioctl with error(%d): %s\n",
			addr, errno, strerror(errno));
		if (retry_count++ < ARK_I2C_RETRY_COUNT)
			goto retry;
		return -1;
	}

	return count;
}