#ifndef __ARK_2D_H__
#define __ARK_2D_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <gc_hal.h>
#include <gc_hal_raster.h>


struct ark_2d_head{
	struct list_head list; 

	int init;
	int user_cnt;

	gcoOS   g_os;
	gcoHAL	g_hal;
	gco2D	g_engine2d;
};


#ifdef __cplusplus
}
#endif

#endif

