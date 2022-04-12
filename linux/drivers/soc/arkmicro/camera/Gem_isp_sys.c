// =============================================================================
// File        : Gem_isp_sys.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp.h"
#include "Gem_isp_sys.h"
#include "Gem_isp_io.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_denoise.h"

#include "Gem_isp_sensor.h"
#include "Gem_isp_awb.h"

void isp_sys_init_io (isp_sys_ptr_t p_sys)
{
  int i, data0, data1, data2, data3, data4, data5, data6;

  // ���֡��������
  // �������һ֡, bit0��������
  data0 = p_sys->vifrasel0;
  Gem_write ((GEM_VIFRASE_BASE + 0x00), data0);
  data0 = p_sys->vifrasel1;
  Gem_write ((GEM_VIFRASE_BASE + 0x04), data0);

  data0 	= (p_sys->ckpolar << 1)
	  		| (p_sys->vcpolar << 2)
			| (p_sys->hcpolar << 3)
			| (p_sys->vmanSftenable << 4)
			| (p_sys->vmskenable << 5)
			| (p_sys->frameratei << 8)
			| (p_sys->framerateo << 16)
			;

  data1 = (p_sys->imagewidth) | (p_sys->imageheight<<16);
  //XM_printf("width  :%d\n",p_sys->imagewidth  );
  //XM_printf("height :%d\n",p_sys->imageheight );
  data2 = p_sys->imagehblank
	  		| ((p_sys->resizebit & 0x03) << 16)
	  		| (p_sys->sensorbit << 18)
			| (p_sys->bayermode << 20)
			| (p_sys->sonyif << 22)
			;
  //XM_printf("Sensor bit set:%d \n", p_sys->sensorbit );
  //XM_printf("Sensor bayer set:%d \n", p_sys->bayermode );

  //XM_printf("//slice line:%d\n", p_sys->ffiqIntdelay);
  data3 = ((p_sys->vchkIntenable & 0x01) <<  0)
	  	  | ((p_sys->pabtIntenable & 0x01) <<  1)
		  | ((p_sys->fendIntenable & 0x01) <<  2)
		  | ((p_sys->fabtIntenable & 0x01) <<  3)
		  | ((p_sys->babtIntenable & 0x01) <<  4)
		  | ((p_sys->ffiqIntenable & 0x01) <<  5)
		  | ((p_sys->pendIntenable & 0x01) <<  6)
		  | ((p_sys->infoIntenable & 0x01) <<  7)		// ISP�ڲ���AE��Ϣ��ͳ�����, �ⲿ������Ի�ȡAE��Ϣ, ִ����һ���ع⴦��

		 // | ((0xF << 9))	// 9,  11, 12��Ϊ1��ʹ�ܶ�Ӧ��pabt, fabt, babt�쳣��λframe sync.
		 // | ((0x1F << 9))	// 9,  11, 12 ��Ϊ1��ʹ�ܶ�Ӧ��pabt, fabt, babt�쳣��λframe sync.
			  					// 10 idle״̬��λ�ж�ʹ��/��ֹ,
			  					//		0 ���ж�, �����ISP����
			  					//		1   �ж�, ��ͬ���쳣����(����쳣״̬, �ָ�ISP)
			  					// 13 ʹ��/��ֹidle״̬��֡����
			  					//		0 ��ֹ
			  					//		1 ʹ��
			  					//	ȱʡʱӦ������λȫ������Ϊ1
			| (0xFF << 9)

			| ((p_sys->ffiqIntdelay  & 0xFF) << 24)
			;

  data4 = (p_sys->zonestridex) | (p_sys->zonestridey << 16);
  data5 = (p_sys->sonysac1 << 16) | (p_sys->sonysac2 << 0);
  data6 = (p_sys->sonysac3 << 16) | (p_sys->sonysac4 << 0);

  Gem_write ((GEM_SYS_BASE+0x00), data0);
  Gem_write ((GEM_SYS_BASE+0x04), data1);
  Gem_write ((GEM_SYS_BASE+0x08), data2);
  Gem_write ((GEM_SYS_BASE+0x0c), data3);
  Gem_write ((GEM_SYS_BASE+0x10), data4);
  Gem_write ((GEM_SYS_BASE+0x60), data5);
  Gem_write ((GEM_SYS_BASE+0x64), data6);

  data0 	= (p_sys->debugmode)
	  		| (p_sys->testenable << 1)
			| (p_sys->rawmenable << 2)
			| (p_sys->yuvenable << 3)
			| (p_sys->refenable << 4)
			| (p_sys->yuvformat << 5)
			| (p_sys->dmalock << 7)
			| (p_sys->hstride << 16)
			;
	Gem_write ((GEM_SYS_BASE+0x14), data0);
   // XM_printf("p_sys->hstride :%d\n", p_sys->hstride );
   // XM_printf("p_sys->yuvformat = %d\n", p_sys->yuvformat );

	data0 = p_sys->refaddr;
	Gem_write ((GEM_SYS_BASE+0x18), data0);

	data0 = p_sys->rawaddr0;
	data1 = p_sys->rawaddr1;
	data2 = p_sys->rawaddr2;
	data3 = p_sys->rawaddr3;
	Gem_write ((GEM_SYS_BASE+0x1c), data0);
	Gem_write ((GEM_SYS_BASE+0x20), data1);
	Gem_write ((GEM_SYS_BASE+0x24), data2);
	Gem_write ((GEM_SYS_BASE+0x28), data3);

	data0 = p_sys->yaddr0;
	data1 = p_sys->yaddr1;
	data2 = p_sys->yaddr2;
	data3 = p_sys->yaddr3;
	// YUV ��ַ  Y
	Gem_write ((GEM_SYS_BASE+0x2c), data0);
	Gem_write ((GEM_SYS_BASE+0x30), data1);
	Gem_write ((GEM_SYS_BASE+0x34), data2);
	Gem_write ((GEM_SYS_BASE+0x38), data3);

	data0 = p_sys->uaddr0;
	data1 = p_sys->uaddr1;
	data2 = p_sys->uaddr2;
	data3 = p_sys->uaddr3;
	Gem_write ((GEM_SYS_BASE+0x3c), data0);
	Gem_write ((GEM_SYS_BASE+0x40), data1);
	Gem_write ((GEM_SYS_BASE+0x44), data2);
	Gem_write ((GEM_SYS_BASE+0x48), data3);

	data0 = p_sys->vaddr0;
	data1 = p_sys->vaddr1;
	data2 = p_sys->vaddr2;
	data3 = p_sys->vaddr3;
	Gem_write ((GEM_SYS_BASE+0x4c), data0);
	Gem_write ((GEM_SYS_BASE+0x50), data1);
	Gem_write ((GEM_SYS_BASE+0x54), data2);
	Gem_write ((GEM_SYS_BASE+0x58), data3);

  	if (p_sys->vmanSftset == 1)
	{
		// �����ID�ţ� �����λ������һ��
      // ����һ�������ź�
		data0 = (0x00);
		Gem_write ((GEM_MSK_BASE+0x00), data0);
	}

	if (p_sys->vchkIntclr == 1)// ֡ͷ ͬ�� �ж� ���
	{//
		data0 = (0x01);
		Gem_write ((GEM_MSK_BASE+0x00), data0);
	}

  if (p_sys->pabtIntclr == 1)
  {
    data0 = (0x02);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->fendIntset == 1)
  {
    for (i = 0; i < 4; i++)
    {
      data0 = (0x03) | (p_sys->fendIntid[i]<<16);
      Gem_write ((GEM_MSK_BASE+0x00), data0);
    }
  }

  if (p_sys->fendIntclr == 1)
  {
    data0 = (0x04);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->fabtIntclr == 1)
  {
    data0 = (0x05);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->babtIntclr == 1)
  {
    data0 = (0x06);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->infoStaclr == 1)
  {
    data0 = (0x07);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->ffiqIntclr == 1)
  {
    data0 = (0x08);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->pendIntclr == 1)
  {
    data0 = (0x09);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if(p_sys->ispenbale)
  {
		//isp_dump_sys_register ();
  }

  data0 =  Gem_read ((GEM_SYS_BASE+0x00));
  data1 = data0 | p_sys->ispenbale;
  Gem_write ((GEM_SYS_BASE+0x00), data1);

}

#ifdef ISP_DEBUG
void isp_dump_sys_register (void)
{
	int i;
	for (i = 0; i <= 0x64; i += 4)
	{
		XM_printf ("GEM_SYS_BASE+0x%02x = 0x%08x\n", i, Gem_read(GEM_SYS_BASE+i));
	}
}
#endif


unsigned int isp_sys_status_read(isp_sys_ptr_t p_sys)
{
  unsigned int isp_status;

  // read mask status
  isp_status = Gem_read (GEM_STS_BASE+0x00);
  //isp_status = rISP_INT_STATUS;

  //1. bit.0:֡��ʼ�жϣ�֡ͷ
  p_sys->vchkIntmsk  = isp_status & 0x01;  //(isp_status<<31) >> 31;
  //2. bit.1:pclk buffer ���ˣ�sensor �����Ѿ����޷�����
  p_sys->pabtIntmsk = (isp_status >> 1) & 0x01; //(isp_status<<30) >> 30;

  //3. bit.2:֡����ж�
  p_sys->fendIntmsk = (isp_status >> 2) & 0x01; //(isp_status<<29) >> 29;
  //4. bit.3:��ַ�쳣 ����ַ ������� ����0
  p_sys->fabtIntmsk = (isp_status >> 3) & 0x01; //(isp_status<<28) >> 28;

  //5. bit.4:���ߴ����쳣��3D���������޷�ͬ����ʱ��ϲ���
 p_sys->babtIntmsk = (isp_status >> 4) & 0x01; //(isp_status<<27) >> 27;
  //6. bit.5:16�� Ϊ��λ��slice�ж�
  p_sys->ffiqIntmsk = (isp_status >> 5) & 0x01;
  //7. bit.6:��ֹ �ж�
  p_sys->pendIntmsk = (isp_status >> 6) & 0x01;
  // ֡���кţ�ȡֵ��0 1 2 3
  p_sys->fendStaid = (isp_status >> 8) & 0x03; //(isp_status<<26) >> 26;

  if(!p_sys->infoIntenable)	// ���жϷ�ʽ(poll)
  {
	  // RAW Interrupt Factor
	  p_sys->infoStadone = ( Gem_read (GEM_STS_BASE+0x04) >> 7) & 0x01;
  }

  //16-23λ���ж�״̬������ ������8λ ����ʾÿ��Ҫ��ɵ���������Ҫ����16��

  // read raw status
  isp_status = Gem_read (GEM_STS_BASE+0x04);


  return isp_status;
}

void isp_sys_infosync_clr (void)
{
   unsigned int infosync_clr;

   infosync_clr = (0x07);
   Gem_write ((GEM_MSK_BASE+0x00), infosync_clr);

}

void isp_sys_set_frame_ready (unsigned int frame_id)
{
	unsigned int frame_set = (0x03) | ((frame_id & 0x03) << 16);
	//XM_printf ("push %d\n", frame_id);
	Gem_write ((GEM_MSK_BASE+0x00), frame_set);
}


extern isp_sys_t p_sys;
extern isp_denoise_t p_denoise;
extern isp_awb_t p_awb;
extern isp_ae_t p_ae;


//#define	UPDATE_AWB_BLACK_USE_ISR

#ifdef USE_ISR_TO_SWITCH_MODE
extern volatile unsigned int new_isp_mode;
#endif

//#define	isp_debug_printf

extern void isp_histogram_bands_data (unsigned int data1, unsigned int data2);
extern void isp_ae_yavg_s_read (isp_ae_ptr_t p_ae);

static int isp_pixel_clock_abnormal_count = 0;

void isp_sys_infomask_clr (void)
{
   unsigned int infomaskclr;

   infomaskclr = (0x07);
   Gem_write ((GEM_MSK_BASE+0x00), infomaskclr);

}

void isp_enable (void)
{
	unsigned int data;
  printk(KERN_ALERT "%s %d.\n", __FUNCTION__, __LINE__);
	data =  Gem_read ((GEM_SYS_BASE+0x00));
	data |= 0x01;
	Gem_write ((GEM_SYS_BASE+0x00), data);
}

void isp_disable (void)
{
	unsigned int data;
	data =  Gem_read ((GEM_SYS_BASE+0x00));
	data &= ~0x01;
	Gem_write ((GEM_SYS_BASE+0x00), data);
}


