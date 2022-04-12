/*
 * Touchscreen driver for WM831x PMICs
 *
 * Copyright 2011 Wolfson Microelectronics plc.
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/pm.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>


struct ark_nop_ts {
	struct input_dev *input_dev;
};

static int ark_nop_ts_probe(struct platform_device *pdev)
{
	int ret = -1;
    s8 phys[32];
	s8 *name = "ark_nop touchscreen";
	struct input_dev *input_dev = NULL;
	struct ark_nop_ts *ts = devm_kzalloc(&pdev->dev, sizeof(struct ark_nop_ts), GFP_KERNEL);

	if(!ts) {
		printk(KERN_ERR "%s devm_kazlloc failed\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev) {
		ret = -ENOMEM;
		goto err_alloc;
	}
	ts->input_dev = input_dev;

	/* set up touch configuration */
    sprintf(phys, "input/ts");
	input_dev->name = name;
	input_dev->phys = phys;
	ts->input_dev->id.bustype = BUS_HOST;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, 4095, 5, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 4095, 5, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 4095, 5, 0);

	input_set_drvdata(input_dev, ts);
	input_dev->dev.parent = &pdev->dev;

	ret = input_register_device(input_dev);
	if (ret)
		goto err_input_register_device;

	platform_set_drvdata(pdev, ts);
	printk("%s success\n", __FUNCTION__);
	return 0;

err_input_register_device:
err_alloc:
	printk(KERN_ERR "%s failed\n", __FUNCTION__);

	return ret;
}

static int ark_nop_ts_remove(struct platform_device *pdev)
{
	struct ark_nop_ts *ts = platform_get_drvdata(pdev);

	if(ts)
	{
		if(ts->input_dev)
			input_unregister_device(ts->input_dev);
	}

	return 0;
}

static const struct of_device_id ark_nop_ts_match[] = {
	{
		.compatible = "arkmicro,ark-nop-ts",
	}, 
	{},
};
MODULE_DEVICE_TABLE(of, ark_nop_ts_match);

static struct platform_driver ark_nop_ts_driver = {
	.probe = ark_nop_ts_probe,
	.remove = ark_nop_ts_remove,
	.driver = {
		.name = "ark_nop_ts",
		.owner	= THIS_MODULE,
		.of_match_table = ark_nop_ts_match,
	},
};

module_platform_driver(ark_nop_ts_driver);

/* Module information */
MODULE_AUTHOR("Arkmicro>");
MODULE_DESCRIPTION("Ark Nop touchscreen driver");
MODULE_LICENSE("GPL");
