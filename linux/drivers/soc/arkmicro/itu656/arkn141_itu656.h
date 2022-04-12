/*
 * Copyright(c) 2013 Hong Kong Applied Science and Technology
 * Research Institute Company Limited (ASTRI), all rights reserved
 * Proprietary and Confidential Information.
 *
 * This source file is the property of ASTRI, and may not be copied or
 * distributed in any isomorphic form without the prior written consent
 * of ASTRI.
 *
 * Name:
 *              itu656_regs.h
 *
 * Description:
 *              This file contains ark809 SOC ITU656 register definitions
 *
 * Author:
 *              Chan Man Chi
 *
 * Remarks:
 *
 */

#ifndef _ARKN141_ITU656_REGS_H_
#define _ARKN141_ITU656_REGS_H_

#include <linux/list.h>

#define ITU656_DEV_CLASS_CREATE                     1

/* ITU656 */
#define DELTA_LINE					                20	
#define DELTA_PIX					                10

// ENABLE_REG            0xE0800000 + 0x930
#define   WRITE_MEMORY_TWO_FIELD                    (0<<14)    
#define   WRITE_MEMORY_SINGLE_FIELD             	(1<<14)   
#define   CBCR_YVYU            	                	(0<<13) 
#define   CBCR_YUYV                                 (1<<13) 
#define   FRAME_INTR_EVEN_FIELD                		(1<<12) 
#define   FRAME_INTR_ODD_FIELD                 		(0<<12) 
#define   H_FILTER_COEF_SOFTWARE                  	(0<<11) 
#define   H_FILTER_COEF_AUTO                   		(1<<11) 
#define   STORE_DATA_SINGLE_TWO_FIELD        		(0<<5)
#define   STORE_DATA_2FIELD_2ADDR                	(1<<5)
#define   STORE_DATA_FRAME                          (3<<5)
#define   CBCR_VYUY                                 (1<<4) 
#define   YCBCR444_422_FILTER_DISABLE          		(0<<3)
#define   YCBCR444_422_FILTER_ENABLE           		(1<<3)
#define   LOW_FILTER_DISABLE                        (0<<2)
#define   LOW_FILTER_ENABLE                         (1<<2)
#define   CR_FIRST                                  (0<<1)   
#define   CB_FIRST                                  (1<<1)   
#define   GLOBAL_DISABLE                            (0<<0)
#define   GLOBAL_ENABLE                             (1<<0)

#define   EVEN_FIELD_INTERRUPT                      (1<<8)
#define   ACTIVE_LINE_CHANGED_INTERRUPT			    (1<<7)
#define   TOTAL_LINE_CHANGED_INTERRUPT			    (1<<6)
#define   ACTIVE_PIX_CHANGED_INTERRUPT			    (1<<5)
#define   TOTAL_PIX_CHANGED_INTERRUPT			    (1<<4)
#define   FRAME_INTERRUPT_INTERRUPT                 (1<<3)
#define   FIFO_ERROR_INTERRUPT					    (1<<2)
#define   PN_CHANGED_INTERRUPT					    (1<<1)
#define   FIELD_INTERRUPT						    (1<<0)

#define	DEINTERLACE_SUCCESS				            (0)	
#define	DEINTERLACE_PARA_ERROR			            (-1)
#define	DEINTERLACE_AXI_ERROR			            (-2)
#define	DEINTERLACE_TIMEOUT				            (-3)

//flame status

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

enum {
	NTSC_PAL = 0,
	FRAME_VALID,
	FRAME_USED,
	ODD_EVEN,
	FILE_MODE     //this bit set mean first is odd, odd-even-odd-even; or else first even, even-odd-even-odd
};

enum itu656_channel {
	ITU656_CH0,
	ITU656_CH1,
	ITU656_CH0_CH1,
};


#define DEINTERLACE_START                       0x00
#define DEINTERLACE_CTRL0                       0x04
#define DEINTERLACE_CTRL1						0x08
#define DEINTERLACE_CTRL2						0x0C
#define DEINTERLACE_CTRL3						0x10
#define DEINTERLACE_FILM_MODECTRL               0x14
#define DEINTERLACE_SADDR0						0x18
#define DEINTERLACE_SADDR1						0x1C
#define DEINTERLACE_SADDR2						0x20
#define DEINTERLACE_DADDRY						0x24
#define DEINTERLACE_DADDRU						0x28
#define DEINTERLACE_DADDRV						0x2C
#define DEINTERLACE_SADDR0_1					0x30
#define DEINTERLACE_SADDR1_1					0x34
#define DEINTERLACE_SADDR2_1					0x38
#define DEINTERLACE_DADDRY_1					0x3C
#define DEINTERLACE_DADDRU_1					0x40
#define DEINTERLACE_DADDRV_1					0x44
#define DEINTERLACE_INT_MASK					0x48
#define DEINTERLACE_RAW_INT						0x4C
#define DEINTERLACE_INT_CLEAR					0x50
#define DEINTERLACE_STATUS						0x54
#define DEINTERLACE_ADDR_SWITCHMODE				0x58


#define ITU656IN_MODULE_EN						0x00
#define ITU656IN_IMR							0x124
#define ITU656IN_ICR							0x128
#define ITU656IN_ISR							0x12C

#define ITU656IN_LINE_NUM_PER_FIELD				0x8f4
#define ITU656IN_PIX_NUM_PER_LINE				0x8f8
#define ITU656IN_DELTA_NUM						0x8fc

#define ITU656IN_INPUT_SEL						0x900
#define ITU656IN_INPUT_CTL						0x904
#define ITU656IN_ENABLE_REG						0x930
#define ITU656IN_MODULE_STATUS					0x934
#define ITU656IN_SIZE							0x938
#define ITU656IN_SLICE_PIXEL_NUM				0x94C
#define ITU656IN_DRAM_Y_ADDR					0x950
#define ITU656IN_DRAM_CBCR_ADDR					0x954
#define ITU656IN_TOTAL_PIX						0x958
#define ITU656IN_DATA_OUT_LINE_NUM_PER_FIELD 	0x95c
#define ITU656IN_H_CUT							0x960
#define ITU656IN_V_CUT							0x964


/* AHB system */
#define SYS_BOOT_SAMPLE        					0x0
#define SYS_CLK_SEL            					0x40
#define SYS_AHB_CLK_EN         					0x44
#define SYS_APB_CLK_EN         					0x48
#define SYS_AXI_CLK_EN         					0x4c
#define SYS_PER_CLK_EN         					0x50
#define SYS_LCD_CLK_CFG        					0x54
#define SYS_SD_CLK_CFG         					0x58
#define SYS_SD1_CLK_CFG        					0x5c
#define SYS_DEVICE_CLK_CFG0    					0x60
#define SYS_DEVICE_CLK_CFG1    					0x64
#define SYS_DEVICE_CLK_CFG2    					0x68
#define SYS_DEVICE_CLK_CFG3    					0x6c
#define SYS_CLK_DLY_REG        					0x70
#define SYS_SOFT_RSTNA         					0x74
#define SYS_SOFT_RSTNB         					0x78
#define SYS_SD2_CLK_CFG        					0x7c

#define SYS_ANALOG_REG0            				0x140
#define SYS_ANALOG_REG1            				0x144
#define SYS_DDR2_PAD_REG           				0x148
#define SYS_PLLRFCK_CTL            				0x14c
#define SYS_CPUPLL_CFG             				0x150
#define SYS_SYSPLL_CFG             				0x154
#define SYS_AUDPLL_CFG             				0x158
#define SYS_DDRDLL_RDCLK_CFG       				0x15c
#define SYS_DDRDLL_WRCLK_CFG       				0x160
#define SYS_DDRDLL_DQS_CFG0        				0x164
#define SYS_DDRDLL_DQS_CFG1        				0x168
#define SYS_DDRDLL_DQS_CFG2        				0x16C
#define SYS_DDRDLL_BIAS_UP_TRIM    				0x170

#define SYS_LVDS_CTRL_CFG      					0x190
#define SYS_DDS_CLK_CFG        					0x198
#define SYS_DDS_IO_CFG         					0x19C

#define SYS_PAD_CTRL00         					0x1c0
#define SYS_PAD_CTRL01         					0x1c4
#define SYS_PAD_CTRL02         					0x1c8
#define SYS_PAD_CTRL03         					0x1cc
#define SYS_PAD_CTRL04         					0x1d0
#define SYS_PAD_CTRL05         					0x1d4
#define SYS_PAD_CTRL06         					0x1d8
#define SYS_PAD_CTRL07         					0x1dc
#define SYS_PAD_CTRL08         					0x1e0
#define SYS_PAD_CTRL09         					0x1e4
#define SYS_PAD_CTRL0A         					0x1e8
#define SYS_PAD_CTRL0B         					0x1ec
#define SYS_PAD_CTRL0C         					0x1f0


#define CVBS_PAL		            0
#define CVBS_NTSC		            1

#define BLOCK_HEIGHT                32
#define ITU656_MAX_FRAME            4
#define PAL_WIDTH	                720
#define PAL_HEIGHT	                288
#define NTSC_WIDTH	                720
#define NTSC_HEIGHT	                240
#define CVBS_WIDTH 	                PAL_WIDTH
#define CVBS_HEIGHT                 PAL_HEIGHT

#define DEINTERLACE_MAX_FRAME       8
#define DEINTERLACE_WIDTH           720
#define DEINTERLACE_HEIGHT          576
#define DVR_MAJOR                   243
#define DEBUG

#define ITU656_FRAME_NUM			4
#define ITU656_BUFFER_EMPTY			0x0 	// buffer is readed by lcd/tv, ITU656 can write
#define ITU656_BUFFER_FULL_LCD		0x1 	// buffer is writed ok by itu656. lcd can read
#define ITU656_BUFFER_FULL_TV 		0x10 	//  buffer is writed ok by itu656. tv can read
#define START_DISCARD_FRAME			6		// must larger than ITU656_FRAME_NUM

#define ITU656_STATIC_FRAME_SIZE

#if defined(CONFIG_BOARD_TYPE_HJSQ) || defined(CONFIG_BOARD_TYPE_N141_PUBLIC)
#define ITU656_MAX_WIDTH			1280
#define ITU656_MAX_HEIGHT			720
#define ITU656_FRAME_SIZE			(ITU656_MAX_WIDTH*ITU656_MAX_HEIGHT*3/2)
#elif defined(ITU656_STATIC_FRAME_SIZE)
#define ITU656_MAX_WIDTH			1920
#define ITU656_MAX_HEIGHT			720//1080
#define ITU656_FRAME_SIZE			(ITU656_MAX_WIDTH*ITU656_MAX_HEIGHT*3/2)
#else
#define ITU656_FIELD_SIZE           (CVBS_WIDTH*CVBS_HEIGHT*3/2)
#define ITU656_FRAME_SIZE           (ITU656_FIELD_SIZE*2)
#endif

#if ((ITU656_MAX_WIDTH == 1920) && (ITU656_MAX_HEIGHT == 1080))
#undef ITU656_FRAME_NUM
#define ITU656_FRAME_NUM			3	//frame buffer is not enouth, must <= 3.
#endif

#define ARK_DVR_IOC_MAGIC  'n'

#define ARK_DVR_START				_IO(ARK_DVR_IOC_MAGIC,  1)
#define ARK_DVR_STOP				_IO(ARK_DVR_IOC_MAGIC,  2)
#define ARK_DVR_INIT				_IOW(ARK_DVR_IOC_MAGIC, 3, struct itu656in_para)
#define ARK_DVR_GET_BUFFER_INFO		_IOW(ARK_DVR_IOC_MAGIC, 4, struct itu656_framebuf_para)
#define ARK_DVR_GET_BUFFER_READY    _IOR(ARK_DVR_IOC_MAGIC, 5, struct itu656_framebuf_addr)
#define ARK_DVR_SET_BUFFER_FREE		_IOW(ARK_DVR_IOC_MAGIC, 6, struct itu656_framebuf_addr)

//#define DEBUG

#ifdef DEBUG
#define itu656_printk(...) printk(KERN_ALERT __VA_ARGS__)
#define itu656_ERROR(fmt, arg...) printk("<0>[ERROR][%s.%d]--" fmt "--\n", __func__, __LINE__, ##arg)
#define itu656_INFO(fmt, arg...) //printk("<0>[INFO][%s.%d]--" fmt "--\n", __func__, __LINE__, ##arg)

#else
#define itu656_printk(fmt, ...)
#define itu656_ERROR(fmt, ...) printk(KERN_ALERT "[ERROR][%s.%d]" fmt "\n", __func__, __LINE__, __VA_ARGS__)
#define itu656_INFO(fmt, ...)
#endif

#define ITU656_USE_DEINTERLACE


struct itu656in_para{
    int system;
    int itu601in;
    int	width;
    int	height;
    int	left_cut;
    int right_cut;
    int	up_cut;
    int	down_cut;
    int interlace;
};

enum itu656_framebuf_status{
    FRAMEBUF_STATUS_FREE,
    FRAMEBUF_STATUS_BUSY,
    FRAMEBUF_STATUS_READY,
};

struct itu656_framebuf_addr {
	unsigned int yaddr;
	unsigned int uvaddr;
};

struct itu656_framebuf_id {
	int id;
	struct list_head list;
};

struct ark_itu656in_context {
    int itu656_irq;
    int deinterlace_irq;
    struct device *dev;
    void __iomem *itu656_base;
    void __iomem *sys_base;
    void __iomem *deinterlace_base;

    spinlock_t spin_lock;

    int work_status;
    int discard_frame;
    int deinter_status;
    int pprev_frame;
    int prev_frame;
    int cur_frame;
    
    u8 *buffer_virtaddr;
    unsigned int buffer_size;
    unsigned int buffer_phyaddr;
    struct itu656_framebuf_addr framebuf_phyaddr[ITU656_FRAME_NUM];
    struct itu656_framebuf_id framebuf_id[ITU656_FRAME_NUM];
    struct list_head framebuf_push_list;
    unsigned int framebuf_status[ITU656_FRAME_NUM];
    unsigned int framebuf_num;
    unsigned int frame_finish_count;
    char frame_finish[ITU656_FRAME_NUM];
	int itu_channel;
    
    struct itu656in_para itu656in;   
};

struct dvr_dev{
    const char *driver_name;
    const char *name;
    int major;
    int minor_start;
    int minor_num;
    int num;
    struct cdev cdev;
    struct class *itu656_class;
    struct device *itu656_device;

    wait_queue_head_t frame_finish_waitq;
    struct fasync_struct *fasync_queue;
    struct timer_list timer;
    
	struct ark_itu656in_context context;
    
    void (*start)(struct ark_itu656in_context *context);
    void (*stop)(struct ark_itu656in_context *context);
};


struct itu656_framebuf_para {
	int num;
	struct itu656_framebuf_addr buf[ITU656_FRAME_NUM];
};



void dvr_start(struct ark_itu656in_context *context);
void dvr_stop(struct ark_itu656in_context *context);
irqreturn_t ark_deinterlace_int_handler(int irq, void *dev_id);
irqreturn_t ark_itu656_int_handler(int irq, void *dev_id);
void dither_timeout_timer(struct timer_list *t);


#endif

