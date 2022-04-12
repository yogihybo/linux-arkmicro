// =============================================================================
// File        : Gem_isp_fesp.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_fesp.h"
#include "Gem_isp_io.h"


void isp_fesp_init_io (isp_fesp_ptr_t p_fesp)
{
	int i, data0, data1, data2, data3;

	// lens shading
	data0 = 	((p_fesp->Lensshade.enable & 0x01) << 0)
			| 	((p_fesp->Lensshade.scale  & 0x03) << 1);
	data1 = 	((p_fesp->Lensshade.rcenterRx & 0xFFFF) <<  0)
			|	((p_fesp->Lensshade.rcenterRy & 0xFFFF) << 16);
	data2 = 	((p_fesp->Lensshade.rcenterGx & 0xFFFF) <<  0)
			| 	((p_fesp->Lensshade.rcenterGy & 0xFFFF) << 16);
	data3 = 	((p_fesp->Lensshade.rcenterBx & 0xFFFF) <<  0)
			| 	((p_fesp->Lensshade.rcenterBy & 0xFFFF) << 16);
	Gem_write ((GEM_LENS_BASE+0x00), data0);
	Gem_write ((GEM_LENS_BASE+0x04), data1);
	Gem_write ((GEM_LENS_BASE+0x08), data2);
	Gem_write ((GEM_LENS_BASE+0x0c), data3);

	// bit15-0        lscofst
	data0 = (p_fesp->Lensshade.lscofst & 0xFFFF);
	Gem_write ((GEM_LENS_LSCOFST_BASE+0x00), data0);

	for (i = 0; i < 195; i++)
	{
		data0 = (0x00) | (i << 8) | ((p_fesp->Lensshade.coef[i] & 0xFFFF) << 16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}


	// FixPattern denoise
	data0	= ((p_fesp->Fixpatt.enable & 0x01) << 0)
			| ((p_fesp->Fixpatt.mode   & 0x01) << 1);
	data1	= ((p_fesp->Fixpatt.rBlacklevel  & 0xFFFF) <<  0)
			| ((p_fesp->Fixpatt.grBlacklevel & 0xFFFF) << 16);
	data2	= ((p_fesp->Fixpatt.gbBlacklevel & 0xFFFF) <<  0)
			| ((p_fesp->Fixpatt.bBlacklevel  & 0xFFFF) << 16);
	Gem_write ((GEM_FIXPATT_BASE+0x00), data0);
	Gem_write ((GEM_FIXPATT_BASE+0x04), data1);
	Gem_write ((GEM_FIXPATT_BASE+0x08), data2);
	for (i = 0; i < 17; i++)
	{
		data0 = (0x09) | (i << 8) | ((p_fesp->Fixpatt.profile[i] & 0xFFFF) << 16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}

	// Bad Pixel
	data0	= ((p_fesp->Badpix.enable & 0x01) << 0)
			| ((p_fesp->Badpix.mode   & 0x01) << 1);
	data1	= ((p_fesp->Badpix.thresh  & 0xFFFF) << 0);
	Gem_write ((GEM_BADPIX_BASE+0x00), data0);
	Gem_write ((GEM_BADPIX_BASE+0x04), data1);
	for (i = 0; i < 16; i++)
	{
		data0 = (0x0a) | (i << 8) | ((p_fesp->Badpix.profile[i] & 0xFF) << 16);
		Gem_write ((GEM_LUT_BASE+0x00), data0);
	}

	// CrossTalk
	//p_fesp->Crosstalk.mode = 0x02;	// 10: use reg thres
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
	for (i = 0; i < 17; i++)
	{
		data1 = (0x01) | (i << 8) | ((p_fesp->Crosstalk.profile[i] & 0xFF) << 16);
		Gem_write ((GEM_LUT_BASE+0x00), data1);
	}

}

