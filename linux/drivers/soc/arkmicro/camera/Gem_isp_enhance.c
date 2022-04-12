// =============================================================================
// File        : Gem_isp_enhance.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_enhance.h"
#include "Gem_isp_io.h"

void isp_enhance_init_io (isp_enhance_ptr_t p_enhance)
{
  int data0, data1, data2;

  data0 	= ((p_enhance->sharp.enable   &  0x01) <<  0)
	  		| ((p_enhance->sharp.mode     &  0x01) <<  1)
			| ((p_enhance->sharp.coring   &  0x07) <<  5)
			| ((p_enhance->sharp.strength &  0xFF) <<  8)
			| ((p_enhance->sharp.gainmax  & 0x3FF) << 16)
			;
  data1 	= ((p_enhance->bcst.enable    & 0x001) << 31)
	  		| ((p_enhance->bcst.bright    & 0x3FF) <<  0)
			| ((p_enhance->bcst.offset0   & 0x3FF) << 10)
			| ((p_enhance->bcst.offset1   & 0x3FF) << 20)
			;
  data2 	= ((p_enhance->bcst.contrast  & 0x7FF) <<  0)
	  		| ((p_enhance->bcst.satuation & 0x7FF) << 11)
			| ((p_enhance->bcst.hue       & 0x0FF) << 24)
			;
  Gem_write ((GEM_ENHANCE_BASE+0x00), data0);
  Gem_write ((GEM_ENHANCE_BASE+0x04), data1);
  Gem_write ((GEM_ENHANCE_BASE+0x08), data2);
}












