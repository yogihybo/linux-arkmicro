#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <debug_uart.h>
#include <asm-generic/gpio.h>
#include <asm/arch/ark-common.h>
#include <linux/usb/musb.h>

DECLARE_GLOBAL_DATA_PTR;

#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0xe4900058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0xe490005c))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0xe4900074))
#define rSYS_SOFT_RSTNB			*((volatile unsigned int *)(0xe4900078))
#define rSYS_DDR_STATUS			*((volatile unsigned int *)(0xe4900180))
#define rSYS_DDR_IO_CFG			*((volatile unsigned int *)(0xe490019C))
#define rSYS_PAD_CTRL00			*((volatile unsigned int *)(0xe49001c0))
#define rSYS_PAD_CTRL01			*((volatile unsigned int *)(0xe49001c4))
#define rSYS_PAD_CTRL02			*((volatile unsigned int *)(0xe49001c8))
#define rSYS_PAD_CTRL04			*((volatile unsigned int *)(0xe49001d0))
#define rSYS_PAD_CTRL05			*((volatile unsigned int *)(0xe49001d4))
#define rSYS_PAD_CTRL06			*((volatile unsigned int *)(0xe49001d8))
#define rSYS_PAD_CTRL07			*((volatile unsigned int *)(0xe49001dc))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0xe49001e0))
#define rSYS_PAD_CTRL09			*((volatile unsigned int *)(0xe49001e4))
#define rSYS_PAD_CTRL0A			*((volatile unsigned int *)(0xe49001e8))
#define rSYS_PAD_CTRL0B			*((volatile unsigned int *)(0xe49001ec))
#define rSYS_PAD_CTRL0C			*((volatile unsigned int *)(0xe49001f0))
#define rSYS_PAD_CTRL0D			*((volatile unsigned int *)(0xe49001f4))
#define rSYS_PAD_CTRL0E			*((volatile unsigned int *)(0xe49001f8))
#define rSYS_PAD_CTRL38			*((volatile unsigned int *)(0xe49001fc))
#define rSYS_PAD_CTRL3E			*((volatile unsigned int *)(0xe4900200))
#define rSYS_PAD_CTRL0F			*((volatile unsigned int *)(0xe4900204))
#define rSYS_CPU_CTL            *((volatile unsigned int *)(0xe4900208))
#define rSYS_MFC_GMAC_CTL		*((volatile unsigned int *)(0xe490020c))
#define rSYS_DEVICE_CLK_CFG7	*((volatile unsigned int *)(0xe4900230))
#define rSYS_DEVICE_CLK_CFG8	*((volatile unsigned int *)(0xe4900234))

#define rWDT_CR		*((volatile unsigned int *)(0xe4b00000))

#define MUSB_BASE				0xe0100000

extern const struct musb_platform_ops ark_musb_ops;

static struct musb_hdrc_config musb_config = {
	.multipoint = 1,
	.dyn_fifo = 0,
	.num_eps = 6,
	.ram_bits = 12
};

static struct musb_hdrc_platform_data musb_platform_data = {
	.mode = MUSB_HOST,
	.config = &musb_config,
	.power = 100,
	.platform_ops = &ark_musb_ops,
};

static void dwmci_select_pad(void)
{
	unsigned int val;

	/* use sd/mmc 0 */
	val = rSYS_PAD_CTRL00;
	val &= ~((0x7<<18)|(0x7<<15)|(0x7<<12)|(0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
	val |= ((0x1<<18)|(0x1<<15)|(0x1<<12)|(0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
	rSYS_PAD_CTRL00 = val;

	val = rSYS_PAD_CTRL0E;
	val &= ~((0x7<<27)|(0x7<<24));
	val |= ((0x1<<27)|(0x1<<24));
	rSYS_PAD_CTRL0E = val;

	val = rSYS_PAD_CTRL38;
	val &= ~((0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
	val |= ((0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
	rSYS_PAD_CTRL38 = val;

	/* use sd/mmc 1 */
    val = rSYS_PAD_CTRL00;
    val &= ~((0x7<<27)|(0x7<<24)|(0x7<<21));
    val |= ((0x1<<27)|(0x1<<24)|(0x1<<21));
    rSYS_PAD_CTRL00 = val;

    val = rSYS_PAD_CTRL01;
    val &= ~((0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
    val |= ((0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
    rSYS_PAD_CTRL01 = val;

	/* use sdio wifi/mmc 2*/
    val = rSYS_PAD_CTRL01;
    val &= ~((0x7<<27)|(0x7<<24)|(0x7<<21)|(0x7<<18)|(0x7<<15)|(0x7<<12));
    val |= ((0x1<<27)|(0x1<<24)|(0x1<<21)|(0x1<<18)|(0x1<<15)|(0x1<<12));
    rSYS_PAD_CTRL01 = val;

    val = rSYS_PAD_CTRL02;
    val &= ~(0x7<<0);
    val |= (0x1<<0);
    rSYS_PAD_CTRL02 = val;
}

/* static void dwmci_reset(void)
{
	rSYS_SOFT_RSTNA &= ~((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB &= ~(1<<3);
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB |= (1<<3);
} */

static void usb_controller_reset(void)
{
	rSYS_SOFT_RSTNA &= ~(3 << 5);
	udelay(100);
	rSYS_SOFT_RSTNA |= 3 << 5;
	udelay(10);
}

#define ARK_MMC_CLK     	24000000
struct dwmci_host dwmcihost[2];
static int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
{
    struct dwmci_host *host = NULL;
    host = &dwmcihost[index];
	memset(host, 0, sizeof(struct dwmci_host));

	dwmci_select_pad();
	//dwmci_reset();

	/* config clk in sample delay */
	rSYS_SD_CLK_CFG &= ~(0x7f << 13);
	rSYS_SD_CLK_CFG |= 52 << 13;
	rSYS_SD1_CLK_CFG &= ~(0x7f << 13);
	rSYS_SD1_CLK_CFG |= 52 << 13;

    host->name = name;
    host->ioaddr = (void *)regbase;
    host->buswidth = bus_width;
    host->dev_index = index;
    host->bus_hz = ARK_MMC_CLK;
    host->fifoth_val = 64;
	host->fifo_mode = 1;

    add_dwmci(host, host->bus_hz, 400000);

    return 0;
}

int board_mmc_init(bd_t *bis)
{
	ark_dwmci_init("ARK_MMC0", 0xec400000, 8, 0);
#ifndef CONFIG_SPL_BUILD
    ark_dwmci_init("ARK_MMC1", 0xec800000, 4, 1);
#endif
	ark_dwmci_init("ARK_MMC2", 0xecc00000, 4, 2);
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
	unsigned int val;

	/* watchdog disable */
	rWDT_CR = 0;

	/* cpu1 disable */
	rSYS_CPU_CTL &= ~(1 << 7);

	/* cpu cnt clk enable */
	rSYS_CPU_CTL |= 1 << 23;

	/* nand pad enable */
	val = rSYS_PAD_CTRL08;
	val &= ~((0x7<<27) | (0x7<<24) | (0x7<<21) | (0x7<<18) | (0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6));
	val |= (0x1<<27) | (0x1<<24) | (0x1<<21) | (0x1<<18) | (0x1<<15) | (0x1<<12) | (0x1<<9) | (0x1<<6);
	rSYS_PAD_CTRL08 = val;

	val = rSYS_PAD_CTRL09;
	val &= ~((0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6) | (0x7<<3) | (0x7<<0));
	val |= (1<<15) | (1<<12) | (1<<9) | (1<<6) | (1<<3) | (1<<0);//enable nand cle, ale,ren,wen
	rSYS_PAD_CTRL09 = val;

	/* spi pad enable */
	val = rSYS_PAD_CTRL09;
	val &= ~((0x7<<27) | (0x7<<24));
	val |= (0x2<<27) | (0x2<<24);
	rSYS_PAD_CTRL09 = val;

	val = rSYS_PAD_CTRL0A;
	val &= ~((0x7<<3) | (0x7<<0));
	val |= (0x2<<0);
	rSYS_PAD_CTRL0A = val;

	/* gmac pad enable */
	rSYS_PAD_CTRL0C = (1 << 27) | (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
						(1 << 6) | (1 << 3) | (1 << 0);
	//rSYS_PAD_CTRL0D = (1 << 24) | (1 << 9) | (1 << 6) | (1 << 3) | (1 << 0);
	rSYS_PAD_CTRL0D = (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
						(1 << 6) | (1 << 3) | (1 << 0);
	/* gmac tx clk out */
	rSYS_PAD_CTRL0F |= (1 << 31);

	/* select rgmii interface */
	rSYS_MFC_GMAC_CTL &= ~(7 << 1);
	rSYS_MFC_GMAC_CTL |= (1 << 1);

	/* mac rx clk inv */
	rSYS_DEVICE_CLK_CFG7 |= (1 << 1);
	/* mac tx clk inv */
	rSYS_DEVICE_CLK_CFG8 |= (1 << 7);

	/* i2s0 sadata in */
	rSYS_PAD_CTRL0F &= ~(1 << 28);

	/* i2s1 sadata out */
	rSYS_PAD_CTRL0F |= (1 << 29);

	/* select pwm0 pad */
	rSYS_PAD_CTRL05 &= ~0x7;
	rSYS_PAD_CTRL05 |= 1;

    return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	debug_uart_init();
#endif
	return 0;
}
#endif

int board_late_init(void)
{
	char cmd[128];
	char *need_update,*update_flash;
	unsigned int loadaddr;
	int do_update = 0, update_from_mmc = 1;

	run_command("sf probe", 0);

#ifdef CONFIG_USB_MUSB_HOST
	usb_controller_reset();
	musb_register(&musb_platform_data, NULL, (void *)MUSB_BASE);
#endif

	update_flash = env_get("update_from_flash");
        printf("++++++++++%s+++++++\n",update_flash);
	need_update = env_get("need_update");
	if (!strcmp(need_update, "yes")) {
		loadaddr = env_get_hex("loadaddr", 0);

		sprintf(cmd, "fatload %s %s %s update-magic", "mmc", env_get("sd_dev_part"), env_get("loadaddr"));
		run_command(cmd, 0);
		if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
			do_update = 1;
			goto update_done;
		} else {
			printf("Wrong update magic, do not update from mmc.\n");
		}

#ifdef CONFIG_USB_MUSB_HOST
		run_command("usb start", 0);
		sprintf(cmd, "fatload %s %s %s update-magic", "usb", "0", env_get("loadaddr"));
		run_command(cmd, 0);
		if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
			do_update = 1;
			update_from_mmc = 0;
		} else {
			printf("Wrong update magic, do not update from usb.\n");
		}
#endif
	}
	else if (!strcmp(update_flash, "yes")){

	    sprintf(cmd, "run updatefromflash");
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);

	}

update_done:
	if (do_update) {
		run_command("nand erase.part userdata", 0);
		env_set("need_update", "no");
		env_set("do_update", "yes");
		if (update_from_mmc) {
			printf("update form mmc...\n");
			env_set("update_dev_type", "mmc");
			env_set("update_dev_part", env_get("sd_dev_part"));
		} else {
			printf("update form usb...\n");
			env_set("update_dev_type", "usb");
			env_set("update_dev_part", "0");
		}
	} else {
		env_set("do_update", "no");
	}

	return 0;
}


