/*
 * Arkmicro pwm driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/types.h>

#define NUM_PWM		4

/* PWM registers and bits definitions */
#define PWM_EN			0x00
#define PWM_DUTY		0x04
#define PWM_CNTR		0x08

#define PWM_DEFAULT_PERIOD	50000u

/**
 * struct ark_pwm_chip - struct representing pwm chip
 *
 * @mmio_base: base address of pwm chip
 * @clk: pointer to clk structure of pwm chip
 * @chip: linux pwm chip representation
 */
struct ark_pwm_chip {
	void __iomem *mmio_base;
	struct clk *clk;
	struct pwm_chip chip;
};

static inline struct ark_pwm_chip *to_ark_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct ark_pwm_chip, chip);
}

static inline u32 ark_pwm_readl(struct ark_pwm_chip *chip, unsigned int num,
				  unsigned long offset)
{
	return readl_relaxed(chip->mmio_base + (num << 4) + offset);
}

static inline void ark_pwm_writel(struct ark_pwm_chip *chip,
				    unsigned int num, unsigned long offset,
				    unsigned long val)
{
	writel_relaxed(val, chip->mmio_base + (num << 4) + offset);
}

static int ark_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ark_pwm_chip *pc = to_ark_pwm_chip(chip);
	u32 val;

	val = ark_pwm_readl(pc, pwm->hwpwm, PWM_EN);
	val |= 1;
	ark_pwm_writel(pc, pwm->hwpwm, PWM_EN, val);

	return 0;
}

static void ark_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ark_pwm_chip *pc = to_ark_pwm_chip(chip);
	u32 val;

	val = ark_pwm_readl(pc, pwm->hwpwm, PWM_EN);
	val &= ~1;
	ark_pwm_writel(pc, pwm->hwpwm, PWM_EN, val);

	clk_disable(pc->clk);
}

static int ark_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			    int duty_ns, int period_ns)
{
	struct ark_pwm_chip *pc = to_ark_pwm_chip(chip);
	u32 clk_rate = clk_get_rate(pc->clk);
	u32 duty, counter;

    /* NOTE: given that pwm clk = 24MHz,
     * the minimum duty_ns   = 1/24 * 10^3 = 41.67ns = 42ns
     * the minimum period_ns = minimum duty_ns = 42ns
     */
	if (duty_ns && duty_ns < 1000000000 / clk_rate) {
         printk("%s %d: invalid input duty_ns %d\n",
             __FUNCTION__, __LINE__, duty_ns);
        return -EINVAL;
    }

	ark_pwm_disable(chip, pwm);

    /* set pwm duty register */
	duty = duty_ns * (clk_rate / 1000000) / 1000;
	ark_pwm_writel(pc, pwm->hwpwm, PWM_DUTY, duty);

    /* set pwm counter register */
	counter = period_ns * (clk_rate / 1000000) / 1000;
	ark_pwm_writel(pc, pwm->hwpwm, PWM_CNTR, counter);

	ark_pwm_enable(chip, pwm);

	return 0;
}

static void ark_pwm_get_state(struct pwm_chip *chip,
				struct pwm_device *pwm,
				struct pwm_state *state)
{
	struct ark_pwm_chip *pc = to_ark_pwm_chip(chip);
	u32 clk_rate;
	u32 val;

	clk_rate = clk_get_rate(pc->clk);

	val = ark_pwm_readl(pc, pwm->hwpwm, PWM_EN);
	if (val & 1)
		state->enabled = true;
	else
		state->enabled = false;

	val = ark_pwm_readl(pc, pwm->hwpwm, PWM_DUTY);
	state->duty_cycle = val * 1000000 / clk_rate * 1000;

	val = ark_pwm_readl(pc, pwm->hwpwm, PWM_CNTR);
	state->period = val * 1000000 / clk_rate * 1000;
}

static int ark_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			   struct pwm_state *state)
{
	struct pwm_state cstate;

	pwm_get_state(pwm, &cstate);

	if (state->enabled) {
		if (cstate.enabled &&
		    cstate.duty_cycle == state->duty_cycle &&
		    cstate.period == state->period) {
			return 0;
		}

		ark_pwm_config(chip, pwm, state->duty_cycle, state->period);
	} else if (cstate.enabled) {
		ark_pwm_disable(chip, pwm);
	}

	return 0;
}

static const struct pwm_ops ark_pwm_ops = {
	.apply = ark_pwm_apply,
	.get_state = ark_pwm_get_state,
	.owner = THIS_MODULE,
};

static int ark_pwm_probe(struct platform_device *pdev)
{
	struct ark_pwm_chip *pc;
	struct resource *r;
	u32 clk_rate;
	u32 counter;
	int ret, i;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->mmio_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(pc->mmio_base))
		return PTR_ERR(pc->mmio_base);

	pc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pc->clk))
		return PTR_ERR(pc->clk);

	platform_set_drvdata(pdev, pc);

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &ark_pwm_ops;
	pc->chip.base = -1;
	pc->chip.npwm = NUM_PWM;

	/* set default period */
	clk_rate = clk_get_rate(pc->clk);
	for (i = 0; i < NUM_PWM; i++) {
		counter = PWM_DEFAULT_PERIOD * (clk_rate / 1000000) / 1000;
		ark_pwm_writel(pc, i, PWM_CNTR, counter);
	}

	ret = clk_prepare(pc->clk);
	if (ret)
		return ret;

	ret = pwmchip_add(&pc->chip);
	if (ret < 0) {
		clk_unprepare(pc->clk);
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
	}

	return ret;
}

static int ark_pwm_remove(struct platform_device *pdev)
{
	struct ark_pwm_chip *pc = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < NUM_PWM; i++)
		pwm_disable(&pc->chip.pwms[i]);

	/* clk was prepared in probe, hence unprepare it here */
	clk_unprepare(pc->clk);
	return pwmchip_remove(&pc->chip);
}

static const struct of_device_id ark_pwm_of_match[] = {
	{ .compatible = "arkmicro,ark-pwm" },
	{ }
};

MODULE_DEVICE_TABLE(of, ark_pwm_of_match);

static struct platform_driver ark_pwm_driver = {
	.driver = {
		.name = "ark-pwm",
		.of_match_table = ark_pwm_of_match,
	},
	.probe = ark_pwm_probe,
	.remove = ark_pwm_remove,
};
//module_platform_driver(ark_pwm_driver);

static int __init ark_pwm_init(void)
{
	return platform_driver_register(&ark_pwm_driver);
}
subsys_initcall(ark_pwm_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro pwm driver");
MODULE_LICENSE("GPL v2");
