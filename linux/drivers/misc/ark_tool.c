// SPDX-License-Identifier: GPL-2.0
/*
 * ark_tool.c -- port of stock's arktool_reg_init()/ark_tool_handle()
 * subsystem (Linux 3.4 stock kernel, function addresses 0x802ff390 and
 * neighbouring code in vmlinux.elf), never previously present in this
 * reconstructed 4.19 tree.
 *
 * Why this exists: this project found stock has a subsystem that
 * ioremap()s both the LCD controller (0xe0500000) and the shared
 * syscon/pinmux block (0xe4900000 -- the same block the touch driver
 * and the LCD/BD37033/RN6752 pin-sharing all live on) into a ~60-entry
 * register-pointer table, exposed via /proc/arktool using a small
 * framed binary protocol (signature 0xAC, XOR checksum, command IDs
 * 0xB1-0xD8). It's triggered by the first successful startup() of
 * uart0 (0xe4200000, ttyS0/console on this tree) -- confirmed via
 * Ghidra decompilation of ark1680_uart_probe() and cross-checked
 * against this tree's own ark1668.dtsi uart0-3 nodes (byte-for-byte
 * match, no DTS changes needed).
 *
 * Confirmed (from disassembly, not guessed): the two ioremap targets
 * and every register-table offset below; the frame signature byte
 * (0xAC) and checksum algorithm (running XOR over the payload,
 * checked against a trailing byte); the command ID range (0xB1-0xD8,
 * a 40-entry jump table keyed on command-0xB1); that most commands are
 * Ypbpr/DDS/PLL analog-video-output configuration and telemetry
 * readback, not touch/audio related (ark1680_ts.c independently
 * confirmed to not reference this subsystem at all -- see
 * docs/HARDWARE_AND_SOC_REFERENCE.md and tools/mcu-handshake/README.md
 * for the investigation that ruled this out as the touch-switch
 * trigger).
 *
 * NOT yet confirmed precisely enough to implement safely: the exact
 * byte-for-byte frame layout beyond signature+checksum (this driver
 * assumes the same [sig][cmd][len][payload...][checksum] shape used
 * elsewhere in this codebase's other protocols, which is a reasonable
 * inference from the pattern but not itself disassembly-confirmed),
 * and the exact payload-byte-to-register mapping for command 0xB7 (the
 * one command confirmed to perform real pinmux writes -- disassembly
 * showed *which* registers it touches, not precisely which payload
 * bytes supply the values). Rather than guess at that and risk writing
 * wrong data into a real pinmux register, this driver validates and
 * logs every received command but does not yet act on 0xB7 or any
 * other command -- a deliberate, safe stopping point. Extending this
 * needs either a live capture of a real 0xB7 frame from the MCU, or
 * further disassembly of the command's exact payload unpacking.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ark_tool.h>

#define ARK_TOOL_LCD_BASE	0xe0500000
#define ARK_TOOL_LCD_SIZE	0x1000
#define ARK_TOOL_PINMUX_BASE	0xe4900000
#define ARK_TOOL_PINMUX_SIZE	0x200

#define ARK_TOOL_SIG		0xAC
#define ARK_TOOL_CMD_MIN	0xB1
#define ARK_TOOL_CMD_MAX	0xD8

struct ark_tool_regs {
	void __iomem *lcd_base;
	void __iomem *pinmux_base;

	/* pinmux-derived pointers (0xe4900000 + offset), confirmed via
	 * disassembly of arktool_reg_init() @ 0x802ff390 */
	void __iomem *pinmux_54;
	void __iomem *pinmux_70;
	void __iomem *pinmux_68;
	void __iomem *pinmux_198;
	void __iomem *pinmux_144;
	void __iomem *pinmux_14c;
	void __iomem *pinmux_154;
	void __iomem *pinmux_150;
	void __iomem *pinmux_190;

	/* lcd-derived pointers (0xe0500000 + offset), same source */
	void __iomem *lcd_04;
	void __iomem *lcd_08;
	void __iomem *lcd_0c;
	void __iomem *lcd_10;
	void __iomem *lcd_2b4;
	void __iomem *lcd_2b8;
	void __iomem *lcd_2bc;
	void __iomem *lcd_2c4;
	void __iomem *lcd_304;
	void __iomem *lcd_3c;
	void __iomem *lcd_74;	/* == ARK1668_LCDC_OSD1_CTL, cross-confirmed
				 * against ark1668_lcdfb.c's own known offset */
	void __iomem *lcd_88;
	void __iomem *lcd_98;
	void __iomem *lcd_800_854[21];	/* sequential run, stride 4 */
	void __iomem *lcd_2b0;
	void __iomem *lcd_134;
	void __iomem *lcd_144;
	void __iomem *lcd_14c;
	void __iomem *lcd_154;
	void __iomem *lcd_1d8;
	void __iomem *lcd_1ec;
	void __iomem *lcd_3d0_3fc[11];	/* sequential run, stride 4 */
};

static struct ark_tool_regs ark_regs;
static bool ark_tool_ready;
static DEFINE_MUTEX(ark_tool_lock);
static struct proc_dir_entry *ark_tool_proc;

static unsigned long ark_tool_rx_frames;
static unsigned long ark_tool_rx_checksum_errors;
static unsigned int ark_tool_last_cmd;

static void ark_tool_build_reg_table(void)
{
	void __iomem *lcd = ark_regs.lcd_base;
	void __iomem *pmx = ark_regs.pinmux_base;
	int i;

	ark_regs.pinmux_54  = pmx + 0x54;
	ark_regs.pinmux_70  = pmx + 0x70;
	ark_regs.pinmux_68  = pmx + 0x68;
	ark_regs.pinmux_198 = pmx + 0x198;
	ark_regs.pinmux_144 = pmx + 0x144;
	ark_regs.pinmux_14c = pmx + 0x14c;
	ark_regs.pinmux_154 = pmx + 0x154;
	ark_regs.pinmux_150 = pmx + 0x150;
	ark_regs.pinmux_190 = pmx + 0x190;

	ark_regs.lcd_04  = lcd + 0x04;
	ark_regs.lcd_08  = lcd + 0x08;
	ark_regs.lcd_0c  = lcd + 0x0c;
	ark_regs.lcd_10  = lcd + 0x10;
	ark_regs.lcd_2b4 = lcd + 0x2b4;
	ark_regs.lcd_2b8 = lcd + 0x2b8;
	ark_regs.lcd_2bc = lcd + 0x2bc;
	ark_regs.lcd_2c4 = lcd + 0x2c4;
	ark_regs.lcd_304 = lcd + 0x304;
	ark_regs.lcd_3c  = lcd + 0x3c;
	ark_regs.lcd_74  = lcd + 0x74;
	ark_regs.lcd_88  = lcd + 0x88;
	ark_regs.lcd_98  = lcd + 0x98;
	for (i = 0; i < 21; i++)
		ark_regs.lcd_800_854[i] = lcd + 0x800 + i * 4;
	ark_regs.lcd_2b0 = lcd + 0x2b0;
	ark_regs.lcd_134 = lcd + 0x134;
	ark_regs.lcd_144 = lcd + 0x144;
	ark_regs.lcd_14c = lcd + 0x14c;
	ark_regs.lcd_154 = lcd + 0x154;
	ark_regs.lcd_1d8 = lcd + 0x1d8;
	ark_regs.lcd_1ec = lcd + 0x1ec;
	for (i = 0; i < 11; i++)
		ark_regs.lcd_3d0_3fc[i] = lcd + 0x3d0 + i * 4;
}

static unsigned char ark_tool_checksum(const unsigned char *data, int len)
{
	unsigned char chk = 0;
	int i;

	for (i = 0; i < len; i++)
		chk ^= data[i];
	return chk;
}

/* Validates and logs a received frame. Deliberately does not act on
 * any command yet -- see the file header comment for why. */
static void ark_tool_handle_frame(unsigned char cmd, const unsigned char *payload,
				   unsigned char len)
{
	ark_tool_last_cmd = cmd;
	ark_tool_rx_frames++;

	if (cmd < ARK_TOOL_CMD_MIN || cmd > ARK_TOOL_CMD_MAX) {
		pr_info("ark_tool: cmd 0x%02x out of known range [0x%02x-0x%02x], ignoring\n",
			cmd, ARK_TOOL_CMD_MIN, ARK_TOOL_CMD_MAX);
		return;
	}

	pr_info("ark_tool: cmd 0x%02x len=%u -- validated, not yet acted on "
		"(payload-to-register mapping not confirmed for this command; "
		"see drivers/misc/ark_tool.c header)\n", cmd, len);
}

static ssize_t ark_tool_proc_write(struct file *file, const char __user *ubuf,
				    size_t count, loff_t *ppos)
{
	unsigned char kbuf[256];
	unsigned char cmd, len, chk_recv, chk_calc;
	size_t n;

	if (count < 4)
		return -EINVAL;
	n = min(count, sizeof(kbuf));
	if (copy_from_user(kbuf, ubuf, n))
		return -EFAULT;

	if (kbuf[0] != ARK_TOOL_SIG) {
		pr_debug("ark_tool: bad signature 0x%02x, expected 0x%02x\n",
			 kbuf[0], ARK_TOOL_SIG);
		return -EINVAL;
	}

	cmd = kbuf[1];
	len = kbuf[2];
	if ((size_t)(3 + len + 1) > n) {
		pr_debug("ark_tool: frame len=%u exceeds received %zu bytes\n", len, n);
		return -EINVAL;
	}

	chk_recv = kbuf[3 + len];
	chk_calc = ark_tool_checksum(&kbuf[1], 2 + len);
	if (chk_calc != chk_recv) {
		ark_tool_rx_checksum_errors++;
		pr_debug("ark_tool: checksum mismatch calc=0x%02x recv=0x%02x\n",
			 chk_calc, chk_recv);
		return -EINVAL;
	}

	mutex_lock(&ark_tool_lock);
	ark_tool_handle_frame(cmd, &kbuf[3], len);
	mutex_unlock(&ark_tool_lock);

	return count;
}

static int ark_tool_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ark_tool: %s\n", ark_tool_ready ? "ready" : "not initialized");
	seq_printf(m, "lcd_base:    0xe0500000 -> %p\n", ark_regs.lcd_base);
	seq_printf(m, "pinmux_base: 0xe4900000 -> %p\n", ark_regs.pinmux_base);
	seq_printf(m, "frames_received: %lu\n", ark_tool_rx_frames);
	seq_printf(m, "checksum_errors: %lu\n", ark_tool_rx_checksum_errors);
	seq_printf(m, "last_cmd: 0x%02x\n", ark_tool_last_cmd);
	seq_puts(m, "note: command execution not yet implemented, see driver source\n");
	return 0;
}

static int ark_tool_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ark_tool_proc_show, NULL);
}

/* This tree predates struct proc_ops (added in 5.6) -- procfs still
 * takes a plain struct file_operations here. */
static const struct file_operations ark_tool_proc_ops = {
	.owner		= THIS_MODULE,
	.open		= ark_tool_proc_open,
	.read		= seq_read,
	.write		= ark_tool_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void ark_tool_notify_uart_open(int line)
{
	/* Matches stock: fires once, on the first successful startup()
	 * of sub-port index 0 (uart0/ttyS0), never again. */
	if (line != 0 || ark_tool_ready)
		return;

	mutex_lock(&ark_tool_lock);
	if (ark_tool_ready) {
		mutex_unlock(&ark_tool_lock);
		return;
	}

	ark_regs.lcd_base = ioremap(ARK_TOOL_LCD_BASE, ARK_TOOL_LCD_SIZE);
	if (!ark_regs.lcd_base) {
		pr_err("ark_tool: failed to ioremap LCD base 0x%x\n", ARK_TOOL_LCD_BASE);
		goto out;
	}
	ark_regs.pinmux_base = ioremap(ARK_TOOL_PINMUX_BASE, ARK_TOOL_PINMUX_SIZE);
	if (!ark_regs.pinmux_base) {
		pr_err("ark_tool: failed to ioremap pinmux base 0x%x\n", ARK_TOOL_PINMUX_BASE);
		iounmap(ark_regs.lcd_base);
		ark_regs.lcd_base = NULL;
		goto out;
	}

	ark_tool_build_reg_table();

	ark_tool_proc = proc_create("arktool", 0600, NULL, &ark_tool_proc_ops);
	if (!ark_tool_proc)
		pr_warn("ark_tool: failed to create /proc/arktool\n");

	ark_tool_ready = true;
	pr_info("ark_tool: initialized on uart0 open (port for /proc/arktool, "
		"see drivers/misc/ark_tool.c)\n");
out:
	mutex_unlock(&ark_tool_lock);
}
EXPORT_SYMBOL_GPL(ark_tool_notify_uart_open);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Port of stock arktool_reg_init()/ark_tool_handle() -- LCD/pinmux register table + /proc/arktool, gated on uart0 open");
