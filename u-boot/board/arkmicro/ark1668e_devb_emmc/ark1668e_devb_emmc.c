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


#define rSYS_PAD_CTL3C			*((volatile unsigned int *)(0xe49000F0))


#define rWDT_CR		*((volatile unsigned int *)(0xe4b00000))

#define MUSB_BASE				0xe0100000

#define CHECKDATA_ERROR          	2


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

void dwmci_reset(void)
{
	rSYS_SOFT_RSTNA &= ~((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB &= ~(1<<3);
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB |= (1<<3);
} 

static void usb_controller_reset(void)
{
	rSYS_PAD_CTRL0F &= ~0xfff;

	rSYS_SOFT_RSTNA &= ~(3 << 5);
	udelay(100);
	rSYS_SOFT_RSTNA |= 3 << 5;
	udelay(10);
}

#define ARK_MMC_CLK     	45000000
struct dwmci_host dwmcihost[2];
static int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
{
    struct dwmci_host *host = NULL;
    host = &dwmcihost[index];
	memset(host, 0, sizeof(struct dwmci_host));

	dwmci_select_pad();
	//dwmci_reset();

	/* config clk in sample delay */
	//rSYS_SD_CLK_CFG &= ~(0x7f << 13);
	//rSYS_SD_CLK_CFG |= 52 << 13;
	//rSYS_SD1_CLK_CFG &= ~(0x7f << 13);
	//rSYS_SD1_CLK_CFG |= 52 << 13;

	/* mmc clk axipll(720M) / ((7 + 1) * 2) */
	rSYS_SD_CLK_CFG &= ~0xfff;
	rSYS_SD_CLK_CFG |= (1 << 8) | (1 << 7) | (1 << 5) | 7;
	rSYS_SD1_CLK_CFG &= ~0xfff;
	rSYS_SD1_CLK_CFG |= (1 << 8) | (1 << 7) | (1 << 5) | 7;

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
//	ark_dwmci_init("ARK_MMC0", 0xec400000, 8, 0);
//#ifndef CONFIG_SPL_BUILD
    ark_dwmci_init("ARK_MMC1", 0xec800000, 4, 0);
//#endif
//	ark_dwmci_init("ARK_MMC2", 0xecc00000, 4, 2);
	ark_dwmci_init("ARK_MMC0", 0xec400000, 8, 1);
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
static int get_data_from_media(char *file_name)
{
	int ret = -1;
	char cmd[128] = { 0 };
	printf("file_name=%s\n", file_name);

	sprintf(cmd, "fatload %s %s %s %s",env_get("update_dev_type"),env_get("update_dev_part"),env_get("loadaddr"), file_name);	
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;
}

static int burn_data_2_emmc_partition(char *partition_name)
{
	char cmd[128] = { 0 };
	int ret = -1;
	int file_size = 0;
	unsigned int i;
	unsigned int *srcdata = (unsigned int *)(env_get_hex("loadaddr", 0));
	unsigned int *dstdata = (unsigned int *)(env_get_hex("cmploadaddr", 0));


	sprintf(cmd, "emmc erase.part %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		return ret;

	file_size = env_get_ulong("filesize", 16, 0x2000);

	sprintf(cmd, "emmc write %s %s 0x%x", env_get("loadaddr"), partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);

	sprintf(cmd, "emmc read %s %s 0x%x",env_get("cmploadaddr"),partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	
	for(i = 0;i< file_size/4;i++)
		if(srcdata[i] != dstdata[i])
		{
		    printf("check %s data error!!!\n",partition_name);
		    return CHECKDATA_ERROR;
		}

	return ret;
}

static int ark_update_emmc_partition(char *partition_name, char *file_name)
{
	int ret;
	ret = get_data_from_media(file_name);
	if (!ret) {
		 return burn_data_2_emmc_partition(partition_name);
	}
	return ret;
}

static int ark_update_emmc_rootfs_from_media(char *partition_name)
{
	char cmd[32];
	int ret;
	sprintf(cmd, "updaterootfs %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;	
}


static int do_update_from_media(void)
{
	char cmd[32];
	unsigned int ret = 0;
	char *update_dev = NULL;
	char *curr_partition = NULL;
	char *update_fdt = NULL;
	unsigned char flag_partiton = 0 ;//0--A partiton,1---B partition 
 

	sprintf(cmd, "mmc dev %s",env_get("emmc_dev_part"));
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);

	sprintf((char *)cmd,"disconfig 0");
	run_command(cmd, 0); 

	update_fdt = env_get("emmcfdt");
	printf("update_fdt:%s\n", update_fdt);


	update_dev = env_get("update_dev_type");
	if (!strcmp(update_dev, "mmc"))
	{
		env_set("updata_status", "none");	     
		env_set("updata_from_part", "A");
		curr_partition = env_get("updata_from_part");
		if(!strcmp(curr_partition, "A"))
			flag_partiton = 0;
		else if(!strcmp(curr_partition, "B"))
			flag_partiton = 1;
	
	}else if(!strcmp(update_dev, "usb"))
	{
		env_set("updata_status", "usb");
		curr_partition = env_get("updata_from_part");
		if(!strcmp(curr_partition, "A"))
			flag_partiton = 1;
		else if(!strcmp(curr_partition, "B"))
			flag_partiton = 0;		
	}
				

	printf("\r\n **** update from update ubootspl .....\r\n");
	ret = ark_update_emmc_partition("bootstrap", "ubootspl.bin");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootstrapsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 5");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;	

	printf("\r\n **** update from update uboot .....\r\n");
	ret = ark_update_emmc_partition("bootloader", "u-boot.img");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootloadersize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 10");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;

	printf("\r\n **** update from update fdt  .....\r\n");
	if (flag_partiton == 0)
		ret = ark_update_emmc_partition("fdt", update_fdt);
	else
		ret = ark_update_emmc_partition("fdt_b", update_fdt);
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv fdtsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 15");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;

	printf("\r\n **** update from update kernel .....\r\n");
	if (flag_partiton == 0)
		ret = ark_update_emmc_partition("kernel", "zImage");
	else
		ret = ark_update_emmc_partition("kernel_b", "zImage");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv kernelsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 25");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;

	printf("\r\n **** update from update bootanimation .....\r\n");
	ret = ark_update_emmc_partition("bootanimation", "bootanimation");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootanimationsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 30");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;

	printf("\r\n **** update from update reversingtrack .....\r\n");
	ret = ark_update_emmc_partition("reversingtrack", "reversingtrack");
	mdelay(30);
	if(!ret)
	{	
		sprintf(cmd, "setenv reversingtracksize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 35");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;

	printf("\r\n **** update from update uboot back .....\r\n");
	ret = ark_update_emmc_partition("bootloader_bak", "u-boot.img");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootloadersize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 40");
		run_command(cmd, 0);
	}
	else  if(ret == CHECKDATA_ERROR)
		goto bootoldsys;
	

	printf("\r\n **** update from update rootfs .....\r\n");

	if (flag_partiton == 0)
		ret = ark_update_emmc_rootfs_from_media("rootfs");
	else 
		ret = ark_update_emmc_rootfs_from_media("rootfs_b");
	if(!ret)
	{
		sprintf((char *)cmd,"disconfig 95");
		run_command(cmd, 0);
	}
	else  if(ret == CHECKDATA_ERROR)
		goto bootoldsys;


	printf("\r\n **** set the env to partition .....\r\n");
	//mmc update set part A	
	if(!strcmp(update_dev, "mmc"))
	{
		env_set("updata_from_part", "A");
		env_set("kernel_part", "kernel");
		env_set("fdt_part", "fdt");
		env_set("emmcroot", "/dev/mmcblk0p10 rw");

	}
	else if(!strcmp(update_dev, "usb"))
	{
		curr_partition = env_get("updata_from_part");	
		if(!strcmp(curr_partition, "A"))
		{
			env_set("updata_from_part", "B");
			env_set("kernel_part", "kernel_b");
			env_set("fdt_part", "fdt_b");
			env_set("emmcroot", "/dev/mmcblk0p14 rw");
		}		
		else if(!strcmp(curr_partition, "B"))
		{
			env_set("updata_from_part", "A");
			env_set("kernel_part", "kernel");
			env_set("fdt_part", "fdt");
			env_set("emmcroot", "/dev/mmcblk0p10 rw");
		}

	}

	mdelay(5);
	env_set("need_update", "no");
	mdelay(5);
	env_set("updata_status", "none");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(30);
	sprintf((char *)cmd,"disconfig 100");
	run_command(cmd, 0);
	printf("\r\n **** update the device over! .....\r\n");	
	return 0;
bootoldsys:
	env_set("updata_status", "error");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(20);
	sprintf((char *)cmd,"disconfig 100");
	run_command(cmd, 0);

	return 1;
}

int board_late_init(void)
{
	char cmd[128];
	char *update_from_ota = NULL,*need_update = NULL;
	unsigned int loadaddr;
	int do_update = 0, update_from_mmc = 1;


#ifdef CONFIG_USB_MUSB_HOST
	usb_controller_reset();
	musb_register(&musb_platform_data, NULL, (void *)MUSB_BASE);
#endif

	update_from_ota = env_get("update_from_ota");	
    printf("update_from_ota %s\n",update_from_ota);
 	need_update = env_get("need_update");
	if (!strcmp(need_update, "yes")) {
		loadaddr = env_get_hex("loadaddr", 0);

		sprintf(cmd, "fatload %s %s %s update-magic", "mmc", env_get("sd_dev_part"), env_get("loadaddr"));
		printf("cmd %s\n",cmd);
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
	else if(!strcmp(update_from_ota, "yes")){

		do_update = 0;		
		sprintf(cmd, "update_from_emmc_ota");
		printf("cmd=%s\n", cmd);	
		run_command(cmd, 0); 
	}
update_done:
	if (do_update) {
		run_command("emmc erase.part userdata", 0);
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
			env_set("updata_status", "usb");
		}
		do_update_from_media();
	} else {
		env_set("do_update", "no");
	}

	return 0;
}
