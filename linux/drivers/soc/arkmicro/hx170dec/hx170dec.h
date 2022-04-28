/*
 * Decoder device driver (kernel module headers)
 *
 * Copyright (C) 2009  Hantro Products Oy.
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _HX170DEC_H_
#define _HX170DEC_H_

#include <linux/ioctl.h>
#include <linux/types.h>

struct core_desc
{
	__u32 id;    /* id of the core */
	__u32 *regs; /* pointer to user registers */
	__u32 size;  /* size of register space */
};

/* Use 'k' as magic number */
#define HX170DEC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HX170DEC_IOCGHWOFFSET		_IOR(HX170DEC_IOC_MAGIC, 3, unsigned long *)
#define HX170DEC_IOCGHWIOSIZE		_IOR(HX170DEC_IOC_MAGIC, 4, unsigned int *)

#define HX170DEC_IOC_MC_OFFSETS		_IOR(HX170DEC_IOC_MAGIC, 7, unsigned long *)
#define HX170DEC_IOC_MC_CORES		_IOR(HX170DEC_IOC_MAGIC, 8, unsigned int *)
#define HX170DEC_IOCS_DEC_PUSH_REG	_IOW(HX170DEC_IOC_MAGIC, 9, struct core_desc *)
#define HX170DEC_IOCS_PP_PUSH_REG	_IOW(HX170DEC_IOC_MAGIC, 10, struct core_desc *)
#define HX170DEC_IOCH_DEC_RESERVE	_IO(HX170DEC_IOC_MAGIC, 11)
#define HX170DEC_IOCT_DEC_RELEASE	_IO(HX170DEC_IOC_MAGIC, 12)
#define HX170DEC_IOCQ_PP_RESERVE	_IO(HX170DEC_IOC_MAGIC, 13)
#define HX170DEC_IOCT_PP_RELEASE	_IO(HX170DEC_IOC_MAGIC, 14)
#define HX170DEC_IOCX_DEC_WAIT		_IOWR(HX170DEC_IOC_MAGIC, 15, struct core_desc *)
#define HX170DEC_IOCX_PP_WAIT		_IOWR(HX170DEC_IOC_MAGIC, 16, struct core_desc *)
#define HX170DEC_IOCS_DEC_PULL_REG	_IOWR(HX170DEC_IOC_MAGIC, 17, struct core_desc *)
#define HX170DEC_IOCS_PP_PULL_REG	_IOWR(HX170DEC_IOC_MAGIC, 18, struct core_desc *)

#define HX170DEC_IOX_ASIC_ID		_IOWR(HX170DEC_IOC_MAGIC, 20, __u32 *)

/*
 * Following are not used yet:
 *
 * #define HX170DEC_PP_INSTANCE		_IO(HX170DEC_IOC_MAGIC, 1)
 * #define HX170DEC_HW_PERFORMANCE	_IO(HX170DEC_IOC_MAGIC, 2)
 * #define HX170DEC_IOC_CLI		_IO(HX170DEC_IOC_MAGIC, 5)
 * #define HX170DEC_IOC_STI		_IO(HX170DEC_IOC_MAGIC, 6)
 * #define HX170DEC_IOCG_CORE_WAIT	_IOR(HX170DEC_IOC_MAGIC, 19, int *)
 * #define HX170DEC_DEBUG_STATUS	_IO(HX170DEC_IOC_MAGIC, 29)
 */

#define HX170DEC_IOC_MAXNR 29
//mfc 中jpeg解码
struct mfc_jpeg_context {
	struct device *dev;
	unsigned int intr_status;
	unsigned int file_size;
	unsigned int src_width;
	unsigned int src_height;
	unsigned int out_width;
	unsigned int out_height;
	unsigned int format;
	int read_buf_mode;
	int anmation_stats;
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
	unsigned int animation_data_size;
	unsigned int animation_display_phyaddr;
	unsigned int animation_display_virtaddr;
	unsigned int animation_display_size;
	unsigned int animation_display_index;
	unsigned int animation_file_phyaddr;	
	unsigned int animation_file_virtaddr;
    struct work_struct animation_work;
    struct workqueue_struct *animation_queue;
	
	struct timer_list animation_timer;
	bool animation_end;
	bool animation_dec_finish;
	bool animation_initdisplay;
};



#define VDEC_MAX_CORES                 1 /* number of cores of the hardware IP */
#define VDEC_NUM_REGS_DEC             60 /* number of registers of the Decoder part */
#define VDEC_NUM_REGS_PP              41 /* number of registers of the Post Processor part */
#define VDEC_DEC_FIRST_REG             0 /* first register (0-based) index */
#define VDEC_DEC_LAST_REG             59 /* last register (0-based) index */
#define VDEC_PP_FIRST_REG             60
#define VDEC_PP_LAST_REG             100

struct vdec_device {
	void __iomem *mmio_base;
	struct clk *clk;
	struct device *dev;
	int irq;
	int num_cores;
	unsigned long iobaseaddr;
	unsigned long iosize;
	wait_queue_head_t dec_wq;
	wait_queue_head_t pp_wq;
	bool dec_irq_done;
	bool pp_irq_done;
	struct semaphore dec_sem;
	struct semaphore pp_sem;
	struct file *dec_owner;
	struct file *pp_owner;
	u32 regs[VDEC_NUM_REGS_DEC + VDEC_NUM_REGS_PP];
	struct mfc_jpeg_context context;
};



#endif /* !_HX170DEC_H_ */
