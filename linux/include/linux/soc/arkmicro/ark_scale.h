#ifndef __SCALE_H__
#define __SCALE_H__

#include <linux/list.h>
#include <linux/clk.h>

typedef unsigned char		u8_t;
typedef unsigned short 		u16_t;
typedef unsigned int 		u32_t;


#define AXI_SCALE_EN     					0x0
#define AXI_SCALE_WB_START					0x4
#define AXI_SCALE_CONTROL	      			0x8

#define AXI_SCALE_VIDEO_ADDR1				0xC
#define AXI_SCALE_VIDEO_ADDR2	      		0x10
#define AXI_SCALE_VIDEO_ADDR3	      		0x14
#define AXI_SCALE_VIDEO_SOURCE_SIZE   		0x18
#define AXI_SCALE_VIDEO_WINDOW_POINT  		0x1c
#define AXI_SCALE_VIDEO_WINDOW_SIZE   		0x20

#define AXI_SCALE_VIDEO_SIZE				0x24
#define AXI_SCALE_SCALE_CTL					0x28
#define AXI_SCALE_SCALE_VXMOD				0x2c
#define AXI_SCALE_SCALE_CTL0		      	0x30
#define AXI_SCALE_SCALE_CTL1          		0x34

#define AXI_SCALE_RIGHT_BOTTOM_CUT_NUM		0x38
#define AXI_SCALE_SCALE_CTL2				0x3c

#define AXI_SCALE_SCALE_CTL3				0x40
#define AXI_SCALE_SCALE_CTL4				0x44
#define AXI_SCALE_HSCAL_COS_VALUE			0x48
#define AXI_SCALE_WB_DEST_YADDR				0x4c

#define AXI_SCALE_WB_DEST_UADDR				0x50
#define AXI_SCALE_WB_DEST_VADDR				0x54
#define AXI_SCALE_WB_DATA_HSIZE_RAM			0x58
#define AXI_SCALE_RESERVED					0x5c

#define AXI_SCALE_INT_CTL					0x60
#define AXI_SCALE_INT_STATUS				0x64
#define AXI_SCALE_CLCD_INT_CLR				0x68
#define AXI_SCALE_WR_Y_VCNT					0x6C
#define AXI_SCALE_WR_UV_VCNT				0x70

/* for ark1668e as follows. */
#define AXI_SCALE_VFILTER_CLR				0x70
#define AXI_SCALE_VFILTER_COEFF				0x74

//Rotate Register
#define AXI_SCALE_ROTATA_CTL				0x78
#define AXI_SCALE_ROTATA_IMG_SIZE			0x7C
#define AXI_SCALE_ROTATA_Y_FADR				0x80
#define AXI_SCALE_ROTATA_C_FADR				0x84
#define AXI_SCALE_ROTATA_CTL_REG			0x88
#define AXI_SCALE_ROTYX_WIDTH				0x4C
#define AXI_SCALE_ROTCX_FADR				0x50

//Peaking&Denoise Register
#define AXI_SCALE_PD_CTL1					0x8c
#define AXI_SCALE_PD_CTL2					0x90
#define AXI_SCALE_DENOISE_CTL3				0x94
#define AXI_SCALE_PEAKING_CTL4				0x98
#define AXI_SCALE_PEAKING_CTL5				0x9C
#define AXI_SCALE_PEAKING_CTL6				0xA0


/*************************************************************************
 * Ioctl command definition
 *************************************************************************/
#define SCALE_IOCTL_BASE			0x99

#define SCALE_IOCTL_START		_IOW(SCALE_IOCTL_BASE, 0, struct ark_scale_param)


enum {
	SOC_TYPE_ARKN141,
	SOC_TYPE_ARK1668,
	SOC_TYPE_ARK1668E,
	SOC_TYPE_END
};

typedef enum ark_scale_rotate{
	SCALE_ROTATE_0,
	SCALE_ROTATE_90,
	SCALE_ROTATE_180,
	SCALE_ROTATE_270,
	SCALE_ROTATE_0_MIRROR,
	SCALE_ROTATE_90_MIRROR,
	SCALE_ROTATE_180_MIRROR,
	SCALE_ROTATE_270_MIRROR,
	SCALE_ROTATE_END
}scalar_rotate;

enum scale_result {
	SCALE_RET_OK = 0,
    SCALE_RET_TIMEOUT = 1,
	SCALE_RET_INV_PARA = 2,
    SCALE_RET_DATA_ERROR = 2,
    SCALE_RET_HW_ERROR = 4,
    SCALE_RET_SYSTEM_ERROR = 5,
    SCALE_RET_HW_RESET = 6,
	SCALE_RET_FAILED = 7,
	SCALE_RET_MALLOC_FAILED = 8,
	SCALE_RET_COMPARE_NG = 9
};

typedef enum ark_scalar_format {
	ARK_SCALE_FORMAT_YUV422 = 0,
	ARK_SCALE_FORMAT_Y_UV422 = (1 << 8) | ARK_SCALE_FORMAT_YUV422,
	ARK_SCALE_FORMAT_Y_U_V420 = 1,
	ARK_SCALE_FORMAT_Y_UV420 = (1 << 8) | ARK_SCALE_FORMAT_Y_U_V420,
	ARK_SCALE_FORMAT_YUYV = 2,
	ARK_SCALE_FORMAT_YVYU = (1 << 8) | ARK_SCALE_FORMAT_YUYV,
	ARK_SCALE_FORMAT_UYVY = (2 << 8) | ARK_SCALE_FORMAT_YUYV,
	ARK_SCALE_FORMAT_VYUY = (3 << 8) | ARK_SCALE_FORMAT_YUYV,
	ARK_SCALE_FORMAT_YUV = 3,
}scalar_in_format;

typedef enum ark_scalar_output_format {
	ARK_SCALE_OUT_FORMAT_Y_UV422 = (1 << 8) | ARK_SCALE_FORMAT_YUV422,
	ARK_SCALE_OUT_FORMAT_Y_UV420 = (1 << 8) | ARK_SCALE_FORMAT_Y_U_V420,
	ARK_SCALE_OUT_FORMAT_YUYV = 2,
	ARK_SCALE_OUT_FORMAT_END
}scalar_out_format;

struct ark_scale_param {
	unsigned int iyaddr;
	unsigned int iuaddr;
	unsigned int ivaddr;
	scalar_in_format iformat;
	unsigned int iwidth;
	unsigned int iheight;
	unsigned int ix;
	unsigned int iy;
	unsigned int iwinwidth;
	unsigned int iwinheight;
	unsigned int left_cut;
	unsigned int right_cut;
	unsigned int up_cut;
	unsigned int bottom_cut;
	unsigned int owidth;
	unsigned int oheight;
	unsigned int oyaddr;
	unsigned int ouaddr;
	unsigned int ovaddr;
	scalar_out_format oformat;	//Only ark1668e.
	scalar_rotate rotate;		//Only ark1668e.
};


struct ark_scale_context {
	int irq;
	struct device *dev;
	void __iomem *mmio_base;
	void __iomem *sys_base;
	struct clk	*clk;
	spinlock_t lock;
	wait_queue_head_t waitq;
	int busy;
	int softreset_reg;
	int softreset_offset;
	int soc_type;
};

struct ark_scale_device {
	const char *driver_name;
	const char *name;
	int major;
	int minor_start;
	int minor_num;
	int num;
	int irq;
	struct cdev cdev;
	struct class *scale_class;
	struct device *scale_device;
	struct fasync_struct *async_queue_wb;
	struct ark_scale_context context;
};

int ark_scale_start(struct ark_scale_context *context, struct ark_scale_param *param);

int ark_scale_dev_init(struct ark_scale_context *context);

irqreturn_t ark_scale_intr_handler(int irq, void *dev_id);

#endif
