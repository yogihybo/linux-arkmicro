// =============================================================================
// File        : Gem_isp_eris.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_eris.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_io.h"


void isp_eris_init_io (isp_eris_ptr_t p_eris)
{
	int i, data0, data1, data2, data3, data4;

	data0 = ((p_eris->enable  & 0x01) <<  0)
			| ((p_eris->manual  & 0x01) <<  1)
			| ((p_eris->varEris & 0x03) <<  5)		// restore��˹��ѡ��, ȱʡ0
			| ((p_eris->dfsEris & 0x01) <<  7)		// ��������˲�ϵ��, ͨ�����(256������)����
																//		0 ��resoli^spacev���� (0 ~ 255), ʹ�ù̶�ϵ��
																// 	1 ʹ���ⲿ���������ǿ��
			| ((p_eris->resoli  & 0xFF) <<  8)
			| ((p_eris->spacev  & 0xFF) << 16)
			| ((p_eris->target  & 0xFF) << 24)
			;

	data1 = (p_eris->black)    | (p_eris->white << 16);
	data2 = (p_eris->gain_max) | (p_eris->gain_min << 16);
	data3 = (p_eris->cont_max) | (p_eris->cont_min << 16);
	data4 = (p_eris->gain_man) | (p_eris->cont_man << 16);
	Gem_write ((GEM_ERIS_BASE+0x00), data0);
	Gem_write ((GEM_ERIS_BASE+0x04), data1);
	Gem_write ((GEM_ERIS_BASE+0x08), data2);
	Gem_write ((GEM_ERIS_BASE+0x0c), data3);
	Gem_write ((GEM_ERIS_BASE+0x10), data4);

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

	data0 = 	(p_eris->eris_hist_thresh[0] <<  0)
			|	(p_eris->eris_hist_thresh[1] <<  8)
			|	(p_eris->eris_hist_thresh[2] << 16)
			|	(p_eris->eris_hist_thresh[3] << 24)
			;
	Gem_write ((GEM_ERIS_HIST_BASE+0x00), data0);
}
