// =============================================================================
// File        : Gem_isp_io.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_IO_H_
#define  _GEM_ISP_IO_H_

#include <linux/io.h>
#include "ark_camera.h"

extern struct ark_camera_context *camera_context;

void Gem_write (unsigned int reg_base, unsigned int data);
unsigned int Gem_read (unsigned int reg_base);

void Gem_io_write (unsigned int reg_base, unsigned int data);
unsigned int Gem_io_read (unsigned int reg_base);

#define Gem_write(reg_base, data)     writel(data, camera_context->base + reg_base)
#define Gem_read(reg_base) 		      readl(camera_context->base + reg_base)



#endif