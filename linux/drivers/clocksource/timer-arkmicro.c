/*
 * Copyright 2018-2019 Arkmicro, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <linux/errno.h>

#define TIMER0_BASE		0x00
#define TIMER1_BASE		0x04
#define TIMER2_BASE		0x08

#if defined(CONFIG_TIMER_ARK1668)
#define TIMER_TCTL		0x00
#define TIMER_TPRS		0x10
#define TIMER_TMOD		0x20
#define TIMER_TCNT		0x30
#define TIMER_TSTA		0x40
#define TIMER_INT_MAINTAIN		0
#define TIMER_CNT_VAL	0xffffff
#define TIMER_CNT_BITS	24
#elif defined(CONFIG_TIMER_ARKN141)
#define TIMER_TCTL		0x00
#define TIMER_TPRS		0x0C
#define TIMER_TMOD		0x18
#define TIMER_TCNT		0x24
#define TIMER_INT_MAINTAIN		(1 << 3)
#define TIMER_INT_STATUS		(1 << 4)
#define TIMER_CNT_VAL	0xffffffff
#define TIMER_CNT_BITS	32
#define USE_SIBLING_TIMER
#endif

#define TIMER_CTRL_IE			(1 << 2)
#define TIMER_CTRL_PERIODIC		(1 << 1)
#define TIMER_CTRL_ENABLE		(1 << 0)

static void __iomem *sched_clock_base;

static u64 notrace ark_read(void)
{
	return ~(readl_relaxed(sched_clock_base + TIMER_TCNT) | ((u64)((1 << (32 - TIMER_CNT_BITS)) - 1) << TIMER_CNT_BITS));
}

void __init ark_timer_disable(void __iomem *base)
{
	writel(0, base + TIMER_TCTL);
}

int  __init ark_clocksource_and_sched_clock_init(void __iomem *base,
						     const char *name,
						     struct clk *clk,
						     int use_sched_clock)
{
	long rate;

	rate = clk_get_rate(clk);
	if (rate == 0)
		return -EINVAL;

	/* setup timer 1 as free-running clocksource */
	writel(0, base + TIMER_TCTL);
	writel(TIMER_CNT_VAL, base + TIMER_TMOD);
	writel(TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC,
		base + TIMER_TCTL);

	clocksource_mmio_init(base + TIMER_TCNT, name,
		rate, 300, TIMER_CNT_BITS, clocksource_mmio_readl_down);

	if (use_sched_clock) {
		sched_clock_base = base;
		sched_clock_register(ark_read, TIMER_CNT_BITS, rate);
	}

	return 0;
}


static void __iomem *clkevt_base;
static unsigned long clkevt_reload;

#ifdef USE_SIBLING_TIMER
static unsigned long usec_reload;
#endif

/*
 * IRQ handler for the timer
 */
static irqreturn_t ark_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	u32 reg;

	/* clear the interrupt */
#if defined(CONFIG_TIMER_ARK1668)
	reg = readl(clkevt_base + TIMER_TSTA);
	reg &= ~1;
	writel(reg, clkevt_base + TIMER_TSTA);
#elif defined(CONFIG_TIMER_ARKN141)
	reg = readl(clkevt_base + TIMER_TCTL);
	reg &= ~TIMER_INT_STATUS;
	writel(reg, clkevt_base + TIMER_TCTL);
#endif

#ifdef USE_SIBLING_TIMER
	writel(0, clkevt_base + TIMER2_BASE + TIMER_TCTL);
#endif

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static inline void timer_shutdown(struct clock_event_device *evt)
{
#if defined(CONFIG_TIMER_ARK1668)
	writel(0, clkevt_base + TIMER_TCTL);
#elif defined(CONFIG_TIMER_ARKN141)
	/* not clear interrupt */
	writel(1 << TIMER_INT_STATUS, clkevt_base + TIMER_TCTL);
#endif
}

static int ark_shutdown(struct clock_event_device *evt)
{
	timer_shutdown(evt);
	return 0;
}

static int ark_set_periodic(struct clock_event_device *evt)
{
	unsigned long ctrl = TIMER_INT_MAINTAIN | TIMER_CTRL_IE | TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE;

	timer_shutdown(evt);

#if defined(CONFIG_TIMER_ARK1668)
	writel(1 << 1, clkevt_base + TIMER_TSTA);
#endif

	writel(clkevt_reload, clkevt_base + TIMER_TMOD);
	writel(ctrl, clkevt_base + TIMER_TCTL);

	return 0;
}

static int ark_set_next_event(unsigned long next,
	struct clock_event_device *evt)
{
	unsigned long ctrl = TIMER_INT_MAINTAIN | TIMER_CTRL_IE | TIMER_CTRL_ENABLE;

	timer_shutdown(evt);

#if defined(CONFIG_TIMER_ARK1668)
	writel(1 << 1, clkevt_base + TIMER_TSTA);
#endif
	writel(next, clkevt_base + TIMER_TMOD);
	writel(ctrl, clkevt_base + TIMER_TCTL);

#ifdef USE_SIBLING_TIMER
	writel(0, clkevt_base + TIMER2_BASE + TIMER_TCTL);
	writel(next + usec_reload, clkevt_base + TIMER2_BASE + TIMER_TMOD);
	writel(ctrl, clkevt_base + TIMER2_BASE + TIMER_TCTL);
#endif

	return 0;
}

static struct clock_event_device ark_clockevent = {
	.features		= CLOCK_EVT_FEAT_PERIODIC |
				  CLOCK_EVT_FEAT_ONESHOT,
	.set_state_shutdown	= ark_shutdown,
	.set_state_periodic	= ark_set_periodic,
	.set_state_oneshot	= ark_shutdown,
	.tick_resume		= ark_shutdown,
	.set_next_event		= ark_set_next_event,
	.rating			= 300,
};

static struct irqaction ark_timer_irq = {
	.name		= "timer",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= ark_timer_interrupt,
	.dev_id		= &ark_clockevent,
};

int __init ark_clockevents_init(void __iomem *base, unsigned int irq, struct clk *clk, const char *name)
{
	struct clock_event_device *evt = &ark_clockevent;
	long rate;

	rate = clk_get_rate(clk);
	if (rate == 0)
		return -EINVAL;

	clkevt_base = base;
	clkevt_reload = DIV_ROUND_CLOSEST(rate, HZ);
#ifdef USE_SIBLING_TIMER
	usec_reload = DIV_ROUND_CLOSEST(rate, 1000000);
#endif
	evt->name = name;
	evt->irq = irq;
	evt->cpumask = cpu_possible_mask;

	writel(0, base + TIMER_TCTL);

	setup_irq(irq, &ark_timer_irq);
	clockevents_config_and_register(evt, rate, 0xf, TIMER_CNT_VAL);

	return 0;
}

#ifdef USE_SIBLING_TIMER
static irqreturn_t ark_sibling_timer_interrupt(int irq, void *dev_id)
{
	u32 ctrl, mod, cnt;

	writel(0, clkevt_base + TIMER2_BASE + TIMER_TCTL);

	//restart clockevent timer
	ctrl = readl(clkevt_base + TIMER_TCTL);
	mod = readl(clkevt_base + TIMER_TMOD);
	cnt = readl(clkevt_base + TIMER_TCNT);
	if (ctrl == (TIMER_INT_MAINTAIN | TIMER_CTRL_IE | TIMER_CTRL_ENABLE) &&
		mod != 0 && cnt == 0) {
		writel(0, clkevt_base + TIMER_TCTL);
		writel(10, clkevt_base + TIMER_TMOD);
		writel(ctrl, clkevt_base + TIMER_TCTL);
	}

	return IRQ_HANDLED;
}

static struct irqaction ark_sibling_timer_irq = {
	.name		= "sibling-timer",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= ark_sibling_timer_interrupt,
};
#endif

static int __init ark_timer_init(struct device_node *np)
{
	static bool initialized = false;
	void __iomem *base;
	int irq, ret = -EINVAL;
	struct clk *clk;
	const char *name = of_get_property(np, "compatible", NULL);

	base = of_iomap(np, 0);
	if (!base)
		return -ENXIO;

	/* Ensure timers are disabled */
	writel(0, base + TIMER_TCTL);
	writel(0, base + TIMER1_BASE + TIMER_TCTL);

	//reset clk prescale
	writel(0, base + TIMER_TPRS);
	writel(0, base + TIMER1_BASE + TIMER_TPRS);

	if (initialized || !of_device_is_available(np)) {
		ret = -EINVAL;
		goto err;
	}

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk))
		goto err;

	irq = irq_of_parse_and_map(np, 0);
	if (irq <= 0)
		goto err;

	ret = ark_clockevents_init(base, irq, clk , name);
	if (ret)
		goto err;

	ret = ark_clocksource_and_sched_clock_init(base + TIMER1_BASE,
						      name, clk, 1);
	if (ret)
		goto err;

#ifdef USE_SIBLING_TIMER
	/* trigger a timer to detect clockevent timer error */
	writel(0, base + TIMER2_BASE + TIMER_TCTL);
	writel(0, base + TIMER2_BASE + TIMER_TPRS);

	irq = irq_of_parse_and_map(np, 1);

   	setup_irq(irq, &ark_sibling_timer_irq);
#endif

	initialized = true;

	return 0;
err:
	iounmap(base);
	return ret;
}
TIMER_OF_DECLARE(arkmicro_timer, "arkmicro,ark-timer", ark_timer_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro timer driver");
MODULE_LICENSE("GPL v2");
