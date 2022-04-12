/*
 * ark1668e_i2s.h
 *
 */

#ifndef __ARK1668E_I2S_H
#define __ARK1668E_I2S_H

#define SLAVE_ON		0
#define MASTER_ON	1

//struct ark1668e_i2s1_data_in{
//	int i2s1_data ;
//};

/*
 * I2S Controller Register and Bit Definitions
 */
#define I2S_SACR0		0x00  /* Global Control Register */
#define I2S_SACR1		0x04  /* Serial Audio I 2 S/MSB-Justified Control Register */
#define I2S_DACR0		0x08  /* Volume Control Register 0 */
#define I2S_DACR1		0x10  /* Volume Control Register 1 */
#define I2S_SASR0		0x0C  /* Serial Audio I 2 S/MSB-Justified Interface and FIFO Status Register */
#define I2S_SAIMR		0x14  /* Serial Audio Interrupt Mask Register */
#define I2S_SAICR		0x18  /* Serial Audio Interrupt Clear Register */
#define I2S_ADCR0		0x1C  /* ADC Contol Register */
#define I2S_SADR		0x80  /* Serial Audio Data Register (TX and RX FIFO access Register). */

#define SACR0_CH_ALIGH		(1 << 27)	/* channel align in 32bit mode */
#define SACR0_RLFIRST		(1 << 26)	/* RX FIFO left ch first */
#define SACR0_TLFIRST		(1 << 25)	/* TX FIFO left ch first */
#define SACR0_CH_LOCK		(1 << 24)	/* Load RLFIRST TLFIRST setting */
#define SACR0_MOLO_MODE		(1 << 23)	/* single channel mode */
#define SACR0_32BIT_MODE	(1 << 22)	/* S32_LE mode */
#define SACR0_SYNC_INV		(1 << 21)	/* Left/Right ch switch */
#define SACR0_VREF_PD	(1 << 21)	/* VREF Power down */
#define SACR0_RFTH_MASK	(0x1F << 16)
#define SACR0_TFTH_MASK	(0x1F << 8)
#define SACR0_RFTH(x)	((x) << 16)	/* Rx FIFO Interrupt or DMA Trigger Threshold */
#define SACR0_TFTH(x)	((x) << 8)	/* Tx FIFO Interrupt or DMA Trigger Threshold */
#define SACR0_STRF		(1 << 7)	/* DAC output clk edge select */
#define SACR0_RDMAEN	(1 << 6)	/* RX DMA Enable */
#define SACR0_ENLBF		(1 << 5)	/* Enable Loopback */
#define SACR0_RST		(1 << 4)	/* FIFO, i2s Register Reset */
#define SACR0_TDMAEN	(1 << 3)	/* TX DMA Enable */
#define SACR0_BCKD		(1 << 2)	/* Bit Clock Direction */
#define SACR0_SYNCD		(1 << 1)	/* Wprd Select Clock Direction */
#define SACR0_ENB		(1 << 0)	/* Enable I2S Link */

#define SACR1_DRPL	(1 << 1)	/* Disable Replaying Function */
#define SACR1_DREC	(1 << 0)	/* Disable Recording Function */

//ark1668
//#define DACR0_LVOL_MASK (0x7f << 0)
//#define DACR0_LVOL(x)   (((x) & 0x7f) << 0)  /* Lefit Channel Volume */
//#define DACR0_RVOL_MASK (0x7f << 8)
//#define DACR0_RVOL(x)   (((x) & 0x7f) << 8)  /* Right Channel Volume */

//ark1668e
#define DACR0_LVOL_MASK (0x3f << 6)
#define DACR0_LVOL(x)   (((x) & 0x3f) << 6)  /* Lefit Channel Volume */
#define DACR0_RVOL_MASK (0x3f << 0)
#define DACR0_RVOL(x)   (((x) & 0x3f) << 0)  /* Right Channel Volume */

#define DACR0_LHPVOL_MASK (0x3f << 0)
#define DACR0_LHPVOL(x)   (((x) & 0x3f) << 0)  /* HPOUT Lefit Channel Volume */
#define DACR0_RHPVOL_MASK (0x3f << 24)
#define DACR0_RHPVOL(x)   (((x) & 0x3f) << 24)  /* HPOUT Right Channel Volume */

#define SASR0_RFL(x) 	((x) << 16) /* Rx FIFO Level */
#define SASR0_TFL(x) 	((x) << 8) 	/* Tx FIFO Level */
#define SASR0_ROR	(1 << 6)	/* Rx FIFO Overrun */
#define SASR0_TUR	(1 << 5)	/* Tx FIFO Underrun */
#define SASR0_RFS	(1 << 4)	/* Rx FIFO Service Request */
#define SASR0_TFS	(1 << 3)	/* Tx FIFO Service Request */
#define SASR0_BSY	(1 << 2)	/* I2S Busy */
#define SASR0_RNE	(1 << 1)	/* Rx FIFO Not Empty */
#define SASR0_TNF	(1 << 0)	/* Tx FIFO Not Full */

#define SAICR_ROR	(1 << 6)	/* Clear Rx FIFO Overrun Interrupt */
#define SAICR_TUR	(1 << 5)	/* Clear Tx FIFO Underrun Interrupt */
#define SAICR_RFS	(1 << 4)	/* Clear Rx FIFO Service Interrupt */
#define SAICR_TFS	(1 << 3)	/* Clear Tx FIFO Service Interrupt */

#define SAIMR_ROR	(1 << 6)	/* Enable Rx FIFO Overrun Condition Interrupt */
#define SAIMR_TUR	(1 << 5)	/* Enable Tx FIFO Underrun Condition Interrupt */
#define SAIMR_RFS	(1 << 4)	/* Enable Rx FIFO Service Interrupt */
#define SAIMR_TFS	(1 << 3)	/* Enable Tx FIFO Service Interrupt */

//#define ADCR0_LVOL_MASK (0xf << 0)
//#define ADCR0_LVOL(x)   (((x) & 0xf) << 0)  /* Lefit Channel Volume */
//#define ADCR0_RVOL_MASK (0xf << 4)
//#define ADCR0_RVOL(x)   (((x) & 0xf) << 4)  /* Right Channel Volume */
//ark1668e
#define ADCR0_LVOL_MASK (0x7f << 0)
#define ADCR0_LVOL(x)   (((x) & 0x7f) << 19)  /* Lefit Channel Digital Volume */
#define ADCR0_RVOL_MASK (0x7f << 4)
#define ADCR0_RVOL(x)   (((x) & 0x7f) << 12)  /* Right Channel Digital Volume */

#define ADCR0_LFS_MASK	(0x3 << 9)			/* Left Filter Sel	Mask */
#define ADCR0_RFS_MASK	(0x3 << 11)			/* Right Filter Sel	Mask */
#define ADCR0_LFS_1P4	(0 << 9)			/* 1/4 sample rate filter */
#define ADCR0_LFS_1P2	(1 << 9)			/* 1/2 sample rate filter */
#define ADCR0_LFS_1P8	(2 << 9)			/* 1/8 sample rate filter */
#define ADCR0_LFS_1		(3 << 9)			/* bypass */
#define ADCR0_RFS_1P4	(0 << 11)			/* 1/4 sample rate filter */
#define ADCR0_RFS_1P2	(1 << 11)			/* 1/2 sample rate filter */
#define ADCR0_RFS_1P8	(2 << 11)			/* 1/8 sample rate filter */
#define ADCR0_RFS_1		(3 << 11)			/* bypass */
#define ADCR0_LME		(1 << 14)			/* Left Channel mic enhance */
#define ADCR0_RME		(1 << 15)			/* Right Channel mic enhance */

#define rSYS_SD_CLK_CFG			0x58
#define rSYS_SD1_CLK_CFG			0x5c
#define rSYS_DEVICE_CLK_CFG0		0x60
#define rSYS_DEVICE_CLK_CFG1		0x64
#define rSYS_DEVICE_CLK_CFG2		0x68
#define rSYS_DEVICE_CLK_CFG3		0x6c
#define rSYS_SOFT_RSTNA			0x74
#define rSYS_SOFT_RSTNB			0x78
#define rSYS_DDR_STATUS			0x180
#define rSYS_DDR_IO_CFG			0x19C
#define rSYS_PAD_CTRL00			0x1c0
#define rSYS_PAD_CTRL01			0x1c4
#define rSYS_PAD_CTRL02			0x1c8
#define rSYS_PAD_CTRL05			0x1d4
#define rSYS_PAD_CTRL06			0x1d8
#define rSYS_PAD_CTRL07			0x1dc
#define rSYS_PAD_CTRL08			0x1e0
#define rSYS_PAD_CTRL09			0x1e4
#define rSYS_PAD_CTRL0A			0x1e8
#define rSYS_PAD_CTRL0B			0x1ec
#define rSYS_PAD_CTRL0C			0x1f0
#define rSYS_PAD_CTRL0D			0x1f4
#define rSYS_PAD_CTRL0E			0x1f8
#define rSYS_PAD_CTRL38			0x1fc
#define rSYS_PAD_CTRL3E			0x200
#define rSYS_PAD_CTRL0F			0x204
#define rSYS_CPU_CTL            		0x208
#define rSYS_MFC_GMAC_CTL		0x20c
#define rSYS_DEVICE_CLK_CFG7		0x230

#define rSYS_I2S_NCO_CFG			0x174
#define rSYS_I2S1_NCO_CFG		0x19c
#define rSYS_I2S2_NCO_CFG		0x178

#define rSYS_PLL_RFCK_CTL		0x14c
#define rSYS_AUDIO_CFG_0			0x240
#define rSYS_AUDIO_CFG_1			0x244
#define rSYS_AUDIO_CFG_2			0x248
#define rSYS_AUDIO_CFG_3			0x24c
#define rSYS_AUDIO_CFG_4			0x250
#define rSYS_AUDIO_CFG_5			0x254

#define SYS_BASE		0xe4900000
#define I2S_BASE		0xe4000000
#define I2S1_BASE		0xe4200000

#define SACR1_DISABLE_REPLAYING		(1<<1)
#define SACR1_DISABLE_RECORD		(1<<0)


#endif

