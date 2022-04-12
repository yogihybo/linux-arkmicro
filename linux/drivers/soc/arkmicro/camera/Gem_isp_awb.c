// =============================================================================
// File        : Gem_isp_awb.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_awb.h"
#include "Gem_isp_io.h"

void isp_awb_init_io (isp_awb_ptr_t p_awb)
{
	int data0, data1, data2;
	
	data0 	= ((p_awb->enable  & 0x01) <<  0) 
				| ((p_awb->mode    & 0x03) <<  1) 	// bit1-bit2     mode 
				// 0: unite gray white average 
				//	1: unite color temperature average  
				//	2: zone color temperature Weight
				| ((p_awb->manual  & 0x01) <<  3) 	// bit3  manual, 0: auto awb  1: mannual awb
				| ((p_awb->black   & 0xFF) <<  8) 
				| ((p_awb->white   & 0xFF) << 16)
				;
	data1 = (p_awb->r2g_min) | (p_awb->b2g_min<<16);
	data2 = (p_awb->r2g_max) | (p_awb->b2g_max<<16); 
	
	Gem_write ((GEM_AWB0_BASE+0x00), data0);
	Gem_write ((GEM_AWB0_BASE+0x04), data1);
	Gem_write ((GEM_AWB0_BASE+0x08), data2);
	
	data0 = (p_awb->r2g_light[0]) | (p_awb->b2g_light[0]<<16);
	Gem_write ((GEM_AWB0_BASE+0x0c), data0);
	data0 = (p_awb->r2g_light[1]) | (p_awb->b2g_light[1]<<16);
	Gem_write ((GEM_AWB0_BASE+0x10), data0);
	data0 = (p_awb->r2g_light[2]) | (p_awb->b2g_light[2]<<16);
	Gem_write ((GEM_AWB0_BASE+0x14), data0);
	data0 = (p_awb->r2g_light[3]) | (p_awb->b2g_light[3]<<16);
	Gem_write ((GEM_AWB0_BASE+0x18), data0);
	data0 = (p_awb->r2g_light[4]) | (p_awb->b2g_light[4]<<16);
	Gem_write ((GEM_AWB0_BASE+0x1c), data0);
	data0 = (p_awb->r2g_light[5]) | (p_awb->b2g_light[5]<<16);
	Gem_write ((GEM_AWB0_BASE+0x20), data0);
	data0 = (p_awb->r2g_light[6]) | (p_awb->b2g_light[6]<<16);
	Gem_write ((GEM_AWB0_BASE+0x24), data0);
	data0 = (p_awb->r2g_light[7]) | (p_awb->b2g_light[7]<<16);
	Gem_write ((GEM_AWB0_BASE+0x28), data0);
	
	data0 = (p_awb->use_light[0] <<  0) 
			| (p_awb->use_light[1] <<  1) 
			| (p_awb->use_light[2] <<  2) 
			| (p_awb->use_light[3] <<  3) 
			| (p_awb->use_light[4] <<  4) 
			| (p_awb->use_light[5] <<  5) 
			| (p_awb->use_light[6] <<  6) 
			| (p_awb->use_light[7] <<  7) 
			| (p_awb->jitter       <<  8)
			; 
	
	Gem_write ((GEM_AWB0_BASE+0x2c), data0);
	
	data0 = (p_awb->gain_g2r) | (p_awb->gain_g2b<<16);
	Gem_write ((GEM_AWB0_BASE+0x30), data0);
	
	data0 = (p_awb->weight[0][0] <<  0) 
			| (p_awb->weight[0][1] <<  4) 
			| (p_awb->weight[0][2] <<  8) 
			| (p_awb->weight[1][0] << 12) 
			| (p_awb->weight[1][1] << 16) 
			| (p_awb->weight[1][2] << 20) 
			| (p_awb->weight[2][0] << 24) 
			| (p_awb->weight[2][1] << 28)
			;
	data1 = (p_awb->weight[2][2] << 0);
	
	Gem_write ((GEM_AWB0_BASE+0x34), data0);
	Gem_write ((GEM_AWB0_BASE+0x38), data1); 

} 

void isp_awb_info_read (isp_awb_ptr_t p_awb)
{
	unsigned int data0, data1;
	
	data0 = Gem_read (GEM_AWB1_BASE+0);
	data1 = Gem_read (GEM_AWB1_BASE+4);
	
	p_awb->gray_num = data0;
	p_awb->gain_r2g = data1&0xffff;
	p_awb->gain_b2g = data1 >> 16;

}

void isp_awb_run (isp_awb_ptr_t p_awb)
{
	int r2g, b2g;
	
	isp_awb_info_read (p_awb);
	
	r2g = p_awb->gain_r2g;
	b2g = p_awb->gain_b2g;
	
}



