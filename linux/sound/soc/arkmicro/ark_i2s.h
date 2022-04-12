/*
 * ark_i2s.h
 *
 */

#ifndef __ARK_I2S_H
#define __ARK_I2S_H

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

#define SACR0_VREF_VOLSEL		(1 << 28)	/* Sel VREF Voltage 0:3.3v 1:2.2v */
#define SACR0_ADC_VOLSET		(1 << 27)	/* Sel ADC PGA op commond voltage 0:��.5v 1:1.65v */
#define SACR0_MIC_LINE_SEL		(1 << 26)	/* Select micin or linein */
#define SACR0_SDRADC_POWEN		(1 << 25)	/* SARADC power Enable */
#define SACR0_DATA_SEL			(1 << 24)	/* Select external i2s data or sdradc data */
#define SACR0_SARADC_DIS		(1 << 23)	/* SARADC Disable */
#define SACR0_DAC_PD	(1 << 22)	/* DAC Power down */
#define SACR0_VREF_PD	(1 << 21)	/* VREF Power down */
#define SACR0_RFTH_MASK	(0x1F << 16)
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

#define DACR0_LVOL_MASK (0x7f << 0)
#define DACR0_LVOL(x)   (((x) & 0x7f) << 0)  /* Lefit Channel Volume */
#define DACR0_RVOL_MASK (0x7f << 8)
#define DACR0_RVOL(x)   (((x) & 0x7f) << 8)  /* Right Channel Volume */

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

#define ADCR0_LVOL_MASK (0xf << 0)
#define ADCR0_LVOL(x)   (((x) & 0xf) << 0)  /* Lefit Channel Volume */
#define ADCR0_RVOL_MASK (0xf << 4)
#define ADCR0_RVOL(x)   (((x) & 0xf) << 4)  /* Right Channel Volume */
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

#endif

