/*
 * drivers/rtc/rtc-ark.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

/* RTC registers */
#define RTC_CTL			0x00 /*control register*/
#define RTC_ANAWEN		0x04 /*analog block write enable register*/
#define RTC_ANACTL		0x08 /*analog block control register*/
#define RTC_IM			0x0c /*interrupt mode register*/
#define RTC_STA			0x10 /*rtc status register*/
#define RTC_ALMDAT		0x14 /*alarm data register*/
#define RTC_DONT		0x18 /*delay on timer register*/
#define RTC_RAM			0x1c /*ram bit register*/
#define RTC_CNTL		0x20 /*rtc counter register*/
#define RTC_CNTH		0x24 /*rtc sec counter register*/

//RTC_CTL register fields defination
#define CTL_ALM_DATA_WEN		(1<<3)
#define CTL_PERIOD_INT_EN		(1<<2)
#define CTL_ALARM_INT_EN		(1<<1)
#define CTL_RESET				(1<<0)

//RTC_ANAWEN register fields defination
#define ANA_CNT_WEN						(1<<7)
#define ANA_RAM_WEN						(1<<6)
#define ANA_DELAY_TIMER_WEN				(1<<5)
#define ANA_CLR_PWR_DET_WEN				(1<<4)
#define ANA_DELAY_POWER_ON_WEN			(1<<3)
#define ANA_FORCE_POWER_OFF_WEN			(1<<2)
#define ANA_FORCE_POWER_ON_WEN			(1<<1)
#define ANA_RTC_WEN						(1<<0)

//RTC_ANACTL register fields defination
#define ANACTL_CLR_PWR					(1<<4)
#define ANACTL_DELAY_POWER_ON			(1<<3)
#define ANACTL_FORCE_POWER_OFF			(1<<2)
#define ANACTL_FORCE_POWER_ON			(1<<1)
#define ANACTL_COUNTER_EN				(1<<0)

/* STATUS_REG */
#define STA_PWR_DET			(1<<6)
#define STA_DELAY_ON		(1<<5)
#define STA_FORCE_OFF		(1<<4)
#define STA_FORCE_ON		(1<<3)
#define STA_RCT_BUSY		(1<<2)
#define STA_PERIOD_INT		(1<<1)
#define STA_ALARM_INT		(1<<0)

struct ark_rtc_config {
	struct rtc_device *rtc;
	struct clk *clk;
	spinlock_t lock;
	void __iomem *ioaddr;
	unsigned int irq_wake;
};

static inline void ark_rtc_clear_interrupt(struct ark_rtc_config *config)
{
	unsigned int val;
	unsigned long flags;

	spin_lock_irqsave(&config->lock, flags);
	val = readl(config->ioaddr + RTC_STA);
	val &= ~CTL_ALARM_INT_EN;
	writel(val, config->ioaddr + RTC_STA);
	spin_unlock_irqrestore(&config->lock, flags);
}

static inline void ark_rtc_enable_interrupt(struct ark_rtc_config *config)
{
	unsigned int val;

	val = readl(config->ioaddr + RTC_CTL);
	if (!(val & CTL_ALARM_INT_EN)) {
		ark_rtc_clear_interrupt(config);
		val |= CTL_ALARM_INT_EN;
		writel(val, config->ioaddr + RTC_CTL);
	}
}

static inline void ark_rtc_disable_interrupt(struct ark_rtc_config *config)
{
	unsigned int val;

	val = readl(config->ioaddr + RTC_CTL);
	if (val & CTL_ALARM_INT_EN) {
		val &= ~CTL_ALARM_INT_EN;
		writel(val, config->ioaddr + RTC_CTL);
	}
}

static void rtc_wait_not_busy(struct ark_rtc_config *config)
{
	int status, count = 0;
	unsigned long flags;

	/* Assuming BUSY may stay active for 80 msec) */
	for (count = 0; count < 80; count++) {
		spin_lock_irqsave(&config->lock, flags);
		status = readl(config->ioaddr + RTC_STA);
		spin_unlock_irqrestore(&config->lock, flags);
		if ((status & STA_RCT_BUSY) == 0)
			break;
		/* check status busy, after each msec */
		msleep(1);
	}
}

static irqreturn_t ark_rtc_irq(int irq, void *dev_id)
{
	struct ark_rtc_config *config = dev_id;
	unsigned long flags, events = 0;
	unsigned int irq_data;

	spin_lock_irqsave(&config->lock, flags);
	irq_data = readl(config->ioaddr + RTC_STA);
	spin_unlock_irqrestore(&config->lock, flags);

	if ((irq_data & CTL_ALARM_INT_EN)) {
		ark_rtc_clear_interrupt(config);
		events = RTC_IRQF | RTC_AF;
		rtc_update_irq(config->rtc, 1, events);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;

}

static void ark_rtc_update_time(struct device *dev, unsigned int time)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	unsigned int val;
	int timeout = 100000;

	val = readl(config->ioaddr + RTC_ANAWEN);
	writel(val | ANA_RTC_WEN, config->ioaddr + RTC_ANAWEN);
	val = readl(config->ioaddr + RTC_ANACTL);
	writel(val | ANACTL_COUNTER_EN, config->ioaddr + RTC_ANACTL);

	//wait rtc_busy;
	rtc_wait_not_busy(config);

	val = readl(config->ioaddr + RTC_ANAWEN);
	writel(val | ANA_CNT_WEN, config->ioaddr + RTC_ANAWEN);
	writel(time, config->ioaddr + RTC_CNTH);

	//wait rtc_busy;
	rtc_wait_not_busy(config);
	while(readl(config->ioaddr + RTC_CNTH) != time) {
		if (timeout-- == 0)
			break;
		cpu_relax();
	}
}

static void ark_rtc_update_alarm_time(struct device *dev, unsigned int time)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	unsigned int val;
	int timeout = 100000;

	val = readl(config->ioaddr + RTC_CTL);
	writel(val | CTL_ALM_DATA_WEN, config->ioaddr + RTC_CTL);

	writel(time, config->ioaddr + RTC_ALMDAT);

	//wait rtc_busy;
	rtc_wait_not_busy(config);
	while(readl(config->ioaddr + RTC_ALMDAT) != time) {
		if (timeout-- == 0)
			break;
		cpu_relax();
	}
}

/*
 * ark_rtc_read_time - set the time
 * @dev: rtc device in use
 * @tm: holds date and time
 *
 * This function read time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ark_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	unsigned int time;

	/* we don't report wday/yday/isdst ... */
	rtc_wait_not_busy(config);

	time = readl(config->ioaddr + RTC_CNTH);

	rtc_time_to_tm(time, tm);

	return 0;
}

/*
 * ark_rtc_set_time - set the time
 * @dev: rtc device in use
 * @tm: holds date and time
 *
 * This function set time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ark_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	long unsigned int time;

	if (rtc_valid_tm(tm) < 0)
		return -EINVAL;

	/* convert tm to seconds. */
	rtc_tm_to_time(tm, &time);

	ark_rtc_update_time(dev, time);

	return 0;
}

/*
 * ark_rtc_read_alarm - read the alarm time
 * @dev: rtc device in use
 * @alm: holds alarm date and time
 *
 * This function read alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ark_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	unsigned int time;

	rtc_wait_not_busy(config);

	time = readl(config->ioaddr + RTC_ALMDAT);

	rtc_time_to_tm(time, &alm->time);

	alm->enabled = readl(config->ioaddr + RTC_CTL) & CTL_ALARM_INT_EN;

	return 0;
}

/*
 * ark_rtc_set_alarm - set the alarm time
 * @dev: rtc device in use
 * @alm: holds alarm date and time
 *
 * This function set alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ark_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	long unsigned int time;

	if (rtc_valid_tm(&alm->time) < 0)
		return -EINVAL;

	/* convert tm to seconds. */
	rtc_tm_to_time(&alm->time, &time);

	ark_rtc_update_alarm_time(dev, time);

	if (alm->enabled)
		ark_rtc_enable_interrupt(config);
	else
		ark_rtc_disable_interrupt(config);

	return 0;
}

static int ark_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct ark_rtc_config *config = dev_get_drvdata(dev);
	int ret = 0;

	ark_rtc_clear_interrupt(config);

	switch (enabled) {
	case 0:
		/* alarm off */
		ark_rtc_disable_interrupt(config);
		break;
	case 1:
		/* alarm on */
		ark_rtc_enable_interrupt(config);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct rtc_class_ops ark_rtc_ops = {
	.read_time = ark_rtc_read_time,
	.set_time = ark_rtc_set_time,
	.read_alarm = ark_rtc_read_alarm,
	.set_alarm = ark_rtc_set_alarm,
	.alarm_irq_enable = ark_alarm_irq_enable,
};

static int ark_rtc_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ark_rtc_config *config;
	int status = 0;
	int irq;

	config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* alarm irqs */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no update irq?\n");
		return irq;
	}

	status = devm_request_irq(&pdev->dev, irq, ark_rtc_irq, 0, pdev->name,
			config);
	if (status) {
		dev_err(&pdev->dev, "Alarm interrupt IRQ%d already claimed\n",
				irq);
		return status;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	config->ioaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(config->ioaddr))
		return PTR_ERR(config->ioaddr);

	config->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(config->clk))
		return PTR_ERR(config->clk);

	status = clk_prepare_enable(config->clk);
	if (status < 0)
		return status;

	spin_lock_init(&config->lock);
	platform_set_drvdata(pdev, config);

	config->rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
					&ark_rtc_ops, THIS_MODULE);
	if (IS_ERR(config->rtc)) {
		dev_err(&pdev->dev, "can't register RTC device, err %ld\n",
				PTR_ERR(config->rtc));
		status = PTR_ERR(config->rtc);
		goto err_disable_clock;
	}

	config->rtc->uie_unsupported = 1;

	if (!device_can_wakeup(&pdev->dev))
		device_init_wakeup(&pdev->dev, 1);

	return 0;

err_disable_clock:
	clk_disable_unprepare(config->clk);

	return status;
}

static int ark_rtc_remove(struct platform_device *pdev)
{
	struct ark_rtc_config *config = platform_get_drvdata(pdev);

	ark_rtc_disable_interrupt(config);
	clk_disable_unprepare(config->clk);
	device_init_wakeup(&pdev->dev, 0);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ark_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_rtc_config *config = platform_get_drvdata(pdev);
	int irq;

	irq = platform_get_irq(pdev, 0);
	if (device_may_wakeup(&pdev->dev)) {
		if (!enable_irq_wake(irq))
			config->irq_wake = 1;
	} else {
		ark_rtc_disable_interrupt(config);
		clk_disable(config->clk);
	}

	return 0;
}

static int ark_rtc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_rtc_config *config = platform_get_drvdata(pdev);
	int irq;

	irq = platform_get_irq(pdev, 0);

	if (device_may_wakeup(&pdev->dev)) {
		if (config->irq_wake) {
			disable_irq_wake(irq);
			config->irq_wake = 0;
		}
	} else {
		clk_enable(config->clk);
		ark_rtc_enable_interrupt(config);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ark_rtc_pm_ops, ark_rtc_suspend, ark_rtc_resume);

static void ark_rtc_shutdown(struct platform_device *pdev)
{
	struct ark_rtc_config *config = platform_get_drvdata(pdev);

	ark_rtc_disable_interrupt(config);
	clk_disable(config->clk);
}

#ifdef CONFIG_OF
static const struct of_device_id ark_rtc_id_table[] = {
	{ .compatible = "arkmicro,ark-rtc" },
	{}
};
MODULE_DEVICE_TABLE(of, ark_rtc_id_table);
#endif

static struct platform_driver ark_rtc_driver = {
	.probe = ark_rtc_probe,
	.remove = ark_rtc_remove,
	.shutdown = ark_rtc_shutdown,
	.driver = {
		.name = "rtc-ark",
		.pm = &ark_rtc_pm_ops,
		.of_match_table = of_match_ptr(ark_rtc_id_table),
	},
};

module_platform_driver(ark_rtc_driver);

MODULE_ALIAS("platform:rtc-ark");
MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
