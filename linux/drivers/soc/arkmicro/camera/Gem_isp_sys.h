// =============================================================================
// File        : Gem_isp_sys.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_SYS_H_
#define  _GEM_ISP_SYS_H_

#include "Gem_isp.h"

#define	USE_ISR_TO_SWITCH_MODE

#define GEM_SYS_BASE  			(0x000)
#define GEM_MSK_BASE  			(0x05c)


// �µ�ERISֱ��ͼ�汾
#define GEM_STS_BASE  (0x1a8 + 0x0c + 0x0c)	// ����3���Ĵ���

#define GEM_VIFRASE_BASE		(0x6d * 4)		// ���֡��

typedef struct isp_sys_
{
   unsigned char ispenbale;  // 1 bit
   unsigned char ckpolar;    // 1 bit
   unsigned char vcpolar;    // 1 bit
   unsigned char hcpolar;    // 1 bit
   unsigned char vmskenable; // 1 bit, ����, ��������Ϊ0
   unsigned char frameratei; // 8 bit
   unsigned char framerateo; // 8 bit, ����, ��������Ϊ0

	unsigned int vifrasel0;	 // 32bit, ���֡���п���, ���ɶ���64֡
	unsigned int vifrasel1;

	unsigned char resizebit;	  // bit16-bit17, Raw data resize (0:normal 1:cut 1bit 2: cut 2 bit 3: cut 3 bit)
   unsigned char sensorbit;   // 2 bit
   unsigned char bayermode;   // 2 bit
   unsigned char sonyif;
   unsigned int sonysac1;
   unsigned int sonysac2;
   unsigned int sonysac3;
   unsigned int sonysac4;
   unsigned int imagewidth; // 16 bit
   unsigned int imageheight; // 16 bit
   unsigned int imagehblank; // 16 bit
   unsigned int zonestridex; // 16 bit
   unsigned int zonestridey;  // 16 bit
   unsigned char vmanSftenable;
   unsigned char vchkIntenable;
   unsigned char pabtIntenable;
   unsigned char fendIntenable;
   unsigned char fabtIntenable;
   unsigned char babtIntenable;
   unsigned char ffiqIntenable;
   unsigned char pendIntenable;
	unsigned char infoIntenable;		// "�ع�״̬��Ϣ�����(��ʾ���Զ�ȡAE״̬��Ϣ)"�ж�����λ,
												//	ISP�ڲ����ع�ͳ�������, �ⲿ�����ʱ������ȷ��ȡAE��Ϣ, ִ����һ���ع⴦��

   unsigned char vmanSftset;
   unsigned char vchkIntclr;
   unsigned char pabtIntclr;
   unsigned char fendIntset;
   unsigned char fendIntclr;
   unsigned char fabtIntclr;
   unsigned char babtIntclr;
   unsigned char ffiqIntclr;
   unsigned char pendIntclr;
   unsigned char infoStaclr;  			// ���ISP���ع�ͳ����ɱ�ʶ

   unsigned char vchkIntraw;
   unsigned char pabtIntraw;
   unsigned char fendIntraw;
   unsigned char fabtIntraw;
   unsigned char babtIntraw;
   unsigned char ffiqIntraw;
   unsigned char pendIntraw;
	unsigned char infoIntraw;

   unsigned char vchkIntmsk;
   unsigned char pabtIntmsk;
   unsigned char fendIntmsk;
   unsigned char fabtIntmsk;
   unsigned char babtIntmsk;
   unsigned char ffiqIntmsk;
   unsigned char pendIntmsk;
	unsigned char infoIntmsk;
   unsigned char preserve0;
   unsigned char preserve1;

   unsigned char fendIntid[4];
   unsigned int ffiqIntdelay;
   unsigned char fendStaid;
   unsigned char infoStadone;

   unsigned char debugmode;  	 // 1 bit, 0 �رյ���ģʽ,     1 �������ģʽ
   unsigned char testenable;   // 1 bit, 0 �ر�dram����ģʽ, 1 ����dram����ģʽ
   unsigned char rawmenable;   // 1 bit, 0: �ر�RAW�������  1: ����RAW�������
   unsigned char yuvenable;    // 1 bit, 0: �ر�YUV�������  1: ����YUV�������
   unsigned char refenable;    // 1 bit���ر�3D����ʱ��Ӧͬʱdisable�ο�֡�� ����ISP�������д�ο�֡��
   unsigned char yuvformat;  	 // 2 bit
   unsigned char dmalock;      // 2 bit, �����ᵼ��H264 codecʱ������
										 //		2:        ʹ��,  ��ȡЧ�ʸ�
										 //		������ֵ: �ر�
   unsigned int hstride;       // 16 bit
   unsigned int refaddr;       // 32 bit
   unsigned int rawaddr0;  // 32 bit
   unsigned int rawaddr1;  // 32 bit
   unsigned int rawaddr2;  // 32 bit
   unsigned int rawaddr3;  // 32 bit
   unsigned int yaddr0;    // 32 bit
   unsigned int yaddr1;    // 32 bit
   unsigned int yaddr2;    // 32 bit
   unsigned int yaddr3;    // 32 bit
   unsigned int uaddr0;    // 32 bit
   unsigned int uaddr1;    // 32 bit
   unsigned int uaddr2;    // 32 bit
   unsigned int uaddr3;    // 32 bit
   unsigned int vaddr0;    // 32 bit
   unsigned int vaddr1;    // 32 bit
   unsigned int vaddr2;    // 32 bit
   unsigned int vaddr3;    // 32 bit

   unsigned int frameno;    // 32 bit

	unsigned int isp_reset_request;		// ISP�쳣����Ҫ��λ����
} isp_sys_t;

typedef struct isp_sys_ *isp_sys_ptr_t;

void isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp);

void isp_sys_init_io (isp_sys_ptr_t p_sys);

unsigned int isp_sys_status_read(isp_sys_ptr_t p_sys);

unsigned int get_isp_framebufferno(void);


unsigned int return_isp_frame(void);

void Reset_isp_frame(void);

void isp_sys_infomask_clr (void);

void isp_disable (void);

void isp_enable (void);

void isp_dump_sys_register (void);

void isp_sys_set_frame_ready (unsigned int frame_id);

#endif