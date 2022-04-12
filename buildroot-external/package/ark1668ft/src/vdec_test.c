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
#include "mfcapi.h"
#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

void *vdec_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    MFCHandle *handle = NULL;
    struct cma_mem *inbuf = NULL;
    unsigned int *databuf = NULL;
    int fdstream = -1, fddata = -1;
    unsigned long streamsize, datasize;
    DWLLinearMem_t streamin;
    OutFrameBuffer dataout;
    int i;

    inbuf = alloc_cma_mem(0x100000);
    if (!inbuf) {
        printf("alloc cma memory fail.\n");
        goto err;
    }

    fdstream = open(VDEC_STREAM_PATH, O_RDONLY);
    streamsize = get_file_size(VDEC_STREAM_PATH);
    if (read(fdstream, inbuf->viraddr, streamsize) != streamsize) {
        printf("read stream file err.\n");
        goto err;
    }

    fddata = open(VDEC_YUVDATA_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open data file fail.\n");
        goto err;
    }
    datasize = get_file_size(VDEC_YUVDATA_PATH);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto err;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto err;
    }

    handle = mfc_init(RAW_STRM_TYPE_MP4);
    if (!handle) {
        printf("mfc_init err.\n");
        goto err;
    }

    streamin.virtualAddress = inbuf->viraddr;
    streamin.busAddress = inbuf->phyaddr;
    streamin.size = streamsize;
    dataout.num = 0;
    mfc_decode(handle, &streamin, &dataout);
    if (dataout.num) {
        unsigned int *tmp = (unsigned int*)dataout.buffer[0].pyVirAddress;
        for (i = 0; i < datasize / 4; i++) {
            if (databuf[i] != tmp[i]) {
                printf("compare data fail.0x%x, 0x%x.\n", databuf[i], tmp[i]);
                goto err;
            }
        }
    } else {
        printf("mfc_decode err.\n");
        goto err;
    }

    mfc_uninit(handle);
    close(fdstream);
    close(fddata);
    free(databuf);
    free_cma_mem(inbuf);
    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (handle)
        mfc_uninit(handle);
    if (fdstream > 0)
        close(fdstream);
    if (fddata > 0)
        close(fddata);
    if (databuf)
        free(databuf);
    if (inbuf)
        free_cma_mem(inbuf);
    rt->finish = 1;
    return (void*)-1;
}