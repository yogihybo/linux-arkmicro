#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include <linux/clk.h>

#include "ark_camera.h"
#include "Gem_isp.h"
#include "Gem_isp_sensor.h"
#include "Gem_isp_sys.h"
#include "Gem_isp_colors.h"
#include "Gem_isp_denoise.h"
#include "Gem_isp_eris.h"
#include "Gem_isp_fesp.h"
#include "Gem_isp_enhance.h"
#include "Gem_isp_awb.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_io.h"
#include "ark_isp_exposure.h"
#include "ark_isp_exposure_cmos.h"

extern void isp_auto_exposure_set_iso (cmos_exposure_t *p_isp_exposure, unsigned int iso);
static void cmos_isp_denoise_run (int inttime_gain, isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae);
static void cmos_isp_eris_run (int inttime_gain, isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae);
static void cmos_isp_fesp_run (int inttime_gain, isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);
static void cmos_isp_crosstalk_run (int inttime_gain, isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);
static void cmos_isp_enhance_run (int inttime_gain, isp_enhance_ptr_t p_enhance);
static void cmos_isp_ae_run (int inttime_gain, isp_ae_ptr_t p_ae);
static void cmos_isp_colors_run (int inttime_gain, isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae);
static void cmos_isp_sharp_run (int inttime_gain, isp_enhance_ptr_t p_enhance);
static void cmos_isp_gamma_run (int inttime_gain, isp_colors_ptr_t p_colors);
static void cmos_isp_awb_run (int inttime_gain, isp_awb_ptr_t p_awb);			// ??????????????
#define	ISP_IDLE		0
#define	ISP_RUN		1

struct ark_camera_context *camera_context = NULL;

static volatile unsigned int isp_state = 0;
static volatile unsigned int isp_stop = 0;
#ifdef ISP_RAW_ENABLE
static volatile unsigned int isp_mode = ISP_WORK_MODE_RAW;
#else
static volatile unsigned int isp_mode = ISP_WORK_MODE_NORMAL;
#endif

#ifdef USE_ISR_TO_SWITCH_MODE
volatile unsigned int new_isp_mode = (unsigned int)(-1);
#endif

isp_param_t g_isp_param;
isp_sys_t p_sys;
isp_sen_t p_sen;
isp_colors_t p_colors;
isp_denoise_t p_denoise;
isp_eris_t p_eris;
isp_fesp_t p_fesp;
isp_enhance_t p_enhance;
isp_awb_t p_awb;
isp_ae_t p_ae;
cmos_exposure_t isp_exposure;

static unsigned int isp_auto_run_state = (unsigned int)0xFFFFFFFF;

void isp_sensor_set_reset_pin_low(void)
{
	gpiod_set_value(camera_context->sensor_reset, 0);
}

void isp_sensor_set_reset_pin_high(void)
{
	gpiod_set_value(camera_context->sensor_reset, 1);
}

#define SYS_AHB_CLK_EN	0x44
#define SYS_PER_CLK_EN	0x50
static void isp_clock_disable (struct ark_camera_context *context)
{
	u32 val;

	//sensor mclk out
	val = readl(context->sysbase + SYS_PER_CLK_EN);
	val &= ~(1 << 29);
	writel(val, context->sysbase + SYS_PER_CLK_EN);

	//isp clk
	val = readl(context->sysbase + SYS_AHB_CLK_EN);
	val &= ~((1 << 26) | (1 << 25) | (1 << 12));
	writel(val, context->sysbase + SYS_AHB_CLK_EN);
}

static void isp_clock_enable (struct ark_camera_context *context)
{
	u32 val;

	//sensor mclk out
	val = readl(context->sysbase + SYS_PER_CLK_EN);
	val |= (1 << 29);
	writel(val, context->sysbase + SYS_PER_CLK_EN);

	//isp clk
	val = readl(context->sysbase + SYS_AHB_CLK_EN);
	val |= (1 << 26) | (1 << 25) | (1 << 12);
	writel(val, context->sysbase + SYS_AHB_CLK_EN);
}

#define SYS_SOFT_RSTNB		0x78
void isp_softreset(struct ark_camera_context *context)
{
	u32 val = readl(context->sysbase + SYS_SOFT_RSTNB);
	val &= ~(1 << 11);
	writel(val, context->sysbase + SYS_SOFT_RSTNB);
	udelay(10);
	val |= (1 << 11);
	writel(val, context->sysbase + SYS_SOFT_RSTNB);
}

int isp_videobuf_alloc (isp_param_ptr_t p_isp)
{
  	unsigned int u_offset, v_offset, i, *refb;
	unsigned int video_format = isp_get_video_format ();

	p_isp->image_width = IMAGE_H_SZ ;
	p_isp->image_height = IMAGE_V_SZ ;
	p_isp->image_stride = (p_isp->image_width+0xf)&0xfff0;


	// Y_UV420/YUV420
	if(video_format == 0 || video_format == 2)
	{
		u_offset = p_isp->image_stride*p_isp->image_height;
		v_offset = (u_offset) + (u_offset>>2);
	}
	// Y_UV422/YUV422
	else
	{
		u_offset = p_isp->image_stride*p_isp->image_height;
		v_offset = (u_offset) + (u_offset>>1);

	}

#if ISP_3D_DENOISE_SUPPORT
	unsigned int ref_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*2));
	if ( !ref_virt  ) {
		printk("alloc ref buf fail.\n");
		return -ENOMEM;
	}
	p_isp->ref_addr = virt_to_phys((void *)ref_virt);
#else
	p_isp->ref_addr = 0;
#endif

#ifdef ISP_RAW_ENABLE
	unsigned int raw_buf0_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*2));
	if ( !raw_buf0_virt  ) {
		printk("alloc raw buf0 fail.\n");
		return -ENOMEM;
	}
	unsigned int raw_buf1_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*2));
	if ( !raw_buf1_virt  ) {
		printk("alloc raw buf1 fail.\n");
		return -ENOMEM;
	}
	unsigned int raw_buf2_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*2));
	if ( !raw_buf2_virt  ) {
		printk("alloc raw buf2 fail.\n");
		return -ENOMEM;
	}
	unsigned int raw_buf3_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*2));
	if ( !raw_buf3_virt  ) {
		printk("alloc raw buf3 fail.\n");
		return -ENOMEM;
	}
	p_isp->raw_addr[0] = virt_to_phys((void *)raw_buf0_virt);
	p_isp->raw_addr[1] = virt_to_phys((void *)raw_buf1_virt);
	p_isp->raw_addr[2] = virt_to_phys((void *)raw_buf2_virt);
	p_isp->raw_addr[3] = virt_to_phys((void *)raw_buf3_virt);
#else
	p_isp->raw_addr[0] = (unsigned int)(0);
	p_isp->raw_addr[1] = (unsigned int)(0);
	p_isp->raw_addr[2] = (unsigned int)(0);
	p_isp->raw_addr[3] = (unsigned int)(0);

#endif
   //XM_printf("raw_addr[0]:0x%08x\n",p_isp->raw_addr[0]);
   //XM_printf("raw_addr[1]:0x%08x\n",p_isp->raw_addr[1]);
   //XM_printf("raw_addr[2]:0x%08x\n",p_isp->raw_addr[2]);
   //XM_printf("raw_addr[3]:0x%08x\n",p_isp->raw_addr[3]);
	unsigned int yuv_buf0_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*3/2));
	if ( !yuv_buf0_virt  ) {
		printk("alloc yuv buf0 fail.\n");
		return -ENOMEM;
	}
	p_isp->y_addr[0] = virt_to_phys((void *)yuv_buf0_virt);
	p_isp->u_addr[0] = p_isp->y_addr[0] + u_offset;
	p_isp->v_addr[0] = p_isp->y_addr[0] + v_offset;
	//XM_printf("yuv_buf0:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[0],p_isp->u_addr[0],p_isp->v_addr[0]);

	unsigned int yuv_buf1_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*3/2));
	if ( !yuv_buf1_virt  ) {
		printk("alloc yuv buf1 fail.\n");
		return -ENOMEM;
	}
	p_isp->y_addr[1] = virt_to_phys((void *)yuv_buf1_virt);
	p_isp->u_addr[1] = p_isp->y_addr[1] + u_offset;
	p_isp->v_addr[1] = p_isp->y_addr[1] + v_offset;
	//XM_printf("yuv_buf1:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[1],p_isp->u_addr[1],p_isp->v_addr[1]);

	unsigned int yuv_buf2_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*3/2));
	if ( !yuv_buf2_virt  ) {
		printk("alloc yuv buf2 fail.\n");
		return -ENOMEM;
	}
	p_isp->y_addr[2] = virt_to_phys((void *)yuv_buf2_virt);
	p_isp->u_addr[2] = p_isp->y_addr[2] + u_offset;
	p_isp->v_addr[2] = p_isp->y_addr[2] + v_offset;
	//XM_printf("yuv_buf2:0x%08x 0x%08x 0x%08x\n",p_isp->y_addr[2],p_isp->u_addr[2],p_isp->v_addr[2]);

	unsigned int yuv_buf3_virt = __get_free_pages(GFP_KERNEL, get_order(IMAGE_H_SZ*IMAGE_V_SZ*3/2));
	if ( !yuv_buf3_virt  ) {
		printk("alloc yuv buf3 fail.\n");
		return -ENOMEM;
	}
	p_isp->y_addr[3] = virt_to_phys((void *)yuv_buf3_virt);
	p_isp->u_addr[3] = p_isp->y_addr[3] + u_offset;
	p_isp->v_addr[3] = p_isp->y_addr[3] + v_offset;

	p_isp->yuv_id[0] = 0;
	p_isp->yuv_id[1] = 1;
	p_isp->yuv_id[2] = 2;
	p_isp->yuv_id[3] = 3;
}

unsigned int isp_get_auto_run_state  (unsigned int item)
{
	if(item >= ISP_AUTO_RUN_COUNT)
		return 0;
	return isp_auto_run_state & (1 << item);
}


static const char *isp_work_mode_name[] = {
	"Normal",
	"Raw",
	"AutoTest",
	"Null"
};

static const char *isp_get_work_mode_name (unsigned int mode)
{
	if(mode >= ISP_WORK_MODE_COUNT)
		return isp_work_mode_name[ISP_WORK_MODE_COUNT];
	else
		return isp_work_mode_name[mode];
}

unsigned int isp_get_work_mode (void)
{
	return isp_mode;
}

int isp_set_work_mode (unsigned int mode)
{
#ifndef ISP_RAW_ENABLE
	if(mode == ISP_WORK_MODE_RAW)
	{
		XM_printf ("current version don't support RAW\n");
		return -1;		// ???RAWß’????
	}
#endif

	if(mode >= ISP_WORK_MODE_COUNT)
		return -1;

	if(isp_mode == mode)
		return 0;

	if(isp_state == ISP_IDLE)
	{
		return -1;
	}

#ifdef USE_ISR_TO_SWITCH_MODE
	isp_mode = mode;
	new_isp_mode = isp_mode;
	mdelay (100); // ???????3?
	if(new_isp_mode == (unsigned int)(-1))
	{
		XM_printf ("isp mode switch to (%s) OK\n", isp_get_work_mode_name(isp_mode) );
	}
	else
	{
		XM_printf ("isp mode switch to (%s) NG\n", isp_get_work_mode_name(isp_mode) );
	}
#else
	isp_mode = mode;
#endif
	return 0;
}

static unsigned int eris_dimlight_enable = 1;

void cmos_exposure_set_eris_dimlight (unsigned int enable)
{
	eris_dimlight_enable = enable;
}

unsigned int cmos_exposure_get_eris_dimlight (void)
{
	return eris_dimlight_enable;
}

static unsigned char raw_sensor_exist = 0;		// ???????????? 1 ???? 0 ????

unsigned char XMSYS_IspGetRawSensorState (void)
{
	return raw_sensor_exist;
}

int xm_arkn141_isp_set_flicker_freq  (int flicker_freq)
{
	return isp_cmos_set_flicker_freq (&isp_exposure, flicker_freq);
}

// ?®ø??????
// Bug 20170913 ?ç}????????????????????
//   ????????????, ??????????????? 2(???????3.20) ~ 15(???????8.00) ???»Œ, ????????????????®ø???(Low-->4  ~  High-->14)?ß›????.
//   ???????Bug?????????ß≥???????( Low-->3, High-->16 ), ???????????ß›??????
//#define	DIM_LUM_HIGH			(14)
//#define	DIM_LUM_LOW				(4)
#define	DIM_LUM_HIGH			(16)
#define	DIM_LUM_LOW				(2)

#define	DIM_LIGHT_COUNT		25	// 1.5??

static unsigned int dim_light_lum_count;
static unsigned int dim_light_lum_index;
static unsigned short dim_light_lum_value[DIM_LIGHT_COUNT];
static void dim_light_mode_init (void)
{
	dim_light_lum_index = 0;
	dim_light_lum_count = 0;
	memset (dim_light_lum_value, 0, sizeof(dim_light_lum_value));
	(void)(dim_light_lum_index);
	(void)(dim_light_lum_count);

}

#define	BACK_LIGHT_COUNT	15
// Back Light ????????
static int backlight_max_lum[BACK_LIGHT_COUNT];
static int backlight_ground_lum[BACK_LIGHT_COUNT];
static unsigned int backlight_lum_count;
static unsigned int backlight_lum_index;
static unsigned int backlight_ae_enable;

static void back_light_mode_init (void)
{
	backlight_lum_index = 0;
	backlight_lum_count = 0;
	backlight_ae_enable = 1;
	memset (backlight_max_lum, 0, sizeof(backlight_max_lum));
	memset (backlight_ground_lum, 0, sizeof(backlight_ground_lum));
}

void camera_work(struct work_struct *work)
{
	struct ark_camera_context *context = container_of(work, struct ark_camera_context, camera_work);
	struct ark_camera_device *carback = container_of(context, struct ark_camera_device, context);
	unsigned int inttime_gain = -1, new_inttime_gain;

#if 1
	// ?2???????????. ????????????????????????????, ???????????????????????????.
	static unsigned int frame_index = 0;
	frame_index ++;
	if(frame_index & 1)
		return;
#endif

	// ????????
	//back_light_mode_monitor (inttime_gain);

	// ?®ø??????
	//dim_light_mode_monitor (inttime_gain);

	///XMINT64 		XM_GetHighResolutionTickCount (void);
	//XMINT64 t = XM_GetHighResolutionTickCount();
	isp_ae_run (&p_ae);
	//t = XM_GetHighResolutionTickCount () - t;
	//XM_printf ("exp_t=%d\n", (unsigned int)t);

	//new_inttime_gain = (isp_exposure.cmos_inttime.exposure_ashort * isp_exposure.cmos_gain.again) / (1 << isp_exposure.cmos_gain.again_shift);
	new_inttime_gain = cmos_calc_inttime_gain (&isp_exposure);

	isp_auto_exposure_set_iso (&isp_exposure, new_inttime_gain);

#if XMSYS_INCLUDE_MOTION_DETECTOR
	xm_motion_detector_set_pixel_threshold (new_inttime_gain);
#endif

	if(new_inttime_gain != inttime_gain)
	{
		inttime_gain = new_inttime_gain;

		if(isp_get_auto_run_state(ISP_AUTO_RUN_AWB))
		{
			cmos_isp_awb_run (inttime_gain, &p_awb);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_AE))
		{
			cmos_isp_ae_run (inttime_gain, &p_ae);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_FESP))
		{
			cmos_isp_fesp_run(new_inttime_gain, &p_fesp, &p_ae);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_CROSSTALK))
		{
			cmos_isp_crosstalk_run(new_inttime_gain, &p_fesp, &p_ae);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_ERIS))
		{
			cmos_isp_eris_run(new_inttime_gain, &p_eris, &p_ae);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_COLOR))
		{
			cmos_isp_colors_run (new_inttime_gain, &p_colors, &p_awb, &p_ae);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_GAMMA))
		{
			cmos_isp_gamma_run (new_inttime_gain, &p_colors);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_ENHANCE))
		{
			cmos_isp_enhance_run (new_inttime_gain, &p_enhance);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_SHARP))
		{
			cmos_isp_sharp_run (new_inttime_gain, &p_enhance);
		}

		if(isp_get_auto_run_state(ISP_AUTO_RUN_DENOISE))
		{
			cmos_isp_denoise_run (new_inttime_gain, &p_denoise, &p_ae);
		}
	}
}

int ark_camera_dev_init(struct ark_camera_device *camera)
{
	printk(KERN_ALERT "%s %d.\n", __FUNCTION__, __LINE__);
	struct ark_camera_context *context = &camera->context;

	camera_context = context;

	isp_clock_disable(context);
	isp_softreset(context);
	gpiod_set_value(context->sensor_reset, 0);
	isp_clock_enable(context);
	gpiod_set_value(context->sensor_reset, 1);
	isp_videobuf_alloc (&g_isp_param);
	arkn141_isp_ae_initialize (&isp_exposure);

	cmos_sensor_t	*cmos_sensor = &isp_exposure.cmos_sensor;

#if 0
	// ?????????????ISP????
	unsigned int sharpening_value = AP_GetMenuItem(APPMENUITEM_SHARPENING);
	if(sharpening_value == 0)
	{
		p_enhance.sharp.strength = 32;		// ???
	}
	else if(sharpening_value == 1)
	{
		p_enhance.sharp.strength = 64;		// ???
	}
	else if(sharpening_value == 2)
	{
		p_enhance.sharp.strength = 160;	// ???
	}
#endif

	// ?????sensor
	if((*cmos_sensor->cmos_isp_sensor_init) (&p_sen) < 0)
	{
		// sensor seems bad or no response
		// ????sensor???????
		//XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CCD0_LOST_CONNECT );
		raw_sensor_exist = 0;
		//goto sensor_no_response;
	}
	else
	{
		raw_sensor_exist = 1;
	}

	if(cmos_sensor->cmos_isp_colors_init)
	{
		//	DEBUG_PRINT ("cmos_isp_colors_init\n");
		cmos_sensor->cmos_isp_colors_init(&p_colors);
	}
	else
	{
		XM_printf ("Warning: Miss cmos_isp_colors_init\n");
	}

	if(cmos_sensor->cmos_isp_denoise_init)
	{
		//DEBUG_PRINT ("cmos_isp_denoise_init\n");
		cmos_sensor->cmos_isp_denoise_init (&p_denoise);
	}
	else
	{
		XM_printf ("Warning: Miss cmos_isp_denoise_init\n");
	}

	if(cmos_sensor->cmos_isp_eris_init)
	{
		//DEBUG_PRINT ("cmos_isp_eris_init\n");
		cmos_sensor->cmos_isp_eris_init (&p_eris);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_eris_init\n");

	if(cmos_sensor->cmos_isp_fesp_init)
	{
		//DEBUG_PRINT ("cmos_isp_fesp_init\n");
		cmos_sensor->cmos_isp_fesp_init (&p_fesp);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_fesp_init\n");

	if(cmos_sensor->cmos_isp_enhance_init)
	{
		//DEBUG_PRINT ("cmos_isp_enhance_init\n");
		cmos_sensor->cmos_isp_enhance_init (&p_enhance);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_enhance_init\n");

	if(cmos_sensor->cmos_isp_awb_init)
	{
		//DEBUG_PRINT ("cmos_isp_awb_init\n");
		cmos_sensor->cmos_isp_awb_init (&p_awb);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_awb_init\n");

	if(cmos_sensor->cmos_isp_ae_init)
	{
		//DEBUG_PRINT ("cmos_isp_ae_init\n");
		cmos_sensor->cmos_isp_ae_init (&p_ae);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_ae_init\n");

	if(cmos_sensor->cmos_isp_sys_init)
	{
		//DEBUG_PRINT("run isp_sys_init()\n");
		cmos_sensor->cmos_isp_sys_init (&p_sys, &g_isp_param);
	}
	else
		XM_printf ("Warning: Miss cmos_isp_sys_init\n");

	//xm_arkn141_isp_set_flicker_freq (50);

	//OS_Delay (150);
	// ISP???????????ISP
    isp_sys_init_io (&p_sys);

	//frame_ticket = 0;

	dim_light_mode_init ();

	back_light_mode_init ();

	spin_lock_init(&context->lock);

	init_waitqueue_head(&context->frame_finish_waitq);

	context->camera_queue = create_singlethread_workqueue("camera_queue");
	if(!context->camera_queue) {
    	printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);
		return -1;
	}
	INIT_WORK(&context->camera_work, camera_work);

	return 0;
}

#define ISP_SCALE_EN     		0
#define	ISP_SCALAR_RUN			writel(0x03, camera_context->scalebase + ISP_SCALE_EN);

static int do_isp_scalar = 0;

void isp_scalar_run(void)
{
	do_isp_scalar = 1;
}

static unsigned int framebufferno=0;


unsigned int get_isp_framebufferno(void)
{
	return framebufferno;
}

static int isp_pixel_clock_abnormal_count = 0;

irqreturn_t ark_camera_intr_handler(int irq, void *dev_id)
{
	struct ark_camera_device *camera = (struct ark_camera_device *)dev_id;
	struct ark_camera_context *context = &camera->context;
	unsigned int frame_set, frame_clr;
	int frame_id = -1;
	unsigned int int_status ;
	unsigned int line, val;
	unsigned int flags;
	int i;

   int_status  = Gem_read(GEM_STS_BASE+0x00);

   // bit.0 ? ??? bit.0:?????ßÿ????
	// frame start interrupt
   if(int_status & 0x01 )
   {
		// write 1 at bit[7:0] means frame start sync interrupt clear
      Gem_write ((GEM_MSK_BASE+0x00), 0x01);
   }

   // bit.1  ????  bit.0:?????ßÿ????
	// sensor data use pixel clk is full (pixel fast than coreclk), sensor FIFO?????????????????, FIFO?????????????????, ????FIFO???.
	// Pixel Clock???????????, ??????????????????????. ???????, ????FIFO????????? Pixel clock abnormal
   if(int_status & (0x01<<1) )
   {
      // ???????????
      // write 2 at bit[7:0] means pixel clock abnormal interrupt clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x02);
      //isp_debug_printf ("isp pixel clock abnormal\n\n");
		isp_pixel_clock_abnormal_count ++;
		// ????????????????????¶ÀISP
		if(isp_pixel_clock_abnormal_count >= 4)
		//if(isp_pixel_clock_abnormal_count >= 8)
		{
#ifdef ISP_RAW_ENABLE
			static unsigned int abnormal_count = 0;
			if(abnormal_count % 15 == 0)
			{
				isp_debug_printf ("isp pixel clock abnormal error\n\n");
			}
			abnormal_count ++;
#else
			isp_debug_printf ("isp pixel clock abnormal error, isp disabled\n\n");
			writel(readl(context->base + ISP_SYS) & ~1, context->base + ISP_SYS);	// µ»¥˝ISP∏¥Œª
			p_sys.isp_reset_request = 1;

			//«øæ≤µÁµº÷¬ispÕ£÷ππ§◊˜£¨÷ÿ∆Ùisp
			isp_disable();//ISP STOP
			msleep(50);
			isp_enable();//ISP START

			// ÂÖ≥Èó≠Êï∞ÊçÆÁÇπÂºÇÂ∏∏‰∏≠Êñ≠ËØ∑Ê±ÇÔºåÈÅøÂÖçÂèØËÉΩÁöÑËøûÁª≠‰∏≠Êñ≠ËØ∑Ê±Ç(Êï∞ÊçÆÁÇπÂºÇÂ∏∏ÂèØËÉΩ‰ºöÊåÅÁª≠Âà∞Ë°åÊ∂àÈöêÊúü)ÂØºËá¥CPUÊåá‰ª§Êó†Ê≥ïÊâßË°å
			// πÿ±’ ˝æ›µ„“Ï≥£÷–∂œ«Î«Û£¨±‹√‚ø…ƒ‹µƒ¡¨–¯÷–∂œ«Î«Û( ˝æ›µ„“Ï≥£ø…ƒ‹ª·≥÷–¯µΩ––œ˚“˛∆⁄)µº÷¬CPU÷∏¡ÓŒﬁ∑®÷¥––
			unsigned int data3 = Gem_read (GEM_SYS_BASE+0x0c);
			data3 &= ~(1 << 1);
			Gem_write (GEM_SYS_BASE+0x0c, data3);
#endif
		}
   }

   // bit.2:?????ßÿ?
	// frame real finish interrupt
   if( int_status & (0x01<<2) ) // frame finish int
	{
		isp_pixel_clock_abnormal_count = 0;

		// ????ID
      	frame_id = (int_status >> 8) & 0x3;		// frame id number
		framebufferno = frame_id;
		// Clear frame finished status
		frame_clr = 0x04;
		Gem_write ((GEM_MSK_BASE+0x00), frame_clr);

		spin_lock_irqsave(&context->lock, flags);
		for (i = 0; i < context->frame_finish_count; i++) {
			if (frame_id == context->frame_finish[i]) {
				printk("frame %d is already poped.\n");
				frame_id = -1;
				break;
			}
		}
		spin_unlock_irqrestore(&context->lock, flags);

#ifdef USE_ISR_TO_SWITCH_MODE
		if(new_isp_mode != (unsigned int)(-1))
		{
			unsigned int data0;

#if ISP_3D_DENOISE_SUPPORT
			if(new_isp_mode == ISP_WORK_MODE_NORMAL)	// ???????????3D
				p_denoise.enable3d = 7;
			else
				p_denoise.enable3d = 0;
			data0 =  ((p_denoise.enable2d & 0x07) <<  0)
					| ((p_denoise.enable3d & 0x07) <<  3)
					| ((p_denoise.sel_3d_table & 0x03) << 8)	// 3D????????
					| ((p_denoise.sensitiv0 & 0x07) << 10)	// 2D?????0(???)?????????, 0 ??????
					| ((p_denoise.sensitiv1 & 0x07) << 13)	// 2D?????1(????)?????????, 0 ??????
					| ((p_denoise.sel_3d_matrix & 0x01) << 16)		// 3D??????????????? 0 ??? 1 ?????
					;
			Gem_write ((GEM_DENOISE_BASE+0x00), data0);
#endif
			//isp_denoise_init_io (&p_denoise);
			if(new_isp_mode == ISP_WORK_MODE_NORMAL)	// ???????????3D
			{
				p_sys.debugmode  = 0;
				p_sys.testenable = 0; // ????dram??????
				p_sys.rawmenable = 0; // 1 ????RAWß’??
				p_sys.yuvenable  = 1; // 0:??????????  1:??
#if ISP_3D_DENOISE_SUPPORT
				p_sys.refenable  = 1; // 1;3D ?¶œ?????? 0:???
#else
				p_sys.refenable  = 0; // 1;3D ?¶œ?????? 0:???
#endif
			}
			else
			{
				// RAW
				p_sys.debugmode  = 1;
				p_sys.testenable = 0; // ????dram??????
				p_sys.rawmenable = 1; // 1 ????RAWß’??
				p_sys.yuvenable  = 1; // 0:??????????  1:??
				p_sys.refenable  = 0; // 1;3D ?¶œ?????? 0:???
			}
			//cmos_sensor->cmos_isp_sys_init (&p_sys, &g_isp_param);
		  	data0 = (p_sys.debugmode)
					| (p_sys.testenable << 1)
					| (p_sys.rawmenable << 2)
					| (p_sys.yuvenable << 3)
					| (p_sys.refenable << 4)
					| (p_sys.yuvformat << 5)
					| (p_sys.dmalock << 7)
					| (p_sys.hstride << 16)
					;
			Gem_write ((GEM_SYS_BASE+0x14), data0);
			new_isp_mode = (unsigned int)(-1);
		}
#endif

		//XM_printf ("pop %d\n", frame_id);
		// 20181103 ???ßÿ?¶ƒ?????????????ßÿ?????
		if(!(Gem_read(GEM_SYS_BASE+0x0c) & (1 << 5)))
		{
			// ???YUV?????????
			if(Gem_read(GEM_SYS_BASE+0x14) & (1 << 3))	// yuvenable
			{
				if (frame_id >= 0) {
					spin_lock_irqsave(&context->lock, flags);
					if (context->frame_finish_count < ISP_FRAME_NUM - 1) {
						context->frame_finish[context->frame_finish_count++] = frame_id;
						//poll
						wake_up_interruptible(&context->frame_finish_waitq);

						//async
						if(camera->async_queue_cam != NULL) {
							//printk(KERN_ALERT "kill_fasync camera frame finish.\n");
							kill_fasync(&camera->async_queue_cam, SIGIO, POLL_IN);
						}
					} else {
						//printk("frame finish fast, drop this frame.\n");
						isp_sys_set_frame_ready(frame_id);
					}
					spin_unlock_irqrestore(&context->lock, flags);
				}
			}
		}

		if(do_isp_scalar)
		{
			do_isp_scalar = 0;
			ISP_SCALAR_RUN;
		}

		extern void isp_gamma_adjust(void);
		isp_gamma_adjust();

#ifdef ISP_ADJUST_CM_SUPPORT
		extern void isp_cm_adjust (void);
		isp_cm_adjust ();
#endif
	}

   // bit.3:????? ????? ??????? ????0
	// ????????????, ?ßÿ????????????????????????????. ???????
	// address abnormal interrupt (overflow or zero abnormal)
   if( int_status  & (0x01<<3) )
   {
		// write 5 at bit[7:0] means address abnoraml interrupt clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x05);
      //isp_debug_printf ("isp address abnormal!\n\n");

		//isp_dump_sys_register ();
   }

   // bit.4:???????????3D???????????????????????
	// bus bandwidth abnormal (means 3d denoise can't sync ,can't capture time)
   if( int_status  & (0x01<<4) )
   {
		isp_debug_printf ("isp bus bandwidth abnormal!\n\n");
		Gem_write ((GEM_MSK_BASE+0x00),  0x06);

		// dma memory to memory ?????, ???????bus lock, ?????????????.
		//  ??????????????, ???????????????bus lock
		if(1)
		//if(reset_index > 1)
		{
			//reset_index = 0;
			// write 6 at bit[7:0] means bus transfer error interrrupt clear
			//Gem_write ((GEM_MSK_BASE+0x00),  0x06);
			//rISP_SYS &= ~1;

			// ISP??¶œ??????????????????CPU??????????????
			// Bus????????????¶œ???????????
			// disable 3D's reference frame
			unsigned int data0 = Gem_read (GEM_SYS_BASE+0x14);
			data0 &= ~(1 << 4);
			Gem_write (GEM_SYS_BASE+0x14, data0);


			//isp_debug_printf ("bus bandwidth abnormal!!\n\n\n");

			// ???bus???ßÿ?????
			unsigned int data3 = Gem_read (GEM_SYS_BASE+0x0c);
			data3 &= ~(1 << 4);
			Gem_write (GEM_SYS_BASE+0x0c, data3);

			p_sys.isp_reset_request = 1;		// ????ISP??¶À
		}
   }

   // bit.5
   // 16?????¶À????ßÿ? fast_int = (isp_status >> 5) & 0x01
	// slice interrupt(use 16 per step)
   if( int_status  & (0x01<<5) )
   {
		// bit16-bit31 means the line number should finish in this fiq interrupt
      	line = (int_status>>16);
		(void)line;
		// write 8 at bit[7:0] means fast interrupt clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x08);
		//isp_debug_printf ("isp slice int=%d!!\n\n\n", line);
		if (frame_id >= 0) {
			spin_lock_irqsave(&context->lock, flags);
			if (context->frame_finish_count < ISP_FRAME_NUM - 1) {
				context->frame_finish[context->frame_finish_count++] = frame_id;
				//poll
				wake_up_interruptible(&context->frame_finish_waitq);

				//async
				if(camera->async_queue_cam != NULL) {
					//printk(KERN_ALERT "kill_fasync camera frame finish.\n");
					kill_fasync(&camera->async_queue_cam, SIGIO, POLL_IN);
				}
			} else {
				printk("frame finish fast, drop this frame.\n");
				isp_sys_set_frame_ready(frame_id);
			}
			spin_unlock_irqrestore(&context->lock, flags);
		}
   }

   // ?????ßÿ? bit.6:??? ?ßÿ?
	// suspend interrupt(means termination)
   if(  int_status  & (0x01<<6) )
   {
      // write 9 at bit[7:0] means suspend interrupt clear
      Gem_write ((GEM_MSK_BASE+0x00), 0x09);
      isp_debug_printf ("isp suspend interrupt!\n\n\n");
   }

	// ??????????ßÿ?
	if( int_status & (0x01 << 7) )
	{
		unsigned int data0;

		isp_pixel_clock_abnormal_count = 0;

		// ????????????????????
#ifdef UPDATE_AWB_BLACK_USE_ISR
		data0 	= ((p_awb.enable  & 0x01) <<  0)
					| ((p_awb.mode    & 0x03) <<  1) 	// bit1-bit2     mode
					// 0: unite gray white average
					//	1: unite color temperature average
					//	2: zone color temperature Weight
					| ((p_awb.manual  & 0x01) <<  3) 	// bit3  manual, 0: auto awb  1: mannual awb
					| ((p_awb.black   & 0xFF) <<  8)
					| ((p_awb.white   & 0xFF) << 16)
					;
		Gem_write ((GEM_AWB0_BASE+0x00), data0);

#endif

		// write 7 at bit[7:0] means frame information sync bit (after ae) clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x07);
		p_sys.infoStadone = 1;
		//isp_debug_printf ("infodone interrupt!\n\n\n");

		data0 = Gem_read (GEM_AE1_BASE+0x04);
		isp_histogram_bands_data (data0, Gem_read (GEM_AE1_BASE+0x08));

		//isp_ae_yavg_s_read (&p_ae);
		isp_ae_info_read (&p_ae);
		isp_ae_sts2_read (&p_ae);	// ????9??????
		isp_awb_info_read (&p_awb);
		//isp_ae_done_event_set ();
		queue_work(context->camera_queue, &context->camera_work);
	}


    return IRQ_HANDLED;
}

void denoise_match_inttime_gain (int inttime_gain, isp_denoise_inttime_polyline_tbl *denoise_tbl,
											const isp_denoise_inttime_polyline_tbl *lp_denoise_inttime_polyline_tbl,
											int cb_denoise_inttime_polyline_tbl)
{
	int i;
	int val;
	const isp_denoise_inttime_polyline_tbl *lo, *hi;
	int count = cb_denoise_inttime_polyline_tbl;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= lp_denoise_inttime_polyline_tbl[i].inttime_gain)
			break;
	}

	// ???
	if(i == count)
	{
		memcpy (denoise_tbl, &lp_denoise_inttime_polyline_tbl[count - 1], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}
	else if(inttime_gain == lp_denoise_inttime_polyline_tbl[i].inttime_gain)
	{
		memcpy (denoise_tbl, &lp_denoise_inttime_polyline_tbl[i], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}
	// ???
	else if(inttime_gain < lp_denoise_inttime_polyline_tbl[0].inttime_gain)
	{
		memcpy (denoise_tbl, &lp_denoise_inttime_polyline_tbl[0], sizeof(isp_denoise_inttime_polyline_tbl));
		return;
	}

	lo = &lp_denoise_inttime_polyline_tbl[i - 1];
	hi = &lp_denoise_inttime_polyline_tbl[i];
	val = (lo->y_thres0 + (hi->y_thres0 - lo->y_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres0 = val;
	val = (lo->u_thres0 + (hi->u_thres0 - lo->u_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres0 = val;
	val = (lo->v_thres0 + (hi->v_thres0 - lo->v_thres0) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres0 = val;
	val = (lo->y_thres1 + (hi->y_thres1 - lo->y_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres1 = val;
	val = (lo->u_thres1 + (hi->u_thres1 - lo->u_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres1 = val;
	val = (lo->v_thres1 + (hi->v_thres1 - lo->v_thres1) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres1 = val;

	val = (lo->y_thres2 + (hi->y_thres2 - lo->y_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->y_thres2 = val;
	val = (lo->u_thres2 + (hi->u_thres2 - lo->u_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->u_thres2 = val;
	val = (lo->v_thres2 + (hi->v_thres2 - lo->v_thres2) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	if(val < 0)
		val = 0;
	denoise_tbl->v_thres2 = val;
}

static void cmos_isp_awb_run (int inttime_gain, isp_awb_ptr_t p_awb)			// ??????????????
{
	if(isp_exposure.cmos_sensor.cmos_isp_awb_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_awb_run) (inttime_gain, p_awb);
		// XM_printf ("\r\np_awb->gain_r2g = 0x%08x\n", p_awb->gain_r2g);
		// XM_printf ("\r\np_awb->gain_b2g = 0x%08x\n", p_awb->gain_b2g);
		return;
	}
}

static void cmos_isp_denoise_run (int inttime_gain, isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae)
{
	unsigned int data0, data1, data2, data3;
	isp_denoise_inttime_polyline_tbl denoise_inttime_tbl;
	unsigned int sensitiv0, sensitiv1;
	int tbl_count = 0;
	const isp_denoise_inttime_polyline_tbl *tbl = NULL;

	if(!isp_get_auto_run_state(ISP_AUTO_RUN_DENOISE))
		return;

	// ?????ßÿ???de-noise??????????
	if(isp_exposure.cmos_sensor.cmos_isp_denoise_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_denoise_run)(inttime_gain, p_denoise, p_ae);
		return;
	}

	// ?????????????
	if(isp_exposure.cmos_sensor.cmos_isp_get_denoise_table)
	{
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_denoise_table) (&tbl_count);
	}
	if(tbl == NULL || tbl_count == 0)
		return;

	denoise_match_inttime_gain ((int)inttime_gain, &denoise_inttime_tbl, tbl, tbl_count);

	// ???3D??UV???????
	//denoise_inttime_tbl.u_thres2 = 0;
	//denoise_inttime_tbl.v_thres2 = 0;

	data1 = (denoise_inttime_tbl.y_thres0 <<  0)
			| (denoise_inttime_tbl.u_thres0 << 10)
			| (denoise_inttime_tbl.v_thres0 << 20);
	data2 = (denoise_inttime_tbl.y_thres1 <<  0)
			| (denoise_inttime_tbl.u_thres1 << 10)
			| (denoise_inttime_tbl.v_thres1 << 20);
	data3 = (denoise_inttime_tbl.y_thres2 <<  0)
			| (denoise_inttime_tbl.u_thres2 << 10)
			| (denoise_inttime_tbl.v_thres2 << 20);




	// ?????????
	p_denoise->y_thres0 = (data1 >>  0) & 0x3ff;
	p_denoise->u_thres0 = (data1 >> 10) & 0x3ff;
	p_denoise->v_thres0 = (data1 >> 20) & 0x3ff;
	p_denoise->y_thres1 = (data2 >>  0) & 0x3ff;
	p_denoise->u_thres1 = (data2 >> 10) & 0x3ff;
	p_denoise->v_thres1 = (data2 >> 20) & 0x3ff;
	p_denoise->y_thres2 = (data3 >>  0) & 0x3ff;
	p_denoise->u_thres2 = (data3 >> 10) & 0x3ff;
	p_denoise->v_thres2 = (data3 >> 20) & 0x3ff;


	Gem_write ((GEM_DENOISE_BASE+0x04), data1);
	Gem_write ((GEM_DENOISE_BASE+0x08), data2);
	Gem_write ((GEM_DENOISE_BASE+0x0c), data3);

	sensitiv0 = p_denoise->sensitiv0;
	sensitiv1 = p_denoise->sensitiv1;

	p_denoise->sensitiv0 = sensitiv0;
	p_denoise->sensitiv1 = sensitiv1;

	data0 =  ((p_denoise->enable2d & 0x07) <<  0)
			| ((p_denoise->enable3d & 0x07) <<  3)
			| ((p_denoise->sel_3d_table & 0x03) << 8)	// 3D????????
			| ((p_denoise->sensitiv0 & 0x07) << 10)	// 2D?????0(???)?????????, 0 ??????
			| ((p_denoise->sensitiv1 & 0x07) << 13)	// 2D?????1(????)?????????, 0 ??????
			| ((p_denoise->sel_3d_matrix & 0x01) << 16)		// 3D??????????????? 0 ??? 1 ?????
			;
	Gem_write ((GEM_DENOISE_BASE+0x00), data0);

}

static unsigned int default_resolt[33] = {
 200,  210,  220,  225,  225,  225,
 225,  225,  225,  225,  225,  225,
 225,  225,  225,  225,  225,  230,
 230,  230,  230,  230,  230,  230,
 230,  230,  230,  230,  230,  225,
 220,  215,  210,
};

static unsigned int default_colort[33] = {
   64,     128,    192,    256,    320,    384,    511,    511,
   511,    511,    511,    511,    511,    511,    511,    511,
   511,    511,    511,    511,    511,    511,    511,    511,
   511,    511,    511,    511,    448,    384,    256,    192,
   128
};

static void match_resolt_colort (int inttime_gain,
								  unsigned char resolt[33],
								  unsigned short colort[33],
								  int *gain_max_data,
								  const isp_eris_polyline_tbl *eris_polyline_tbl,
								  int count
								  )
{
	int i;
	int bright;
	const isp_eris_polyline_tbl *lo, *hi;
	int resolt_ratio, colort_ratio;
	int gain_max;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= eris_polyline_tbl[i].inttime_gain)
			break;
	}
	// ???
	if(i == count)
	{
		resolt_ratio = eris_polyline_tbl[count - 1].resolt_ratio;
		colort_ratio = eris_polyline_tbl[count - 1].colort_ratio;
		gain_max = eris_polyline_tbl[count - 1].gain_max;
	}
	else if(inttime_gain == eris_polyline_tbl[i].inttime_gain)
	{
		resolt_ratio = eris_polyline_tbl[i].resolt_ratio;
		colort_ratio = eris_polyline_tbl[i].colort_ratio;
		gain_max = eris_polyline_tbl[i].gain_max;
	}
	// ???
	else if(inttime_gain < eris_polyline_tbl[0].inttime_gain)
	{
		resolt_ratio = eris_polyline_tbl[0].resolt_ratio;
		colort_ratio = eris_polyline_tbl[0].colort_ratio;
		gain_max = eris_polyline_tbl[0].gain_max;
	}
	else
	{
		lo = &eris_polyline_tbl[i - 1];
		hi = &eris_polyline_tbl[i];
		resolt_ratio = lo->resolt_ratio + (hi->resolt_ratio - lo->resolt_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		colort_ratio = lo->colort_ratio + (hi->colort_ratio - lo->colort_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		gain_max = lo->gain_max + (hi->gain_max - lo->gain_max) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
	}

	if(resolt_ratio < 0)
		resolt_ratio = 0;
	if(colort_ratio < 0)
		colort_ratio = 0;

	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (default_resolt[i] * resolt_ratio) >> 9;
		if(val >= 230)
			val = 230;
		resolt[i] = (unsigned char)val;
	}

	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (default_colort[i] * colort_ratio) >> 9;
		if(val >= 511)
			val = 511;
		colort[i] = (unsigned short)val;
	}

	if(gain_max < 4)
		gain_max = 4;
	else if(gain_max > 512)
		gain_max = 512;
	*gain_max_data = gain_max;
}



static void match_man_resolt_colort (int inttime_gain,
								  unsigned char resolt[33],
								  unsigned short colort[33],
								  int *gain_man_data,
								  int *cont_man_data,
								  const isp_eris_man_polyline_tbl *eris_man_polyline_tbl,
								  int count
									  )
{
	int i;
	int bright;
	const isp_eris_man_polyline_tbl *lo, *hi;
	int resolt_ratio, colort_ratio;
	int gain_man, cont_man;

	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= eris_man_polyline_tbl[i].inttime_gain)
			break;
	}
	// ???
	if(i == count)
	{
		resolt_ratio = eris_man_polyline_tbl[count - 1].resolt_ratio;
		colort_ratio = eris_man_polyline_tbl[count - 1].colort_ratio;
		gain_man = eris_man_polyline_tbl[count - 1].gain_man;
		cont_man = eris_man_polyline_tbl[count - 1].cont_man;
	}
	else if(inttime_gain == eris_man_polyline_tbl[i].inttime_gain)
	{
		resolt_ratio = eris_man_polyline_tbl[i].resolt_ratio;
		colort_ratio = eris_man_polyline_tbl[i].colort_ratio;
		gain_man = eris_man_polyline_tbl[i].gain_man;
		cont_man = eris_man_polyline_tbl[i].cont_man;
	}
	// ???
	else if(inttime_gain < eris_man_polyline_tbl[0].inttime_gain)
	{
		resolt_ratio = eris_man_polyline_tbl[0].resolt_ratio;
		colort_ratio = eris_man_polyline_tbl[0].colort_ratio;
		gain_man = eris_man_polyline_tbl[0].gain_man;
		cont_man = eris_man_polyline_tbl[0].cont_man;
	}
	else
	{
		lo = &eris_man_polyline_tbl[i - 1];
		hi = &eris_man_polyline_tbl[i];
		resolt_ratio = lo->resolt_ratio + (hi->resolt_ratio - lo->resolt_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		colort_ratio = lo->colort_ratio + (hi->colort_ratio - lo->colort_ratio) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		gain_man = lo->gain_man + (hi->gain_man - lo->gain_man) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
		cont_man = lo->cont_man + (hi->cont_man - lo->cont_man) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain);
	}

	if(resolt_ratio < 0)
		resolt_ratio = 0;
	if(colort_ratio < 0)
		colort_ratio = 0;

	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (default_resolt[i] * resolt_ratio) >> 9;
		if(val >= 230)
			val = 230;
		resolt[i] = (unsigned char)val;
	}

	for (i = 0; i < 33; i ++)
	{
		unsigned int val = (default_colort[i] * colort_ratio) >> 9;
		if(val >= 511)
			val = 511;
		colort[i] = (unsigned short)val;
	}

	if(gain_man < 4)
		gain_man = 4;
	else if(gain_man > 512)
		gain_man = 512;
	*gain_man_data = gain_man;
	if(cont_man < 16)
		cont_man = 16;
	else if(cont_man > 256)
		cont_man = 256;
	*cont_man_data = cont_man;
}

//#define	BACKLIGHT_COMP	1
// ???????????(???????????????°§, ???),
// 	???°§??????????????????????, ???????, ????????????????.???????????gain??????, ????????, ???????????????????(??????????).
//		??????????????????????, ???????ßÿ????????????????????????.
// ???????????????
// 	ratio = 9???????????????? / (3?????????(20+21+22)???????) ?gain???????
//		ratio > 2??????????, ?????????????????, ????????
//
static void cmos_isp_eris_run (int inttime_gain, isp_eris_ptr_t p_eris ,isp_ae_ptr_t p_ae)
{
	int i;
	unsigned int data0, data2, data4;

	int gain_max;
	int gain_man, cont_man;

	if(isp_exposure.cmos_sensor.cmos_isp_eris_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_eris_run)(inttime_gain, p_eris, p_ae);
		return;
	}

	if(p_eris->manual)
	{
		// ?????
		const isp_eris_man_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		if(isp_exposure.cmos_sensor.cmos_isp_get_eris_man_table)
		{
			tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_eris_man_table) (&tbl_count);
		}

		if(tbl == NULL || tbl_count == 0)
			return;

		// ????????, ???????????, ????????????????????
		match_man_resolt_colort (inttime_gain, p_eris->resolt, p_eris->colort, &gain_man, &cont_man, tbl, tbl_count);
		for (i = 0; i < 33; i++)
		{
			data0 = (0x02) | (i << 8) | (p_eris->resolt[i]<<16);
			Gem_write ((GEM_LUT_BASE+0x00), data0);
		}

		for (i = 0; i < 33; i++)
		{
			data0 = (0x03) | (i << 8) | (p_eris->colort[i]<<16);
			Gem_write ((GEM_LUT_BASE+0x00), data0);
		}


		p_eris->gain_man = gain_man;
		p_eris->cont_man = cont_man;
		data4 = (p_eris->gain_man) | (p_eris->cont_man << 16);
		Gem_write ((GEM_ERIS_BASE+0x10), data4);
	}
	else
	{
		// ?????
		const isp_eris_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		if(isp_exposure.cmos_sensor.cmos_isp_get_eris_auto_table)
		{
			tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_eris_auto_table) (&tbl_count);
		}

		if(tbl == NULL || tbl_count == 0)
			return;

		match_resolt_colort (inttime_gain, p_eris->resolt, p_eris->colort, &gain_max, tbl, tbl_count);
		for (i = 0; i < 33; i++)
		{
			data0 = (0x02) | (i << 8) | (p_eris->resolt[i]<<16);
			Gem_write ((GEM_LUT_BASE+0x00), data0);
		}

		for (i = 0; i < 33; i++)
		{
			data0 = (0x03) | (i << 8) | (p_eris->colort[i]<<16);
			Gem_write ((GEM_LUT_BASE+0x00), data0);
		}

		p_eris->gain_max = gain_max;
		p_eris->gain_min = gain_max;
		data2 = (p_eris->gain_max) | (p_eris->gain_min << 16);
		Gem_write ((GEM_ERIS_BASE+0x08), data2);
	}
}




static void lsc_match (int inttime_gain, isp_lsc_polyline_tbl *lsc_tbl,
									  const isp_lsc_polyline_tbl *table,
									  int count
									)
{
	int i;
	int lscoff;
	const isp_lsc_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= table[i].inttime_gain)
			break;
	}
	if(i == count)
	{
		lscoff = table[count - 1].lscoff;
	}
	// ???
	else if(inttime_gain == table[i].inttime_gain)
	{
		lscoff = table[i].lscoff;
	}
	// ???
	else if(inttime_gain < table[0].inttime_gain)
	{
		lscoff =  table[0].lscoff;
	}
	else
	{
		lo = &table[i - 1];
		hi = &table[i];
		lscoff = (int)(lo->lscoff + (hi->lscoff - lo->lscoff) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}

	if(lscoff > 250)
		lscoff = 250;
	else if(lscoff < 50)
		lscoff = 50;

	lsc_tbl->lscoff = lscoff;
}

static void fpn_match (int inttime_gain, isp_fpn_polyline_tbl *fpn_tbl,
									  const isp_fpn_polyline_tbl *table,
									  int count
									)
{
	int i;
	int rBlacklevel;
	int grBlacklevel;
	int gbBlacklevel;
	int bBlacklevel;

	const isp_fpn_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= table[i].inttime_gain)
			break;
	}
	if(i == count)
	{
		rBlacklevel = table[count - 1].rBlacklevel;
		grBlacklevel = table[count - 1].grBlacklevel;
		gbBlacklevel = table[count - 1].gbBlacklevel;
		bBlacklevel = table[count - 1].bBlacklevel;
	}
	// ???
	else if(inttime_gain == table[i].inttime_gain)
	{
		rBlacklevel = table[i].rBlacklevel;
		grBlacklevel = table[i].grBlacklevel;
		gbBlacklevel = table[i].gbBlacklevel;
		bBlacklevel = table[i].bBlacklevel;
	}
	// ???
	else if(inttime_gain < table[0].inttime_gain)
	{
		rBlacklevel =  table[0].rBlacklevel;
		grBlacklevel =  table[0].grBlacklevel;
		gbBlacklevel =  table[0].gbBlacklevel;
		bBlacklevel =  table[0].bBlacklevel;
	}
	else
	{
		lo = &table[i - 1];
		hi = &table[i];
		rBlacklevel = (int)(lo->rBlacklevel + (hi->rBlacklevel - lo->rBlacklevel) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
		grBlacklevel = (int)(lo->grBlacklevel + (hi->grBlacklevel - lo->grBlacklevel) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
		gbBlacklevel = (int)(lo->gbBlacklevel + (hi->gbBlacklevel - lo->gbBlacklevel) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
		bBlacklevel = (int)(lo->bBlacklevel + (hi->bBlacklevel - lo->bBlacklevel) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}

	if(rBlacklevel > 512)
		rBlacklevel = 512;
	else if(rBlacklevel < 0)
		rBlacklevel = 0;

	fpn_tbl->rBlacklevel = rBlacklevel;

	if(grBlacklevel > 512)
		grBlacklevel = 512;
	else if(grBlacklevel < 0)
		grBlacklevel = 0;

	fpn_tbl->grBlacklevel = grBlacklevel;

	if(gbBlacklevel > 512)
		gbBlacklevel = 512;
	else if(gbBlacklevel < 0)
		gbBlacklevel = 0;

	fpn_tbl->gbBlacklevel = gbBlacklevel;

	if(bBlacklevel > 512)
		bBlacklevel = 512;
	else if(bBlacklevel < 0)
		bBlacklevel = 0;

	fpn_tbl->bBlacklevel = bBlacklevel;
}

static void cmos_isp_fesp_run (int inttime_gain, isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae)
{
	// CrossTalk
	unsigned int data0, data1, data2;
	int rBlacklevel;
	int grBlacklevel;
	int gbBlacklevel;
	int bBlacklevel;
	isp_lsc_polyline_tbl lsc_tbl;
	isp_fpn_polyline_tbl fpn_tbl;

	if(isp_exposure.cmos_sensor.cmos_isp_fesp_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_fesp_run)(inttime_gain, p_fesp, p_ae);
		return;
	}

	int lscoff;
	if(isp_exposure.cmos_sensor.cmos_isp_get_fpn_table)
	{
		const isp_fpn_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_fpn_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			fpn_match (inttime_gain, &fpn_tbl, tbl, tbl_count);
			rBlacklevel = fpn_tbl.rBlacklevel;
			grBlacklevel = fpn_tbl.grBlacklevel;
			gbBlacklevel = fpn_tbl.gbBlacklevel;
			bBlacklevel = fpn_tbl.bBlacklevel;
			p_fesp->Fixpatt.rBlacklevel = rBlacklevel;
			p_fesp->Fixpatt.grBlacklevel = grBlacklevel;
			p_fesp->Fixpatt.gbBlacklevel = gbBlacklevel;
			p_fesp->Fixpatt.bBlacklevel = bBlacklevel;
			data1	= ((p_fesp->Fixpatt.rBlacklevel  & 0xFFFF) <<  0)
					| ((p_fesp->Fixpatt.grBlacklevel & 0xFFFF) << 16);
			data2	= ((p_fesp->Fixpatt.gbBlacklevel & 0xFFFF) <<  0)
					| ((p_fesp->Fixpatt.bBlacklevel  & 0xFFFF) << 16);
			Gem_write ((GEM_FIXPATT_BASE+0x04), data1);
			Gem_write ((GEM_FIXPATT_BASE+0x08), data2);
		}
	}

	// lens shading correct
	if(isp_exposure.cmos_sensor.cmos_isp_get_lsc_table)
	{
		const isp_lsc_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_lsc_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			// Lense Shading correct
			// ???????????????, ????????????????ßµ????????????, ???????????????ßµ??????.
			// ????????lsc????????????, ???????????????????????, ??ß≥ßµ???????????, ????????
			// ??????????
			lsc_match (inttime_gain, &lsc_tbl, tbl, tbl_count);
			lscoff = lsc_tbl.lscoff;
			p_fesp->Lensshade.lscofst = lscoff;
			// bit15-0        lscofst
			data0 = (p_fesp->Lensshade.lscofst & 0xFFFF);
			Gem_write ((GEM_LENS_LSCOFST_BASE+0x00), data0);
		}
	}
}

static void crosstalk_match (int inttime_gain, isp_crosstalk_polyline_tbl *crosstalk_tbl,
									  const isp_crosstalk_polyline_tbl *table,
									  int count)
{
	int i;
	int thres;
	const isp_crosstalk_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= table[i].inttime_gain)
			break;
	}
	if(i == count)
	{
		thres = table[count - 1].thres;
	}
	// ???
	else if(inttime_gain == table[i].inttime_gain)
	{
		thres = table[i].thres;
	}
	// ???
	else if(inttime_gain < table[0].inttime_gain)
	{
		thres =  table[0].thres;
	}
	else
	{
		lo = &table[i - 1];
		hi = &table[i];
		thres = (int)(lo->thres + (hi->thres - lo->thres) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}

	if(thres > 36)
		thres = 36;
	else if(thres < 3)
		thres = 3;

	crosstalk_tbl->thres = thres;
}

static void cmos_isp_crosstalk_run (int inttime_gain, isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae)
{
	// CrossTalk
	unsigned int data0, data1;
	isp_crosstalk_polyline_tbl crosstalk_tbl;

	int thres;

	// ???????????ßÿ?????????
	if(isp_exposure.cmos_sensor.cmos_isp_crosstalk_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_crosstalk_run)(inttime_gain, p_fesp, p_ae);
		return;
	}

	// ??????????
	// crosstalk de-noise
	if(isp_exposure.cmos_sensor.cmos_isp_get_crosstalk_table)
	{
		const isp_crosstalk_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_crosstalk_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			crosstalk_match (inttime_gain, &crosstalk_tbl, tbl, tbl_count);
			thres = crosstalk_tbl.thres;

			p_fesp->Crosstalk.thresh = thres;
			p_fesp->Crosstalk.thres1cgf = thres;
			p_fesp->Crosstalk.thres0cgf = thres;
			data0 = ((p_fesp->Crosstalk.enable    & 0x0001) << 0 ) 	// bit0 crosstalk enable (1: enable 0:disable)
					| ((p_fesp->Crosstalk.mode      & 0x0003) << 1 )	// bit1-bit2 crosstalk mode  (00: unite filter thres=128 10: use reg thres x1: base on lut)
					| ((p_fesp->Crosstalk.snsCgf    & 0x0003) << 3 )	// bit3-bit4 snsCgf
					| ((p_fesp->Crosstalk.thres0cgf & 0xFFFF) << 16)	// bit16-bit31 thres0cgf
					;
			data1 = ((p_fesp->Crosstalk.thresh    & 0xFFFF) <<  0)		// bit0-bit15     Crosstalk_thresh       thres2cgf
					| ((p_fesp->Crosstalk.thres1cgf & 0xFFFF) << 16)		// bit16-bit31    Crosstalk_thresh       thres1cgf
					;

			Gem_write ((GEM_CROSS_BASE+0x00), data0);
			Gem_write ((GEM_CROSS_BASE+0x04), data1);
		}
	}
}

static void satuation_match (int inttime_gain, isp_satuation_polyline_tbl *satuation_tbl,
									  const isp_satuation_polyline_tbl *satuation_polyline_tbl,
									  int count
									)
{
	int i;
	int satuation;
	const isp_satuation_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= satuation_polyline_tbl[i].inttime_gain)
			break;
	}
	if(i == count)
	{
		satuation = satuation_polyline_tbl[count - 1].satuation;
	}
	// ???
	else if(inttime_gain == satuation_polyline_tbl[i].inttime_gain)
	{
		satuation = satuation_polyline_tbl[i].satuation;
	}
	// ???
	else if(inttime_gain < satuation_polyline_tbl[0].inttime_gain)
	{
		satuation =  satuation_polyline_tbl[0].satuation;
	}
	else
	{
		lo = &satuation_polyline_tbl[i - 1];
		hi = &satuation_polyline_tbl[i];
		satuation = (int)(lo->satuation + (hi->satuation - lo->satuation) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}

	if(satuation > 1280)
		satuation = 1280;
	else if(satuation < 256)
		satuation = 256;

	satuation_tbl->satuation = satuation;
}

static void enhance_match (int inttime, isp_enhance_polyline_tbl *enhance_tbl,
									const isp_enhance_polyline_tbl *enhance_polyline_tbl,
									int count)
{
	int i;
	int bright;
	int contrast;
	const isp_enhance_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= enhance_polyline_tbl[i].inttime)
			break;
	}
	if(i == count)
	{
		bright = enhance_polyline_tbl[count - 1].bright;
		contrast = enhance_polyline_tbl[count - 1].contrast;
	}
	// ???
	else if(inttime == enhance_polyline_tbl[i].inttime)
	{
		bright = enhance_polyline_tbl[i].bright;
		contrast = enhance_polyline_tbl[i].contrast;
	}
	// ???
	else if(inttime < enhance_polyline_tbl[0].inttime)
	{
		bright =  enhance_polyline_tbl[0].bright;
		contrast =  enhance_polyline_tbl[0].contrast;
	}
	else
	{
		lo = &enhance_polyline_tbl[i - 1];
		hi = &enhance_polyline_tbl[i];
		bright = lo->bright + (hi->bright - lo->bright) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		contrast = lo->contrast + (hi->contrast - lo->contrast) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
	}

	if(bright > 64)
		bright = 64;
	else if(bright < -64)
		bright = -64;
	if(contrast > 2047)
		contrast = 2047;
	else if(contrast < 1)
		contrast = 1;
	enhance_tbl->bright = bright;
	enhance_tbl->contrast = contrast;
}


static void cmos_isp_enhance_run (int inttime_gain, isp_enhance_ptr_t p_enhance)
{
	// ?????????(????????)
	unsigned int data1, data2;
	int bright;
	int contrast;
	int satuation;
	isp_enhance_polyline_tbl enhance_polyline_tbl;
	isp_satuation_polyline_tbl satuation_tbl;
	//
	if(isp_exposure.cmos_sensor.cmos_isp_enhance_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_enhance_run)(inttime_gain, p_enhance);
		return;
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_enhance_table)
	{
		const isp_enhance_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_enhance_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			enhance_match (inttime_gain, &enhance_polyline_tbl, tbl, tbl_count);
			p_enhance->bcst.bright = enhance_polyline_tbl.bright;
			p_enhance->bcst.contrast = enhance_polyline_tbl.contrast;
			data1 	= ((p_enhance->bcst.enable    & 0x001) << 31)
					| ((p_enhance->bcst.bright    & 0x3FF) <<  0)
					| ((p_enhance->bcst.offset0   & 0x3FF) << 10)
					| ((p_enhance->bcst.offset1   & 0x3FF) << 20)
					;
		   data2 	= ((p_enhance->bcst.contrast  & 0x7FF) <<  0)
					| ((p_enhance->bcst.satuation & 0x7FF) << 11)
					| ((p_enhance->bcst.hue       & 0x0FF) << 24)
					;
			Gem_write ((GEM_ENHANCE_BASE+0x04), data1);
  			Gem_write ((GEM_ENHANCE_BASE+0x08), data2);
		}
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_satuation_table)
	{
		const isp_satuation_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_satuation_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			satuation_match (inttime_gain, &satuation_tbl, tbl, tbl_count);
			satuation = satuation_tbl.satuation;
			p_enhance->bcst.satuation = satuation;
			data2 	= ((p_enhance->bcst.contrast  & 0x7FF) <<  0)
					| ((p_enhance->bcst.satuation & 0x7FF) << 11)
					| ((p_enhance->bcst.hue       & 0x0FF) << 24)
					;
			Gem_write ((GEM_ENHANCE_BASE+0x08), data2);
		}
	}
}

static void sharp_match (int inttime, isp_sharp_polyline_tbl *sharp_tbl,
									const isp_sharp_polyline_tbl *sharp_polyline_tbl,
									int count)
{
	int i;
	int	strength;		// ??????????????64
	int	gainmax;			// ????????ó®????????256

	const isp_sharp_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= sharp_polyline_tbl[i].inttime)
			break;
	}
	if(i == count)
	{
		strength = sharp_polyline_tbl[count - 1].strength;
		gainmax = sharp_polyline_tbl[count - 1].gainmax;
	}
	// ???
	else if(inttime == sharp_polyline_tbl[i].inttime)
	{
		strength = sharp_polyline_tbl[i].strength;
		gainmax = sharp_polyline_tbl[i].gainmax;
	}
	// ???
	else if(inttime < sharp_polyline_tbl[0].inttime)
	{
		strength =  sharp_polyline_tbl[0].strength;
		gainmax = sharp_polyline_tbl[0].gainmax;
	}
	else
	{
		lo = &sharp_polyline_tbl[i - 1];
		hi = &sharp_polyline_tbl[i];
		strength = lo->strength + (hi->strength - lo->strength) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		gainmax = lo->gainmax + (hi->gainmax - lo->gainmax) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
	}

	if(strength > 255)
		strength = 255;
	else if(strength < 1)
		strength = 1;
	if(gainmax > 255)
		gainmax = 255;
	else if(gainmax < 1)
		gainmax = 1;

	sharp_tbl->strength = strength;
	sharp_tbl->gainmax = gainmax;
}

static void cmos_isp_sharp_run (int inttime_gain, isp_enhance_ptr_t p_enhance)
{
	// ?????????(????????)
	unsigned int data0;
	int	strength;		// ??????????????64
	int	gainmax;			// ????????ó®????????256
	isp_sharp_polyline_tbl sharp_polyline_tbl;

	if(isp_exposure.cmos_sensor.cmos_isp_sharp_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_sharp_run)(inttime_gain, p_enhance);
		return;
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_sharp_table)
	{
		const isp_sharp_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_sharp_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			sharp_match (inttime_gain, &sharp_polyline_tbl, tbl, tbl_count);
			p_enhance->sharp.strength = sharp_polyline_tbl.strength;
			p_enhance->sharp.gainmax = sharp_polyline_tbl.gainmax;

		   data0 	= ((p_enhance->sharp.enable   &  0x01) <<  0)
					| ((p_enhance->sharp.mode     &  0x01) <<  1)
					| ((p_enhance->sharp.coring   &  0x07) <<  5)
					| ((p_enhance->sharp.strength &  0xFF) <<  8)
					| ((p_enhance->sharp.gainmax  & 0x3FF) << 16)
					;

			Gem_write ((GEM_ENHANCE_BASE+0x00), data0);
		}
	}

}


static void ae_match (int inttime, isp_ae_polyline_tbl *ae_tbl,
							 const isp_ae_polyline_tbl *ae_polyline_tbl,
							 int count
							 )
{
	int i;
	int black_target;
	const isp_ae_polyline_tbl *lo, *hi;
	ae_tbl->inttime = inttime;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= ae_polyline_tbl[i].inttime)
			break;
	}
	// ???
	if(i == count)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[count - 1].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[count - 1].compensation;
		ae_tbl->black_target = ae_polyline_tbl[count - 1].black_target;
	}
	else if(inttime == ae_polyline_tbl[i].inttime)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[i].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[i].compensation;
		ae_tbl->black_target = ae_polyline_tbl[i].black_target;
	}
	// ???
	else if(inttime < ae_polyline_tbl[0].inttime)
	{
		memcpy (ae_tbl->window_weight, ae_polyline_tbl[0].window_weight, 9);
		ae_tbl->compensation = ae_polyline_tbl[0].compensation;
		ae_tbl->black_target = ae_polyline_tbl[0].black_target;
	}
	else
	{
		int index;

		lo = &ae_polyline_tbl[i - 1];
		hi = &ae_polyline_tbl[i];

		for (index = 0; index < 9; index ++)
		{
			int w = lo->window_weight[index] + (hi->window_weight[index] - lo->window_weight[index]) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
			if(w < 0)
				w = 0;
			else if(w > 15)
				w = 15;
			ae_tbl->window_weight[index] = w;
		}

		int comp = lo->compensation + (hi->compensation - lo->compensation) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		if(comp < 1)
			comp = 1;
		else if(comp > 64)
			comp = 64;
		ae_tbl->compensation = comp;


		black_target = lo->black_target + (hi->black_target - lo->black_target) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
		if(black_target < 64)
			black_target = 64;
		else if(black_target > 128)
			black_target = 128;
		ae_tbl->black_target = black_target;

	}
	/*
	printf ("win=[%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d]\n",
			  ae_tbl->window_weight[0], ae_tbl->window_weight[1], ae_tbl->window_weight[2],
			  ae_tbl->window_weight[3], ae_tbl->window_weight[4], ae_tbl->window_weight[5],
			  ae_tbl->window_weight[6], ae_tbl->window_weight[7], ae_tbl->window_weight[8]);
	*/
}

static void cmos_isp_ae_run (int inttime_gain, isp_ae_ptr_t p_ae)
{
	isp_ae_polyline_tbl ae_tbl;

	if(isp_exposure.cmos_sensor.cmos_isp_ae_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_ae_run)(inttime_gain, p_ae);
		return;
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_ae_table)
	{
		const isp_ae_polyline_tbl *table;
		int count = 0;
		table = (*isp_exposure.cmos_sensor.cmos_isp_get_ae_table) (&count);
		if(table && count)
		{
			ae_match (inttime_gain, &ae_tbl, table, count);
			isp_ae_window_weight_write (&isp_exposure.cmos_ae, (u8_t (*)[3])(ae_tbl.window_weight));

			isp_system_ae_compensation_write (&isp_exposure.cmos_ae, (u8_t)ae_tbl.compensation);

			isp_system_ae_black_target_write (&isp_exposure.cmos_ae, (u8_t)ae_tbl.black_target);

			isp_auto_exposure_compensation (&isp_exposure.cmos_ae, isp_exposure.cmos_ae.histogram.bands);
		}
	}

}


static void demosaic_match (int inttime_gain, isp_demosaic_polyline_tbl *demosaic_tbl,
									 const isp_demosaic_polyline_tbl *demosaic_polyline_tbl,
									 int count)
{
	int i;
	int demk;
	const isp_demosaic_polyline_tbl *lo, *hi;
	for (i = 0; i < count; i ++)
	{
		if(inttime_gain <= demosaic_polyline_tbl[i].inttime_gain)
			break;
	}
	// ???
	if(i == count)
	{
		demk = demosaic_polyline_tbl[count - 1].demk;
	}
	else if(inttime_gain == demosaic_polyline_tbl[i].inttime_gain)
	{
		demk = demosaic_polyline_tbl[i].demk;
	}
	// ???
	else if(inttime_gain < demosaic_polyline_tbl[0].inttime_gain)
	{
		demk =  demosaic_polyline_tbl[0].demk;
	}
	else
	{
		lo = &demosaic_polyline_tbl[i - 1];
		hi = &demosaic_polyline_tbl[i];
		demk = (int)(lo->demk + (hi->demk - lo->demk) * (inttime_gain - lo->inttime_gain) / (hi->inttime_gain - lo->inttime_gain));
	}

	if(demk > 512)
		demk = 512;
	else if(demk < 24)
		demk = 24;

	demosaic_tbl->demk = demk;
}


static void isp_ae_gamma_match (int inttime, isp_gamma_polyline_tbl *gamma_tbl,
										  const isp_gamma_polyline_tbl *gamma_polyline_tbl,
										  int count)
{
	int i;
	const isp_gamma_polyline_tbl *lo, *hi;
	gamma_tbl->inttime = inttime;
	for (i = 0; i < count; i ++)
	{
		if(inttime <= gamma_polyline_tbl[i].inttime)
			break;
	}
	// ???
	if(i == count)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[count - 1].gamma_lut, sizeof(int) * 65);
	}
	else if(inttime == gamma_polyline_tbl[i].inttime)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[i].gamma_lut, sizeof(int) * 65);
	}
	// ???
	else if(inttime < gamma_polyline_tbl[0].inttime)
	{
		memcpy (gamma_tbl->gamma_lut, gamma_polyline_tbl[0].gamma_lut, sizeof(int) * 65);
	}
	else
	{
		lo = &gamma_polyline_tbl[i - 1];
		hi = &gamma_polyline_tbl[i];
		for (i = 0; i < 65; i ++)
		{
			int w = lo->gamma_lut[i] + (hi->gamma_lut[i] - lo->gamma_lut[i]) * (inttime - lo->inttime) / (hi->inttime - lo->inttime);
			if(w < 0)
				w = 0;
			else if(w > 65535)
			{
				w = 65535;
			}
			gamma_tbl->gamma_lut[i] = w;
		}
	}
}

static unsigned short gamma_adjust_table[65];
static int do_gamma_adjust;

// gamma?????????ISP"??????"(??????ßÿ?)?ß’???, ??????"????????"???????????????????????????.
//		?????ßª?????????Gamma????, ?????????????????????????????.
//		???ISP"??????"???, ?????????????????????.
void isp_gamma_adjust(void)
{
	// ???gamma??????????????
	if(do_gamma_adjust)
	{
		int i;
		// ß’?????????
		for (i = 0; i < 65; i++)
		{
		  unsigned int data0 = (0x04) | (i << 8) | (gamma_adjust_table[i]<<16);
		  Gem_write ((GEM_LUT_BASE+0x00), data0);
		}
		do_gamma_adjust = 0;
	}
}

static void cmos_isp_colors_run (int inttime_gain, isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae)
{
	// ????demosaic, demk????????????
	isp_demosaic_polyline_tbl demosaic_tbl;
	int demk;
	unsigned int data0;

	// ?????ßÿ????????????
	if(isp_exposure.cmos_sensor.cmos_isp_colors_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_colors_run)(inttime_gain, p_colors, p_awb, p_ae);
		return;
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_demosaic_table)
	{
		const isp_demosaic_polyline_tbl *tbl = NULL;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_demosaic_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			demosaic_match (inttime_gain, &demosaic_tbl, tbl, tbl_count);
			demk = demosaic_tbl.demk;
			p_colors->demosaic.demk = demk;

			// ?°„∑⁄ISP????demosaic????, ???2??demosaic??
			//  0 ~  7  ????????????????8¶À???, ????64
			// 20 ~ 27  ??????16
			// 31       ????0?
			data0 = ((p_colors->demosaic.mode & 1) << 31)
					| ((p_colors->demosaic.coff_00_07 & 0xFF) << 0)
					| ((p_colors->demosaic.coff_20_27 & 0xFF) << 20)
					| ((p_colors->demosaic.demk & 0xFFF) << 8)		// demk	bit19-8, 12bit
					;
			Gem_write ((GEM_DEMOSAIC_BASE+0x00), data0);
		}
	}

}

static void cmos_isp_gamma_run (int inttime_gain, isp_colors_ptr_t p_colors)
{
	int i;
	if(isp_exposure.cmos_sensor.cmos_isp_gamma_run)
	{
		(*isp_exposure.cmos_sensor.cmos_isp_gamma_run)(inttime_gain, p_colors);
		return;
	}

	if(isp_exposure.cmos_sensor.cmos_isp_get_gamma_table)
	{
		const isp_gamma_polyline_tbl *tbl;
		int tbl_count = 0;
		tbl = (*isp_exposure.cmos_sensor.cmos_isp_get_gamma_table) (&tbl_count);
		if(tbl && tbl_count)
		{
			isp_gamma_polyline_tbl gamma_tbl;
			isp_ae_gamma_match (inttime_gain, &gamma_tbl, tbl, tbl_count);
			for (i = 0; i < 65; i ++)
			{
				gamma_adjust_table[i] = gamma_tbl.gamma_lut[i];
				p_colors->gamma.gamma_lut[i] = gamma_adjust_table[i];
			}
			do_gamma_adjust = 1;		// ???????????
		}
	}
}

