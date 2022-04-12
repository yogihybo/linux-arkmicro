/*
 * Arkmicro watchdog driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/watchdog.h>
#include <linux/gpio/consumer.h>

#define ARK_WTCON		0x00
#define ARK_WTPSR		0x04
#define ARK_WTCNT		0x08
#define ARK_WTCLRINT	0x10
#define ARK_WTRCR		0x14

#define ARK_WTCNT_MAXCNT	0xffff
#define ARK_WTCON_MAXDIV	0x80

#define ARK_WTCON_ENABLE	(1 << 0)
#define ARK_WTCON_RSTEN		(1 << 1)
#define ARK_WTCON_INTEN		(1 << 2)

#define ARK_WTCON_DIV16		(0 << 4)
#define ARK_WTCON_DIV32		(1 << 4)
#define ARK_WTCON_DIV64		(2 << 4)
#define ARK_WTCON_DIV128	(3 << 4)
#define ARK_WTCON_DIVMASK	(0x3 << 4)

#define ARK_WTCON_PRESCALE_MAX	0xffff

#define ARK_WATCHDOG_DEFAULT_TIME	(15)

struct ark_wdt {
	struct device		*dev;
	void __iomem		*reg_base;
	struct gpio_desc	*reset_gpio;
	struct clk		*clock;
	struct watchdog_device	wdd;
	unsigned int		count;
	spinlock_t		lock;
};

static int wdt_timeout = ARK_WATCHDOG_DEFAULT_TIME;
static bool nowayout = WATCHDOG_NOWAYOUT;
static int soft_noboot = 1;

module_param(wdt_timeout, int, 0);
MODULE_PARM_DESC(wdt_timeout,
	"Watchdog timeout in seconds. (default = "
	__MODULE_STRING(WDT_DEFAULT_TIMEOUT) ")");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
	"Watchdog cannot be stopped once started (default="
	__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

module_param(soft_noboot, int, 0);
MODULE_PARM_DESC(soft_noboot, "Watchdog action, set to 1 to ignore reboots, 0 to reboot (default 0)");

static int ark_wdt_keepalive(struct watchdog_device *wdd)
{
	struct ark_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);
	writel(wdt->count, wdt->reg_base + ARK_WTCNT);
	spin_unlock(&wdt->lock);

	return 0;
}

static void __ark_wdt_stop(struct ark_wdt *wdt)
{
	unsigned long wtcon;

	wtcon = readl(wdt->reg_base + ARK_WTCON);
	wtcon &= ~(ARK_WTCON_ENABLE | ARK_WTCON_RSTEN);
	writel(wtcon, wdt->reg_base + ARK_WTCON);
}

static int ark_wdt_stop(struct watchdog_device *wdd)
{
	struct ark_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);
	__ark_wdt_stop(wdt);
	spin_unlock(&wdt->lock);

	return 0;
}

static int ark_wdt_start(struct watchdog_device *wdd)
{
	unsigned long wtcon;
	struct ark_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);

	__ark_wdt_stop(wdt);

	wtcon = readl(wdt->reg_base + ARK_WTCON);
	wtcon &= ~ARK_WTCON_DIVMASK;
	wtcon |= ARK_WTCON_ENABLE | ARK_WTCON_DIV128;
	if (soft_noboot) {
		wtcon |= ARK_WTCON_INTEN;
		wtcon &= ~ARK_WTCON_RSTEN;
	} else {
		wtcon &= ~ARK_WTCON_INTEN;
		wtcon |= ARK_WTCON_RSTEN;
	}

	dev_dbg(wdt->dev, "Starting watchdog: count=0x%08x, wtcon=%08lx\n",
		wdt->count, wtcon);

	writel(wdt->count, wdt->reg_base + ARK_WTCNT);
	writel(wtcon, wdt->reg_base + ARK_WTCON);
	spin_unlock(&wdt->lock);

	return 0;
}

static unsigned int ark_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout_ms)
{
	struct ark_wdt *wdt = watchdog_get_drvdata(wdd);
	unsigned long freq = clk_get_rate(wdt->clock);
	unsigned int count;
	unsigned int divisor = 1;

	freq = DIV_ROUND_UP(freq, 128);
	count = timeout_ms * DIV_ROUND_UP(freq, 1000);

	dev_dbg(wdt->dev, "Heartbeat: count=%d, timeout=%d(ms), freq=%lu\n",
		count, timeout_ms, freq);

	/* if the count is bigger than the watchdog register,
	   then work out what we need to do (and if) we can
	   actually make this value
	*/

	if (count > ARK_WTCNT_MAXCNT) {
		divisor = DIV_ROUND_UP(count, ARK_WTCNT_MAXCNT);
		if (divisor > ARK_WTCON_PRESCALE_MAX) {
			dev_err(wdt->dev, "timeout %d(ms) too big\n", timeout_ms);
			return -EINVAL;
		}
	}

	dev_dbg(wdt->dev, "Heartbeat: timeout=%d(ms), divisor=%d, count=%d (%08x)\n",
		timeout_ms, divisor, count, DIV_ROUND_UP(count, divisor));

	count = DIV_ROUND_UP(count, divisor);
	wdt->count = count;

	/* update the pre-scaler */
	writel(divisor, wdt->reg_base + ARK_WTPSR);

	writel(count, wdt->reg_base + ARK_WTCNT);

	return (count * divisor) / freq;
}

static int ark_wdt_set_heartbeat(struct watchdog_device *wdd,
				    unsigned int timeout)
{
	if (timeout < 1)
		return -EINVAL;

	wdd->timeout = ark_wdt_set_timeout(wdd, timeout * 1000);

	return 0;
}

static int ark_wdt_restart(struct watchdog_device *wdd, unsigned long action,
			      void *data)
{
	struct ark_wdt *wdt = watchdog_get_drvdata(wdd);
	void __iomem *wdt_base = wdt->reg_base;
#ifdef CONFIG_SOC_ARK1668E
	void __iomem *sys_base;
#endif

	/* disable watchdog, to be safe  */
	writel(0, wdt_base + ARK_WTCON);

#ifdef CONFIG_SOC_ARK1668E
	if (wdt->reset_gpio)
		gpiod_direction_output(wdt->reset_gpio, 1);
	else
		ark_wdt_set_timeout(wdd, 2000);
#else
	ark_wdt_set_timeout(wdd, 100);
#endif

	/* set the watchdog to go and reset... */
	writel(ARK_WTCON_ENABLE | ARK_WTCON_DIV128 |
		ARK_WTCON_RSTEN, wdt_base + ARK_WTCON);

#ifdef CONFIG_SOC_ARK1668E
#define ARK1668E_SYSREG_BASE 	0xe4900000
#define ARK1668E_SOFT_RSTA		0x74
#define ARK1668E_SOFT_RSTB		0x78
	sys_base = ioremap(ARK1668E_SYSREG_BASE, 0x1000);
	if (sys_base) {
		writel(0, sys_base + ARK1668E_SOFT_RSTA);
		writel(0, sys_base + ARK1668E_SOFT_RSTB);
	}
#endif

	/* wait for reset to assert... */
	mdelay(3000);

	return 0;
}

static const struct watchdog_info ark_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
	.identity = "ark Watchdog",
};

static const struct watchdog_ops ark_wdt_ops = {
	.owner = THIS_MODULE,
	.start = ark_wdt_start,
	.stop = ark_wdt_stop,
	.ping = ark_wdt_keepalive,
	.restart = ark_wdt_restart,
	.set_timeout = ark_wdt_set_heartbeat,
};

/* interrupt handler code */
static irqreturn_t ark_wdt_irq(int irqno, void *param)
{
	struct ark_wdt *wdt = platform_get_drvdata(param);

	//dev_info(wdt->dev, "watchdog timer expired (irq)\n");

	ark_wdt_keepalive(&wdt->wdd);
	writel(0x1, wdt->reg_base + ARK_WTCLRINT);

	return IRQ_HANDLED;
}

static inline unsigned int ark_wdt_max_timeout(struct clk *clock)
{
	unsigned long freq = clk_get_rate(clock);

	return ARK_WTCNT_MAXCNT / (freq / ARK_WTCON_PRESCALE_MAX
				       / ARK_WTCON_MAXDIV);
}

static int ark_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct watchdog_device *wdd;
	struct ark_wdt *wdt;
	struct resource *res;
	void __iomem *regs;
	int ret;
	int irq;

	wdt = devm_kzalloc(&pdev->dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt)
		return -ENOMEM;

	wdt->dev = dev;
	spin_lock_init(&wdt->lock);

	wdd = &wdt->wdd;
	wdd->timeout = wdt_timeout;
	wdd->info = &ark_wdt_info;
	wdd->ops = &ark_wdt_ops;

	watchdog_set_drvdata(wdd, wdt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	wdt->reg_base = regs;

	wdt->clock = devm_clk_get(dev, NULL);
	if (IS_ERR(wdt->clock)) {
		dev_err(dev, "failed to find watchdog clock source\n");
		return PTR_ERR(wdt->clock);
	}

    wdt->reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(wdt->reset_gpio))
		wdt->reset_gpio = NULL;

	wdt->wdd.min_timeout = 1;
	wdt->wdd.max_timeout = ark_wdt_max_timeout(wdt->clock);
	ret = watchdog_init_timeout(wdd, wdt_timeout, &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "unable to set timeout value\n");
		return ret;
	}
	ret = ark_wdt_set_heartbeat(&wdt->wdd, wdt_timeout);
	if (ret) {
		dev_err(&pdev->dev, "failed to set timeout value\n");
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "unable to get irq\n");
		return irq;
	}
	ret = devm_request_irq(dev, irq, ark_wdt_irq, 0, pdev->name, pdev);
	if (ret != 0) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
		return ret;
	}

	watchdog_set_nowayout(wdd, nowayout);
	watchdog_set_restart_priority(&wdt->wdd, 128);

	ret = watchdog_register_device(wdd);
	if (ret) {
		dev_err(&pdev->dev, "failed to register watchdog device\n");
		return ret;
	}

	platform_set_drvdata(pdev, wdt);

	ark_wdt_start(&wdt->wdd);

	dev_info(&pdev->dev, "initialized (timeout = %d sec, nowayout = %d)\n",
		 wdt_timeout, nowayout);

	return 0;
}

static int ark_wdt_remove(struct platform_device *pdev)
{
	struct ark_wdt *wdt = platform_get_drvdata(pdev);

	ark_wdt_stop(&wdt->wdd);

	watchdog_unregister_device(&wdt->wdd);

	return 0;
}

static const struct of_device_id ark_wdt_of_match[] = {
	{ .compatible = "arkmicro,ark-wdt", },
	{ }
};
MODULE_DEVICE_TABLE(of, ark_wdt_of_match);

static struct platform_driver ark_wdt_driver = {
	.probe		= ark_wdt_probe,
	.remove		= ark_wdt_remove,
	.driver		= {
		.name	= "ark_wdt",
		.of_match_table = ark_wdt_of_match,
	}
};
module_platform_driver(ark_wdt_driver);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro Watchdog Timer driver");
MODULE_LICENSE("GPL v2");
