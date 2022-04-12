// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Richard Jones, rjones@nexus-tech.net
 */

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <s_record.h>
#include <net.h>
#include <ata.h>
#include <asm/io.h>
#include <mapmem.h>
#include <part.h>
#include <fat.h>
#include <fs.h>
#include <environment.h>


typedef enum {
	MMC = 0,
	USB,
	NFLASH
} MEDIA_TYPE;

static int get_data_from_media(MEDIA_TYPE type, char index, char *file_name)
{
	int ret = -1;
	char cmd[128] = { 0 };
	printf("file_name=%s\n", file_name);
	if (type == NFLASH)
		sprintf(cmd, "ubifsload %s %s ",  env_get("loadaddr"), file_name);
	else if (type == MMC)
		sprintf(cmd, "fatload mmc %d %s %s", index, env_get("loadaddr"), file_name);
	else if (type == USB)
		sprintf(cmd, "fatload usb %d %s %s", index, env_get("loadaddr"), file_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;
}

static int burn_data_2_partition(char *partition_name)
{
	char cmd[128] = { 0 };
	int ret = -1;
	int file_size = 0;

	sprintf(cmd, "nand erase.part %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		return ret;

	file_size = env_get_ulong("filesize", 16, 0x2000);

//      sprintf(cmd, "nand write ${loadaddr} %s ${filesize}",partition_name);

	sprintf(cmd, "nand write %s %s 0x%x", env_get("loadaddr"), partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;
}

static int ark_update_partition(MEDIA_TYPE type, char card_index, char *partition_name, char *file_name)
{
	int ret;
	ret = get_data_from_media(type, card_index, file_name);
	if (!ret) {
		burn_data_2_partition(partition_name);
		return 0;
	}
	return 1;
}


int do_UpdataOther(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	unsigned long type = 0;
	unsigned long card_index = 0;
	char *filename = NULL;
	char *partname = NULL;
	unsigned int ret = 0;

	strict_strtoul(argv[1], 16, &type);
	strict_strtoul(argv[2], 16, &card_index);
	partname = argv[3];
	filename = argv[4];

	ret = ark_update_partition(type, card_index, partname, filename);
	mdelay(1000);

	return ret;
}

U_BOOT_CMD(Updata, 5, 1, do_UpdataOther, "updata boot loader and save the ", "<interface> <dev[:part]> <filename>\n");

unsigned int updata_the_ubootspl(void)
{
	int ret;
	ret = get_data_from_media(2, 0, "ubootspl.bin");
	if (!ret)
	{
	    run_command("switchecc 1", 0);
	    mdelay(100);
	    burn_data_2_partition("bootstrap");
	    mdelay(100);
	    run_command("switchecc 0", 0);
	    return 1;
	}
	return 0;	
}


#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"
int do_Update_flash(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	char cmd[32];
	unsigned int index = 0;
	unsigned int loadaddr;
	char *update_fdt = NULL;
	unsigned int ret = 0;

	sprintf((char *)cmd,"disconfig 0");
	run_command(cmd, 0);

	sprintf(cmd, "ubi part ota");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	sprintf((char *)cmd,"disconfig 5");
	run_command(cmd, 0);
        
	mdelay(300);
	sprintf(cmd, "ubifsmount ubi:ota");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(300);
	sprintf((char *)cmd,"disconfig 8");
	run_command(cmd, 0);

	loadaddr = env_get_hex("loadaddr", 0);
	update_fdt = env_get("nandfdt");
	printf("update_fdt:%s\n", update_fdt);

	sprintf(cmd, "ubifsload %s %s ", env_get("loadaddr"),"update-magic");
	run_command(cmd, 0);
	if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
		//env_set("update_from_flash", "no");
	    	//sprintf(cmd, "saveenv");
		printf("ARK1668_UPDATE_MAGIC CHECK OK!!\n");
		//run_command(cmd, 0);
	} else {
		printf("Wrong update magic, do not update from flash.\n");
		goto bootoldsys;
	}
        sprintf((char *)cmd,"disconfig 10");
        run_command(cmd, 0);

	printf("\r\n *****Now update ubootspl.bin ......\r\n");
		updata_the_ubootspl();

	printf("\r\n **** update from update bootloader .....\r\n");
	sprintf(cmd, "Updata 0x%x 0x%x %s %s", NFLASH, index, "bootloader", "u-boot.img");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	sprintf((char *)cmd,"disconfig 15");
	run_command(cmd, 0);


	printf("\r\n **** update from update fdt .....\r\n");
	sprintf(cmd, "Updata 0x%x 0x%x %s %s", NFLASH, index, "fdt", update_fdt);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	mdelay(30);
	if(ret == 0)
	{
		sprintf(cmd, "setenv fdtsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);
	}
    sprintf((char *)cmd,"disconfig 20");
    run_command(cmd, 0);

	printf("\r\n **** update from update kernel .....\r\n");
	sprintf(cmd, "Updata 0x%x 0x%x %s %s", NFLASH, index, "kernel", "zImage");
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	mdelay(30);
	if(ret == 0)
	{
		sprintf(cmd, "setenv kernelsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
    sprintf((char *)cmd,"disconfig 35");
    run_command(cmd, 0);
        
	printf("\r\n **** update from update rootfs .....\r\n");
	sprintf(cmd, "Updata 0x%x 0x%x %s %s", NFLASH, index, "rootfs", "rootfs.ubi");
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	mdelay(30);
	if(ret == 0)
	{
		sprintf(cmd, "setenv rootfssize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);	
		mdelay(30);
	}
    sprintf((char *)cmd,"disconfig 80");
    run_command(cmd, 0);

	printf("\r\n **** update from update bootloader back.....\r\n");
	sprintf(cmd, "Updata 0x%x 0x%x %s %s", NFLASH, index, "bootloader_back", "u-boot.img");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
    sprintf((char *)cmd,"disconfig 90");
    run_command(cmd, 0);

	env_set("update_from_flash", "no");
	mdelay(10);
	env_set("need_update", "no");
	mdelay(10);
    sprintf((char *)cmd,"disconfig 100");
    run_command(cmd, 0);
bootoldsys:
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(20);
	sprintf(cmd, "run nandboot");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	return 1;
}

U_BOOT_CMD(UpdateFlash, 4, 0, do_Update_flash, "updata Flash ", "<interface> <dev[:part]> <filename>\n");
