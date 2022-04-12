#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"
#include "display.h"

#define	DEINTERLACE_SUCCESS			(0)
#define	DEINTERLACE_PARA_ERROR		(-1)
#define	DEINTERLACE_AXI_ERROR		(-2)
#define	DEINTERLACE_TIMEOUT			(-3)

enum {
	DEINTERLACE_LINE_SIZE_720H = 0,
	DEINTERLACE_LINE_SIZE_960H
};

enum {
	DEINTERLACE_DATA_MODE_420 = 0,
	DEINTERLACE_DATA_MODE_422
};

enum {
	DEINTERLACE_TYPE_PAL = 0,		// 576
	DEINTERLACE_TYPE_NTSC			// 480
};

enum {
	DEINTERLACE_FIELD_ODD = 0,
	DEINTERLACE_FIELD_EVEN
};

static u32* deintbase = NULL;
struct cma_mem *deint_mem = NULL;

#define rDEINTERLACE_START			    *(volatile u32*)(deintbase + 0x00/4)
#define rDEINTERLACE_CTRL0			    *(volatile u32*)(deintbase + 0x04/4)
#define rDEINTERLACE_CTRL1			    *(volatile u32*)(deintbase + 0x08/4)
#define rDEINTERLACE_CTRL2			    *(volatile u32*)(deintbase + 0x0C/4)
#define rDEINTERLACE_CTRL3			    *(volatile u32*)(deintbase + 0x10/4)
#define rDEINTERLACE_FILM_MODECTRL	    *(volatile u32*)(deintbase + 0x14/4)
#define rDEINTERLACE_SADDR0			    *(volatile u32*)(deintbase + 0x18/4)
#define rDEINTERLACE_SADDR1			    *(volatile u32*)(deintbase + 0x1C/4)
#define rDEINTERLACE_SADDR2			    *(volatile u32*)(deintbase + 0x20/4)
#define rDEINTERLACE_DADDRY			    *(volatile u32*)(deintbase + 0x24/4)
#define rDEINTERLACE_DADDRU			    *(volatile u32*)(deintbase + 0x28/4)
#define rDEINTERLACE_DADDRV			    *(volatile u32*)(deintbase + 0x2C/4)
#define rDEINTERLACE_SADDR0_1			*(volatile u32*)(deintbase + 0x30/4)
#define rDEINTERLACE_SADDR1_1			*(volatile u32*)(deintbase + 0x34/4)
#define rDEINTERLACE_SADDR2_1			*(volatile u32*)(deintbase + 0x38/4)
#define rDEINTERLACE_DADDRY_1			*(volatile u32*)(deintbase + 0x3C/4)
#define rDEINTERLACE_DADDRU_1			*(volatile u32*)(deintbase + 0x40/4)
#define rDEINTERLACE_DADDRV_1			*(volatile u32*)(deintbase + 0x44/4)
#define rDEINTERLACE_INT_MASK			*(volatile u32*)(deintbase + 0x48/4)
#define rDEINTERLACE_RAW_INT			*(volatile u32*)(deintbase + 0x4C/4)
#define rDEINTERLACE_INT_CLEAR			*(volatile u32*)(deintbase + 0x50/4)
#define rDEINTERLACE_STATUS			    *(volatile u32*)(deintbase + 0x54/4)
#define rDEINTERLACE_ADDR_SWITCHMODE	*(volatile u32*)(deintbase + 0x58/4)


static void deinterlace_reset(void)
{
	rDEINTERLACE_CTRL0 = (1 << 0);
	usleep(10);
	rDEINTERLACE_CTRL0 = (0 << 0);
}

static void deinterlace_init(void)
{
	deinterlace_reset();
	rDEINTERLACE_INT_CLEAR = 0x3;
	rDEINTERLACE_INT_MASK = 0x3;
}

static int deinterlace_process (unsigned int deinterlace_size,  unsigned int data_mode, unsigned int deinterlace_type,
	unsigned int deinterlace_field, unsigned int src_field_addr_0, unsigned int src_field_addr_1,unsigned int src_field_addr_2,
	unsigned int dst_y_addr, unsigned int dst_u_addr, unsigned int dst_v_addr)
{
	unsigned int pixel_per_line;
	unsigned int total_line;
	unsigned int pn;
	unsigned int denoise_bypass;
	unsigned int stride;
	unsigned int only_wr_1_field;
	unsigned int field;
	unsigned int out_mode;
	unsigned int vary_level_th;
	int ret = DEINTERLACE_SUCCESS;

	if(deinterlace_field != DEINTERLACE_FIELD_ODD
		&&	deinterlace_field != DEINTERLACE_FIELD_EVEN){
		printf("invalid deinterlace field (%d)\r\n", deinterlace_field);
		return DEINTERLACE_PARA_ERROR;
	}
	field = deinterlace_field;

	if(deinterlace_size == DEINTERLACE_LINE_SIZE_960H){
		pixel_per_line = 120 * (1 + data_mode);
	}else if(deinterlace_size == DEINTERLACE_LINE_SIZE_720H){
		pixel_per_line = 90 * (1 + data_mode);
	}else{
		printf("invalid deinterlace size (%d)\r\n", deinterlace_size);
		return DEINTERLACE_PARA_ERROR;
	}

	if(data_mode == DEINTERLACE_DATA_MODE_420){
		denoise_bypass = 1;
		stride = pixel_per_line;
		only_wr_1_field = 1;
	}else if(data_mode == DEINTERLACE_DATA_MODE_422){
		if(dst_y_addr == 0){
			printf("illegal deinterlace dst Y address\r\n");
			return DEINTERLACE_PARA_ERROR;
		}
		denoise_bypass = 1;
		stride = 0;
		only_wr_1_field = 0;
	}else{
		printf("invalid deinterlace data mode (%d)\r\n", data_mode);
		return DEINTERLACE_PARA_ERROR;
	}

	if(deinterlace_type == DEINTERLACE_TYPE_PAL){
		pn = 0;
		total_line = 288;
		vary_level_th = 0x800;
	}else if(deinterlace_type == DEINTERLACE_TYPE_NTSC){
		pn = 1;
		total_line = 240;
		vary_level_th = 0x8;
	}else{
		printf("invalid deinterlace type (%d)\r\n", deinterlace_type);
		return DEINTERLACE_PARA_ERROR;
	}

	out_mode = DEINTERLACE_DATA_MODE_422;

 	rDEINTERLACE_CTRL0 = (out_mode<<31)
						|  (0x0 << 29)
						|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field
						|  (pixel_per_line << 20)       // pixel_pl
						|  (total_line << 11)           // total_line
						|  (stride << 3)                // stride
						|  (data_mode << 2)             // data_mode
						|  (field << 1);                 // field

	rDEINTERLACE_CTRL1 = (0x8 << 24 )
					|(vary_level_th << 8)
					|(0X8 << 0);

	rDEINTERLACE_CTRL2 = 0x300A4230;

	// denoise bypass  pn: 1:n display_motion line_intra global_cnt display_mv_0
	rDEINTERLACE_CTRL3 = (0x07 << 16)				//motion_ctrl_reg5\uff1a\u964d\u566a\u95e8\u9650
						| (denoise_bypass << 15)
						| (pn << 13)
						| (0x1 << 11)
						| (0x1 << 10)
						| (0x1 << 9)
						| (0x0 << 6)
						| (0x0 << 5)
						| (0x1 << 3)
						| (0x0 << 2)  // max_ycbcr_ena :use for control the max control
						| (0x1 << 1)
						| (0x0 << 0);

	rDEINTERLACE_FILM_MODECTRL = (0x0 << 31);

	rDEINTERLACE_SADDR0 = src_field_addr_0;	// s0_0
	rDEINTERLACE_SADDR1 = src_field_addr_1;	// s1_0
	rDEINTERLACE_SADDR2 = src_field_addr_2;	// s2_0

	rDEINTERLACE_DADDRY = dst_y_addr;	// dy_0  when filed = 1 ,data_mode = 0  then dy_0 = s0_0 , when filed = 0 ,data_mode = 0  then dy_0 = s2_0
	rDEINTERLACE_DADDRU = dst_u_addr;	// du_0
	rDEINTERLACE_DADDRV = dst_v_addr;	// dv_0

	//	pingpong addr fetch
	rDEINTERLACE_ADDR_SWITCHMODE = 0x0;

	// \u6e05\u9664\u4e2d\u65ad
	rDEINTERLACE_INT_CLEAR = 0x3;

	// start de-interlace
	rDEINTERLACE_START = 0x1;

	return ret;
}

void *deinterlace_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    u32 field1_phyaddr, field2_phyaddr, field3_phyaddr;
    char *field1_viraddr, *field2_viraddr, *field3_viraddr;
    u32 dst_phyaddr;
    char *dst_viraddr;
    int fdfield = -1;
    unsigned int *databuf = NULL;
    int fddata = -1;
    unsigned long datasize;
    u32 *tmp;
    int i;

    deintbase = map_phy_memory(0xe0d00000, 0x1000, 1);
    if (!deintbase)
        goto err;

    deint_mem = alloc_cma_mem(0x200000);
    if (!deint_mem) {
        printf("alloc_cma_mem fail.\n");
        goto err;
    }
    field1_phyaddr = deint_mem->phyaddr;
    field1_viraddr = deint_mem->viraddr;
    field2_phyaddr = field1_phyaddr + NTSC_WIDTH * NTSC_HEIGHT;
    field2_viraddr = field1_viraddr + NTSC_WIDTH * NTSC_HEIGHT;
    field3_phyaddr = field2_phyaddr + NTSC_WIDTH * NTSC_HEIGHT;
    field3_viraddr = field2_viraddr + NTSC_WIDTH * NTSC_HEIGHT;
    dst_phyaddr = field3_phyaddr + NTSC_WIDTH * NTSC_HEIGHT;
    dst_viraddr = field3_viraddr + NTSC_WIDTH * NTSC_HEIGHT;

    fdfield = open(DEINT_FIELD_DATA_PATH, O_RDONLY);
    if (fdfield < 0) {
        printf("open %s fail.\n", DEINT_FIELD_DATA_PATH);
        goto err;
    }
    if (read(fdfield, field1_viraddr, NTSC_WIDTH * NTSC_HEIGHT * 3) !=
            NTSC_WIDTH * NTSC_HEIGHT * 3) {
        printf("read data err.\n");
        goto err;
    }

    deinterlace_init();
    deinterlace_process (
                DEINTERLACE_LINE_SIZE_720H,
                DEINTERLACE_DATA_MODE_422,
                DEINTERLACE_TYPE_NTSC,
                DEINTERLACE_FIELD_ODD,
                field1_phyaddr,
                field2_phyaddr,
                field3_phyaddr,
                dst_phyaddr,
                0,	  // for yuv420
                0);   // for yuv420
    usleep(100000);

    /* int fd_frame = open("frame.yuv", O_WRONLY | O_CREAT | O_TRUNC);
    write(fd_frame, dst_viraddr, NTSC_WIDTH * NTSC_HEIGHT * 2);
    close(fd_frame);
    } */

    fddata = open(DEINT_FRAME_DATA_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open data file fail.\n");
        goto err;
    }
    datasize = get_file_size(DEINT_FRAME_DATA_PATH);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto err;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto err;
    }

    tmp = (unsigned int*)dst_viraddr;
    for (i = 0; i < datasize / 4; i++) {
        if (databuf[i] != tmp[i]) {
            printf("compare data fail.0x%x, 0x%x.\n", databuf[i], tmp[i]);
            goto err;
        }
    }

    free(databuf);
    close(fddata);
    close(fdfield);
    free_cma_mem(deint_mem);
    unmap_phy_memory(deintbase, 0x1000);

    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (databuf)
        free(databuf);
    if (fddata > 0)
        close(fddata);
    if (fdfield > 0)
        close(fdfield);
    if (deint_mem)
        free_cma_mem(deint_mem);
    if (!deintbase)
        unmap_phy_memory(deintbase, 0x1000);

    rt->finish = 1;
    return (void*)-1;
}