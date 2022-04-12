// =============================================================================
// File        : Gem_isp.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#ifndef  _GEM_ISP_H_
#define  _GEM_ISP_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define  DEBUG_PRINT   printk
#define MAX_YUV_BUF_N   4

#define	INFO_STA_USE_INTR			// "曝光状态信息完成"使用中断方式

//ISP 时钟 相位：0 ：正沿采样， 1：下沿采样
#define  SENSOR_OUTPUT_CLKPOLAY           0 
#define  SENSOR_INPUT_CLKPOLAY            0


typedef struct isp_param_ 
{
	unsigned int image_width;
	unsigned int image_height;
	unsigned int image_stride;
	unsigned int ref_addr;
	unsigned int raw_addr[4];
	unsigned int y_addr[4];
	unsigned int u_addr[4];
	unsigned int v_addr[4];
	unsigned int yuv_id[4];
} isp_param_t; 

typedef struct isp_param_ *isp_param_ptr_t;

int isp_videobuf_alloc (isp_param_ptr_t p_isp);
void get_isp_dispaddr(unsigned int order,unsigned int *Yaddr, unsigned int *Uaddr , unsigned int *Vaddr );
void get_isp_rawaddr(unsigned int order,unsigned int *Yaddr  );


#define	ISP_EVENT_INFO_DONE			0x01		// 曝光统计信息已准备
#define	ISP_EVENT_AE_STOP				0x02		// 自动曝光结束
#define	ISP_EVENT_ABNORMAL			0x04		// ISP异常
#define	ISP_EVENT_MODE_CHANGE		0x08		// 工作模式修改
#define	ISP_EVENT_TICKET				0x10		// ISP定时器事件

void isp_ae_done_event_set (void);

enum {
	ISP_WORK_MODE_NORMAL	= 0x00,
	ISP_WORK_MODE_RAW,
	ISP_WORK_MODE_AUTOTEST,
	ISP_WORK_MODE_COUNT
};

unsigned int isp_get_work_mode (void);
int isp_set_work_mode (unsigned work_mode);

extern void isp_sensor_set_reset_pin_low (void);
extern void isp_sensor_set_reset_pin_high (void);

extern void isp_sensor_set_standby_pin_low (void);
extern void isp_sensor_set_standby_pin_high (void);




#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _GEM_ISP_H_
