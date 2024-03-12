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
#include <asm/arch/ark-common.h>
extern struct SYS_INFO sys_info;
extern int burn_data_to_partition(char *partition_name);
extern int ark_check_data_from_partition(char *part_name,unsigned int crc_data);
extern int ark_check_data_from_devide(char *file_name,unsigned int crc_data);
typedef enum {
	MMC = 0,
	USB,
	NFLASH
} MEDIA_TYPE;
static int get_data_from_ota(char *file_name)
{
	int ret = -1;
	char cmd[128] = { 0 };
	unsigned int file_size,crc_src;
	unsigned int *srcdata = (unsigned int *)(env_get_hex("loadaddr", 0));
	printf("file_name=%s\n", file_name);
	sprintf(cmd, "ubifsload %s %s ",  env_get("loadaddr"), file_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
		return 1;
	file_size = env_get_ulong("filesize", 16, 0x2000);
	crc_src = crc32(0, (const unsigned char *)srcdata, file_size);
	ret = ark_check_data_from_devide(file_name,crc_src);
	if(ret)
		return 1;
	return 0;
}
static int ark_update_partition(char *partition_name, char *file_name)
{
	int ret;
	ret = get_data_from_ota(file_name);
	if (!ret) {
		burn_data_to_partition(partition_name);
		return 0;
	}
	return 1;
}
unsigned int updata_the_ubootspl(void)
{
	int ret;
	ret = get_data_from_ota("ubootspl.bin");
	if (!ret)
	{
	    run_command("switchecc 1", 0);
	    mdelay(100);
		ret = burn_data_to_partition("bootstrap");
	    mdelay(100);
	    run_command("switchecc 0", 0);
		if(ret)
			return 1;
	}
	return 0;
}
int get_crc_data_from_ota(char *file_name)
{
	int ret = -1;
	unsigned int file_size;
	char cmd[128] = { 0 };
	unsigned int *srcdata = (unsigned int *)(env_get_hex("loadaddr", 0));
	memset(&sys_info, 0, sizeof(sys_info));
	printf("file_name=%s\n", file_name);
	sprintf(cmd, "ubifsload %s %s ", env_get("loadaddr"),file_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
		return 1;
	file_size = env_get_ulong("filesize", 16, 0x2000);
	memcpy(&sys_info, srcdata, file_size);

	printf("uboot_crc 		  = 0x%x\n",sys_info.uboot_crc);
	printf("fdt_crc			  = 0x%x\n",sys_info.fdt_crc);
	printf("zImage_crc 		  = 0x%x\n",sys_info.zImage_crc);
	printf("rootfs_crc		  = 0x%x\n",sys_info.rootfs_crc);
	printf("ubootspl_crc	  = 0x%x\n",sys_info.ubootspl_crc);
	printf("bootanimation_crc = 0x%x\n",sys_info.bootanimation_crc);
	printf("bootlogo_crc	  = 0x%x\n",sys_info.bootlogo_crc);
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
	unsigned int resetuboot = 0;
	unsigned int dis_pos_x;
	unsigned int dis_pos_y;
	resetuboot = env_get_hex("ubootreset", 0);
	printf("resetuboot = %d \n",resetuboot);
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
	update_fdt = env_get("boardfdt");
	printf("update_fdt:%s\n", update_fdt);
	sprintf(cmd, "ubifsload %s %s ", env_get("loadaddr"),"update-magic");
	run_command(cmd, 0);
	if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
		printf("ARK1668_UPDATE_MAGIC CHECK OK!!\n");
	}else {
		printf("Wrong update magic, do not update from flash.\n");
		goto bootoldsys;
	  }
    sprintf((char *)cmd,"disconfig 10");
    run_command(cmd, 0);
	get_crc_data_from_ota("crcdata.bin");
	dis_pos_x = 6;
	dis_pos_y = 20;
	printf("\r\n *****Now update ubootspl.bin ......\r\n");
	ret = updata_the_ubootspl();
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootstrapsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 5");
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootstrap",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		mdelay(30);
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootstrap",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("bootloader", "u-boot.img");
	if(!ret)
	{
		sprintf(cmd, "setenv bootloadersize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 10");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		if(resetuboot == 0 )
		{
			env_set("ubootreset", "1");
			env_set("update_from_ota", "yes");
			env_set("need_update", "no");
			run_command("saveenv", 0);
			mdelay(20);
			run_command("reset", 0);
			while(1);
		}
		env_set("ubootreset", "0");
		run_command("saveenv", 0);
		mdelay(20);

	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("fdt", update_fdt);
	if(!ret)
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf(cmd, "setenv fdtsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 15");
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"fdt",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"fdt",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("kernel", "zImage");
	if(!ret)
	{
		sprintf(cmd, "setenv kernelsize_a %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 25");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"kernel",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"kernel",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
#if 0
	ret = ark_update_partition("bootanimation", "bootanimation");
	if(!ret)
	{
		sprintf(cmd, "setenv bootanimationsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 30");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootanimation",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootanimation",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
#endif
	ret = ark_update_partition("bootloader_bak", "u-boot.img");
	if(!ret)
	{
		sprintf((char *)cmd,"disconfig 40");
		run_command(cmd, 0);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader_bak",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader_bak",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("rootfs","rootfs.ubi");
	if(!ret)
	{
		sprintf((char *)cmd,"disconfig 95");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"rootfs",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"rootfs",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	printf("##set update_from_flash no##\n");
	env_set("update_from_ota", "no");
	mdelay(10);
	env_set("need_update", "no");
	mdelay(10);
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
    sprintf((char *)cmd,"disconfig 100");
    run_command(cmd, 0);
	return 0;
bootoldsys:
	env_set("update_from_ota", "no");
	mdelay(10);
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
