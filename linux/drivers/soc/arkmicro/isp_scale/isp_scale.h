#ifndef __ISP_SCALE_H__
#define __ISP_SCALE_H__

#include <linux/list.h>
#include <linux/clk.h>

#define XM_printf			printk

typedef unsigned char		u8_t;
typedef unsigned short 		u16_t;
typedef unsigned int 		u32_t;

typedef void (*xm_mid_line_callback) (void *xm_mid_line_user_data);

#define ISBUF_FIFO_DEPTH		4

/*isp scale use and 601in use */
#define ISP_SCALE_EN     					0x0
#define ISP_SCALE_WB_START					0x4
#define ISP_SCALE_CONTROL	      			0x8
#define ISP_SCALE_VIDEO_ADDR1				0xC
#define ISP_SCALE_VIDEO_ADDR2	      		0x10
#define ISP_SCALE_VIDEO_ADDR3	      		0x14
#define ISP_SCALE_VIDEO_SOURCE_SIZE   		0x18
#define ISP_SCALE_VIDEO_WINDOW_POINT  		0x1c
#define ISP_SCALE_VIDEO_WINDOW_SIZE   		0x20
#define ISP_SCALE_VIDEO_SIZE				0x24
#define ISP_SCALE_SCALE_CTL					0x28
#define ISP_SCALE_SCALE_VXMOD				0x2c
#define ISP_SCALE_SCALE_CTL0		      	0x30
#define ISP_SCALE_SCALE_CTL1          		0x34
#define ISP_SCALE_RIGHT_BOTTOM_CUT_NUM		0x38
#define ISP_SCALE_SCALE_CTL2				0x3c
#define ISP_SCALE_SCALE_CTL3				0x40
#define ISP_SCALE_SCALE_CTL4				0x44
#define ISP_SCALE_HSCAL_COS_VALUE			0x48
#define ISP_SCALE_WB_DEST_YADDR				0x4c
#define ISP_SCALE_WB_DEST_UADDR				0x50
#define ISP_SCALE_WB_DEST_VADDR				0x54
#define ISP_SCALE_WB_DATA_HSIZE_RAM			0x58
#define ISP_SCALE_RESERVED					0x5c
#define ISP_SCALE_INT_CTL					0x60
#define ISP_SCALE_INT_STATUS				0x64
#define ISP_SCALE_CLCD_INT_CLR				0x68
#define ISP_SCALE_WR_Y_VCNT					0x6C
//#defin rISP_SCALE_WB_YADDR                0x4c
//#defin rISP_SCALE_WB_UVADDR               0x50
#define ISP_SCALE_WB_STATUS         		0x70
#define ISP_SCALE_WB_CTL					0x74
#define ISP_SCALE_WB_FINISH_YADDR			0x78
#define ISP_SCALE_WB_FINISH_UADDR			0x78
#define ISP_SCALE_FRAME_MASK    			0x7c



// \u8bbe\u7f6e\u6700\u5927\u652f\u6301\u7684ISP scalar\u5206\u8fa8\u7387
// 1) \u5f53ISP scalar\u4ec5\u7528\u4e8eLCD\u663e\u793a\u8f93\u51fa\u65f6, \u53ef\u5c06ISP scalar\u7684\u5206\u8fa8\u7387\u8bbe\u7f6e\u4e3a\u663e\u793a\u5c4f\u7684\u89c6\u9891OSD\u5c3a\u5bf8
// 2) \u5f53ISP scalar\u7528\u4e8eLCD\u663e\u793a\u8f93\u51fa\u53ca\u5176\u4ed6\u8f93\u51fa\u65f6, \u5c06ISP scalar\u7684\u5206\u8fa8\u7387\u8bbe\u7f6e\u4e3a\u6700\u5927\u5c3a\u5bf8
#define	MAX_SCALAR_WIDTH			480
#define	MAX_SCALAR_HEIGHT			272

#define	XM_ISP_SCALAR_ERRCODE_OK					(0)
#define	XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA			(-1)		// \u65e0\u6548\u7684\u53c2\u6570
#define	XM_ISP_SCALAR_ERRCODE_DEV_NO_INIT			(-2)		// \u8bbe\u5907\u672a\u521d\u59cb\u5316
#define	XM_ISP_SCALAR_ERRCODE_OBJ_RE_CREATE			(-3)		// scalar\u4e3a\u4e92\u65a5\u5bf9\u8c61,\u6709\u4e14\u4ec5\u6709\u4e00\u4e2a\u5bf9\u8c61\u53ef\u4ee5\u521b\u5efa
#define	XM_ISP_SCALAR_ERRCODE_OBJ_INVALID			(-4)		// \u65e0\u6548\u5bf9\u8c61

enum {
	XM_ISP_SCALAR_FORMAT_YUV420 = 0,		// YUV420 planar (Y\U\V\u6570\u636e\u662f\u5206\u5f00\u5b58\u653e)
	XM_ISP_SCALAR_FORMAT_Y_UV420,			// NV12, YUV420 Semi-Planar (U\u3001V\u662f\u4ea4\u53c9\u5b58\u653e)
	XM_ISP_SCALAR_FORMAT_YUV422,			// YUV422 Planar (Y\U\V\u6570\u636e\u662f\u5206\u5f00\u5b58\u653e)
	XM_ISP_SCALAR_FORMAT_Y_UV422			// YUV422 Semi-Planar (U\u3001V\u662f\u4ea4\u53c9\u5b58\u653e)
};

// \u6e90\u56fe\u50cf\u901a\u9053\u5b9a\u4e49
enum {
	XM_ISP_SCALAR_SRC_CHANNEL_ISP = 0,		// ISP\u8f93\u51fa\u901a\u9053
	XM_ISP_SCALAR_SRC_CHANNEL_ITU601			// ITU601\u8f93\u51fa\u901a\u9053
};

// \u89c6\u9891\u56fe\u50cf\u7684\u884c\u540c\u6b65(hsync)/\u5217\u540c\u6b65(vsync)\u4fe1\u53f7\u6781\u6027\u5b9a\u4e49
enum {
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL = 0,	// \u9ad8\u7535\u5e73\u6709\u6548(ISP VIDEO\u8f93\u51fa\u7f3a\u7701\u4e3a\u9ad8\u7535\u5e73)
	XM_ISP_SCALAR_SRC_SYNC_PLOARITY_LOW_LEVEL = 1,	// \u4f4e\u7535\u5e73\u6709\u6548

};

enum {
	XM_ISP_SCALAR_SRC_YCBCR_UYVY = 0,
	XM_ISP_SCALAR_SRC_YCBCR_YUYV = 1
};

#define	XM_ISP_SCALAR_CALLBACK_COMMAND_READY		0x00000001		// ISP-Scalar\u5e27\u5df2\u6b63\u786e\u63a5\u6536\u5b8c\u6bd5, \u7b49\u5f85\u8c03\u7528\u8005\u5904\u7406
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RECYCLE		0x00000002		// ISP-Scalar\u5e27\u65e0\u6548, \u8c03\u7528\u8005\u9700\u8981\u5c06\u8be5\u5e27\u56de\u6536
#define	XM_ISP_SCALAR_CALLBACK_COMMAND_RESET		0x00000003		// ISP-Scalar\u5f02\u5e38, \u8c03\u7528\u8005\u9700\u8981\u590d\u4f4disp-scalar\u5bf9\u8c61\u5e76\u91cd\u65b0\u6267\u884c

// 5	R/W	0	AXI SCALE write back middle finish interupt
//	2	R/W	0	AXI SCALE write back bresp error interrupt
// 0	R/W	0	AXI SCALE write back frame finish interupt
#define  FRAME_FINISH                      		 	(1 << 0)
#define	write_back_bresp_error_interrupt				(1 << 2)
#define	write_back_middle_finish_interupt			(1 << 5)

// 2	R/W	0	AXI SCALE write back middle finish interupt clear
//					Write 1 to clear
// 1	R/W	0	AXI SCALE write back bresp error interrupt clear
//					Write 1 to clear
//0	R/W	0	AXI SCALE write back frame finish interupt clear
//					Write 1 to clear
#define  CLEAR_FRAME_FINISH                 			(1 << 0)
#define	write_back_bresp_error_interrupt_clear		(1 << 1)
#define	write_back_middle_finish_interupt_clear		(1 << 2)


// ISP Scalar \u914d\u7f6e\u53c2\u6570\u8868
// \u5b9a\u4e49\u4e86\u5982\u4f55\u5c06\u6e90\u89c6\u9891\u56fe\u50cf\u7684\u5b9a\u4e49\u533a\u57df(\u6e90\u7a97\u53e3, \u5305\u542b\u6307\u7a97\u53e3\u4f4d\u7f6e/\u7a97\u53e3\u5927\u5c0f )\u6216\u6574\u4e2a\u56fe\u50cf\u590d\u5236\u5230\u76ee\u6807\u89c6\u9891\u56fe\u50cf\u7684\u5b9a\u4e49\u533a\u57df(\u76ee\u6807\u7a97\u53e3, \u5305\u542b\u7a97\u53e3\u4f4d\u7f6e/\u7a97\u53e3\u5927\u5c0f)
typedef struct _xm_isp_scalar_configuration_parameters {
	u16_t		src_channel;			// \u6e90\u56fe\u50cf\u7684\u8f93\u5165\u901a\u9053\u9009\u62e9 (\u5185\u90e8isp\u8f93\u51fa \u6216\u8005 \u5916\u90e8itu601\u8f93\u5165)
											//		\u5185\u90e8isp\u8f93\u51fa\u4e00\u822c\u4e3aY_UV420(NV12)\u683c\u5f0f
											//		\u5916\u90e8itu601\u8f93\u5165\u4e00\u822c\u4e3aYUV422, YUV420\u7b49\u683c\u5f0f
	u8_t		src_hsync_polarity;	// \u6e90\u56fe\u50cf\u7684\u884c\u540c\u6b65\u4fe1\u53f7\u6781\u6027\u9009\u62e9
	u8_t		src_vsync_polarity;	// \u6e90\u56fe\u50cf\u7684\u5217\u540c\u6b65\u4fe1\u53f7\u6781\u6027\u9009\u62e9


	// \u6e90\u56fe\u50cf\u7684\u5b9a\u4e49(cmos sensor\u6216\u8005601 in)
	u16_t 	src_format;				// YUV422/Y_UV422/YUV420/Y_UV420
	u16_t		src_ycbcr_sequence;	// y/cbcr\u7684\u987a\u5e8f, 0:uyvy 1:yuyv
	u16_t		src_width;				// \u6e90\u56fe\u50cf\u7684\u5bbd\u5ea6
	u16_t		src_height;				// \u6e90\u56fe\u50cf\u7684\u9ad8\u5ea6
	u16_t		src_stride;				// \u6e90\u56fe\u50cf\u7684\u884c\u5bbd\u5ea6
	// \u6e90\u7a97\u53e3\u5b9a\u4e49(\u81ea\u5de6\u5411\u53f3, \u81ea\u4e0a\u800c\u4e0b)
	u16_t		src_window_x;			// \u6e90\u7a97\u53e3\u76f8\u5bf9\u4e8e\u6e90\u56fe\u50cf\u539f\u70b9\u7684X\u65b9\u5411\u504f\u79fb
	u16_t		src_window_y;			// \u6e90\u7a97\u53e3\u76f8\u5bf9\u4e8e\u6e90\u56fe\u50cf\u539f\u70b9\u7684Y\u65b9\u5411\u504f\u79fb
	u16_t		src_window_width;		// \u6e90\u7a97\u53e3\u7684\u5bbd\u5ea6
	u16_t		src_window_height;	// \u6e90\u7a97\u53e3\u7684\u9ad8\u5ea6

	// \u76ee\u6807\u56fe\u50cf\u5b9a\u4e49
	u16_t		dst_format;				// \u4ec5\u652f\u6301Y_UV420(NV12)
	u16_t		dst_width;				// \u76ee\u6807\u56fe\u50cf\u7684\u5bbd\u5ea6
	u16_t		dst_height;				// \u76ee\u6807\u56fe\u50cf\u7684\u9ad8\u5ea6
	u16_t		dst_stride;				// \u76ee\u6807\u56fe\u50cf\u7684\u884c\u5bbd\u5ea6, \u5fc5\u987b\u4e3a16\u7684\u500d\u6570
	// \u76ee\u6807\u7a97\u53e3\u5b9a\u4e49(\u81ea\u5de6\u5411\u53f3, \u81ea\u4e0a\u800c\u4e0b)
	u16_t		dst_window_x;			// \u76ee\u6807\u7a97\u53e3\u76f8\u5bf9\u4e8e\u76ee\u6807\u56fe\u50cf\u539f\u70b9\u7684X\u65b9\u5411\u504f\u79fb
	u16_t		dst_window_y;			// \u76ee\u6807\u7a97\u53e3\u76f8\u5bf9\u4e8e\u76ee\u6807\u56fe\u50cf\u539f\u70b9\u7684Y\u65b9\u5411\u504f\u79fb
	u16_t		dst_window_width;		// \u76ee\u6807\u7a97\u53e3\u7684\u5bbd\u5ea6
	u16_t		dst_window_height;	// \u76ee\u6807\u7a97\u53e3\u7684\u9ad8\u5ea6

	// \u90e8\u5206\u5904\u7406\u5b8c\u6210\u884c\u4e2d\u65ad
	u16_t 	mid_line; 								// mid_line\u8ba1\u6570\u503c(\u4ee5\u884c\u4e3a\u5355\u4f4d)
	void *	mid_line_user_data;					// \u7528\u6237\u79c1\u6709\u6570\u636e
	xm_mid_line_callback mid_line_user_callback;	// \u7528\u6237\u56de\u8c03\u51fd\u6570

} xm_isp_scalar_configuration_parameters;

enum isp_buf_status{
    ISBUF_STATUS_FREE,
    ISBUF_STATUS_BUSY,
	ISBUF_STATUS_READY,
};

struct isp_scale_buf {
	unsigned int yaddr;
	unsigned int uvaddr;
};

struct ispbuf_id {
	int id;
	struct list_head list;
};

struct ark_isp_scale_context {
	int irq;
	struct device *dev;
	void __iomem *mmio_base;
	void __iomem *sys_base;
	struct clk	*clk;
	spinlock_t lock;
	int	isbuf_num;
	struct isp_scale_buf isbuf[ISBUF_FIFO_DEPTH];
	int isbuf_status[ISBUF_FIFO_DEPTH];
	struct ispbuf_id isbuf_id[ISBUF_FIFO_DEPTH];
	struct list_head isbuf_push_list;
	xm_isp_scalar_configuration_parameters config;
};

struct ark_isp_scale_device {
	const char *driver_name;
	const char *name;
    int major;
    int minor_start;
    int minor_num;
    int num;
	int irq;
	struct cdev cdev;
	struct class *isp_scale_class;
	struct device *isp_scale_device;
	struct fasync_struct *async_queue_wb;
	wait_queue_head_t frame_finish_waitq;
	struct ark_isp_scale_context context;
};


struct isp_scale_init_para {
	unsigned int src_channel;
	unsigned int src_hsync_polarity;
	unsigned int src_vsync_polarity;
	unsigned int src_format;
	unsigned int src_ycbcr_sequence;

	unsigned int src_width;
	unsigned int src_height;
	unsigned int src_window_x;
	unsigned int src_window_y;
	unsigned int src_window_width;
	unsigned int src_window_height;

	unsigned int dst_width;
	unsigned int dst_height;

	unsigned int mid_line_counter;
};

struct isp_scale_sbuf_para {
	int num;
	struct isp_scale_buf buf[ISBUF_FIFO_DEPTH];
};


/*************************************************************************
 * Ioctl command definition
 *************************************************************************/
#define ISP_SCALE_IOCTL_BASE			0x98

#define ISP_SCALE_IOCTL_INIT			_IOW(ISP_SCALE_IOCTL_BASE, 0, struct isp_scale_init_para)
#define ISP_SCALE_IOCTL_SET_BUFFER	_IOW(ISP_SCALE_IOCTL_BASE, 1, struct isp_scale_sbuf_para)
#define ISP_SCALE_IOCTL_START		_IO(ISP_SCALE_IOCTL_BASE, 2)
#define ISP_SCALE_IOCTL_STOP			_IO(ISP_SCALE_IOCTL_BASE, 3)
#define ISP_SCALE_IOCTL_GET_READY	_IOR(ISP_SCALE_IOCTL_BASE, 4, struct isp_scale_buf)
#define ISP_SCALE_IOCTL_SET_FREE    _IOW(ISP_SCALE_IOCTL_BASE, 5, struct isp_scale_buf)

int xm_isp_scalar_config (struct ark_isp_scale_context *context,
			xm_isp_scalar_configuration_parameters  *scalar_parameters);

int ark_isp_scale_dev_init(struct ark_isp_scale_context *context);

irqreturn_t ark_isp_scale_intr_handler(int irq, void *dev_id);

void isp_scale_buffer_init(struct ark_isp_scale_context *context);

void isp_scale_softreset(struct ark_isp_scale_context *context);

static inline void isp_scale_push_buffer(struct ark_isp_scale_context *context,
		int frameid)
{
	writel(context->isbuf[frameid].yaddr, context->mmio_base + ISP_SCALE_WB_DEST_YADDR);
	writel(context->isbuf[frameid].uvaddr, context->mmio_base + ISP_SCALE_WB_DEST_UADDR);
}

#endif
