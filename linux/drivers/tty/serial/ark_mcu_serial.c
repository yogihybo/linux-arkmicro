/*
 * Arkmicro mcu serial driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sizes.h>
#include <linux/io.h>
#include <linux/acpi.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#define RX_BUF_SIZE		PAGE_SIZE

struct mcu_serial_info {
	int rx_head;
	int rx_tail;
	int carback_ready;
	unsigned char *rx_buf;
	spinlock_t lock;
	struct work_struct rx_task;
};

extern int mcu_serial_send(const unsigned char *buf, int len);
extern void mcu_serial_register_rev_handler(void (*handler)(unsigned char ch), struct work_struct *task);
extern void mcu_serial_unregister_rev_handler(void);

#ifdef CONFIG_ARK1668E_CARBACK
extern void get_mcu_carback_data(unsigned char ch);
#endif

static struct mcu_serial_info *msinfo;

#ifdef CONFIG_ARK1668E_CARBACK
void ark_set_track_ready(void)
{
	msinfo->carback_ready = 1;
}
EXPORT_SYMBOL(ark_set_track_ready);

void ark_set_track_noready(void)
{
	msinfo->carback_ready = 0;
}
EXPORT_SYMBOL(ark_set_track_noready);
#endif

static void mcu_serial_get_ch(unsigned char ch)
{
	struct mcu_serial_info *info = msinfo;
	unsigned long flags;

	spin_lock_irqsave(&info->lock, flags);
	//printk(KERN_ALERT "++++++ mcu_serial_get_ch rev ch = %d\n",ch);
	info->rx_buf[info->rx_head] = ch;
#ifdef CONFIG_ARK1668E_CARBACK
	if(info->carback_ready)
		get_mcu_carback_data(info->rx_buf[info->rx_head]);
#endif
	info->rx_head = (info->rx_head + 1) & (RX_BUF_SIZE - 1);
	if (info->rx_head == info->rx_tail) {
		printk("rev buf is full, lost ch.\n");
		if (--info->rx_head < 0)
			info->rx_head = RX_BUF_SIZE - 1;
	}

	spin_unlock_irqrestore(&info->lock, flags);
}

static void mcu_serial_rx_task(struct work_struct *work)
{
	struct mcu_serial_info *info =
		container_of(work, struct mcu_serial_info, rx_task);
	int count;
	unsigned long flags;

	spin_lock_irqsave(&info->lock, flags);
	count = info->rx_head - info->rx_tail;
	spin_unlock_irqrestore(&info->lock, flags);

	if (count < 0)
		count += RX_BUF_SIZE;

	printk("rx count=%d.\n", count);
}

static int ark_mcu_serial_probe(struct platform_device *pdev)
{
	struct mcu_serial_info *info = devm_kzalloc(&pdev->dev, sizeof(struct mcu_serial_info),
			   GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->rx_buf = devm_kzalloc(&pdev->dev, RX_BUF_SIZE, GFP_KERNEL);
	if (!info->rx_buf)
		return -ENOMEM;

	msinfo = info;
	info->carback_ready = 0;

	INIT_WORK(&info->rx_task, mcu_serial_rx_task);

	mcu_serial_register_rev_handler(mcu_serial_get_ch, &info->rx_task);

	platform_set_drvdata(pdev, info);

	printk(KERN_ALERT "enable get mcu data\n");

	return 0;	
}

static int ark_mcu_serial_remove(struct platform_device *pdev)
{
	struct mcu_serial_info *info = platform_get_drvdata(pdev);

	cancel_work_sync(&info->rx_task);
	mcu_serial_unregister_rev_handler();

	return 0;
}

static const struct of_device_id ark_mcu_serial_of_match[] = {
	{ .compatible = "arkmicro,ark-mcu-serial", },
	{},
};

static struct platform_driver ark_mcu_serial_platform_driver = {
	.probe		= ark_mcu_serial_probe,
	.remove		= ark_mcu_serial_remove,
	.driver	= {
		.name	= "ark-mcu-serial",
		.of_match_table = of_match_ptr(ark_mcu_serial_of_match),
	},
};

static int __init ark_mcu_serial_init(void)
{
	return platform_driver_register(&ark_mcu_serial_platform_driver);
}

static void __exit ark_mcu_serial_exit(void)
{
	platform_driver_unregister(&ark_mcu_serial_platform_driver);
}

/*
 * While this can be a module, if builtin it's most likely the console
 * So let's leave module_exit but move module_init to an earlier place
 */
arch_initcall(ark_mcu_serial_init);
module_exit(ark_mcu_serial_exit);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro mcu serial driver");
MODULE_LICENSE("GPL v2");

