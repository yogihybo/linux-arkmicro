#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "memalloc.h"
#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668eft.h"

void *gpu_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;

    rt->finish = 1;
    rt->pass = 0;
    return (void*)0;

err:
    rt->finish = 1;
    return (void*)-1;
}