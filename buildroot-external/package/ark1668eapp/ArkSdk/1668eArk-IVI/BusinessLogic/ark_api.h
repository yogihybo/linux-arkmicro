#ifndef __ARK_API_H__
#define __ARK_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include "basetype.h"
#include "mfcapi.h"
#include "ark_list.h"


enum mirror_direction {
	LANDSCAPE,
	VERTICAL,
};

/* Surface formats. */
enum ark_2d_format
{
	/* RGB formats. */
	ARK_gcvSURF_R5G6B5              = 209,
	ARK_gcvSURF_R8G8B8,
	ARK_gcvSURF_A8R8G8B8            = 212,

	/* BGR formats. */
	ARK_gcvSURF_B5G6R5              = 302,
	ARK_gcvSURF_B8G8R8,
	ARK_gcvSURF_A8B8G8R8            = 306,

	/* YUV formats. */
	ARK_gcvSURF_YUY2                = 500,
	ARK_gcvSURF_UYVY,
	ARK_gcvSURF_YV12,
	ARK_gcvSURF_I420,
	ARK_gcvSURF_NV12,
	ARK_gcvSURF_NV21,
	ARK_gcvSURF_NV16,
	ARK_gcvSURF_NV61,
	ARK_gcvSURF_YVYU,
	ARK_gcvSURF_VYUY,
};


enum ark_disp_layer {
	PRIMARY_LAYER,      //fb0 for UI
	VIDEO_LAYER,        //video/phonelink display
	OVER_LAYER,         //overlay for UI
	TVOUT_LAYER,        //tvout  display
	AUX_LAYER,          //itu601/656 display
	MAX_LAYERS
};

 enum ark_disp_mode {
	DISP_NONE = -1,
	DISP_PLAYER,
	DISP_PHONELINK,
	DISP_AUX,
	DISP_MODE_NUM,
};

enum ark_disp_format {
	ARK_LCDC_FORMAT_YUV422,
	ARK_LCDC_FORMAT_YUV420,
	ARK_LCDC_FORMAT_VYUY,
	ARK_LCDC_FORMAT_YUV,
	ARK_LCDC_FORMAT_RGBI555,
	ARK_LCDC_FORMAT_R5G6B5,
	ARK_LCDC_FORMAT_RGBA888,
	ARK_LCDC_FORMAT_RGB888,

	ARK_LCDC_FORMAT_Y_UV422 = 0x10,
	ARK_LCDC_FORMAT_Y_UV420 = 0x11,
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

enum ark_yuv_order {
	ARK_LCDC_ORDER_VYUY = 0x000000,
	ARK_LCDC_ORDER_UYVY = 0x010000,
	ARK_LCDC_ORDER_YVYU = 0x020000,
	ARK_LCDC_ORDER_YUYV = 0x030000,
};

enum ark_rgb_order {
	ARK_LCDC_ORDER_RGB = 0x00000000,
	ARK_LCDC_ORDER_RBG = 0x01000000,
	ARK_LCDC_ORDER_GRB = 0x02000000,
	ARK_LCDC_ORDER_GBR = 0x03000000,
	ARK_LCDC_ORDER_BRG = 0x04000000,
	ARK_LCDC_ORDER_BGR = 0x05000000,
};

enum ark_platform_type {
	ARK_PLATFORM_ARK1668,
	ARK_PLATFORM_ARKN141,
	ARK_PLATFORM_ARK1668E,
	ARK_PLATFORM_MAX
};

typedef enum ark_scale_rotate {
	SCALE_ROTATE_0,
	SCALE_ROTATE_90,
	SCALE_ROTATE_180,
	SCALE_ROTATE_270,
	SCALE_ROTATE_0_MIRROR,
	SCALE_ROTATE_90_MIRROR,
	SCALE_ROTATE_180_MIRROR,
	SCALE_ROTATE_270_MIRROR,
	SCALE_ROTATE_END
} scale_rotate;

typedef struct ark_platform_info {
	u32 type;
	u32 reserve[32];
} platform_info;

typedef struct ark_disp_window {
	int pos_x;
	int pos_y;
	int width;
	int height;
	int format;
	int yuyv_order;
	int rgb_order;
} disp_window;

/* lcd display video layer scale */
typedef struct ark_disp_scaler {
	int src_w;
	int src_h;
	int win_x;
	int win_y;
	int win_w;
	int win_h;
	int out_x;
	int out_y;
	int out_w;
	int out_h;
	int cut_top;
	int cut_bottom;
	int cut_left;
	int cut_right;
} disp_scaler;

typedef struct ark_disp_color {
	u8 contrast;
	u8 brightness;
	u8 saturation;
	u8 hue;
} disp_color;

typedef struct ark_disp_vp {
	struct ark_disp_color color;
	int reg[6];
}disp_vp;

typedef struct ark_screen {
	u32 type;
	u32 width;
	u32 height;

	u32 disp_x;
	u32 disp_y;
	u32 disp_width;
	u32 disp_height;
} screen_info;

typedef struct ark_disp_addr {
	u32 yaddr;
	u32 cbaddr;
	u32 craddr;
	u32 wait_vsync;
} disp_addr;

#define FRAME_MAX               5
#define ROTATE_BUF_MAX          4

typedef struct ark_reqbuf{
	struct list_head list;
	u32 count;
	u32 size;
	u32 phy_addr[FRAME_MAX];
	u32 *virt_addr[FRAME_MAX];
}reqbuf_info;


typedef struct{
	int fd;
	int reqbuf_cnt;
	struct list_head reqbuf_list;
} memalloc_handle;

typedef struct ark_disp_handle{
	struct list_head list;
	memalloc_handle *mem_handle;

	int fd;
	int layer_id;
	int display_mode;
	int atomic_stat;
	int show;
	int force;

	disp_window win;
	disp_addr addr;
	disp_scaler scaler;
	disp_color color;
	screen_info screen;
	platform_info platform;
	reqbuf_info reqbuf;

	void *disp_head;
	u32 reserve[32];
}disp_handle;

typedef struct{
	u32 win_src_x;
	u32 win_src_y;
	u32 win_src_width;
	u32 win_src_height;
	u32 disp_x;
	u32 disp_y;
	u32 disp_width;
	u32 disp_height;
	u32 direction;
	i32 keep_aspect_ratio;
}video_cfg;

typedef struct{
	u32 src_width;
	u32 src_height;

	u32 win_src_x;
	u32 win_src_y;
	u32 win_src_width;
	u32 win_src_height;

	u32 dst_width;
	u32 dst_height;

	u32 win_dst_x;
	u32 win_dst_y;
	u32 win_dst_width;
	u32 win_dst_height;

	int src_format;
	int dst_format;

	int direction;
	int keep_aspect_ratio;
}ark2d_cfg;

typedef struct{
	struct list_head list;

	void * head_2d;
	ark2d_cfg cfg_2d;

	u32 out_width;
	u32 out_height;

}ark2d_handle;


typedef struct{
	struct list_head list;
	sem_t sem_lock;
	memalloc_handle *handle_mem;

	disp_handle *handle_disp;
	int first_show;
	unsigned int last_display_addr;
	unsigned int last_render_yaddr;
	unsigned int last_render_uaddr;
	unsigned int last_render_vaddr;

	int dec_first_frame;
	int rotate_buffer_index;

	ark2d_handle *handle_2d;
	int cur_direction;
	video_cfg cfg_vid;
	int active;


	MFCHandle *handle_mfc;
	OutFrameBuffer out_buffer;
	DWLLinearMem_t in_buffer;
	DWLLinearMem_t rotate_buffer[ROTATE_BUF_MAX];

}video_handle;


struct vin_para{
        int xpos;
        int ypos;
        int width;
        int height;
        int source;
        int tvout;
        int hori_mirror;
        int vert_mirror;
        int left_blank;
        int right_blank;
        int top_blank;
        int bottom_blank;
        int progressive;
        int itu601en;
};

struct vin_screen {
	unsigned int  disp_x;
	unsigned int  disp_y;
	unsigned int  disp_width;
	unsigned int  disp_height;
};

enum dvr_source {
	DVR_SOURCE_CAMERA,
	DVR_SOURCE_AUX,
	DVR_SOURCE_DVD,
};


/*-----------------------------ARK  API: VIDEO------------------------------------*/
int arkapi_video_set_display_mode(int mode);
video_handle *arkapi_video_init(int stream_type);
int arkapi_video_set_config(video_handle *handle, video_cfg *cfg_vid);
int arkapi_video_get_config(video_handle *handle, video_cfg *cfg_vid);
int arkapi_video_play(video_handle *handle, const void *src_addr, int len, int fps);
int arkapi_video_play_eof(video_handle *handle, int fps);
int arkapi_video_show(video_handle *handle, int enable);// 1: show  0: hide
int arkapi_video_show_mode(video_handle *handle, int mode, int enable);
void arkapi_video_release(video_handle *handle);
int arkapi_enter_carback(void);
int arkapi_exit_carback(void);

/*-----------------------------ARK API BASE:  DISPLAY------------------------------*/
disp_handle *arkapi_display_open_layer(enum ark_disp_layer layer);
void arkapi_display_close_layer(disp_handle *handle);
int arkapi_display_show_layer(disp_handle *handle);
int arkapi_display_hide_layer(disp_handle *handle);
int arkapi_display_set_mode(disp_handle *handle, int mode);

int arkapi_display_recycle_layer(enum ark_disp_layer layer);
int arkapi_display_force_show_layer(enum ark_disp_layer layer);
int arkapi_display_force_hide_layer(enum ark_disp_layer layer);

int arkapi_display_set_layer_pos(disp_handle *handle, int x, int y);
int arkapi_display_set_layer_size(disp_handle *handle, int width, int height);
int arkapi_display_set_layer_format(disp_handle *handle, int format);
int arkapi_display_set_layer_scaler(disp_handle *handle, disp_scaler* spara);
int arkapi_display_set_layer_addr(disp_handle *handle, u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr);
int arkapi_set_layer_priority(disp_handle *handle,int video_pri, int video2_pri, int win1_pri,int win2_pri,int win3_pri);

int arkapi_display_set_layer_pos_atomic(disp_handle *handle, int x, int y);
int arkapi_display_set_layer_size_atomic(disp_handle *handle, int width, int height);
int arkapi_display_set_layer_format_atomic(disp_handle *handle, int format);
int arkapi_display_set_layer_addr_atomic(disp_handle *handle, u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr);
int arkapi_display_set_layer_scaler_atomic(disp_handle *handle, disp_scaler *spara);
int arkapi_display_layer_update_commit(disp_handle *handle);

int arkapi_display_layer_set_color(enum ark_disp_layer layer, disp_color* vp);
int arkapi_display_layer_get_color(enum ark_disp_layer layer, disp_color* vp);

int arkapi_display_wait_for_vsync(disp_handle *handle);
int arkapi_display_get_vsync_status(disp_handle *handle);
int arkapi_display_get_layer_addr(disp_handle *handle, u32 *pyrgbaddr, u32 *pcbaddr, u32 *pcraddr);
int arkapi_display_get_screen_info(screen_info *screen);
int arkapi_display_set_screen_info(screen_info *screen);


/*-----------------------------ARK API BASE:  MEM-----------------------------------------*/
memalloc_handle *arkapi_memalloc_init(void);
reqbuf_info *arkapi_memalloc_reqbuf(memalloc_handle *handle, int size, int count);
void arkapi_memalloc_release(memalloc_handle *handle);
void arkapi_memalloc_free(memalloc_handle *handle, reqbuf_info *pbuf);

void* arkapi_mem_map(unsigned int phy_addr, unsigned int size);
int arkapi_mem_unmap(void* virt_addr, unsigned int size);


/*-----------------------------------ARK API BASE:  2D-----------------------------------*/
ark2d_handle *arkapi_2d_init(void);
int arkapi_2d_set_config(ark2d_handle *handle, ark2d_cfg *cfg_2d);
int arkapi_2d_get_config(ark2d_handle *handle, ark2d_cfg *cfg_2d);
int arkapi_2d_process(ark2d_handle *handle, u32 src_phyaddr, u32 dst_phyaddr);
void arkapi_2d_release(ark2d_handle *handle);

/*-----------------------------------ARK API BASE:  VIN-----------------------------------*/
int arkapi_open_vin(void);

int arkapi_close_vin(int vin_fd);

int arkapi_vin_config(int vin_fd,int progressive, int itu601en);


int arkapi_vin_start(int vin_fd);

int arkapi_vin_stop(int vin_fd);

int arkapi_vin_detect_signal(int vin_fd);

int arkapi_vin_switch_channel(int vin_fd, enum dvr_source source);

/*-----------------------------------ARK API BASE:  ARK API SCALAR -----------------------*/
int arkapi_scalar(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	scalar_in_format iformat,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int iwindow_x,
	unsigned int iwindow_y,
	unsigned int iwinwidth,
	unsigned int iwinheight,
	unsigned int left_cut,
	unsigned int right_cut,
	unsigned int up_cut,
	unsigned int bottom_cut,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int oyaddr,
	unsigned int ouaddr,
	unsigned int ovaddr,
	scalar_out_format oformat,	//oformat: not effect for arkn141.
	scale_rotate rotate);		//rotate: not effect for arkn141.

int arkapi_scalar_nowait(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	scalar_in_format iformat,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int iwindow_x,
	unsigned int iwindow_y,
	unsigned int iwinwidth,
	unsigned int iwinheight,
	unsigned int left_cut,
	unsigned int right_cut,
	unsigned int up_cut,
	unsigned int bottom_cut,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int oyaddr,
	unsigned int ouaddr,
	unsigned int ovaddr,
	scalar_out_format oformat,	//oformat: not effect for arkn141.
	scale_rotate rotate);		//rotate: not effect for arkn141.

int arkapi_scalar_wait_idle(void);


/*-----------------------------------ARK API BASE:  ARK API N141 SCALAR -----------------------*/


//\D2\D4\CF½ӿ\DA\C6\F4\B6\AF\A3\AC\C7\EBʹ\D3\C3arkapi_scalar() \B4\FA\CC\E6arkapi_n141_scalar().


/* <<< ******	Abandon Warning	****** >>>
 *
 *		arkn141 interface have been abandoned as follows,
 *		please using arkapi_scalar() instead it.
 *
 */

int arkapi_n141_scalar(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	unsigned int window_x,
	unsigned int window_y,
	unsigned int window_width,
	unsigned int window_height,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int ostride,
	unsigned int format,
	unsigned int yout,
	unsigned int uout,
	unsigned int vout);
int arkapi_n141_scalar_lock(void);
int arkapi_n141_scalar_unlock(void);

#define LIBARKAPI_ARK1668	0
#define LIBARKAPI_ARKN141	1
#define LIBARKAPI_ARK1668E	2

/*----------------------------------------END------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
