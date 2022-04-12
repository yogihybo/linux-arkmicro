
#ifndef _ARK1668_ITU656_REGS_H_
#define _ARK1668_ITU656_REGS_H_



#define DELTA_LINE					20	
#define DELTA_PIX					10

// ENABLE_REG            0xE0800000 + 0x930
#define   WRITE_MEMORY_TWO_FIELD                 	(0<<14)    
#define   WRITE_MEMORY_SINGLE_FIELD             	(1<<14)   
#define   CBCR_YVYU            	                	(0<<13) 
#define   CBCR_YUYV                    			(1<<13) 
#define   FRAME_INTR_EVEN_FIELD                		(1<<12) 
#define   FRAME_INTR_ODD_FIELD                 		(0<<12) 
#define   H_FILTER_COEF_SOFTWARE                  	(0<<11) 
#define   H_FILTER_COEF_AUTO                   		(1<<11) 
#define   STORE_DATA_SINGLE_TWO_FIELD        		(0<<5)
#define   STORE_DATA_2FIELD_2ADDR                	(1<<5)
#define   STORE_DATA_FRAME                              (3<<5)
#define   CBCR_VYUY                                     (1<<4) 
#define   YCBCR444_422_FILTER_DISABLE          		(0<<3)
#define   YCBCR444_422_FILTER_ENABLE           		(1<<3)
#define   LOW_FILTER_DISABLE                            (0<<2)
#define   LOW_FILTER_ENABLE                             (1<<2)
#define   CR_FIRST                                      (0<<1)   
#define   CB_FIRST                                      (1<<1)   
#define   GLOBAL_DISABLE                                (0<<0)
#define   GLOBAL_ENABLE                                 (1<<0)

//INTERRUPT REGISTER      0xE0800000 + 0x12C
#define   EVEN_FIELD_INTERRUPT                          (1<<8)
#define   ACTIVE_LINE_CHANGED_INTERRUPT			(1<<7)
#define   TOTAL_LINE_CHANGED_INTERRUPT			(1<<6)
#define   ACTIVE_PIX_CHANGED_INTERRUPT			(1<<5)
#define   TOTAL_PIX_CHANGED_INTERRUPT			(1<<4)
#define   FRAME_INTERRUPT_INTERRUPT                     (1<<3)
#define   FIFO_ERROR_INTERRUPT			        (1<<2)
#define   PN_CHANGED_INTERRUPT			        (1<<1)
#define   FIELD_INTERRUPT			        (1<<0)

#define	DEINTERLACE_SUCCESS			        (0)
#define	DEINTERLACE_PARA_ERROR			        (-1)
#define	DEINTERLACE_AXI_ERROR			        (-2)
#define	DEINTERLACE_TIMEOUT			        (-3)

//flame status

enum {
	DEINTERLACE_LINE_SIZE_720H = 0,
	DEINTERLACE_LINE_SIZE_960H
};

enum {
	DEINTERLACE_DATA_MODE_420 = 0,
	DEINTERLACE_DATA_MODE_422
};

enum {
	DEINTERLACE_TYPE_PAL = 0,		// 576
	DEINTERLACE_TYPE_NTSC			// 480
};

enum {
	DEINTERLACE_FIELD_ODD = 0,
	DEINTERLACE_FIELD_EVEN
};

enum {
	NTSC_PAL = 0,
	FRAME_VALID,
	FRAME_USED,
	ODD_EVEN,
	FILE_MODE     //this bit set mean first is odd, odd-even-odd-even; or else first even, even-odd-even-odd
};


#define ARK1668_DEINTERLACE_START			        0x00 
#define ARK1668_DEINTERLACE_CTRL0			        0x04 
#define ARK1668_DEINTERLACE_CTRL1			        0x08 
#define ARK1668_DEINTERLACE_CTRL2			        0x0C
#define ARK1668_DEINTERLACE_CTRL3			        0x10 
#define ARK1668_DEINTERLACE_FILM_MODECTRL	 		0x14 
#define ARK1668_DEINTERLACE_SADDR0			        0x18
#define ARK1668_DEINTERLACE_SADDR1			        0x1C
#define ARK1668_DEINTERLACE_SADDR2			        0x20
#define ARK1668_DEINTERLACE_DADDRY			        0x24
#define ARK1668_DEINTERLACE_DADDRU			        0x28
#define ARK1668_DEINTERLACE_DADDRV			        0x2C
#define ARK1668_DEINTERLACE_SADDR0_1	     		        0x30
#define ARK1668_DEINTERLACE_SADDR1_1		 		0x34
#define ARK1668_DEINTERLACE_SADDR2_1		 		0x38
#define ARK1668_DEINTERLACE_DADDRY_1		 		0x3C
#define ARK1668_DEINTERLACE_DADDRU_1		 		0x40
#define ARK1668_DEINTERLACE_DADDRV_1		 		0x44
#define ARK1668_DEINTERLACE_INT_MASK		 		0x48
#define ARK1668_DEINTERLACE_RAW_INT			        0x4C
#define ARK1668_DEINTERLACE_INT_CLEAR		 		0x50
#define ARK1668_DEINTERLACE_STATUS			        0x54
#define ARK1668_DEINTERLACE_ADDR_SWITCHMODE	 		0x58


#define ARK1668_ITU656_MODULE_EN			        0x00
#define ARK1668_ITU656_IMR			                0x124
#define ARK1668_ITU656_ICR				        0x128
#define ARK1668_ITU656_ISR				        0x12C

#define ARK1668_ITU656_LINE_NUM_PER_FIELD    		        0x8f4
#define ARK1668_ITU656_PIX_NUM_PER_LINE      		        0x8f8 //read the reg 240 n, 288 pal
#define ARK1668_ITU656_PIX_LINE_NUM_DELTA			0x8fc

#define ARK1668_ITU656_INPUT_SEL			        0x900
#define ARK1668_ITU656_SEP_MODE_SEL			        0x904
#define ARK1668_ITU656_H_STRAT        				0x908
#define ARK1668_ITU656_H_END			                0x90C
#define ARK1668_ITU656_H_WIDTH        				0x910
#define ARK1668_ITU656_V_START_0			        0x914
#define ARK1668_ITU656_V_END_0        				0x918
#define ARK1668_ITU656_V_START_1			        0x91C
#define ARK1668_ITU656_V_END_1        				0x920
#define ARK1668_ITU656_V_FIELD_0			        0x924
#define ARK1668_ITU656_V_FIELD_1      				0x928
#define ARK1668_ITU656_P_N_DETECT			        0x92C
#define ARK1668_ITU656_ENABLE_REG		                0x930
#define ARK1668_ITU656_SIZE				        0x938
#define ARK1668_ITU656_TOTAL_PIX			        0x94C
#define ARK1668_ITU656_DRAM_DEST1     				0x950
#define ARK1668_ITU656_DRAM_DEST2     				0x954
#define ARK1668_ITU656_DATA_OUT_NUM   				0x958
#define ARK1668_ITU656_OUTLINE_NUM_PER_FIELD 		        0x95c

#endif

