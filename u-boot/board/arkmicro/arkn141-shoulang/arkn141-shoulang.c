#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define ARKN141_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define rSYS_AHB_CLK_EN			*((volatile unsigned int *)(0x40408044))
#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0x40408058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0x4040805c))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0x40408074))
#define rSYS_PAD_CTRL00			*((volatile unsigned int *)(0x404081c0))
#define rSYS_PAD_CTRL03			*((volatile unsigned int *)(0x404081cc))
#define rSYS_PAD_CTRL05			*((volatile unsigned int *)(0x404081d4))
#define rSYS_PAD_CTRL07			*((volatile unsigned int *)(0x404081dc))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0x404081e0))
#define rSYS_PAD_CTRL09			*((volatile unsigned int *)(0x404081e4))
#define rSYS_PAD_CTRL0B			*((volatile unsigned int *)(0x404081ec))
#define rSYS_PAD_DRIVE00		*((volatile unsigned int *)(0x404081f4))
#define rSYS_PAD_DRIVE01		*((volatile unsigned int *)(0x404081f8))

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

	/* SPI pad enable */
	tmp = rSYS_PAD_CTRL08;
	tmp &= ~(0xFF << 16);
	tmp |= (0xA8 << 16);
	rSYS_PAD_CTRL08 = tmp;

	/* wifi power enable */
	/* power GPIO115 high for sdio wifi module in shoulang carcorder project */
	rSYS_PAD_CTRL09 &= ~(3 << 30);
	gpio_direction_output(115, 1);

	/* GPIO04 high for power on system */
	rSYS_PAD_CTRL00 &= ~(7 << 12);
	gpio_direction_output(4, 1);

	/* GPIO124 for camera reset */
	//rSYS_PAD_CTRL00 &= ~(7 << 9);
	//gpio_direction_output(124, 1);

	/* select gpio 61-69, 74-79 pad */
	rSYS_PAD_CTRL07 |= (1 << 30);

	rSYS_PAD_DRIVE00 = 0;
	rSYS_PAD_DRIVE01 = 0;

    return 0;
}

int board_late_init(void)
{
	char cmd[128];
	char *need_update;
	unsigned int loadaddr;

	gpio_direction_input(30);
	if (!gpio_get_value(30))
		env_set("need_update", "yes");

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
		}
	}	

	return 0;
}
