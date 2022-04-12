#pragma once

#include "ECMirrorRender.h"

//define  compile  mode
#define COMPILE_USE_ARK_VIDEO_API          1                
#define COMPILE_MFC_STORE_FILE_NO_ARK_API  2             
#define COMPILE_TEST_ARK_API               3         
      
//define  test mode
#define USE_ARK_VIDEO_API              1                
#define USE_ARK_DISPLAY_2D_API         2
#define USE_ARK_RAW2D_DISPLAY_API      3
#define ARK1668_MFC_DISPLAY_API        4
#define ARKN141_MFC_DISPLAY_API        5         

///////////////////////////////////////////////////////////////

#define ARK_COMPILE_MODE                COMPILE_TEST_ARK_API

//////////////////////////////////////////////////////////////

#if (ARK_COMPILE_MODE == COMPILE_USE_ARK_VIDEO_API)
#include "ark_api.h"

class ARKMirrorRender : public ECMirrorRender
{
public:
	ARKMirrorRender();
	virtual ~ARKMirrorRender();
	virtual void initialize();
	virtual void play(const void *data, int len, MirrorDirection direction = LANDSCAPE);
	virtual void show();
	virtual void hide();
	virtual void release();

private:

	MirrorDirection cur_direction;
	video_handle * handle_vid;
};


#else

#include <gc_hal.h>
#include <gc_hal_raster.h>
#include "mfcapi.h"
#include "memalloc.h"
#include "ark_api.h"


/*!d
 * \brief 亿连解码渲染接口
 */
class ARKMirrorRender : public ECMirrorRender
{
public:
	ARKMirrorRender();
	virtual ~ARKMirrorRender();

	/*!
	 * \brief 初始化解码器
	 * 根据实现需要内部可直接使用成员变量中的屏幕和镜像分辨率
	 */
	virtual void initialize();

	/*!
	 * \brief 解码渲染
	 * \param[in] data 镜像数据
	 * \param[in] len 镜像数据长度
	 * \param[in] direction 镜像的显示方向，即横屏或竖屏
	 */
	virtual void play(const void *data, int len, MirrorDirection direction = LANDSCAPE);

	/*!
	 * \brief 显示镜像画面
	 */
	virtual void show();

	/*!
	 * \brief 隐层镜像画面
	 */
	virtual void hide();

	/*!
	 * \brief 释放解码器
	 */
	virtual void release();

private:
	int alloc_buffer(void);
	void free_buffer(void);
	void data_process(unsigned int src_addr, unsigned int dst_addr, MirrorDirection direction);
	void render(unsigned int addr);

	bool initialized;
	int fd_memalloc;
	//int fd_fb;
	unsigned int last_display_addr;
	int first_show;
	int dec_first_frame;
	int rotate_buffer_index;
	int out_width;
	int out_height;
	MirrorDirection cur_direction;
	OutFrameBuffer out_buffer;
	DWLLinearMem_t in_buffer;
	DWLLinearMem_t rotate_buffer[3];
	gcoOS           g_os;
	gcoHAL          g_hal;
	gco2D           g_engine2d;

	MFCHandle *handle_mfc;

	/**************************USE_ARK_VIDEO_API****************************/
	video_handle * handle_vid;

	/**************************USE_ARK_2D_API******************************/
	ark2d_handle *handle_2d;
	disp_handle *handle_display;
	memalloc_handle * handle_mem;

};

#endif
