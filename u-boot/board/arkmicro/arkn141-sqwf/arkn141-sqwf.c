#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <mach/ark-common.h>


DECLARE_GLOBAL_DATA_PTR;

#define ARKN141_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define IRAM_BASE				0x300000

#define rSYS_AHB_CLK_EN			*((volatile unsigned int *)(0x40408044))
#define rSYS_APB_CLK_EN			*((volatile unsigned int *)(0x40408048))
#define rSYS_PER_CLK_EN			*((volatile unsigned int *)(0x40408050))
#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0x40408058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0x4040805c))
#define rSYS_DEVICE_CLK_CFG1	*((volatile unsigned int *)(0x40408064))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0x40408074))
#define rSYS_PLLRFCK_CTL		*((volatile unsigned int *)(0x4040814c))
#define rSYS_AUDPLL_CFG			*((volatile unsigned int *)(0x40408158))
#define rSYS_I2S1_NCO_CFG		*((volatile unsigned int *)(0x4040816c))
#define rSYS_PAD_CTRL03			*((volatile unsigned int *)(0x404081cc))
#define rSYS_PAD_CTRL05			*((volatile unsigned int *)(0x404081d4))
#define rSYS_PAD_CTRL07			*((volatile unsigned int *)(0x404081dc))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0x404081e0))
#define rSYS_PAD_CTRL09			*((volatile unsigned int *)(0x404081e4))
#define rSYS_PAD_CTRL0A			*((volatile unsigned int *)(0x404081e8))
#define rSYS_PAD_CTRL0B			*((volatile unsigned int *)(0x404081ec))
#define rSYS_PAD_CTRL0F			*((volatile unsigned int *)(0x40408180))
#define rSYS_PAD_DRIVE00		*((volatile unsigned int *)(0x404081f4))
#define rSYS_PAD_DRIVE01		*((volatile unsigned int *)(0x404081f8))

/* UART */
enum {
	ARKN141_UART0 = 0,
	ARKN141_UART1 = 1,
	ARKN141_UART2 = 2,
	ARKN141_UART3 = 3
};


extern unsigned int pcm_data[];
extern int get_pcm_data_size(void);

static void uart_pad_gpio_mode_cfg(unsigned int port, int disable_uart)
{
	unsigned int tmp;

	if(port > 4)
		return ;

	if(disable_uart) {
		unsigned int val;
		int offset;

		//uartx APB CLK disable
		offset = port + 7;
		val = rSYS_APB_CLK_EN;
		val &= ~(1 << offset);
		rSYS_APB_CLK_EN = val;

		//uartx UART CLK disable
		offset = port + 19;
		val = rSYS_PER_CLK_EN;
		val &= ~(1 << offset);
		rSYS_PER_CLK_EN = val;
	}

	if(port == ARKN141_UART2) {
		/* uart2 tx/rx pin config to gpio mode */
		tmp = rSYS_PAD_CTRL09;
		tmp &= ~(0xf << 24);
		rSYS_PAD_CTRL09 = tmp;

		//uart2 rx(gpio2)  tx(gpio3)
		//gpio_direction_output(2, 1);	/* trigger low edge wakeup mcu(mcu in stand-by mode when system setup) */
	}
}

static void dwmci_select_pad(void)
{
	/* use sd/mmc 0 */
	rSYS_PAD_CTRL09 |= 0x7F;

	rSYS_SD_CLK_CFG = 0x00000420;

	/* use sd/mmc 1 pad sd1_2 */
	rSYS_PAD_CTRL0B |= (1 << 5) | (1 << 2);
	rSYS_PAD_CTRL05 &= ~(0x1FFFFF << 0);
	rSYS_PAD_CTRL05 |= (2 << 18) | (2 << 15) | (2 << 12) | (2 << 9) | (2 << 6) |
					   (2 << 3) | (2 << 0);

	rSYS_SD1_CLK_CFG = 0x00000420;
}

static void dwmci_reset(void)
{
	rSYS_AHB_CLK_EN &= ~((1 << 4) | (1 << 3));
	rSYS_SOFT_RSTNA &= ~((1 << 31) | (1 << 12));
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1 << 31) | (1 << 12));
	rSYS_AHB_CLK_EN |= ((1 << 4) | (1 << 3));
}

#define ARK_MMC_CLK     	24000000
int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
{
    struct dwmci_host *host = NULL;
    host = malloc(sizeof(struct dwmci_host));
    if (!host) {
    	printf("dwmci_host malloc fail!\n");
        return 1;
    }
	memset(host, 0, sizeof(struct dwmci_host));

	dwmci_select_pad();
	dwmci_reset();

    host->name = name;
    host->ioaddr = (void *)regbase;
    host->buswidth = bus_width;
    host->dev_index = index;
    host->bus_hz = ARK_MMC_CLK;
	host->fifo_mode = 1;

    add_dwmci(host, host->bus_hz, 400000);

    return 0;
}

int board_mmc_init(bd_t *bis)
{
	ark_dwmci_init("ARK_MMC0", 0x60000000, 4, 0);
    ark_dwmci_init("ARK_MMC1", 0x68000000, 4, 0);

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				    CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

int board_init(void)
{
	unsigned int tmp;

	/* uart2 tx/rx pin config to gpio mode */
	uart_pad_gpio_mode_cfg(ARKN141_UART2, 0);
	gpio_direction_output(2, 1);	/* trigger low edge wakeup mcu(mcu in stand-by mode when system setup) */

	/* SPI pad enable */
	tmp = rSYS_PAD_CTRL08;
	tmp &= ~(0xFF << 16);
	tmp |= (0xA8 << 16);
	rSYS_PAD_CTRL08 = tmp;

	/* power GPIO0 high for sdio wifi module in shangqi carcorder project */
	rSYS_PAD_CTRL0A &= ~(0x3 << 4);
	gpio_direction_output(0, 1);

	/* select gpio 61-69, 74-79 pad */
	rSYS_PAD_CTRL07 |= (1 << 30);

	/* set pad drive */
	rSYS_PAD_DRIVE00 = 0;
	rSYS_PAD_DRIVE01 = 0;
	rSYS_PAD_CTRL0F &= ~(3 << 30);	/* 24M OSC drive */
	rSYS_PAD_CTRL0F |= 2 << 30;
	

	/* disable unused module clk */
	rSYS_AHB_CLK_EN &= ~1;
	rSYS_PER_CLK_EN &= ~((1 << 2) | (1 << 3) | (1 << 12) | (1 << 13) | (1 << 24));

    return 0;
}


/* IIS */
#define rIIS_SACR0						*((volatile unsigned int *)(0x40800000))
#define rIIS_SACR1						*((volatile unsigned int *)(0x40800004))
#define rIIS_DACR0						*((volatile unsigned int *)(0x40800008))
#define rIIS_SASR0						*((volatile unsigned int *)(0x4080000c))
#define rIIS_DACR1						*((volatile unsigned int *)(0x40800010))
#define rIIS_SAIMR						*((volatile unsigned int *)(0x40800014))
#define rIIS_SAICR						*((volatile unsigned int *)(0x40800018))
#define rIIS_ADCR0						*((volatile unsigned int *)(0x4080001c))
#define rIIS_SADR						*((volatile unsigned int *)(0x40800080))
#define rIIS_SATR						*((volatile unsigned int *)(0x40800080))
#define I2S_DATA_FIFO					0x40800080


static unsigned int get_audpll_freq(void)
{
	unsigned int val;
	unsigned int ref_clk;
	unsigned int no, nf;

	val = rSYS_PLLRFCK_CTL;
	ref_clk = 12000000 * ((val >> 6) & 0x1);

	val = rSYS_AUDPLL_CFG;
	no = (val >> 12) & 0x03;
	no = (1 << no);
	nf = val & 0xff;

	return ref_clk * nf / no;
}


static void dac_init(void)
{
	//reset
	rIIS_SACR0 |= (0x1 << 4 );
	rIIS_SACR0 &=~ (0x1 << 4 );
	
	rIIS_SACR0 =  (0x10<<16) | (0xf<<8) | (0x0<<6) | (0x1<<3) | (0x1<<2) | (0x1<<1) | 0x1;
	rIIS_SACR0 &= ~(0x1 << 21); 
	rIIS_SACR0 &= ~(0x1 << 22); 
	rIIS_SACR0 &= ~(0x1 << 24); // 0:external i2s data
	
	rIIS_SACR0 |= (1 << 3);
	
	rIIS_SAIMR = 0x00;
		
	//set audpll clk source 
	rSYS_DEVICE_CLK_CFG1 |= (1 << 24);

	//set rate 16k
	rSYS_I2S1_NCO_CFG = (512 << 16) | (get_audpll_freq() / 16000);

	//set volume
	rIIS_DACR0 = 0x6464;

	//mute off
	gpio_direction_output(104, 1);

	//start play
	rIIS_SACR1 = 0x1;
}

#define DMA_BASE			0x70020000

/*Source Address Register*/
#define rDMA_SAR0_L       	*((volatile unsigned int *)(DMA_BASE + 0x000))
#define rDMA_SAR0_H      	*((volatile unsigned int *)(DMA_BASE + 0x004))
/*Destination Address Register*/
#define rDMA_DAR0_L			*((volatile unsigned int *)(DMA_BASE + 0x008))
#define rDMA_DAR0_H			*((volatile unsigned int *)(DMA_BASE + 0x00c))
/*Linked List Pointer Register*/
#define rDMA_LLP0_L			*((volatile unsigned int *)(DMA_BASE + 0x010))
#define rDMA_LLP0_H			*((volatile unsigned int *)(DMA_BASE + 0x014))
/*Control Register*/
#define rDMA_CTL0_L			*((volatile unsigned int *)(DMA_BASE + 0x018))
#define rDMA_CTL0_H			*((volatile unsigned int *)(DMA_BASE + 0x01c))
/*Configuration Register*/
#define rDMA_CFG0_L			*((volatile unsigned int *)(DMA_BASE + 0x040))
#define rDMA_CFG0_H			*((volatile unsigned int *)(DMA_BASE + 0x044))

#define rDMA_CFG_L          *((volatile unsigned int *)(DMA_BASE + 0x398))
#define rDMA_CHEN_L         *((volatile unsigned int *)(DMA_BASE + 0x3a0))

#define I2S_DMA_TRANS_SIZE 	4092
#define I2S_DMA_TRANS_BYTES	(I2S_DMA_TRANS_SIZE * 4)

struct dw_lli {
	/* values that are not changed by hardware */
	u32		sar;
	u32		dar;
	u32		llp;		/* chain to next lli */
	u32		ctllo;
	/* values that may get written back: */
	u32		ctlhi;
	/* sstat and dstat can snapshot peripheral register state.
	 * silicon config may discard either or both...
	 */
	u32		sstat;
	u32		dstat;
};

#define DMA_WIDTH_32				2
#define DMA_BURST_SIZE_16			3
#define M2P_DMAC 					1
#define REQ_I2S_TX					19

#define I2S_TX_LLI_CTL_L 		((1<<28)\
								|(1<<27)\
								|(0<<25)\
								|(1<<23)\
								|(M2P_DMAC<<20)\
								|(0<<18)\
								|(0<<17)\
								|(DMA_BURST_SIZE_16<<14)\
								|(DMA_BURST_SIZE_16<<11)\
								|(0<<9)\
								|(2<<7)\
								|(DMA_WIDTH_32<<4)\
								|(DMA_WIDTH_32<<1)\
								|(0<<0))

#define I2S_TX_DMA_CFG_H		((REQ_I2S_TX<<12)\
								|(0<<7)\
								|(1<<6)\
								|(1<<5)\
								|(1<<1))		

static void dac_start_dma(void)
{
	int lli_count = 0;
	int i;
	struct dw_lli *plli;
	int tfr_size = get_pcm_data_size();
	int ch = 0;

	lli_count = DIV_ROUND_UP(tfr_size,  I2S_DMA_TRANS_BYTES);
	plli = (struct dw_lli *)IRAM_BASE;
	for (i = 0; i < lli_count; i++) {
		plli[i].sar = (u32)pcm_data + i * I2S_DMA_TRANS_BYTES;
		plli[i].dar = I2S_DATA_FIFO;
		plli[i].llp = (u32)&plli[i+1] & ~3;
		plli[i].ctllo = I2S_TX_LLI_CTL_L; 
		plli[i].ctlhi = tfr_size > I2S_DMA_TRANS_BYTES ? I2S_DMA_TRANS_SIZE : tfr_size / 4;
		tfr_size -= I2S_DMA_TRANS_BYTES;
	}
	plli[i-1].llp = 0;
	flush_dcache_range(IRAM_BASE, IRAM_BASE + lli_count * sizeof(struct dw_lli));

	rDMA_CFG_L |= (1 << 0); //enable dma

	rDMA_SAR0_L = (u32)pcm_data;
	rDMA_DAR0_L = I2S_DATA_FIFO;
	rDMA_LLP0_L	= (u32)&plli[0] & ~3;;
	rDMA_CTL0_L = plli[0].ctllo;
	rDMA_CTL0_H = plli[0].ctlhi;
	rDMA_CFG0_L	= 0;
	rDMA_CFG0_H = I2S_TX_DMA_CFG_H;
	rDMA_CHEN_L = (1 << (8 + ch)) | (1 << ch);//channel enable
}

void play_audio(const unsigned int *data, int size)
{
	printf("size=%d.\n", size);
	dac_init();
	dac_start_dma();
}

static int mcu_serial_send_and_check(unsigned char cmd)
{
	unsigned char recv_buf[256];
	const unsigned char cmd_package[2][5] = {
		{0x55, 0x80, 0xA0, 0x0, 0x20},	//update cmd
		{0x55, 0x80, 0xA1, 0x0, 0x21}	//update complete cmd
	};
	const unsigned char ack_package[2][6] = {
		{0x55, 0x80, 0xA0, 0x1, 0x0, 0x21},
		{0x55, 0x80, 0xA1, 0x1, 0x0, 0x20}
	};
	int timeout = 0;
	int cmd_id;
	int size;
	int i;

	size = sizeof(cmd_package)/sizeof(cmd_package[0]);
	for(i=0; i<size; i++) {
		if(cmd == cmd_package[i][2])
			break;
	}
	if(i == size)
		return -1;

	cmd_id = i;
	while(timeout++ <= 14){	//5s: 31
		mcu_serial_send(cmd_package[cmd_id], sizeof(cmd_package[cmd_id]));
		mdelay(10*timeout);
#if 0
		size = mcu_serial_read(recv_buf);
		for(i=0; i<size; i+=sizeof(ack_package[cmd_id])) {
			if(((i+1)*sizeof(ack_package[cmd_id])) > size)
				break;
			if(memcmp(&recv_buf[i], ack_package[cmd_id], sizeof(ack_package[cmd_id])) == 0) {
				return 0;
			}
		}
#else
		size = mcu_serial_receive(recv_buf, sizeof(recv_buf));
		for(i=0; i<size; i++) {
			if(recv_buf[i] == ack_package[cmd_id][0]) {
				if(memcmp(&recv_buf[i], ack_package[cmd_id], sizeof(ack_package[cmd_id])) == 0) {
					//printf("### timeout:%d\n", timeout);
					return 0;
				}
			}
		}
#endif
	};

	return -1;
}

int board_late_init(void)
{
	char cmd[128];
	char *need_update;
	unsigned int loadaddr;
	int update = 0;

	run_command("sf probe", 0);

	gpio_direction_output(2, 0);	/* trigger low edge wakeup mcu(mcu in stand-by mode when system setup) */

	gpio_direction_input(32);
	if (!gpio_get_value(32)) {
		env_set("need_update", "yes");
		update = 1;
	}

	if (update) {
		int state = 0;
		int i,j;

		gpio_direction_output(30, 1);
		gpio_direction_output(31, 1);
		mdelay(500);

		for(i=1; i<=10; i++){
			mcu_serial_init();
			if(mcu_serial_send_and_check(0xA0) != 0) {
				printf("mcu ack timeout, retry(%d)...\n", i);
				uart_pad_gpio_mode_cfg(ARKN141_UART2, 1);
				state = 0;
				for(j=0; j<3; j++) {
					gpio_direction_output(2, state);
					state = !state;
					mdelay(i*100);
				}
				update = 0;
				continue;
			}

			update = 1;
			break;
		}

		if(update == 1) {
			printf("mcu wakeup success\n");
		} else if(update == 0) {
			printf("mcu wakeup failed\n");
			//env_set("need_update", "no");
		}
	}

	need_update = env_get("need_update");
	if (!strcmp(need_update, "yes")) {
		sprintf(cmd, "fatload %s %s %s update-magic", env_get("update_dev"),
			env_get("dev_part"), env_get("loadaddr"));
		run_command(cmd, 0);
		loadaddr = env_get_hex("loadaddr", 0);
		if (loadaddr && memcmp((void*)loadaddr, ARKN141_UPDATE_MAGIC, 
				strlen(ARKN141_UPDATE_MAGIC))) {
			printf("Wrong update magic, do not update.\n");
			env_set("need_update", "no");
		} else {
			//run_command("sf erase reserve 0", 0);
			//run_command("sf erase customid 0", 0);
			//run_command("sf erase usrdata 0", 0);
			run_command("env default -f -a", 0);
		}
	}	

	play_audio(pcm_data, get_pcm_data_size());

	return 0;
}
