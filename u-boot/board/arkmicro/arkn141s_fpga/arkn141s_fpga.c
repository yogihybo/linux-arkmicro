#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <debug_uart.h>
#include <asm-generic/gpio.h>
#include <asm/arch/ark-common.h>

DECLARE_GLOBAL_DATA_PTR;

#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define rSYS_AHB_CLK_EN			*((volatile unsigned int *)(0x40408044))
#define rSYS_APB_CLK_EN			*((volatile unsigned int *)(0x40408048))
#define rSYS_PER_CLK_EN			*((volatile unsigned int *)(0x40408050))
#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0x40408058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0x4040805C))
#define rSYS_DEVICE_CLK_CFG1		*((volatile unsigned int *)(0x40408064))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0x40408078))
#define rSYS_PAD_CTRL00			*((volatile unsigned int *)(0x404080c0))
#define rSYS_PAD_CTRL01			*((volatile unsigned int *)(0x404080c4))
#define rSYS_PAD_CTRL02			*((volatile unsigned int *)(0x404080c8))
#define rSYS_PAD_CTRL03			*((volatile unsigned int *)(0x404080cc))
#define rSYS_PAD_CTRL04			*((volatile unsigned int *)(0x404080d0))

#define rSYS_PAD_DRIVE00		*((volatile unsigned int *)(0x404080d4))
#define rSYS_PAD_DRIVE01		*((volatile unsigned int *)(0x404080d8))
#define rSYS_PAD_DRIVE02		*((volatile unsigned int *)(0x404080dc))
#define rSYS_PAD_DRIVE03		*((volatile unsigned int *)(0x404080E0))


#define rSYS_PAD_USB_CFG    		*((volatile unsigned int *)(0x404080E4))
#define rSYS_PAD_USB_CFG1   		*((volatile unsigned int *)(0x404080E8))
#define rSYS_PAD_USB_CFG2   		*((volatile unsigned int *)(0x404080Ec))



static void dwmci_select_pad(void)
{
	unsigned int val;
	/* use sd/mmc 0 */
	val = rSYS_PAD_CTRL02;
	val &= ~((0x3<<24)|(0x3<<22)|(0x3<<20)|(0x3<<18)|(0x3<<16)|(0x3<<14)|(0x3<<12));
	val |=  ((0x1<<24)|(0x1<<22)|(0x1<<20)|(0x1<<18)|(0x1<<16)|(0x1<<14)|(0x1<<12));
	rSYS_PAD_CTRL02 = val;
	udelay(100);
	/* use emmc 1 */
	val = rSYS_PAD_CTRL03;
	val &= ~((0x3<<10)|(0x3<<8)|(0x3<<6)|(0x3<<4)|(0x3<<2)|(0x3<<0));
	val |=  ((0x1<<10)|(0x1<<8)|(0x1<<6)|(0x1<<4)|(0x1<<2)|(0x1<<0));
	rSYS_PAD_CTRL03 = val;

}

 static void dwmci_reset(void)
{
	rSYS_SOFT_RSTNA &= ~((1<<29)|(1<<16));
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1<<29)|(1<<16));
}

#define ARK_MMC_CLK     	12000000
static int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
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
    host->fifoth_val = 64;
	host->fifo_mode = 1;

    add_dwmci(host, host->bus_hz, 400000);

    return 0;
}

int board_mmc_init(bd_t *bis)
{
    ark_dwmci_init("ARK_MMC0", 0x70070000, 4, 0);
    ark_dwmci_init("ARK_MMC1", 0x70080000, 4, 0);
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

	/* cpu1 disable */
//	rSYS_CPU_CTL &= ~(1 << 7);

	/* nand pad enable */
//	val = rSYS_PAD_CTRL08;
//	val &= ~((0x7<<27) | (0x7<<24) | (0x7<<21) | (0x7<<18) | (0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6));
//	val |= (0x1<<27) | (0x1<<24) | (0x1<<21) | (0x1<<18) | (0x1<<15) | (0x1<<12) | (0x1<<9) | (0x1<<6);
//	rSYS_PAD_CTRL08 = val;

//	val = rSYS_PAD_CTRL09;
//	val &= ~((0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6) | (0x7<<3) | (0x7<<0));
//	val |= (1<<15) | (1<<12) | (1<<9) | (1<<6) | (1<<3) | (1<<0);//enable nand cle, ale,ren,wen
//	rSYS_PAD_CTRL09 = val;

	/* spi pad enable */
//	val = rSYS_PAD_CTRL09;
//	val &= ~((0x7<<27) | (0x7<<24));
//	val |= (0x2<<27) | (0x2<<24);
//	rSYS_PAD_CTRL09 = val;

//	val = rSYS_PAD_CTRL0A;
//	val &= ~((0x7<<3) | (0x7<<0));
//	val |= (0x2<<0);
//	rSYS_PAD_CTRL0A = val;

	/* spi 0 pad enable */
	val = rSYS_PAD_CTRL01;
	val &= ~((0x3<<10) | (0x3 << 8)|(0x3 << 6)|(0x3 << 4));
//	val |= ((0x1<<10) |(0x1 << 8)|(0x1 << 6)|(0x1 << 4));
	val |= ((0x1 << 8)|(0x1 << 6)|(0x1 << 4));
	rSYS_PAD_CTRL01 = val;


	/* gmac pad enable */
//	rSYS_PAD_CTRL0C = (1 << 27) | (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
//						(1 << 6) | (1 << 3) | (1 << 0);
//	rSYS_PAD_CTRL0D = (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
//						(1 << 6) | (1 << 3) | (1 << 0);
//	rSYS_PAD_CTRL0F |= (1 << 31);

	/* select rgmii interface */
//	rSYS_MFC_GMAC_CTL &= ~(7 << 1);
//	rSYS_MFC_GMAC_CTL |= (1 << 1);

	/* mac tx clk inv */
//	rSYS_DEVICE_CLK_CFG7 |= 1;

	/* i2s0 sadata in */
//	rSYS_PAD_CTRL0F &= ~(1 << 28);

	/* i2s1 sadata out */
//	rSYS_PAD_CTRL0F |= (1 << 29);

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
        printf("+++++++++board_late_init+++++++\n");
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

#ifdef CONFIG_USB_MUSB_HCD
		//use old musb driver
		run_command("usb start", 0);
#endif
		sprintf(cmd, "fatload %s %s %s update-magic", "usb", "0", env_get("loadaddr"));
		run_command(cmd, 0);
		if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
			do_update = 1;
			update_from_mmc = 0;
		} else {
			printf("Wrong update magic, do not update from usb.\n");
		}
	}
	else if (!strcmp(update_flash, "yes")){

	    sprintf(cmd, "run updatefromflash");
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);

	}

update_done:
	if (do_update) {
		//run_command("nand erase.part userdata", 0);
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
#if 0
#ifdef CONFIG_SPL_BUILD
static inline void ApbWriteFun(unsigned int addr, unsigned int data)
{
	* (volatile unsigned int *) addr = data;
}

void mem_init(void)
{
	ApbWriteFun(0xE490006c, 0x80000);  //device_cfg for ddr2_ref_clk
	udelay(20);
	ApbWriteFun(0xE4900214, 0x0);

	//softa
	ApbWriteFun(0xE4900078, 0xfffffffd);

	udelay(2);
	ApbWriteFun(0xE4900214, 0xFFF0BFFF);//PLL_PDN=[13]=1
	udelay(20); // > 50us
	ApbWriteFun(0xE4900214, 0xFFF8BFFF);//PLL_RSTN=[19]=1
	udelay(10);
	ApbWriteFun(0xE4900214, 0xFFF8FFFF);//DLL_PDN=[14]=1
	udelay(50);//> 100us
	ApbWriteFun(0xE4900214, 0xFFFBFFFF);//DDR_PHY_RSTN=[16]=[17]=1
	udelay(10);
	ApbWriteFun(0xE4900214, 0xFFFFFFFF);//DDR_DPHY_RSTN=[18]=1
	udelay(10);
	ApbWriteFun(0xE4900214, 0xFFFFEFFF);//=[12]=ddr_srf_req=0;
	udelay(10);
	ApbWriteFun(0xE4900210, 0x70000800);
	ApbWriteFun(0xE4900214, 0xFFEFE074);
	udelay(10);
	ApbWriteFun(0xE4900078, 0xffffffff);

	ddr3_sdramc_init();
}
#endif
#endif
