#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#ifdef  CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <linux/clk.h>
#include <linux/reset.h>

#define DRIVER_NAME			"ark_i2c"
#define VERSION				"Version 0.1"

//#define CONFIG_I2C_ARK_DBG
#ifdef 	CONFIG_I2C_ARK_DBG
#define I2C_DBG(d, f, a...) \
	printk(KERN_INFO "I2CDBG: [%s:%d] "f, __FUNCTION__, __LINE__, ##a)
#else	/* !CONFIG_I2C_ARK_DBG */
#define I2C_DBG(...)
#endif	/* CONFIG_I2C_ARK_DBG */

enum i2c_speed {
	I2C_STANDARD = 0,
	I2C_FAST,
	I2C_HIGH,
	NUM_SPEEDS
};

enum ark_i2c_status {
	STATUS_IDLE = 0,
	STATUS_READ_START,
	STATUS_READ_IN_PROGRESS,
	STATUS_READ_SUCCESS,
	STATUS_WRITE_START,
	STATUS_WRITE_SUCCESS,
	STATUS_WRITE_IN_PROGRESS,
	STATUS_XFER_ABORT,
	STATUS_STANDBY
};

/* Platform data for arkmicro i2c controller */
struct ark_i2c_platdata {
	unsigned int speed_mode;
	unsigned int hs_master_code;
};

struct i2c_xfer_info {
	int rw;	//0:write 1:read
	u8 *buf;
	int send_left;
	int rev_left;
	int rx_thld;
};

struct ark_i2c_private {
	struct i2c_adapter adap;
	struct device *dev;
	void __iomem *base;
	int irq;
	int speed;
	int hs_mcode;
	struct completion complete;
	int abort;
	u8 *rx_buf;
	int rx_buf_len;
	volatile enum ark_i2c_status status;
	struct i2c_msg *msg;
	struct mutex lock;
	struct clk *clk;
	struct i2c_xfer_info xfer_info;
	struct reset_control *rst;
};

/* =========================================================================
 * 	Registers definition
 * =========================================================================
 */
/* Control register */
#define IC_CON					0x00
#define SLV_DIS					(1 << 6)		/* Disable slave mode */
#define RESTART					(1 << 5)		/* Send a Restart condition */
#define ADDR_10BIT				(1 << 4)		/* 10-bit addressing */
#define STANDARD_MODE			(1 << 1)		/* standard mode */
#define FAST_MODE				(2 << 1)		/* fast mode */
#define HIGH_MODE				(3 << 1)		/* high speed mode */
#define MASTER_EN				(1 << 0)		/* Master mode */

/* Target address register */
#define IC_TAR					0x04
#define IC_TAR_10BIT_ADDR		(1 << 12)		/* 10-bit addressing */
#define IC_TAR_SPECIAL			(1 << 11)		/* Perform special I2C cmd */
#define IC_TAR_GC_OR_START		(1 << 10)		/* 0: Gerneral Call Address */
												/* 1: START BYTE */
/* Slave Address Register */
#define IC_SAR					0x08			/* Not used in Master mode */

/* High Speed Master Mode Code Address Register */
#define IC_HS_MADDR				0x0c
#define IC_HS_MAR				0xFF

/* Rx/Tx Data Buffer and Command Register */
#define IC_DATA_CMD				0x10
#define IC_RD					(1 << 8)		/* 1: Read 0: Write */

/* Standard Speed Clock SCL High Count Register */
#define IC_SS_SCL_HCNT			0x14

/* Standard Speed Clock SCL Low Count Register */
#define IC_SS_SCL_LCNT			0x18

/* Fast Speed Clock SCL High Count Register */
#define IC_FS_SCL_HCNT			0x1c

/* Fast Spedd Clock SCL Low Count Register */
#define IC_FS_SCL_LCNT			0x20

/* High Speed Clock SCL High Count Register */
#define IC_HS_SCL_HCNT			0x24

/* High Speed Clock SCL Low Count Register */
#define IC_HS_SCL_LCNT			0x28

/* Interrupt Status Register */
#define IC_INTR_STAT			0x2c			/* Read only */
#define R_GEN_CALL				(1 << 11)
#define R_START_DET				(1 << 10)
#define R_STOP_DET				(1 << 9)
#define R_ACTIVITY				(1 << 8)
#define R_RX_DONE				(1 << 7)
#define R_TX_ABRT				(1 << 6)
#define R_RD_REQ				(1 << 5)
#define R_TX_EMPTY				(1 << 4)
#define R_TX_OVER				(1 << 3)
#define R_RX_FULL				(1 << 2)
#define R_RX_OVER				(1 << 1)
#define R_RX_UNDER				(1 << 0)

/* Interrupt Mask Register */
#define IC_INTR_MASK			0x30			/* Read and Write */
#define M_GEN_CALL				(1 << 11)
#define M_START_DET				(1 << 10)
#define M_STOP_DET				(1 << 9)
#define M_ACTIVITY				(1 << 8)
#define M_RX_DONE				(1 << 7)
#define M_TX_ABRT				(1 << 6)
#define M_RD_REQ				(1 << 5)
#define M_TX_EMPTY				(1 << 4)
#define M_TX_OVER				(1 << 3)
#define M_RX_FULL				(1 << 2)
#define M_RX_OVER				(1 << 1)
#define M_RX_UNDER				(1 << 0)

/* Raw Interrupt Status Register */
#define IC_RAW_INTR_STAT		0x34			/* Read Only */
#define GEN_CALL				(1 << 11)		/* General call */
#define START_DET				(1 << 10)		/* (RE)START occurred */
#define STOP_DET				(1 << 9)		/* STOP occurred */
#define ACTIVITY				(1 << 8)		/* Bus busy */
#define RX_DONE					(1 << 7)		/* Not used in Master mode */
#define TX_ABRT					(1 << 6)		/* Transmit Abort */
#define RD_REQ					(1 << 5)		/* Not used in Master mode */
#define TX_EMPTY				(1 << 4)		/* TX FIFO <= threshold */
#define TX_OVER					(1 << 3)		/* TX FIFO overflow */
#define RX_FULL					(1 << 2)		/* RX FIFO >= threshold */
#define RX_OVER					(1 << 1)		/* RX FIFO overflow */
#define RX_UNDER				(1 << 0)		/* RX FIFO empty */

/* Receive FIFO Threshold Register */
#define IC_RX_TL				0x38

/* Transmit FIFO Treshold Register */
#define IC_TX_TL				0x3c

/* Clear Combined and Individual Interrupt Register */
#define IC_CLR_INTR				0x40
#define CLR_INTR				(1 << 0)

/* Clear RX_UNDER Interrupt Register */
#define IC_CLR_RX_UNDER			0x44
#define CLR_RX_UNDER			(1 << 0)

/* Clear RX_OVER Interrupt Register */
#define IC_CLR_RX_OVER			0x48
#define CLR_RX_OVER				(1 << 0)

/* Clear TX_OVER Interrupt Register */
#define IC_CLR_TX_OVER			0x4c
#define CLR_TX_OVER				(1 << 0)

#define IC_CLR_RD_REQ			0x50

/* Clear TX_ABRT Interrupt Register */
#define IC_CLR_TX_ABRT			0x54
#define CLR_TX_ABRT				(1 << 0)
#define IC_CLR_RX_DONE			0x58

/* Clear ACTIVITY Interrupt Register */
#define IC_CLR_ACTIVITY			0x5c
#define CLR_ACTIVITY			(1 << 0)

/* Clear STOP_DET Interrupt Register */
#define IC_CLR_STOP_DET			0x60
#define CLR_STOP_DET			(1 << 0)

/* Clear START_DET Interrupt Register */
#define IC_CLR_START_DET		0x64
#define CLR_START_DET			(1 << 0)

/* Clear GEN_CALL Interrupt Register */
#define IC_CLR_GEN_CALL			0x68
#define CLR_GEN_CALL			(1 << 0)

/* Enable Register */
#define IC_ENABLE				0x6c
#define ENABLE					(1 << 0)

/* Status Register */
#define IC_STATUS				0x70			/* Read Only */
#define STAT_SLV_ACTIVITY		(1 << 6)		/* Slave not in idle */
#define STAT_MST_ACTIVITY		(1 << 5)		/* Master not in idle */
#define STAT_RFF				(1 << 4)		/* RX FIFO Full */
#define STAT_RFNE				(1 << 3)		/* RX FIFO Not Empty */
#define STAT_TFE				(1 << 2)		/* TX FIFO Empty */
#define STAT_TFNF				(1 << 1)		/* TX FIFO Not Full */
#define STAT_ACTIVITY			(1 << 0)		/* Activity Status */

/* Transmit FIFO Level Register */
#define IC_TXFLR				0x74			/* Read Only */
#define TXFLR					(1 << 0)		/* TX FIFO level */

/* Receive FIFO Level Register */
#define IC_RXFLR				0x78			/* Read Only */
#define RXFLR					(1 << 0)		/* RX FIFO level */

/* Transmit Abort Source Register */
#define IC_TX_ABRT_SOURCE		0x80
#define ABRT_SLVRD_INTX			(1 << 15)
#define ABRT_SLV_ARBLOST		(1 << 14)
#define ABRT_SLVFLUSH_TXFIFO	(1 << 13)
#define ARB_LOST				(1 << 12)
#define ABRT_MASTER_DIS			(1 << 11)
#define ABRT_10B_RD_NORSTRT		(1 << 10)
#define ABRT_SBYTE_NORSTRT		(1 << 9)
#define ABRT_HS_NORSTRT			(1 << 8)
#define ABRT_SBYTE_ACKDET		(1 << 7)
#define ABRT_HS_ACKDET			(1 << 6)
#define ABRT_GCALL_READ			(1 << 5)
#define ABRT_GCALL_NOACK		(1 << 4)
#define ABRT_TXDATA_NOACK		(1 << 3)
#define ABRT_10ADDR2_NOACK		(1 << 2)
#define ABRT_10ADDR1_NOACK		(1 << 1)
#define ABRT_7B_ADDR_NOACK		(1 << 0)

/* Enable Status Register */
#define IC_ENABLE_STATUS		0x9c
#define IC_EN					(1 << 0)		/* I2C in an enabled state */

/* Component Parameter Register 1*/
#define IC_COMP_PARAM_1			0xf4
#define APB_DATA_WIDTH			(0x3 << 0)

/* Minimum High/Low Period
 * IC_xCNT = ROUNDUP(MIN_SCL_xxx(ns) * IC_CLK(MHz))
 */
#define SS_MIN_SCL_HIGH			4000
#define SS_MIN_SCL_LOW			4700
#define FS_MIN_SCL_HIGH			600
#define FS_MIN_SCL_LOW			1300
#define HS_MIN_SCL_HIGH_100PF	60
#define HS_MIN_SCL_LOW_100PF	120

#define I2C_FIFO_DEPTH			8

/* =========================================================================
 *	Functions
 * =========================================================================
 */
static int ark_i2c_disable(struct i2c_adapter *adap)
{
	struct ark_i2c_private *i2c = i2c_get_adapdata(adap);
	int err = 0;
	int count = 0;
	int ret1, ret2;
	static const u16 delay[NUM_SPEEDS] = {100, 25, 3};

	/* Set IC_ENABLE to 0 */
	writel(0, i2c->base + IC_ENABLE);

	/* Check if device is busy */
	I2C_DBG(&adap->dev, "i2c disable\n");
	while ((ret1 = readl(i2c->base + IC_ENABLE_STATUS) & 0x1)
			|| (ret2 = readl(i2c->base + IC_STATUS) & 0x1)) {
		udelay(delay[i2c->speed]);
		writel(0, i2c->base + IC_ENABLE);
		I2C_DBG(&adap->dev, "i2c is busy, count is %d speed %d\n",
				count, i2c->speed);
		if (count++ > 10) {
			err = -ETIMEDOUT;
			break;
		}
	}

	/* Clear all interrupts */
	readl(i2c->base + IC_CLR_INTR);
	readl(i2c->base + IC_CLR_STOP_DET);
	readl(i2c->base + IC_CLR_START_DET);
	readl(i2c->base + IC_CLR_ACTIVITY);
	readl(i2c->base + IC_CLR_TX_ABRT);
	readl(i2c->base + IC_CLR_RX_OVER);
	readl(i2c->base + IC_CLR_RX_UNDER);
	readl(i2c->base + IC_CLR_TX_OVER);
	readl(i2c->base + IC_CLR_RX_DONE);
	readl(i2c->base + IC_CLR_GEN_CALL);

	/* Disable all interupts */
	writel(0x0000, i2c->base + IC_INTR_MASK);

	return err;
}

static const unsigned short _hcnt[NUM_SPEEDS] = {
	500, 120, 14
};
static const unsigned short _lcnt[NUM_SPEEDS] = {
	570, 150, 18
};

static int ark_i2c_hwinit(struct ark_i2c_private *i2c)
{
	int err;
	unsigned int hcnt, lcnt;
	unsigned long i2c_clk = clk_get_rate(i2c->clk) / 1000000;

	/* ASIC IC_CLK, 24MHz
	 * FPGA IC_CLK, system clock
	 */
	hcnt = (_hcnt[i2c->speed] * i2c_clk) / 100;
	lcnt = (_lcnt[i2c->speed] * i2c_clk) / 100;

	//printk("i2c clk %lu hcnt %u lcnt %u\n", i2c_clk, hcnt, lcnt);

	/* Disable i2c first */
	err = ark_i2c_disable(&i2c->adap);
	if (err)
		return err;

	/*
	 * Setup clock frequency and speed mode
	 * Enable restart condition,
	 * enable master FSM, disable slave FSM,
	 * use target address when initiating transfer
	 */

	writel((i2c->speed + 1) << 1 | SLV_DIS | RESTART | MASTER_EN,
		i2c->base + IC_CON);
	writel(hcnt, i2c->base + (IC_SS_SCL_HCNT + (i2c->speed << 3)));
	writel(lcnt, i2c->base + (IC_SS_SCL_LCNT + (i2c->speed << 3)));

	/* Set SDA phase delay */
	writel(0x30, i2c->base + 0x84);

	/* Set tranmit & receive FIFO threshold to zero */
	writel(I2C_FIFO_DEPTH / 2, i2c->base + IC_RX_TL);
	writel(I2C_FIFO_DEPTH / 2, i2c->base + IC_TX_TL);

	return 0;
}

static void ark_i2c_reset(struct ark_i2c_private *i2c)
{
	reset_control_assert(i2c->rst);
	udelay(10);
	reset_control_deassert(i2c->rst);
}

static u32 ark_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static inline bool ark_i2c_address_neq(
	const struct i2c_msg *p1, const struct i2c_msg *p2)
{
	if (p1->addr != p2->addr)
			return 1;
	if ((p1->flags ^ p2->flags) & I2C_M_TEN)
			return 1;
	return 0;
}

static void ark_i2c_abort(struct ark_i2c_private *i2c)
{
	/* Read about source register */
	int abort = i2c->abort;
	struct i2c_adapter *adap = &i2c->adap;

	/* Single transfer error check:
	 * According to databook, TX/RX FIFOs would be flushed when
	 * the abort interrupt occurred.
	 */
	if (abort & ABRT_MASTER_DIS)
		dev_err(&adap->dev,
			"initiate master operation with master mode disabled.\n");
	if (abort & ABRT_10B_RD_NORSTRT)
		dev_err(&adap->dev,
			"RESTART disabled and master sent READ cmd in 10-bit addressing.\n");

	if (abort & ABRT_SBYTE_NORSTRT) {
		dev_err(&adap->dev,
			"RESTART disabled and user is trying to send START byte.\n");
		writel(~ABRT_SBYTE_NORSTRT, i2c->base + IC_TX_ABRT_SOURCE);
		writel(RESTART, i2c->base + IC_CON);
		writel(~IC_TAR_SPECIAL, i2c->base + IC_TAR);
	}

	if (abort & ABRT_SBYTE_ACKDET)
		dev_err(&adap->dev,
			"START byte was not acknowledged.\n");
	if (abort & ABRT_TXDATA_NOACK)
		I2C_DBG(&adap->dev,
			"No acknowledgement received from slave.\n");
	if (abort & ABRT_10ADDR2_NOACK)
		I2C_DBG(&adap->dev,
			"The 2nd address byte of the 10-bit address was not acknowledged.\n");
	if (abort & ABRT_10ADDR1_NOACK)
		I2C_DBG(&adap->dev,
			"The 1st address byte of 10-bit address was not acknowledged.\n");
	if (abort & ABRT_7B_ADDR_NOACK)
		I2C_DBG(&adap->dev,
			"I2C slave device not acknowledged.\n");

	/* Clear TX_ABRT bit */
	readl(i2c->base + IC_CLR_TX_ABRT);
	i2c->status = STATUS_XFER_ABORT;
}

static int xfer_read(struct i2c_adapter *adap, unsigned char *buf, int length)
{
	struct ark_i2c_private *i2c = i2c_get_adapdata(adap);
	int num;
	int err;
	int thld;

	/*if (length >= 256) {
		dev_err(&adap->dev,
			"I2C FIFO cannot support larger than 256 bytes\n");
		return -EMSGSIZE;
	}*/

	reinit_completion(&i2c->complete);

	thld = length - 1;
	if(thld > I2C_FIFO_DEPTH / 2) thld = I2C_FIFO_DEPTH / 2;

	writel(thld, i2c->base + IC_RX_TL);
	readl(i2c->base + IC_CLR_INTR);

	i2c->status = STATUS_READ_START;

	i2c->xfer_info.rw = 1;
	i2c->xfer_info.buf = buf;
	i2c->xfer_info.rx_thld = thld;
	num = length;
	if(num > I2C_FIFO_DEPTH) num = I2C_FIFO_DEPTH;
	i2c->xfer_info.send_left = length - num;
	i2c->xfer_info.rev_left = length;
	while (num--)
		writel(IC_RD, i2c->base + IC_DATA_CMD);

	writel(0x0044, i2c->base + IC_INTR_MASK);

	err = wait_for_completion_interruptible_timeout(&i2c->complete, adap->timeout);
	if (!err) {
		dev_err(&adap->dev, "Timeout for ACK from I2C slave device\n");
		ark_i2c_hwinit(i2c);
		return -ETIMEDOUT;
	}

	if (i2c->status == STATUS_READ_SUCCESS) {
		struct timeval tv1, tv2, tv3;

		do_gettimeofday(&tv1);
		do_gettimeofday(&tv2);

		while(i2c->xfer_info.rev_left) {
			u32 status = readl(i2c->base + IC_STATUS);

			if(status & STAT_RFNE) {
				*i2c->xfer_info.buf++ = readl(i2c->base + IC_DATA_CMD);
				i2c->xfer_info.rev_left--;
			} else {
				u32 interval;

				do_gettimeofday(&tv3);
				interval = (tv3.tv_sec - tv1.tv_sec) * 1000000 + tv3.tv_usec - tv1.tv_usec;
				if(interval > 50000)
					break;
				interval = (tv3.tv_sec - tv2.tv_sec) * 1000000 + tv3.tv_usec - tv2.tv_usec;
				if (interval > 500) {
					cpu_relax();
					do_gettimeofday(&tv2);
				}
			}
		}

		if (i2c->xfer_info.rev_left) {
			printk(KERN_INFO "rev timeout.\n");
			return -EIO;
		} else {
			return 0;
		}
	} else {
		if (i2c->status == STATUS_XFER_ABORT) {
			//printk(KERN_INFO "i2c tx abort.\n");
			ark_i2c_reset(i2c);
			ark_i2c_hwinit(i2c);
			return -EAGAIN;
		}
		return -EIO;
	}
}

static int xfer_write(struct i2c_adapter *adap, unsigned char *buf, int length)
{
	struct ark_i2c_private *i2c = i2c_get_adapdata(adap);
	int err;
	u32 val;
	int timeout = 500;

	/*if (length >= 256) {
		dev_err(&adap->dev,
			"I2C FIFO cannot support larger than 256 bytes\n");
		return -EMSGSIZE;
	}*/
	reinit_completion(&i2c->complete);

	/* XXXJACK - set TX threshold to length - 1 */
	//writel(length - 1, i2c->base + IC_TX_TL);

	readl(i2c->base + IC_CLR_INTR);

	i2c->xfer_info.rw = 0;
	i2c->xfer_info.buf = buf;
	i2c->xfer_info.send_left = length;
	i2c->status = STATUS_WRITE_START;

	writel(0x0050, i2c->base + IC_INTR_MASK);

	err = wait_for_completion_interruptible_timeout(&i2c->complete, adap->timeout);
	if (!err) {
		dev_err(&adap->dev, "Timeout for I2C write\n");
		ark_i2c_hwinit(i2c);
		return -ETIMEDOUT;
	} else {
		if (i2c->status != STATUS_WRITE_SUCCESS) {
			dev_err(&adap->dev, "I2C write status error i2c->status=%d.\n", i2c->status);
			if (i2c->status == STATUS_XFER_ABORT) {
				printk(KERN_INFO "i2c tx abort 1.\n");
				ark_i2c_reset(i2c);
				ark_i2c_hwinit(i2c);
				return -EAGAIN;
			}
			return -EIO;
		}
	}

	writel(0x40, i2c->base + IC_INTR_MASK);
	//udelay(100);

	while (timeout--) {
		if (i2c->status == STATUS_XFER_ABORT) {
			printk(KERN_INFO "i2c tx abort 2.\n");
			ark_i2c_reset(i2c);
			ark_i2c_hwinit(i2c);
			return -EIO;
		}
		val = readl(i2c->base + IC_STATUS);
		if ((val & STAT_TFE) && !(val & STAT_MST_ACTIVITY))
			break;
		udelay(100);
	}

	if (timeout <= 0) {
		printk(KERN_INFO "wait i2c idle timeout.\n");
		ark_i2c_reset(i2c);
		ark_i2c_hwinit(i2c);
		return -EIO;
	}

	return 0;
}

static int ark_i2c_setup(struct i2c_adapter *adap,	struct i2c_msg *pmsg)
{
	struct ark_i2c_private *i2c = i2c_get_adapdata(adap);
	int err;
	u32 reg;
	u32 bit_mask;
	u32 mode;

	/* Disable device first */
	err = ark_i2c_disable(adap);
	if (err) {
		dev_err(&adap->dev,	"Cannot disable i2c controller, timeout\n");
		return err;
	}

	mode = (1 + i2c->speed) << 1;
	/* set the speed mode */
	reg = readl(i2c->base + IC_CON);
	if ((reg & 0x06) != mode) {
		I2C_DBG(&adap->dev, "set mode %d\n", i2c->speed);
		writel((reg & ~0x6) | mode, i2c->base + IC_CON);
	}

	/* use 7-bit addressing */
	if (pmsg->flags & I2C_M_TEN) {
		if ((reg & ADDR_10BIT) != ADDR_10BIT) {
			I2C_DBG(&adap->dev, "set i2c 10 bit address mode\n");
			writel(reg | ADDR_10BIT, i2c->base + IC_CON);
		}
	} else {
		if ((reg & ADDR_10BIT) != 0x0) {
			I2C_DBG(&adap->dev, "set i2c 7 bit address mode\n");
			writel(reg & ~ADDR_10BIT, i2c->base + IC_CON);
		}
	}
	/* enable restart conditions */
	if (pmsg->flags & I2C_M_NOSTART) {
		if ((reg & RESTART) != RESTART) {
			I2C_DBG(&adap->dev, "enable restart conditions\n");
			writel(reg | RESTART, i2c->base + IC_CON);
		}
	} else {
		if ((reg & RESTART) != 0) {
			I2C_DBG(&adap->dev, "enable restart conditions\n");
			writel(reg & ~RESTART, i2c->base + IC_CON);
		}
	}

	/* enable master FSM */
	reg = readl(i2c->base + IC_CON);
	I2C_DBG(&adap->dev, "ic_con reg is 0x%x\n", reg);
	writel(reg | MASTER_EN, i2c->base + IC_CON);
	if ((reg & SLV_DIS) != SLV_DIS) {
		I2C_DBG(&adap->dev, "enable master FSM\n");
		writel(reg | SLV_DIS, i2c->base + IC_CON);
		I2C_DBG(&adap->dev, "ic_con reg is 0x%x\n", reg);
	}

	/* use target address when initiating transfer */
	reg = readl(i2c->base + IC_TAR);
	bit_mask = IC_TAR_SPECIAL | IC_TAR_GC_OR_START;

	if ((reg & bit_mask) != 0x0) {
		I2C_DBG(&adap->dev,
			"WR: use target address when intiating transfer, i2c_tx_target\n");
		writel(reg & ~bit_mask, i2c->base + IC_TAR);
	}

	/* set target address to the I2C slave address */
	I2C_DBG(&adap->dev,
		"set target address to the I2C slave address, addr is %x\n",
		pmsg->addr);
	writel(pmsg->addr | (pmsg->flags & I2C_M_TEN ? IC_TAR_10BIT_ADDR : 0),
		i2c->base + IC_TAR);

	/* set master code for high-speed mode */
	if (i2c->speed == I2C_HIGH) {
		writel((i2c->hs_mcode & IC_HS_MAR), i2c->base + IC_HS_MADDR);
	}

	/* Enable I2C controller */
	writel(ENABLE, i2c->base + IC_ENABLE);

	return 0;
}

static int ark_i2c_xfer(
	struct i2c_adapter *adap, struct i2c_msg *pmsg, int num)
{
	struct ark_i2c_private *i2c = i2c_get_adapdata(adap);
	int i, err = 0;

	/* if number of messages equal 0*/
	if (num == 0)
		return 0;

#ifdef  CONFIG_PM_RUNTIME
	pm_runtime_get(i2c->dev);
#endif

	mutex_lock(&i2c->lock);
	I2C_DBG(&adap->dev, "ark_i2c_xfer, process %d msg(s)\n", num);
	I2C_DBG(&adap->dev, "slave address is %x\n", pmsg->addr);


	if (i2c->status != STATUS_IDLE && i2c->status != STATUS_XFER_ABORT) {
		dev_err(&adap->dev, "Adapter %d in transfer/standby\n", adap->nr);
		mutex_unlock(&i2c->lock);
#ifdef  CONFIG_PM_RUNTIME
		pm_runtime_put(i2c->dev);
#endif
		return -1;
	}


	for (i = 1; i < num; i++) {
		/* Message address equal? */
		if (unlikely(ark_i2c_address_neq(&pmsg[0], &pmsg[i]))) {
			dev_err(&adap->dev, "Invalid address in msg[%d]\n", i);
			mutex_unlock(&i2c->lock);
#ifdef  CONFIG_PM_RUNTIME
			pm_runtime_put(i2c->dev);
#endif
			return -EINVAL;
		}
	}

	if (ark_i2c_setup(adap, pmsg)) {
		mutex_unlock(&i2c->lock);
#ifdef  CONFIG_PM_RUNTIME
		pm_runtime_put(i2c->dev);
#endif
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		i2c->msg = pmsg;
		i2c->status = STATUS_IDLE;
		/* Read or Write */
		if (pmsg->flags & I2C_M_RD) {
			I2C_DBG(&adap->dev, "I2C_M_RD\n");
			err = xfer_read(adap, pmsg->buf, pmsg->len);
		} else {
			I2C_DBG(&adap->dev, "I2C_M_WR\n");
			err = xfer_write(adap, pmsg->buf, pmsg->len);
		}
		if (err < 0)
			break;
		I2C_DBG(&adap->dev, "msg[%d] transfer complete\n", i);
		pmsg++;		 /* next message */
	}

	/* Mask interrupts */
	writel(0x0000, i2c->base + IC_INTR_MASK);
	/* Clear all interrupts */
	readl(i2c->base + IC_CLR_INTR);

	udelay(100);

	i2c->status = STATUS_IDLE;
	mutex_unlock(&i2c->lock);
#ifdef  CONFIG_PM_RUNTIME
	pm_runtime_put(i2c->dev);
#endif
	return err ? err : i;
}

#ifdef	CONFIG_PM_RUNTIME
static int ark_i2c_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_i2c_private *i2c = platform_get_drvdata(pdev);
	struct i2c_adapter *adap = to_i2c_adapter(dev);

	if (i2c->status != STATUS_IDLE)
		return -1;

	ark_i2c_disable(adap);

	i2c->status = STATUS_STANDBY;

	return 0;
}
static int ark_i2c_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_i2c_private *i2c = platform_get_drvdata(pdev);

	if (i2c->status != STATUS_STANDBY)
		return 0;

	i2c->status = STATUS_IDLE;

	ark_i2c_hwinit(i2c);

	return 0;
}
#endif	/* CONFIG_PM_RUNTIME */

static int i2c_isr_read(struct ark_i2c_private *i2c)
{
	u32 status;
	u32 tx_level, rx_level;
	int wnum, rnum;
	int i;

	if (!(i2c->xfer_info.rw == 1 && i2c->xfer_info.buf != NULL))
	{
		dev_err(i2c->dev, "i2c_isr_read invalid parameter.\n");
		return 1;
	}

	status = readl(i2c->base + IC_STATUS);
	tx_level = readl(i2c->base + IC_TXFLR);
	rx_level = readl(i2c->base + IC_RXFLR);
	if(tx_level == 0 && !(status & STAT_TFE)) tx_level = I2C_FIFO_DEPTH;
	if(rx_level == 0 && (status & STAT_RFF)) rx_level = I2C_FIFO_DEPTH;

	wnum = I2C_FIFO_DEPTH - tx_level;
	if(i2c->xfer_info.send_left < wnum) wnum = i2c->xfer_info.send_left;
	for(i = 0; i < wnum; i++)
		writel(IC_RD, i2c->base + IC_DATA_CMD);
	i2c->xfer_info.send_left -= wnum;

	rnum = rx_level;
	for(i = 0; i < rnum; i++)
		*i2c->xfer_info.buf++ = readl(i2c->base + IC_DATA_CMD);
	i2c->xfer_info.rev_left -= rnum;

	if(i2c->xfer_info.rev_left <= i2c->xfer_info.rx_thld)
	{
		i2c->status = STATUS_READ_SUCCESS;
		return 0;
	}

	i2c->status = STATUS_READ_IN_PROGRESS;

	return 1;
}

static int i2c_isr_write(struct ark_i2c_private *i2c)
{
    u32 status;
	u32 tx_level;
	int wnum;
	int i;

	if(!(i2c->xfer_info.rw == 0 && i2c->xfer_info.buf != NULL))
	{
		dev_err(i2c->dev, "i2c_isr_write invalid parameter.\n");
		return 1;
	}

	if(i2c->xfer_info.send_left == 0 && i2c->status == STATUS_WRITE_IN_PROGRESS)
	{
		i2c->status = STATUS_WRITE_SUCCESS;
		return 0;
	}

	status = readl(i2c->base + IC_STATUS);
	tx_level = readl(i2c->base + IC_TXFLR);
	if(tx_level == 0 && !(status & STAT_TFE)) tx_level = I2C_FIFO_DEPTH;
	wnum = I2C_FIFO_DEPTH - tx_level;
	if(i2c->xfer_info.send_left < wnum) wnum = i2c->xfer_info.send_left;
	for(i = 0; i < wnum; i++)
		writel((u16)(*(i2c->xfer_info.buf + i)), i2c->base + IC_DATA_CMD);
	i2c->xfer_info.buf += wnum;
	i2c->xfer_info.send_left -= wnum;

	i2c->status = STATUS_WRITE_IN_PROGRESS;

	return 1;
}

static irqreturn_t ark_i2c_isr(int this_irq, void *dev)
{
	struct ark_i2c_private *i2c = dev;
	u32 stat;

#ifdef  CONFIG_PM_RUNTIME
	if (pm_runtime_suspended(i2c->dev))
		return IRQ_NONE;
#endif

	stat = readl(i2c->base + IC_INTR_STAT);
	if (!stat)
		return IRQ_NONE;

	//I2C_DBG(&i2c->adap.dev, "%s, stat = 0x%x\n", __func__, stat);

	if (i2c->status == STATUS_IDLE || i2c->status == STATUS_STANDBY)
		goto err;

	if (stat & TX_ABRT) {
		i2c->abort = readl(i2c->base + IC_TX_ABRT_SOURCE);
		printk(KERN_INFO "abort=0x%x.\n", i2c->abort);
		readl(i2c->base + IC_CLR_INTR);
		ark_i2c_abort(i2c);
		goto exit;
	}

	if (stat & RX_FULL) {
		if(i2c_isr_read(i2c)) goto err;
	}

	if (stat & TX_EMPTY) {
		if(i2c_isr_write(i2c)) goto err;
	}

exit:
	if (i2c->status == STATUS_READ_SUCCESS ||
		i2c->status == STATUS_WRITE_SUCCESS ||
		i2c->status == STATUS_XFER_ABORT) {
		/* Mask interrupts */
		writel(0, i2c->base + IC_INTR_MASK);
		complete(&i2c->complete);
	}
err:
	return IRQ_HANDLED;
}

static struct i2c_algorithm ark_i2c_algorithm = {
	.master_xfer	= ark_i2c_xfer,
	.functionality	= ark_i2c_func,
};

static const struct i2c_adapter_quirks ark_i2c_quirks = {
	.flags = I2C_AQ_NO_ZERO_LEN,
};

#ifdef 	CONFIG_PM_RUNTIME
static const struct dev_pm_ops ark_i2c_pm_ops = {
	.runtime_suspend 	= ark_i2c_runtime_suspend,
	.runtime_resume 	= ark_i2c_runtime_resume,
};
#define ARK_I2C_PM_OPS	(&ark_i2c_pm_ops)
#else	/* !CONFIG_PM_RUNTIME */
#define ARK_I2C_PM_OPS	NULL
#endif	/* CONFIG_PM_RUNTIME */

static int ark_i2c_probe(struct platform_device *pdev)
{
	struct ark_i2c_private *i2c;
	struct resource *mem;
	int err, irq;
	void __iomem *base = NULL;
	struct clk *i2c_clk;
	struct reset_control *rst;
	u32 speed;

	I2C_DBG(&pdev->dev, "Get into probe function for I2C\n");

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq; /* -ENXIO */
	}

	base = devm_ioremap_resource(&pdev->dev, mem);
	if (base == NULL) {
		dev_err(&pdev->dev, "failure mapping io resources\n");
		err = -EBUSY;
		goto fail;
	}

	i2c_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c_clk)) {
		err = PTR_ERR(i2c_clk);
		goto fail;
	}

	rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(rst)) {
		dev_err(&pdev->dev,
			"missing or invalid reset controller device tree entry\n");
		err = PTR_ERR(rst);
		goto fail;
	}

	/* Allocate the per-device data structure, ark_i2c_private */
	i2c = devm_kzalloc(&pdev->dev, sizeof(struct ark_i2c_private), GFP_KERNEL);
	if (i2c == NULL) {
		dev_err(&pdev->dev, "can't allocate interface\n");
		err = -ENOMEM;
		goto fail;
	}

	/* Initialize struct members */
	strlcpy(i2c->adap.name, "ArkMicro I2C adapter", sizeof(i2c->adap.name));
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.class = I2C_CLASS_DEPRECATED;
	i2c->adap.retries = 3;
	i2c->adap.timeout = 3 * HZ;
	i2c->adap.algo = &ark_i2c_algorithm;
	i2c->adap.quirks = &ark_i2c_quirks;
	i2c->adap.nr = pdev->id;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.dev.of_node = pdev->dev.of_node;

	i2c->dev = &pdev->dev;
	i2c->base = base;
	i2c->irq = irq;
	i2c->rst = rst;
	i2c->speed = I2C_STANDARD;
	i2c->abort = 0;
	i2c->rx_buf_len = 0;
	i2c->status = STATUS_IDLE;

	platform_set_drvdata(pdev, i2c);
	i2c_set_adapdata(&i2c->adap, i2c);

	i2c->clk = i2c_clk;

	if(!of_property_read_u32(pdev->dev.of_node, "speed-mode", &speed)) {
	    if(speed < NUM_SPEEDS)
			i2c->speed = speed;
	}

	/* Initialize i2c controller */
	err = ark_i2c_hwinit(i2c);
	if (err < 0) {
		dev_err(&pdev->dev, "I2C interface initialization failed\n");
		goto fail;
	}

	mutex_init(&i2c->lock);
	init_completion(&i2c->complete);

	/* Clear all interrupts */
	readl(i2c->base + IC_CLR_INTR);
	writel(0x0000, i2c->base + IC_INTR_MASK);

	err = devm_request_irq(&pdev->dev, i2c->irq, ark_i2c_isr, IRQF_SHARED, pdev->name, i2c);
	if (err) {
		dev_err(&pdev->dev, "Failed to request IRQ %d for I2C controller: "
			"%s\n",	i2c->irq, pdev->name);
		goto fail;
	}

	/* Adapter registration */
	err = i2c_add_numbered_adapter(&i2c->adap);
	if (err) {
		dev_err(&pdev->dev, "Adapter %s registration failed\n",
			i2c->adap.name);
		goto fail;
	}

	I2C_DBG(&pdev->dev, "ArkMicro I2C bus %d driver bind success.\n",
		pdev->id);

#ifdef  CONFIG_PM_RUNTIME
	pm_runtime_enable(&pdev->dev);
#endif
	return 0;

fail:
	return err;
}

static int ark_i2c_remove(struct platform_device *pdev)
{
	struct ark_i2c_private *i2c = platform_get_drvdata(pdev);

	ark_i2c_disable(&i2c->adap);
	i2c_del_adapter(&i2c->adap);

	return 0;
}

static const struct of_device_id ark_i2c_dt_ids[] = {
	{
		.compatible = "arkmicro,ark-i2c",
	} , {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ark_i2c_dt_ids);

static struct platform_driver ark_i2c_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(ark_i2c_dt_ids),
		.pm		= ARK_I2C_PM_OPS,
	},
	.probe		= ark_i2c_probe,
	.remove		= ark_i2c_remove,
};

static int __init ark_i2c_init_driver(void)
{
	return platform_driver_register(&ark_i2c_driver);
}

static void __exit ark_i2c_exit_driver(void)
{
	platform_driver_unregister(&ark_i2c_driver);
}

subsys_initcall(ark_i2c_init_driver);
module_exit(ark_i2c_exit_driver);


MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("I2C driver for Ark Platform");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
