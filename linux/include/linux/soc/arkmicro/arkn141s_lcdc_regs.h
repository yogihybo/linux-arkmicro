/*
 *  Header file for ARKN141S LCD Controller
 *
 */

#ifndef __ARKN141S_LCDC_REGS_H__
#define __ARKN141S_LCDC_REGS_H__


#define ARKN141S_LCDC_DMABADDR1						0x00
#define ARKN141S_LCDC_DMABADDR2						0x04
#define ARKN141S_LCDC_DMAFRMPT1						0x08
#define ARKN141S_LCDC_DMAFRMPT2						0x0c
#define ARKN141S_LCDC_DMAFRMADD1					0x10
#define ARKN141S_LCDC_DMAFRMADD2					0x14

#define ARKN141S_LCDC_HEIGHT_OFFSET					12

/* LCD */
#define ARKN141S_LCDC_EANBLE									0x000
#define ARKN141S_LCDC_CONTROL									0x004
#define ARKN141S_LCDC_OSD1_EN_OFFSET					        7
#define ARKN141S_LCDC_TIMING0									0x008
#define	ARKN141S_LCDC_HFP							            (0x3ffU <<  0)
#define	ARKN141S_LCDC_HBP_OFFSET						        10
#define	ARKN141S_LCDC_HBP							            (0x3ffU <<  ARKN141S_LCDC_HBP_OFFSET)
#define	ARKN141S_LCDC_HPW_OFFSET						        20
#define	ARKN141S_LCDC_HPW							            (0x3ffU << ARKN141S_LCDC_HPW_OFFSET)
#define ARKN141S_LCDC_TIMING1									0x00c
#define	ARKN141S_LCDC_VPW_OFFSET						        13
#define	ARKN141S_LCDC_VPW							            (0x3fU <<  ARKN141S_LCDC_VPW_OFFSET)
#define	ARKN141S_LCDC_VFP_OFFSET						        19
#define	ARKN141S_LCDC_VFP							            (0x3ffU <<  ARKN141S_LCDC_VFP_OFFSET)
#define ARKN141S_LCDC_TIMING2									0x010
#define	ARKN141S_LCDC_VBP							            (0x3ffU << 0)
#define ARKN141S_LCDC_IOE_OFFSET						        23
#define ARKN141S_LCDC_IHS_OFFSET						        22
#define ARKN141S_LCDC_IVS_OFFSET						        21
#define ARKN141S_LCDC_LPS_OFFSET						        10
#define ARKN141S_LCDC_TIMING3									0x014
#define ARKN141S_LCDC_TIMING_FRAME_START_CNT_LCD				0x018
#define ARKN141S_LCDC_BACK_COLOR								0x01C
#define ARKN141S_LCDC_BLD_MODE_LCD_REG0						    0x020
#define ARKN141S_LCDC_BLD_MODE_LCD_REG1							0x024
#define ARKN141S_LCDC_BLEND_POST_CTL  							0x028
#define ARKN141S_LCDC_ITU_CONTROL                               0x02C
#define ARKN141S_LCDC_ITU_SRGB_GENERATION_CTL					0x030
#define ARKN141S_LCDC_ITU_TIMIING_REFERENCE_CODE_DEFINE			0x034//new
#define ARKN141S_LCDC_YCLCD_ITU_TIMING_REFERENCE_CODE_DEFINE	0x038//new
#define ARKN141S_LCDC_DITHERING									0x03C//new
#define ARKN141S_LCDC_DITHERING_V_H_SIZE						0x040//new
#define ARKN141S_LCDC_DITHERING_TEST							0x044//new


#define ARKN141S_LCDC_TV_CONTROL							    0x060
#define ARKN141S_LCDC_TIMING0_TV								0x064
#define ARKN141S_LCDC_TIMING1_TV								0x068
#define ARKN141S_LCDC_TIMING2_TV								0x06c
#define ARKN141S_LCDC_TIMING3_TV								0x070
#define ARKN141S_LCDC_TIMING_FRAME_START_CNT_TV					0x074
#define ARKN141S_LCDC_BACK_COLOR_TV								0x078
#define ARKN141S_LCDC_BLD_MODE_TV_REG0							0x07C
#define ARKN141S_LCDC_BLD_MODE_TV_REG1							0x080

//TV Encoder param
#define ARKN141S_LCDC_TV_PARAM_REG0                             0x084
#define ARKN141S_LCDC_TV_PARAM_REG1                             0x088
#define ARKN141S_LCDC_TV_PARAM_REG2                             0x08c
#define ARKN141S_LCDC_TV_PARAM_REG3                             0x090
#define ARKN141S_LCDC_TV_PARAM_REG4                             0x094
#define ARKN141S_LCDC_TV_PARAM_REG5                             0x098
#define ARKN141S_LCDC_TV_PARAM_REG6                             0x09C
#define ARKN141S_LCDC_TV_PARAM_REG7                             0x0A0
#define ARKN141S_LCDC_TV_PARAM_REG8                             0x0A4
#define ARKN141S_LCDC_TV_PARAM_REG9                             0x0A8
#define ARKN141S_LCDC_TV_PARAM_REG10                            0x0AC
#define ARKN141S_LCDC_TV_PARAM_REG11                            0x0B0
#define ARKN141S_LCDC_TV_PARAM_REG12                            0x0B4
#define ARKN141S_LCDC_TV_PARAM_REG13                            0x0B8
#define ARKN141S_LCDC_TV_PARAM_REG14                            0x0BC
#define ARKN141S_LCDC_TV_PARAM_REG15                            0x0C0
#define ARKN141S_LCDC_TV_PARAM_REG16                            0x0C4
#define ARKN141S_LCDC_TV_PARAM_REG17                            0x0C8
#define ARKN141S_LCDC_TV_PARAM_REG18                            0x0CC
#define ARKN141S_LCDC_TV_PARAM_REG19                            0x0D0
#define ARKN141S_LCDC_TV_PARAM_REG20                            0x0D4
#define ARKN141S_LCDC_TV_PARAM_REG21                            0x0D8


#define ARKN141S_LCDC_VIDEO1_BURST_CTL							0x100
#define ARKN141S_LCDC_VIDEO1_CTL							0x104
#define ARKN141S_LCDC_VIDEO1_ALPHA1_ALPHA0_BLENDING_COEFF				0x108
#define ARKN141S_LCDC_VIDEO1_SOURCE_SIZE						0x10c
#define ARKN141S_LCDC_VIDEO1_WIN_SIZE							0x110
#define ARKN141S_LCDC_VIDEO1_SIZE							0x114
#define ARKN141S_LCDC_VIDEO1_WIN_POINT							0x118
#define ARKN141S_LCDC_VIDEO1_POSITION							0x11c
#define ARKN141S_LCDC_VIDEO1_ADDR1							0x120
#define ARKN141S_LCDC_VIDEO1_ADDR2							0x124
#define ARKN141S_LCDC_VIDEO1_ADDR3							0x128
#define ARKN141S_LCDC_VIDEO1_ADDR1_GROUP1						0x12c
#define ARKN141S_LCDC_VIDEO1_ADDR2_GROUP1						0x130
#define ARKN141S_LCDC_VIDEO1_ADDR3_GROUP1						0x134
#define ARKN141S_LCDC_BLD_CUT_LEFT_RIGHT_VIDEO1						0x138
#define ARKN141S_LCDC_BLD_CUT_UP_DOWN_VIDEO1						0x13C

#define ARKN141S_LCDC_COLOR_KEY_MASK_VALUE_VIDEO1					0x140
#define ARKN141S_LCDC_COLOR_KEY_MASK_THLD_VIDEO1					0x144
#define ARKN141S_LCDC_VIDEO1_RIGHT_BOTTOM_CUT_NUM					0x148
#define ARKN141S_LCDC_VIDEO1_SCALE_VXMOD						0x14C
#define ARKN141S_LCDC_VIDEO1_SCALE_CTL							0x150
#define ARKN141S_LCDC_VIDEO1_SCAL_CTL0							0x154
#define ARKN141S_LCDC_VIDEO1_SCAL_CTL1							0x158
#define ARKN141S_LCDC_VIDEO1_SCAL_CTL2							0x15C
#define ARKN141S_LCDC_VIDEO1_SCAL_CTL3							0x160
#define ARKN141S_LCDC_VIDEO1_SCAL_CTL4							0x164
#define ARKN141S_LCDC_VIDEO1_HSCAL_COS_VALUE				    		0x168

#define ARKN141S_LCDC_VIDEO2_BURST_CTL							0x180
#define ARKN141S_LCDC_VIDEO2_CTL							0x184
#define ARKN141S_LCDC_VIDEO2_ALPHA1_ALPHA0_BLENDING_COEFF				0x188//NEW
#define ARKN141S_LCDC_VIDEO2_SOURCE_SIZE						0x18c
#define ARKN141S_LCDC_VIDEO2_WIN_POINT							0x190
#define ARKN141S_LCDC_VIDEO2_WIN_SIZE							0x194
#define ARKN141S_LCDC_VIDEO2_SIZE							0x198
#define ARKN141S_LCDC_VIDEO2_POSITION							0x19c
#define ARKN141S_LCDC_VIDEO2_ADDR1							0x1A0
#define ARKN141S_LCDC_VIDEO2_ADDR2							0x1A4
#define ARKN141S_LCDC_VIDEO2_ADDR3							0x1A8
#define ARKN141S_LCDC_VIDEO2_ADDR1_GROUP1						0x1AC
#define ARKN141S_LCDC_VIDEO2_ADDR2_GROUP1						0x1B0
#define ARKN141S_LCDC_VIDEO2_ADDR3_GROUP1						0x1B4
#define ARKN141S_LCDC_BLD_CUT_LEFT_RIGHT_VIDEO2						0x1B8
#define ARKN141S_LCDC_BLD_CUT_UP_DOWN_VIDEO2						0x1BC
#define ARKN141S_LCDC_COLOR_KEY_MASK_VALUE_VIDEO2					0x1C0
#define ARKN141S_LCDC_COLOR_KEY_MASK_THLD_VIDEO2					0x1C4

#define ARKN141S_LCDC_OSD1_BURST_CTL							0x1E0
#define ARKN141S_LCDC_OSD1_CTL								0x1E4
#define ARKN141S_LCDC_ALPHA1_0_OSD1							0x1E8
#define ARKN141S_LCDC_OSD1_SOURCE_SIZE							0x1EC
#define ARKN141S_LCDC_OSD1_SIZE								0x1F0
#define ARKN141S_LCDC_OSD1_WIN_POINT							0x1F4
#define ARKN141S_LCDC_OSD1_POSITION							0x1F8
#define ARKN141S_LCDC_OSD1_ADDR								0x1FC
#define ARKN141S_LCDC_OSD1_ADDR_GROUP1							0x200
#define ARKN141S_LCDC_BLD_CUT_LEFT_RIGHT_OSD1						0x204
#define ARKN141S_LCDC_BLD_CUT_UP_DOWN_OSD1						0x208
#define ARKN141S_LCDC_COLOR_KEY_MASK_VALUE_OSD1						0x20c
#define ARKN141S_LCDC_COLOR_KEY_MASK_THLD_OSD1						0x210


#define ARKN141S_LCDC_OSD2_BURST_CTL							0x230
#define ARKN141S_LCDC_OSD2_CTL								0x234
#define ARKN141S_LCDC_ALPHA1_0_OSD2							0x238
#define ARKN141S_LCDC_OSD2_SOURCE_SIZE							0x23C
#define ARKN141S_LCDC_OSD2_SIZE								0x240
#define ARKN141S_LCDC_OSD2_WIN_POINT							0x244
#define ARKN141S_LCDC_OSD2_POSITION							0x248
#define ARKN141S_LCDC_OSD2_ADDR								0x24C
#define ARKN141S_LCDC_OSD2_ADDR_GROUP1							0x250
#define ARKN141S_LCDC_BLD_CUT_LEFT_RIGHT_OSD2						0x254
#define ARKN141S_LCDC_BLD_CUT_UP_DOWN_OSD2						0x258
#define ARKN141S_LCDC_COLOR_KEY_MASK_VALUE_OSD2						0x25c
#define ARKN141S_LCDC_COLOR_KEY_MASK_THLD_OSD2						0x260


#define ARKN141S_LCDC_OSD3_BURST_CTL							0x280
#define ARKN141S_LCDC_OSD3_CTL								0x284
#define ARKN141S_LCDC_ALPHA1_0_OSD3							0x288
#define ARKN141S_LCDC_OSD3_SOURCE_SIZE							0x28C
#define ARKN141S_LCDC_OSD3_SIZE								0x290
#define ARKN141S_LCDC_OSD3_WIN_POINT							0x294
#define ARKN141S_LCDC_OSD3_POSITION							0x298
#define ARKN141S_LCDC_OSD3_ADDR								0x29C
#define ARKN141S_LCDC_OSD3_ADDR_GROUP1							0x2A0
#define ARKN141S_LCDC_BLD_CUT_LEFT_RIGHT_OSD3						0x2A4
#define ARKN141S_LCDC_BLD_CUT_UP_DOWN_OSD3						0x2A8
#define ARKN141S_LCDC_COLOR_KEY_MASK_VALUE_OSD3						0x2Ac
#define ARKN141S_LCDC_COLOR_KEY_MASK_THLD_OSD3						0x2B0

#define ARKN141S_LCDC_TIMING_INIT                               0x2CC
#define ARKN141S_LCDC_INTERRUPT_CTL                             0x2D0
#define ARKN141S_LCDC_INTERRUPT_STATUS                          0x2D4
#define ARKN141S_LCDC_INT_LCD_FRAME  				            (1 << 0)
#define ARKN141S_LCDC_PARAMTERS_SYNC_SWITCH                     0x2D8//new

#define ARKN141S_LCDC_GAMMA_REG_0                               0x2Dc
#define ARKN141S_LCDC_GAMMA_REG_1                               0x2E0
#define ARKN141S_LCDC_GAMMA_REG_2                               0x2E4
#define ARKN141S_LCDC_GAMMA_REG_3                               0x2E8
#define ARKN141S_LCDC_GAMMA_REG_4                               0x2Ec
#define ARKN141S_LCDC_GAMMA_REG_5                               0x2F0
#define ARKN141S_LCDC_GAMMA_REG_6                               0x2F4
#define ARKN141S_LCDC_GAMMA_REG_7                               0x2F8
#define ARKN141S_LCDC_GAMMA_REG_8                               0x2Fc
#define ARKN141S_LCDC_GAMMA_REG_9                               0x300
#define ARKN141S_LCDC_GAMMA_REG_10                              0x304
#define ARKN141S_LCDC_GAMMA_REG_11                              0x308
#define ARKN141S_LCDC_GAMMA_REG_12                              0x30c
#define ARKN141S_LCDC_GAMMA_REG_13                              0x310
#define ARKN141S_LCDC_GAMMA_REG_14                              0x314
#define ARKN141S_LCDC_GAMMA_REG_15                              0x318
#define ARKN141S_LCDC_GAMMA_REG_16                              0x31c
#define ARKN141S_LCDC_GAMMA_REG_17                              0x320
#define ARKN141S_LCDC_GAMMA_REG_18                              0x324
#define ARKN141S_LCDC_GAMMA_REG_19                              0x328
#define ARKN141S_LCDC_GAMMA_REG_20                              0x32c
#define ARKN141S_LCDC_GAMMA_REG_21                              0x330
#define ARKN141S_LCDC_GAMMA_REG_22                              0x334
#define ARKN141S_LCDC_GAMMA_REG_23                              0x338
#define ARKN141S_LCDC_GAMMA_REG_24                              0x33c
#define ARKN141S_LCDC_GAMMA_REG_25                              0x340
#define ARKN141S_LCDC_GAMMA_REG_26                              0x344
#define ARKN141S_LCDC_GAMMA_REG_27                              0x348
#define ARKN141S_LCDC_GAMMA_REG_28                              0x34c
#define ARKN141S_LCDC_GAMMA_REG_29                              0x350
#define ARKN141S_LCDC_GAMMA_REG_30                              0x354
#define ARKN141S_LCDC_GAMMA_REG_31                              0x358
#define ARKN141S_LCDC_GAMMA_REG_32                              0x35c
#define ARKN141S_LCDC_GAMMA_REG_33                              0x360
#define ARKN141S_LCDC_GAMMA_REG_34                              0x364
#define ARKN141S_LCDC_GAMMA_REG_35                              0x368
#define ARKN141S_LCDC_GAMMA_REG_36                              0x36c
#define ARKN141S_LCDC_GAMMA_REG_37                              0x370
#define ARKN141S_LCDC_GAMMA_REG_38                              0x374
#define ARKN141S_LCDC_GAMMA_REG_39                              0x378
#define ARKN141S_LCDC_GAMMA_REG_40                              0x37c
#define ARKN141S_LCDC_GAMMA_REG_41                              0x380
#define ARKN141S_LCDC_GAMMA_REG_42                              0x384
#define ARKN141S_LCDC_GAMMA_REG_43                              0x388
#define ARKN141S_LCDC_GAMMA_REG_44                              0x38c
#define ARKN141S_LCDC_GAMMA_REG_45                              0x390
#define ARKN141S_LCDC_GAMMA_REG_46                              0x394
#define ARKN141S_LCDC_GAMMA_REG_47                              0x398
#define ARKN141S_LCDC_GAMMA_REG_48                              0x39c



#define ARKN141S_LCDC_TV_GAMMA_REG_0                            0x3A0
#define ARKN141S_LCDC_TV_GAMMA_REG_1                            0x3A4
#define ARKN141S_LCDC_TV_GAMMA_REG_2                            0x3A8
#define ARKN141S_LCDC_TV_GAMMA_REG_3                            0x3AC
#define ARKN141S_LCDC_TV_GAMMA_REG_4                            0x3B0
#define ARKN141S_LCDC_TV_GAMMA_REG_5                            0x3B4
#define ARKN141S_LCDC_TV_GAMMA_REG_6                            0x3B8
#define ARKN141S_LCDC_TV_GAMMA_REG_7                            0x3BC
#define ARKN141S_LCDC_TV_GAMMA_REG_8                            0x3C0
#define ARKN141S_LCDC_TV_GAMMA_REG_9                            0x3C4
#define ARKN141S_LCDC_TV_GAMMA_REG_10                           0x3C8
#define ARKN141S_LCDC_TV_GAMMA_REG_11                           0x3CC
#define ARKN141S_LCDC_TV_GAMMA_REG_12                           0x3D0
#define ARKN141S_LCDC_TV_GAMMA_REG_13                           0x3D4
#define ARKN141S_LCDC_TV_GAMMA_REG_14                           0x3D8
#define ARKN141S_LCDC_TV_GAMMA_REG_15                           0x3DC
#define ARKN141S_LCDC_TV_GAMMA_REG_16                           0x3E0
#define ARKN141S_LCDC_TV_GAMMA_REG_17                           0x3E4
#define ARKN141S_LCDC_TV_GAMMA_REG_18                           0x3E8
#define ARKN141S_LCDC_TV_GAMMA_REG_19                           0x3EC
#define ARKN141S_LCDC_TV_GAMMA_REG_20                           0x3F0
#define ARKN141S_LCDC_TV_GAMMA_REG_21                           0x3F4
#define ARKN141S_LCDC_TV_GAMMA_REG_22                           0x3F8
#define ARKN141S_LCDC_TV_GAMMA_REG_23                           0x3FC
#define ARKN141S_LCDC_TV_GAMMA_REG_24                           0x400
#define ARKN141S_LCDC_TV_GAMMA_REG_25                           0x404
#define ARKN141S_LCDC_TV_GAMMA_REG_26                           0x408
#define ARKN141S_LCDC_TV_GAMMA_REG_27                           0x40C
#define ARKN141S_LCDC_TV_GAMMA_REG_28                           0x410
#define ARKN141S_LCDC_TV_GAMMA_REG_29                           0x414
#define ARKN141S_LCDC_TV_GAMMA_REG_30                           0x418
#define ARKN141S_LCDC_TV_GAMMA_REG_31                           0x41C
#define ARKN141S_LCDC_TV_GAMMA_REG_32                           0x420
#define ARKN141S_LCDC_TV_GAMMA_REG_33                           0x424
#define ARKN141S_LCDC_TV_GAMMA_REG_34                           0x428
#define ARKN141S_LCDC_TV_GAMMA_REG_35                           0x42C
#define ARKN141S_LCDC_TV_GAMMA_REG_36                           0x430
#define ARKN141S_LCDC_TV_GAMMA_REG_37                           0x434
#define ARKN141S_LCDC_TV_GAMMA_REG_38                           0x438
#define ARKN141S_LCDC_TV_GAMMA_REG_39                           0x43C
#define ARKN141S_LCDC_TV_GAMMA_REG_40                           0x440
#define ARKN141S_LCDC_TV_GAMMA_REG_41                           0x444
#define ARKN141S_LCDC_TV_GAMMA_REG_42                           0x448
#define ARKN141S_LCDC_TV_GAMMA_REG_43                           0x44C
#define ARKN141S_LCDC_TV_GAMMA_REG_44                           0x450
#define ARKN141S_LCDC_TV_GAMMA_REG_45                           0x454
#define ARKN141S_LCDC_TV_GAMMA_REG_46                           0x458
#define ARKN141S_LCDC_TV_GAMMA_REG_47                           0x45C
#define ARKN141S_LCDC_TV_GAMMA_REG_48                           0x460


#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_0                  0x464
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_1                  0x468
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_2                  0x46C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_3                  0x470
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_4                  0x474
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_5                  0x478
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_6                  0x47C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_7                  0x480
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_8                  0x484
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_9                  0x488
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_10                 0x48C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_11                 0x490
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_12                 0x494
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_13                 0x498
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_14                 0x49C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_15                 0x4A0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_16                 0x4A4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_17                 0x4A8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_18                 0x4AC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_19                 0x4B0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_20                 0x4B4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_21                 0x4B8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_22                 0x4BC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_23                 0x4C0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_24                 0x4C4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_25                 0x4C8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_26                 0x4CC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_27                 0x4D0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_28                 0x4D4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_29                 0x4D8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_30                 0x4DC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_31                 0x4E0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_32                 0x4E4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_33                 0x4E8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_34                 0x4EC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_35                 0x4F0
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_36                 0x4F4
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_37                 0x4F8
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_38                 0x4FC
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_39                 0x500
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_40                 0x504
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_41                 0x508
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_42                 0x50C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_43                 0x510
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_44                 0x514
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_45                 0x518
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_46                 0x51C
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_47                 0x520
#define ARKN141S_LCDC_VIDEO1_LAYER_GAMMA_REG_48                 0x524

/*
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_0                  0x528
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_1                  0x52C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_2                  0x530
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_3                  0x534
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_4                  0x538
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_5                  0x53C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_6                  0x540
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_7                  0x544
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_8                  0x548
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_9                  0x54C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_10                 0x550
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_11                 0x554
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_12                 0x558
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_13                 0x55C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_14                 0x560
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_15                 0x564
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_16                 0x568
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_17                 0x56C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_18                 0x570
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_19                 0x574
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_20                 0x578
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_21                 0x57C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_22                 0x580
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_23                 0x584
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_24                 0x588
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_25                 0x58C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_26                 0x590
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_27                 0x594
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_28                 0x598
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_29                 0x59C
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_30                 0x5A0
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_31                 0x5A4
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_32                 0x5A8
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_33                 0x5AC
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_34                 0x5B0
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_35                 0x5B4
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_36                 0x5B8
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_37                 0x5BC
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_38                 0x5C0
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_39                 0x5C4
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_40                 0x5C8
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_41                 0x5CC
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_42                 0x5D0
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_43                 0x5D4
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_44                 0x5DC
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_45                 0x5E8
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_46                 0x5E0
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_47                 0x5E4
#define ARKN141S_LCDC_VIDEO2_LAYER_GAMMA_REG_48                 0x5E8
*/
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG0                    0x5EC
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG1                    0x5F0
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG2                    0x5F4
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG3                    0x5F8
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG4                    0x5FF
#define ARKN141S_LCDC_LCD_COLOUR_MATRIX_REG5                    0x600

#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG0                     0x604
#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG1                     0x608
#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG2                     0x60C
#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG3                     0x610
#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG4                     0x614
#define ARKN141S_LCDC_TV_COLOUR_MATRIX_REG5                     0x618

#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG0                 0x61C
#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG1                 0x620
#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG2                 0x624
#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG3                 0x628
#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG4                 0x62C
#define ARKN141S_LCDC_VIDEO1_COLOUR_MATRIX_REG5                 0x630
/*
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG0                 0x634
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG1                 0x638
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG2                 0x63C
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG3                 0x640
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG4                 0x644
#define ARKN141S_LCDC_VIDEO2_COLOUR_MATRIX_REG5                 0x648
*/
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG0                   0x64C
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG1                   0x650
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG2                   0x654
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG3                   0x658
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG4                   0x65C
#define ARKN141S_LCDC_OSD1_COLOUR_MATRIX_REG5                   0x660
/*
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG0                   0x664
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG1                   0x668
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG2                   0x66C
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG3                   0x670
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG4                   0x674
#define ARKN141S_LCDC_OSD2_COLOUR_MATRIX_REG5                   0x678

#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG0                   0x67C
#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG1                   0x680
#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG2                   0x684
#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG3                   0x688
#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG4                   0x68C
#define ARKN141S_LCDC_OSD3_COLOUR_MATRIX_REG5                   0x690
*/
#endif /* __ARKN141S_LCDC_REGS_H__ */
