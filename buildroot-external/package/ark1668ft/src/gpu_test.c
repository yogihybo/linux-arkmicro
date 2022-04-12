#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "gc_hal.h"
#include "gc_hal_raster.h"
#include "memalloc.h"
#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

static void data_process(unsigned int src_addr, unsigned int dst_addr)
{
	gceSTATUS status;
    gcoOS g_os;
    gcoHAL g_hal;
    gco2D g_engine2d;
	gctUINT8 horKernel = 5, verKernel = 7;
	gctUINT32 srcAddress[3] = {0, 0, 0};
	gctUINT32 dstAddress[3] = {0, 0, 0};
	gctUINT32 srcStride[3] = {0, 0, 0};
	gctUINT32 dstStride[3] = {0, 0, 0};
	gcsRECT srcRect, dstRect;

    status = gcoOS_Construct(gcvNULL, &g_os);
    if (status < 0)
    {
        printf("*ERROR* Failed to construct OS object (status = %d)\n", status);
        return;
    }

    /* Construct the gcoHAL object. */
    status = gcoHAL_Construct(gcvNULL, g_os, &g_hal);
    if (status < 0)
    {
        printf("*ERROR* Failed to construct GAL object (status = %d)\n", status);
        return;
    }

	status = gcoHAL_Get2DEngine(g_hal, &g_engine2d);
    if (status < 0)
    {
        printf("*ERROR* Failed to get 2D engine object (status = %d)\n", status);
        return;
    }

	srcAddress[0] = src_addr;
    srcAddress[1] = srcAddress[0] + GPU_SRC_WIDTH * GPU_SRC_HEIGHT;
    dstAddress[0] = dst_addr;
	srcStride[1] = srcStride[0] = GPU_SRC_WIDTH;
    dstStride[0] = GPU_DST_WIDTH * 4;

	srcRect.left = 0;
	srcRect.top = 0;
	srcRect.right = GPU_SRC_WIDTH;
	srcRect.bottom = GPU_SRC_HEIGHT;

	dstRect.left = 0;
	dstRect.top = 0;
	dstRect.right = GPU_DST_WIDTH;
	dstRect.bottom = GPU_DST_HEIGHT;

	// set clippint rect
	gcmONERROR(gco2D_SetClipping(g_engine2d, &srcRect));

	// set kernel size
    gcmONERROR(gco2D_EnableUserFilterPasses(g_engine2d, gcvTRUE, gcvTRUE));
	gcmONERROR(gco2D_SetKernelSize(g_engine2d, horKernel, verKernel));

    gcmONERROR(gco2D_FilterBlitEx2(g_engine2d,
        srcAddress, 2,
        srcStride, 2,
        gcvLINEAR, gcvSURF_NV12,
        gcvSURF_0_DEGREE, GPU_SRC_WIDTH,
        GPU_SRC_HEIGHT, &srcRect,
        dstAddress, 1,
        dstStride, 1,
        gcvLINEAR, gcvSURF_A8R8G8B8,
        gcvSURF_0_DEGREE, GPU_DST_WIDTH,
        GPU_DST_HEIGHT,
        &dstRect, gcvNULL));

	gcmONERROR(gco2D_Flush(g_engine2d));

	gcmONERROR(gcoHAL_Commit(g_hal, gcvTRUE));

    if (g_hal != gcvNULL) {
        gcoHAL_Commit(g_hal, gcvTRUE);
        gcoHAL_Destroy(g_hal);
    }

    if (g_os != gcvNULL) {
        gcoOS_Destroy(g_os);
    }

    return;

OnError:
    printf("2d func failure.\n");
    return;
}

void *gpu_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    struct cma_mem *srcbuf = NULL;
    struct cma_mem *dstbuf = NULL;
    unsigned int *databuf = NULL;
    unsigned int *tmp = NULL;
    int fdstream = -1;
    int streamsize = 0;
    int fddata = -1, datasize = 0;
    int i;

    srcbuf = alloc_cma_mem(0x100000);
    if (!srcbuf) {
        printf("alloc cma memory fail.\n");
        goto err;
    }

    dstbuf = alloc_cma_mem(0x100000);
    if (!dstbuf) {
        printf("alloc cma memory fail.\n");
        goto err;
    }

    fdstream = open(GPU_SRCFILE_PATH, O_RDONLY);
    streamsize = get_file_size(GPU_SRCFILE_PATH);
    if (read(fdstream, srcbuf->viraddr, streamsize) != streamsize) {
        printf("read stream file err.\n");
        goto err;
    }

    data_process(srcbuf->phyaddr, dstbuf->phyaddr);

    fddata = open(GPU_DSTFILE_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open data file fail.\n");
        fddata = open(GPU_DSTFILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
        if (fddata < 0)
            printf("open write data file fail.\n");
        else
            write(fddata, dstbuf->viraddr, GPU_DST_WIDTH * GPU_DST_HEIGHT * 4);
        goto err;
    }
    datasize = get_file_size(GPU_DSTFILE_PATH);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto err;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto err;
    }

    tmp = (unsigned int*)dstbuf->viraddr;
    for (i = 0; i < datasize / 4; i++) {
        if (databuf[i] != tmp[i]) {
            printf("compare data fail.0x%x, 0x%x.\n", databuf[i], tmp[i]);
            goto err;
        }
    }

    free(databuf);
    close(fdstream);
    close(fddata);
    free_cma_mem(srcbuf);
    free_cma_mem(dstbuf);
    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (databuf)
        free(databuf);
    if (fdstream > 0)
        close(fdstream);
    if (fddata > 0)
        close(fddata);
    if (srcbuf)
        free_cma_mem(srcbuf);
    if (dstbuf)
        free_cma_mem(dstbuf);
    rt->finish = 1;
    return (void*)-1;
}