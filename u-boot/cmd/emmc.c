// SPDX-License-Identifier: GPL-2.0+
/*
 *
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <environment.h>
#include <malloc.h>
#include <mmc.h>
#include <linux/math64.h>
#include <part.h>
#include <fat.h>
#include <fs.h>

#define BDEVNAME_SIZE   32
#define PAGE_SIZE (2048)
#define CHECKDATA_ERROR          	2

typedef unsigned long long sector_t;

struct cmdline_subpart {
	char name[BDEVNAME_SIZE]; /* partition name, such as 'rootfs' */
	sector_t from;
	sector_t size;
	struct cmdline_subpart *next_subpart;
};

struct cmdline_parts {
	char name[BDEVNAME_SIZE]; /* block device, such as 'mmcblk0' */
	unsigned int nr_subparts;
	struct cmdline_subpart *subpart;
	struct cmdline_parts *next_parts;
};

static struct cmdline_parts *bdev_parts;

static unsigned long long memparse(const char *ptr, char **retptr)
{
    char *endptr;   /* local pointer to end of parsed string */

    unsigned long long ret = simple_strtoull(ptr, &endptr, 0);

    switch (*endptr) {
    case 'E':
    case 'e':
        ret <<= 10;
    case 'P':
    case 'p':
        ret <<= 10;
    case 'T':
    case 't':
        ret <<= 10;
    case 'G':
    case 'g':
        ret <<= 10;
    case 'M':
    case 'm':
        ret <<= 10;
    case 'K':
    case 'k':
        ret <<= 10;
        endptr++;
    default:
        break;
    }

    if (retptr)
        *retptr = endptr;

    return ret;
}


static int parse_subpart(struct cmdline_subpart **subpart, char *partdef)
{
	int ret = 0;
	struct cmdline_subpart *new_subpart;

	*subpart = NULL;

	new_subpart = malloc(sizeof(struct cmdline_subpart));
	if (!new_subpart)
		return -ENOMEM;
	memset(new_subpart, 0, sizeof(struct cmdline_subpart));

	if (*partdef == '-') {
		new_subpart->size = (sector_t)(~0ULL);
		partdef++;
	} else {
		new_subpart->size = (sector_t)memparse(partdef, &partdef);
		if (new_subpart->size < (sector_t)PAGE_SIZE) {
			pr_warn("cmdline partition size is invalid.");
			ret = -EINVAL;
			goto fail;
		}
	}

	if (*partdef == '@') {
		partdef++;
		new_subpart->from = (sector_t)memparse(partdef, &partdef);
	} else {
		new_subpart->from = (sector_t)(~0ULL);
	}

	if (*partdef == '(') {
		int length;
		char *next = strchr(++partdef, ')');

		if (!next) {
			pr_warn("cmdline partition format is invalid.");
			ret = -EINVAL;
			goto fail;
		}

		length = min_t(int, next - partdef,
			       sizeof(new_subpart->name) - 1);
		strncpy(new_subpart->name, partdef, length);
		new_subpart->name[length] = '\0';

		partdef = ++next;
	} else
		new_subpart->name[0] = '\0';

	*subpart = new_subpart;
	return 0;
fail:
	free(new_subpart);
	return ret;
}

static void free_subpart(struct cmdline_parts *parts)
{
	struct cmdline_subpart *subpart;

	while (parts->subpart) {
		subpart = parts->subpart;
		parts->subpart = subpart->next_subpart;
		free(subpart);
	}
}

static int parse_parts(struct cmdline_parts **parts, const char *bdevdef)
{
	int ret = -EINVAL;
	char *next;
	int length;
	struct cmdline_subpart **next_subpart;
	struct cmdline_parts *newparts;
	char buf[BDEVNAME_SIZE + 32 + 4];

	*parts = NULL;

	newparts = malloc(sizeof(struct cmdline_parts));
	if (!newparts)
		return -ENOMEM;
	memset(newparts, 0, sizeof(struct cmdline_parts));

	next = strchr(bdevdef, ':');
	if (!next) {
		pr_warn("cmdline partition has no block device.");
		goto fail;
	}

	length = min_t(int, next - bdevdef, sizeof(newparts->name) - 1);
	strncpy(newparts->name, bdevdef, length);
	newparts->name[length] = '\0';
	newparts->nr_subparts = 0;

	next_subpart = &newparts->subpart;

	while (next && *(++next)) {
		bdevdef = next;
		next = strchr(bdevdef, ',');

		length = (!next) ? (sizeof(buf) - 1) :
			min_t(int, next - bdevdef, sizeof(buf) - 1);

		strncpy(buf, bdevdef, length);
		buf[length] = '\0';

		ret = parse_subpart(next_subpart, buf);
		if (ret)
			goto fail;

		newparts->nr_subparts++;
		next_subpart = &(*next_subpart)->next_subpart;
	}

	if (!newparts->subpart) {
		pr_warn("cmdline partition has no valid partition.");
		ret = -EINVAL;
		goto fail;
	}

	*parts = newparts;

	return 0;
fail:
	free_subpart(newparts);
	free(newparts);
	return ret;
}

void cmdline_parts_free(struct cmdline_parts **parts)
{
	struct cmdline_parts *next_parts;

	while (*parts) {
		next_parts = (*parts)->next_parts;
		free_subpart(*parts);
		free(*parts);
		*parts = next_parts;
	}
}

static int cmdline_parts_parse(struct cmdline_parts **parts, const char *cmdline)
{
	int ret;
	char *buf;
	char *pbuf;
	char *next;
	struct cmdline_parts **next_parts;

	*parts = NULL;

	next = pbuf = buf = strdup(cmdline);
	if (!buf)
		return -ENOMEM;

	next_parts = parts;

	while (next && *pbuf) {
		next = strchr(pbuf, ';');
		if (next)
			*next = '\0';

		ret = parse_parts(next_parts, pbuf);
		if (ret)
			goto fail;

		if (next)
			pbuf = ++next;

		next_parts = &(*next_parts)->next_parts;
	}

	if (!*parts) {
		pr_warn("cmdline partition has no valid partition.");
		ret = -EINVAL;
		goto fail;
	}

	ret = 0;
done:
	free(buf);
	return ret;

fail:
	cmdline_parts_free(parts);
	goto done;
}

static int emmcparts_init(struct mmc *mmc)
{
	static int emmcparts_has_init = 0;
	const char *cmdline = CONFIG_EMMCPARTS_DEFAULT;

	if (emmcparts_has_init)
		return 0;

	if (cmdline && cmdline_parts_parse(&bdev_parts, cmdline)) {
		bdev_parts = NULL;
		return -1;
	}

	sector_t from = 0;
	sector_t disk_size = mmc->capacity;
	struct cmdline_subpart *subpart;

	for (subpart = bdev_parts->subpart; subpart; subpart = subpart->next_subpart) {
		if (subpart->from == (sector_t)(~0ULL))
			subpart->from = from;
		else
			from = subpart->from;

		if (from >= disk_size)
			break;

		if (subpart->size > (disk_size - from))
			subpart->size = disk_size - from;

		pr_debug("subpart %s from 0x%llx, size 0x%llx.\n", subpart->name, subpart->from, subpart->size);

		from += subpart->size;
	}

	emmcparts_has_init = 1;

	return 0;
}

static struct mmc *init_emmc_device(void)
{
	struct mmc *mmc;
	int dev = simple_strtoul(CONFIG_EMMC_DEV_PART, NULL, 10);

	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no emmc device at slot %x\n", dev);
		return NULL;
	}

	if (mmc_init(mmc))
		return NULL;

	emmcparts_init(mmc);

	return mmc;
}

int get_part(const char *partname, loff_t *off, loff_t *size, loff_t *maxsize)
{
	struct cmdline_subpart *subpart;

	if (!bdev_parts) {
		printf("not found emmc partition table.\n");
		return -1;	
	}

	for (subpart = bdev_parts->subpart; subpart; subpart = subpart->next_subpart) {
		if (!strcmp(subpart->name, partname)) {
			*off = subpart->from;
			*size = subpart->size;
			*maxsize = subpart->size;
			return 0;			
		}
	}

	printf("not found part %s.\n", partname);
	return -1;
}

static int emmc_arg_off(const char *arg, loff_t *off, loff_t *size,
		loff_t *maxsize, uint64_t chipsize)
{
	if (!str2off(arg, off))
		return get_part(arg, off, size, maxsize);

	if (*off >= chipsize) {
		puts("Offset exceeds device limit\n");
		return -1;
	}

	*maxsize = chipsize - *off;
	*size = *maxsize;
	return 0;
}

static int emmc_arg_off_size(int argc, char *const argv[], loff_t *off,
		     loff_t *size, uint64_t chipsize)
{
	loff_t maxsize;
	int ret;

	if (argc == 0) {
		*off = 0;
		*size = chipsize;
		goto print;
	}

	ret = emmc_arg_off(argv[0], off, size, &maxsize, chipsize);
	if (ret)
		return ret;

	if (argc == 1)
		goto print;	

	if (!str2off(argv[1], size)) {
		printf("'%s' is not a number\n", argv[1]);
		return -1;
	}

	if (*size > maxsize) {
		puts("Size exceeds partition or device limit\n");
		return -1;
	}

print:
	if (*size == chipsize)
		puts("whole chip\n");
	else
		printf("offset 0x%llx, size 0x%llx\n",
		       (unsigned long long)*off, (unsigned long long)*size);

	return 0;
}

static int do_emmc_read(cmd_tbl_t *cmdtp, int flag,
		       int argc, char * const argv[])
{
	struct mmc *mmc;
	loff_t off, size;
	u32 blk, cnt, n;
	void *addr;

	if (argc != 3 && argc != 4)
		return CMD_RET_USAGE;

	addr = (void *)simple_strtoul(argv[1], NULL, 16);

	mmc = init_emmc_device();
	if (!mmc)
		return CMD_RET_FAILURE;

	emmc_arg_off_size(argc - 2, argv + 2, &off, &size, mmc->capacity);

	struct blk_desc *blkdesc = mmc_get_blk_desc(mmc);
	if (off & blkdesc->blksz) {
		printf("off(0x%llx) is not align to blksz(%lu).\n", off, blkdesc->blksz);
		return CMD_RET_FAILURE;
	}
	blk = div_u64(off, blkdesc->blksz);

	u32 remainder;
	cnt = div_u64_rem(size, blkdesc->blksz, &remainder);
	if (remainder) cnt += 1;
	printf("EMMC read: block # %d, count %d ... ",
	       blk, cnt);

	n = blk_dread(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks read: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

#if CONFIG_IS_ENABLED(MMC_WRITE)
static int do_emmc_write(cmd_tbl_t *cmdtp, int flag,
			int argc, char * const argv[])
{
	struct mmc *mmc;
	loff_t off, size;
	u32 blk, cnt, n;
	void *addr;

	if (argc != 3 && argc != 4)
		return CMD_RET_USAGE;

	addr = (void *)simple_strtoul(argv[1], NULL, 16);

	mmc = init_emmc_device();
	if (!mmc)
		return CMD_RET_FAILURE;

	emmc_arg_off_size(argc - 2, argv + 2, &off, &size, mmc->capacity);

	struct blk_desc *blkdesc = mmc_get_blk_desc(mmc);
	if (off & blkdesc->blksz) {
		printf("off(0x%llx) is not align to blksz(%lu).\n", off, blkdesc->blksz);
		return CMD_RET_FAILURE;
	}
	blk = div_u64(off, blkdesc->blksz);

	u32 remainder;
	cnt = div_u64_rem(size, blkdesc->blksz, &remainder);
	if (remainder) cnt += 1;
	printf("EMMC write: block # %d, count %d ... ",
	       blk, cnt);

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return CMD_RET_FAILURE;
	}
	n = blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks written: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int do_emmc_erase(cmd_tbl_t *cmdtp, int flag,
			int argc, char * const argv[])
{
	struct mmc *mmc;
	loff_t off, size;
	u32 blk, cnt, n;
	int args = 2;

	if (argc > 3)
		return CMD_RET_USAGE;

	mmc = init_emmc_device();
	if (!mmc)
		return CMD_RET_FAILURE;

	if (argv[0][5] != 0) {
		if (!strcmp(&argv[0][5], ".part")) {
			args = 1;
		} else if (!strcmp(&argv[0][5], ".chip")) {
			args = 0;
		} else {
			goto usage;
		}
	}

	emmc_arg_off_size(args, argv + 1, &off, &size, mmc->capacity);

	struct blk_desc *blkdesc = mmc_get_blk_desc(mmc);
	if (off & blkdesc->blksz) {
		printf("off(0x%llx) is not align to blksz(%lu).\n", off, blkdesc->blksz);
		return CMD_RET_FAILURE;
	}
	blk = div_u64(off, blkdesc->blksz);

	u32 remainder;
	cnt = div_u64_rem(size, blkdesc->blksz, &remainder);
	if (remainder) cnt += 1;
	printf("EMMC erase: block # %d, count %d ... ",
	       blk, cnt);

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return CMD_RET_FAILURE;
	}
	n = blk_derase(mmc_get_blk_desc(mmc), blk, cnt);
	printf("%d blocks erased: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;

usage:
	return CMD_RET_USAGE;
}
#endif

static cmd_tbl_t cmd_mmc[] = {
	U_BOOT_CMD_MKENT(read, 4, 1, do_emmc_read, "", ""),
#if CONFIG_IS_ENABLED(MMC_WRITE)
	U_BOOT_CMD_MKENT(write, 4, 0, do_emmc_write, "", ""),
	U_BOOT_CMD_MKENT(erase, 3, 0, do_emmc_erase, "", ""),
#endif
};

static int do_emmcops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_mmc, ARRAY_SIZE(cmd_mmc));

	/* Drop the mmc command */
	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

#ifdef CONFIG_SYS_LONGHELP
static char emmc_help_text[] =
	"emmc read - addr off|partition size\n"
	"emmc write - addr off|partition size\n"
	"    read/write 'size' bytes starting at offset 'off'\n"
	"    to/from memory address 'addr'.\n"
	"emmc erase off size - erase 'size' bytes "
	"from offset 'off'\n"
	"emmc erase.part partition - erase entire partition'\n"
	"emmc erase.chip - erase entire chip'\n"
	"";
#endif

U_BOOT_CMD(
	emmc, CONFIG_SYS_MAXARGS, 1, do_emmcops,
	"EMMC sub-system", emmc_help_text
);

/**
 * Format and print out a partition list for each device from global device
 * list.
 */
static void list_partitions(void)
{
	struct cmdline_subpart *subpart;
	int part_num = 0;

	printf("\ndevice %s, # parts = %d\n",
		bdev_parts->name, bdev_parts->nr_subparts);
	printf(" #: name\t\tsize\t\toffset\n");

	for (subpart = bdev_parts->subpart; subpart; subpart = subpart->next_subpart) {
		printf("%2d: %-20s0x%08llx\t0x%08llx\n",
				part_num++, subpart->name, subpart->size,
				subpart->from);
	}
}

static int do_emmcparts(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	struct mmc *mmc;

	mmc = init_emmc_device();
	if (!mmc)
		return CMD_RET_FAILURE;

	if (!bdev_parts) {
		printf("not found emmc partition table.\n");
		return CMD_RET_FAILURE;
	}
	
	list_partitions();

	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char emmcparts_help_text[] =
	"\n"
	"    - list emmc partition table\n"
	"";
#endif

U_BOOT_CMD(
	emmcparts,	1,	0,	do_emmcparts,
	"list emmc partitions", emmcparts_help_text
);
void get_Emmc_Parition_info(const char *filename,loff_t *off,loff_t *rtsize,loff_t *maxsize)
{
	char cmd[128];
	unsigned int pos = 0,i;
	unsigned int size = 0;
	unsigned int leftsize ;
	struct mmc *mmc;
	 
	mmc = init_emmc_device();
	if (!mmc)
	   return CMD_RET_FAILURE;

	if(get_part(filename,off,rtsize, maxsize) == 0)
	    printf("get Emmc part info filename %s!!\n",filename);

	printf("off = 0x%llx ,rtsize = 0x%llx,maxsize = 0x%llx\n",*off,*rtsize,*maxsize);
	return 0;
}


#if 1
#define FAT_READ_SIZE   0x2000000
#define ROOTFS_SIZE  	0x1f400000

static int do_update_rootfs (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int rootfsize = 0;
	char cmd[128];
	unsigned int pos = 0,i = 0;
	unsigned int size = 0;
	unsigned int leftsize,k;
	struct mmc *mmc;
	loff_t off  = 0;
	loff_t rtsize = 0;
	loff_t maxsize= 0;
	unsigned int count = 0;
	unsigned int ret = 0;
	unsigned int file_size = 0;	 
	unsigned int *srcdata = (unsigned int *)(env_get_hex("loadaddr", 0));
	unsigned int *dstdata = (unsigned int *)(env_get_hex("cmploadaddr", 0));

	mmc = init_emmc_device();
	if (!mmc)
	   return CMD_RET_FAILURE;
    printf("argv[0] %s , argv[1] %s\n",argv[0],argv[1]);
	if(get_part(argv[1],&off, &rtsize, &maxsize) == 0)
	    printf("get Emmc part info argv[1] %s!!\n",argv[1]);

	printf("off = 0x%llx ,rtsize = 0x%llx,maxsize = 0x%llx\n",off,rtsize,maxsize);
#if 1
	sprintf(cmd, "fatsize %s %s rootfs.ext2", env_get("update_dev_type"), env_get("update_dev_part"),argv[2]);
	printf("cmd = %s\n",cmd);
	ret = run_command(cmd, 0);
	if(ret)
		return ret;

	mdelay(100);
	rootfsize = env_get_hex("filesize", 0);
	printf("rootfsize = 0x%x\n",rootfsize);
	if(rootfsize == 0)
	{
	   printf("Error:rootfs file maybe not in device!!!\n");
	   return 1;
	}
#endif
	size = FAT_READ_SIZE;
	count = 55/((rootfsize/size)+1);

	for(pos = 0,i = 1;pos < rootfsize;i++)
	{
        
        sprintf(cmd, "fatload %s %s %s rootfs.ext2 0x%x 0x%x", env_get("update_dev_type"), env_get("update_dev_part"), env_get("loadaddr"),size,pos);
        printf("cmd = %s\n",cmd);
        ret = run_command(cmd, 0);
        if(ret)
           return ret;
        mdelay(100); 
        file_size = env_get_ulong("filesize", 16, 0x2000);
        sprintf(cmd, "emmc erase 0x%llx 0x%x",off,size);
        printf("cmd = %s\n",cmd);
        run_command(cmd, 0);
        mdelay(100);
	sprintf((char *)cmd,"disconfig %d",(40+i*count));
	printf("cmd = %s\n",cmd);
	run_command(cmd, 0);
        sprintf(cmd, "emmc write %s 0x%llx 0x%x",env_get("loadaddr"),off,size);
        printf("cmd = %s\n",cmd);
        run_command(cmd, 0);
        mdelay(100);	 

        sprintf(cmd, "emmc read %s 0x%llx 0x%x",env_get("cmploadaddr"),off,size);
        printf("cmd = %s\n",cmd);
        run_command(cmd, 0);
        mdelay(100);	
	for(k = 0;k< file_size/4;k++)
		if(srcdata[k] != dstdata[k])
		{
		    printf("check rootfs data error!!!\n");
		    return CHECKDATA_ERROR;
		}
  
        off = off + size;		
        pos += size;
        if(file_size < FAT_READ_SIZE)
        {
            printf("data is over!!!");            
            break;
        }
	}

	sprintf(cmd, "setenv rootfssize 0x%x ",rootfsize);
	printf("cmd = %s\n",cmd);
	run_command(cmd, 0);
	return 0;
}


U_BOOT_CMD(
	updaterootfs,	6,	0,	do_update_rootfs,
	"load binary file from a dos filesystem",
	"<interface> <dev[:part]> <addr> <filename> [<bytes>]\n"
	"- write file 'filename' from the address 'addr' in RAM\n"
	"to 'dev' on 'interface'"

);

static int do_update_rootfs_from_ota(char *partition_name, char *file_name)
{
	unsigned int rootfsize = 0;
	char cmd[128];
	unsigned int pos = 0,i = 0,k;
	unsigned int size = 0;
	unsigned int leftsize ;
	struct mmc *mmc;
	loff_t off  = 0;
	loff_t rtsize = 0;
	loff_t maxsize= 0;
	unsigned int ret = 0;	 
	unsigned int file_size = 0;
	unsigned int count=0,step = 0; 
	unsigned int *srcdata = (unsigned int *)(env_get_hex("loadaddr", 0));
	unsigned int *dstdata = (unsigned int *)(env_get_hex("cmploadaddr", 0));	

	mmc = init_emmc_device();
	if (!mmc)
	   return CMD_RET_FAILURE;

    	printf("partition_name %s ,file_name %s\n",partition_name,file_name);

	if(get_part(partition_name,&off, &rtsize, &maxsize) == 0)
	     printf("get Emmc part info partition_name %s!!\n",partition_name);


	size = FAT_READ_SIZE;
	count = 55/((ROOTFS_SIZE/size)+1);
    while(1)
	{     
		i++;  
		sprintf(cmd, "fatload emmc ota %s %s 0x%x 0x%x",env_get("loadaddr"),file_name,size,pos);
		printf("cmd = %s\n",cmd);
		ret = run_command(cmd, 0);
		if(ret)
		    return ret;         
		file_size = env_get_ulong("filesize", 16, 0x2000);
		if(file_size == 0)
		    break;
		sprintf(cmd, "emmc erase 0x%llx 0x%x",off,size);
		printf("cmd = %s\n",cmd);
		run_command(cmd, 0);
		mdelay(200);
		step = (40+i*5);
		if(step>95)
			step = 95;  
		sprintf((char *)cmd,"disconfig %d",step);
		printf("cmd = %s\n",cmd);
		run_command(cmd, 0);
		mdelay(200);

		sprintf(cmd, "emmc write %s 0x%llx 0x%x",env_get("loadaddr"),off,size);
		printf("cmd = %s\n",cmd);
		run_command(cmd, 0);
		mdelay(200);

	 	sprintf(cmd, "emmc read %s 0x%llx 0x%x",env_get("cmploadaddr"),off,size);
		printf("cmd = %s\n",cmd);
		run_command(cmd, 0);
		mdelay(100);	
		for(k = 0;k< file_size/4;k++)
			if(srcdata[k] != dstdata[k])
			{
			    printf("check rootfs data error!!!\n");
			    return CHECKDATA_ERROR;
			}


		off = off + size;	
		pos += size;
		rootfsize = rootfsize + size;
		if(file_size < FAT_READ_SIZE)
		    break; 
	}
	sprintf(cmd, "setenv rootfssize 0x%x ",rootfsize);
	printf("cmd = %s\n",cmd);
	run_command(cmd, 0);
	return 0;
}


static int get_data_from_ota(char *file_name)
{
	int ret = -1;
	char cmd[128] = { 0 };
	printf("file_name=%s\n", file_name);

	sprintf(cmd, "fatload emmc ota %s %s",env_get("loadaddr"), file_name);	
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;
}

static int burn_data_2_emmc_partition(char *partition_name)
{
	char cmd[128] = { 0 };
	int ret = -1;
	int file_size = 0;

	sprintf(cmd, "emmc erase.part %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		return ret;

	file_size = env_get_ulong("filesize", 16, 0x2000);

	sprintf(cmd, "emmc write %s %s 0x%x", env_get("loadaddr"), partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	return ret;
}

static int ark_update_emmc_partition(char *partition_name, char *file_name)
{
	int ret;
	ret = get_data_from_ota(file_name);
	if (!ret) {
		burn_data_2_emmc_partition(partition_name);
	}
	return ret;
}


static int ark_update_emmc_rootfs_from_ota(char *partition_name, char *file_name)
{
	char cmd[32];
	unsigned int ret = 0;
    	ret = do_update_rootfs_from_ota(partition_name,file_name);
	return ret;
}

#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"
int do_update_from_ota(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	char cmd[32];
	unsigned int index = 0;
	unsigned int ret = 0;
	unsigned int loadaddr; 
	char *update_from_ota = NULL;
	char *update_fdt = NULL;
	update_from_ota = env_get("update_from_ota");	
	unsigned char flag_partiton = 0 ;
	char *curr_partition = NULL;

	sprintf(cmd, "mmc dev %s",env_get("emmc_dev_part"));
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);

	sprintf((char *)cmd,"disconfig 0");
	run_command(cmd, 0);

	update_fdt = env_get("emmcfdt");
	printf("update_fdt:%s\n", update_fdt);

	env_set("updata_status", "ota");

	loadaddr = env_get_hex("loadaddr", 0);

	if (loadaddr)
		memset((void*)loadaddr, 0, strlen(ARK1668_UPDATE_MAGIC));

	sprintf(cmd, "fatload emmc ota %s %s ", env_get("loadaddr"),"update-magic");
	run_command(cmd, 0);
	if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
		printf("ARK1668_UPDATE_MAGIC CHECK OK!!\n");

	} else {
		printf("Wrong update magic, do not update from emmc.\n");
		goto bootoldsys;
	}
	sprintf((char *)cmd,"disconfig 8");
	run_command(cmd, 0);

	curr_partition = env_get("updata_from_part");
	if(!strcmp(curr_partition, "A"))
		flag_partiton = 1;
	else if(!strcmp(curr_partition, "B"))
		flag_partiton = 0;

	printf("\r\n **** update from update ubootspl .....\r\n");
	ret = ark_update_emmc_partition("bootstrap", "ubootspl.bin");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootstrapsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 10");
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
	else if (flag_partiton == 1)
		ret = ark_update_emmc_partition("fdt_b", update_fdt);

	mdelay(30);
	if(!ret)
	{
		if(flag_partiton == 0)		
			sprintf(cmd, "setenv fdtsize_a %s",env_get("filesize"));
		else
			sprintf(cmd, "setenv fdtsize_b %s",env_get("filesize"));
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
	else if (flag_partiton == 1)
		ret = ark_update_emmc_partition("kernel_b", "zImage");

	mdelay(30);
	if(!ret)
	{
		if(flag_partiton == 0)		
			sprintf(cmd, "setenv kernelsize_a %s",env_get("filesize"));
		else
			sprintf(cmd, "setenv kernelsize_b %s",env_get("filesize"));
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
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;
   
	printf("\r\n **** update from update rootfs .....\r\n");
	if (flag_partiton == 0)
		ret = ark_update_emmc_rootfs_from_ota("rootfs", "rootfs.ext2");
	else if (flag_partiton == 1)
		ret = ark_update_emmc_rootfs_from_ota("rootfs_b", "rootfs.ext2");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv rootfssize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);	
		mdelay(30);
		sprintf((char *)cmd,"disconfig 95");
		run_command(cmd, 0);
	}
	else if(ret == CHECKDATA_ERROR)
		goto bootoldsys;


	curr_partition = env_get("updata_from_part");	
	if(!strcmp(curr_partition, "A"))
	{
		env_set("updata_from_part", "B");
		env_set("kernel_part", "kernel_b");
		env_set("fdt_part", "fdt_b");
		env_set("emmcroot", "/dev/mmcblk0p14 ro");
		sprintf(cmd, "setenv fdtsize %s",env_get("fdtsize_b"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);

		sprintf(cmd, "setenv kernelsize %s",env_get("kernelsize_b"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);

	}		
	else if(!strcmp(curr_partition, "B"))
	{
		env_set("updata_from_part", "A");
		env_set("kernel_part", "kernel");
		env_set("fdt_part", "fdt");
		env_set("emmcroot", "/dev/mmcblk0p10 ro");

		sprintf(cmd, "setenv fdtsize %s",env_get("fdtsize_a"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);

		sprintf(cmd, "setenv kernelsize %s",env_get("kernelsize_a"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);	
		mdelay(30);

	}

	env_set("updata_status", "ok");
	mdelay(30);
	env_set("need_update", "no");
	mdelay(30);
	env_set("update_from_ota", "no");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(30);
	sprintf((char *)cmd,"disconfig 100");
	run_command(cmd, 0);
	mdelay(20);
	sprintf(cmd, "run emmcboot");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);

	return 0;
bootoldsys:
	env_set("update_from_ota", "no");
	mdelay(30);
	env_set("updata_status", "error");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(20);
	sprintf(cmd, "run emmcboot");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);

	return 1;
}

U_BOOT_CMD(
	update_from_emmc_ota, 4, 0, do_update_from_ota, 
	"updata from emmc ota partiton", 
	"<interface> <dev[:part]> <filename>\n"
);
#if 0
int do_boot_from_part(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	char cmd[32];
	unsigned int index = 0;
	unsigned int ret = 0;
	char *load_from_kernel_part = NULL;

	load_from_kernel_part = env_get("load_from_part");
	if(!strcmp(load_from_kernel_part, "A"))
	{
	     env_set("kernel_part", "kernel");
	     env_set("emmcroot", "/dev/mmcblk1p10 rw");  
	}
	else
	{
	     env_set("kernel_part", "kernel_b");
	     env_set("emmcroot", "/dev/mmcblk1p13 rw");
	}
    printf("load_from_kernel_part %s\n",load_from_kernel_part);

    sprintf(cmd, "saveenv");
    printf("cmd=%s\n", cmd);
    run_command(cmd, 0);
	return 1;
}

U_BOOT_CMD(
	boot_from_part_set, 4, 0, do_boot_from_part, 
	"updata from emmc ota partiton", 
	"<interface> <dev[:part]> <filename>\n"
);
#endif

#endif


