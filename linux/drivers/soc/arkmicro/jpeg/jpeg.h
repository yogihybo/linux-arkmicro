/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Name:
 *      ark_jpeg.h
 *
 * Description:
 *
 * 
 * Author:
 *       Sim
 * Remarks:
 *
 */

#ifndef __ARK_JPEG_H__
#define __ARK_JPEG_H__

/*************************************************************************
 * Driver information definitions
 *************************************************************************/
#define ARK_JPEG_MAJOR_RELEASE              0
#define ARK_JPEG_MINOR_RELEASE              0
#define ARK_JPEG_BUILD                      1
#define ARK_JPEG_VERSION_CODE ((ARK_JPEG_MAJOR_RELEASE << 16) | \
    (ARK_JPEG_MINOR_RELEASE << 8) | ARK_JPEG_BUILD)

//#define ARK_JPEG_DBG
/*************************************************************************
 * Debug print functions
 *************************************************************************/
#ifdef ARK_JPEG_DBG
#define ARKJPEG_DBGPRTK(...) printk(KERN_ALERT __VA_ARGS__)
#else
#define ARKJPEG_DBGPRTK(...)
#endif

/*************************************************************************
 * register definitions
 *************************************************************************/
#define ARK_JPEG_REG_BASE    JPEG_BASE
#define ARK_JPEG_REG_SIZE    0x1000

/*************************************************************************
 * driver control structures
 *************************************************************************/
#define JPEG_API_TIMEOUT		3000
#define JPEG_TIMEOUT			30000

#define JPEG_BLOCK_HEIGHT		32
#define JPEG_MAX_HEADERSIZE		100*1024
//#define JPEG_FILE_BUFFERSIZE  4
#define JPEG_FILE_BUFFERSIZE	200
#define JPEG_INPUT_BUFFERSIZE 	JPEG_FILE_BUFFERSIZE*1024

#define MAXIMAGEWIDTH			8000
#define MAXIMAGEHEIGHT			8000

/* JPEG Registers */
#define JPEG_0                      0x00
#define JPEG_1                      0x04
#define JPEG_2                      0x08
#define JPEG_3                      0x0C
#define JPEG_4                      0x10
#define JPEG_5                      0x14
#define JPEG_6                      0x18
#define JPEG_7                      0x1C
#define JPEG_STATUS                 0x20
#define JPEG_WRSTA                  0x24
#define JPEG_WREND                  0x28
#define JPEG_CTRL                   0x2c
#define JPEG_START                  0x30
#define JPEG_INTCTRL                0x34
#define JPEG_INTMASK                0x38
#define JPEG_INTCLR                 0x3c
#define JPEG_LINE_NUM               0x40
#define JPEG_FIFO                   0x44
#define JPEG_WRSTA1                 0x48
#define JPEG_WREND1                 0x4c
#define JPEG_COUNT                  0x50
#define JPEG_WRSTA2                 0x54
#define JPEG_WREND2                 0x58
#define JPEG_TEST                   0x88
#define JPEG_DEC_RD_BASE_ADDR       0x5c
#define JPEG_ENC_RD_BASE_ADDR       0x60
#define JPEG_ENC_WR_BASE_ADDR       0x64

#define JPEG_QT                     0x100
#define JPEG_SYMB                   0x500
#define JPEG_BASE                   0xe00
#define JPEG_MIN                    0xf00


/* PRE-SCALER */
#define PRESCALE_SRC_ADDR                  0x00
#define PRESCALE_DES_ADDR                  0x04
#define PRESCALE_HV_ADDR1                  0x08
#define PRESCALE_HV_ADDR2                  0x0c
#define PRESCALE_HV_ADDR3                  0x10
#define PRESCALE_COEF_H                    0x14
#define PRESCALE_COEF_V                    0x18
#define PRESCALE_H_FILTER_COEF_3TO0        0x1c
#define PRESCALE_H_FILTER_COEF_7TO4        0x20
#define PRESCALE_H_FILTER_COEF_10TO8       0x24
#define PRESCALE_V_FILTER_COEF_3TO0        0x28
#define PRESCALE_V_FILTER_COEF_7TO4        0x2c
#define PRESCALE_V_FILTER_COEF_10TO8       0x30
#define PRESCALE_FILTER_CTL                0x34
#define PRESCALE_LINE_CTL                  0x38
#define PRESCALE_HEIGHT_CTL                0x3c
#define PRESCALE_CTL                       0x40
#define PRESCALE_BLK_HEIGHT                0x44
#define PRESCALE_BLK_NUM                   0x48
#define PRESCALE_ROW_NUM_LAST_BLK          0x4c
#define PRESCALE_FRAME_START               0x50
#define PRESCALE_BLK_START                 0x54
#define PRESCALE_INT_STATUS                0x58
#define PRESCALE_INT_CLR                   0x5c
#define PRESCALE_HDATA_VALID               0x60
#define PRESCALE_WORK_MODE                 0x64
#define PRESCALE_HALF_LINE                 0x68
#define PRESCALE_HDATA_VALID_VIDEO         0x6c
#define PRESCALE_VDATA_VALID_VIDEO         0x70
#define PRESCALE_SHIFT_CTL                 0x74
#define PRESCALE_SCREEN_KIND               0x78
#define PRESCALE_EVEN_ODD_MODE             0x7c
#define PRESCALE_DES_ADDR2                 0x80
#define PRESCALE_SRC_MODE                  0x84
#define PRESCALE_FILTER3_CTL               0x88
#define PRESCALE_COS                       0x8c
#define PRESCALE_WIN_POS                   0x90
#define PRESCALE_WIN_CTL                   0x94
#define PRESCALE_REFRESH                   0x98
#define PRESCALE_INT_MASK                  0x9C

struct ark_jpeg_context {
	int irq;
	int psirq;
	spinlock_t lock;
	wait_queue_head_t waitq;
	struct device *dev;
	void __iomem *mmio_base;
	void __iomem *ps_mmio_base;

	struct completion decstart_completion;
	struct completion api_completion;
	struct completion apidone_completion;
	struct completion decdone_completion;
	struct workqueue_struct *wrok_queue;
	struct work_struct jpeg_work;

	unsigned int intr_status;

	unsigned int file_size;

	bool break_decode;
	bool hard_head_parser;
	bool scaler_enable;
	JPEG_DEC_RESULT decode_result;
	unsigned int decode_size;
	unsigned int src_width;
	unsigned int src_height;
	unsigned int out_width;
	unsigned int out_height;
	unsigned int format;
	int read_buf_mode;
	JPEG_DEC_STATUS decode_status;
	JPEG_SCALER_MODE scaler_mode;
	JPEG_ZOOM_MODE zoom_mode;
	ROTATE_ANGLE rotate_angle;
	JPEG_API_INFO api_info;
	JPEG_API_RETINFO api_retinfo;
	unsigned int dst_width;
	unsigned int dst_height;
	void *buf_base_virt;
	unsigned int buf_base_phys;
	unsigned int buf_size;
	void *decode_buf_base_virt;
	unsigned int decode_buf_base_phys;
	unsigned int decode_buf_size;
	unsigned int repeat_scaler;
	unsigned int repeat_src_width;
	unsigned int repeat_src_height;
	unsigned int repeat_src_image_addr;
	unsigned int animation_data_phyaddr;
	unsigned int animation_data_virtaddr;
	unsigned int animation_display_phyaddr;
	unsigned int animation_display_virtaddr;
	unsigned int animation_display_size;
	unsigned int animation_display_index;
	unsigned int animation_file_phyaddr;
	struct timer_list animation_timer;
	bool animation_end;
	bool animation_dec_finish;
	bool animation_initdisplay;
	struct completion psblockint_completion;
	struct completion psframeint_completion;
};

struct ark_jpeg_device {
	const char *driver_name;
	const char *name;
	int major;
	int minor_start;
	int minor_num;
	int num;
	struct cdev cdev;
	struct class *jpeg_class;
	struct device *jpeg_device;
	struct ark_jpeg_context context;
};

#endif //#ifndef __ARK_JPEG_H__
