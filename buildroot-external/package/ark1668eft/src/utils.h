#ifndef __UTILS_H__
#define __UTILS_H__

#include <inttypes.h>

struct cma_mem {
    unsigned int phyaddr;
    void *viraddr;
    int size;
    int fdmem;
};

void *map_phy_memory(unsigned int phyaddr, int size, int nocache);
void unmap_phy_memory(void * viraddr, int size);
struct cma_mem *alloc_cma_mem(int size);
void free_cma_mem(struct cma_mem *cma);

int is_dir_exist(char *path);
int is_file_exist(char *path);
int mount_device(char *mount_path, char *dev_path);
void umount_device(char *mount_path);
unsigned long get_file_size(const char *path);

/*** constants ***/

/*** gpio functions ***/
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64
int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, const char *dir);
int gpio_set_value(unsigned int gpio, unsigned int value);
int gpio_get_value(unsigned int gpio, unsigned int *value);
int gpio_set_output_value(unsigned int gpio, unsigned int value);

ssize_t ark_i2c_read(int fd, uint8_t addr, uint8_t reg, void *buf, size_t count);
ssize_t ark_i2c_write(int fd, uint8_t addr, uint8_t reg, void *buf, size_t count);

#endif