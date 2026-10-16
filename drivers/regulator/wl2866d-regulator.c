// SPDX-License-Identifier: GPL-2.0-only
/*
 * WL2866D camera PMIC regulator driver
 *
 * Based on downstream driver by XiaoMi, Inc.
 *
 * Register map (from downstream source):
 *   0x02  discharge control   0x8f = enable, 0x00 = disable
 *   0x03  DVDD1 voltage       Vout = 600 + code*6 mV
 *   0x04  DVDD2 voltage       Vout = 600 + code*6 mV
 *   0x05  AVDD1 voltage       Vout = 1200 + code*12.5 mV
 *   0x06  AVDD2 voltage       Vout = 1200 + code*12.5 mV
 *   0x0e  output enable       bit0=DVDD1 bit1=DVDD2 bit2=AVDD1 bit3=AVDD2
 *   0x03  reads 0x64 or 0x55 as chip ID
 */

#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#define WL2866D_REG_DISCHARGE		0x02
#define WL2866D_REG_DVDD1_VOLT		0x03
#define WL2866D_REG_DVDD2_VOLT		0x04
#define WL2866D_REG_AVDD1_VOLT		0x05
#define WL2866D_REG_AVDD2_VOLT		0x06
#define WL2866D_REG_ENABLE		0x0e

#define WL2866D_DISCHARGE_ENABLE	0x8f
#define WL2866D_DISCHARGE_DISABLE	0x00

#define WL2866D_DVDD_MIN_UV		600000
#define WL2866D_DVDD_STEP_UV		6000
#define WL2866D_DVDD_N_VOLTAGES		151	/* (1500-600)/6 + 1 */

#define WL2866D_AVDD_MIN_UV		1200000
#define WL2866D_AVDD_STEP_UV		12500
#define WL2866D_AVDD_N_VOLTAGES		145	/* (3000-1200)/12.5 + 1 */

struct wl2866d {
	struct device *dev;
	struct regmap *regmap;
	struct gpio_desc *enable_gpio;
};

static int wl2866d_reg_enable(struct regulator_dev *rdev)
{
	struct wl2866d *wl = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);

	/* Briefly pulse discharge to clean output caps */
	regmap_write(wl->regmap, WL2866D_REG_DISCHARGE, WL2866D_DISCHARGE_ENABLE);
	usleep_range(100, 200);
	regmap_write(wl->regmap, WL2866D_REG_DISCHARGE, WL2866D_DISCHARGE_DISABLE);

	return regmap_update_bits(wl->regmap, WL2866D_REG_ENABLE,
				  BIT(id), BIT(id));
}

static int wl2866d_reg_disable(struct regulator_dev *rdev)
{
	struct wl2866d *wl = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);

	regmap_update_bits(wl->regmap, WL2866D_REG_ENABLE, BIT(id), 0);

	/* Discharge to avoid leakage */
	return regmap_write(wl->regmap, WL2866D_REG_DISCHARGE,
			    WL2866D_DISCHARGE_ENABLE);
}

static int wl2866d_reg_is_enabled(struct regulator_dev *rdev)
{
	struct wl2866d *wl = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	unsigned int val;
	int ret;

	ret = regmap_read(wl->regmap, WL2866D_REG_ENABLE, &val);
	if (ret)
		return ret;

	return !!(val & BIT(id));
}

static const struct regulator_ops wl2866d_reg_ops = {
	.enable		= wl2866d_reg_enable,
	.disable	= wl2866d_reg_disable,
	.is_enabled	= wl2866d_reg_is_enabled,
	.list_voltage	= regulator_list_voltage_linear,
	.map_voltage	= regulator_map_voltage_linear,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

#define WL2866D_REG_DESC(_name, _id, _vreg, _min, _step, _n)	\
	{							\
		.name		= _name,			\
		.of_match	= _name,			\
		.regulators_node = "regulators",		\
		.id		= _id,				\
		.ops		= &wl2866d_reg_ops,		\
		.type		= REGULATOR_VOLTAGE,		\
		.owner		= THIS_MODULE,			\
		.min_uV		= _min,				\
		.uV_step	= _step,			\
		.n_voltages	= _n,				\
		.vsel_reg	= _vreg,			\
		.vsel_mask	= 0xff,				\
		.enable_reg	= WL2866D_REG_ENABLE,		\
		.enable_mask	= BIT(_id),			\
	}

static const struct regulator_desc wl2866d_regs[] = {
	WL2866D_REG_DESC("dvdd1", 0, WL2866D_REG_DVDD1_VOLT,
			 WL2866D_DVDD_MIN_UV, WL2866D_DVDD_STEP_UV,
			 WL2866D_DVDD_N_VOLTAGES),
	WL2866D_REG_DESC("dvdd2", 1, WL2866D_REG_DVDD2_VOLT,
			 WL2866D_DVDD_MIN_UV, WL2866D_DVDD_STEP_UV,
			 WL2866D_DVDD_N_VOLTAGES),
	WL2866D_REG_DESC("avdd1", 2, WL2866D_REG_AVDD1_VOLT,
			 WL2866D_AVDD_MIN_UV, WL2866D_AVDD_STEP_UV,
			 WL2866D_AVDD_N_VOLTAGES),
	WL2866D_REG_DESC("avdd2", 3, WL2866D_REG_AVDD2_VOLT,
			 WL2866D_AVDD_MIN_UV, WL2866D_AVDD_STEP_UV,
			 WL2866D_AVDD_N_VOLTAGES),
};

static const struct regmap_config wl2866d_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= 0x0f,
};

static int wl2866d_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct wl2866d *wl;
	struct regulator_config cfg = {};
	unsigned int id;
	int i, ret;

	pr_info("wl2866d: probe entered, addr=0x%02x\n", client->addr);

	wl = devm_kzalloc(dev, sizeof(*wl), GFP_KERNEL);
	if (!wl)
		return -ENOMEM;

	wl->dev = dev;

	wl->regmap = devm_regmap_init_i2c(client, &wl2866d_regmap_config);
	if (IS_ERR(wl->regmap))
		return dev_err_probe(dev, PTR_ERR(wl->regmap),
				     "failed to init regmap\n");

	/* EN pin must be high for the chip to respond on I2C */
	wl->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(wl->enable_gpio))
		return dev_err_probe(dev, PTR_ERR(wl->enable_gpio),
				     "failed to get enable gpio\n");

	usleep_range(2000, 3000);

	/* Chip ID: reads 0x64 or 0x55 from DVDD1 register before any write */
	ret = regmap_read(wl->regmap, WL2866D_REG_DVDD1_VOLT, &id);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read chip ID\n");

	if (id != 0x64 && id != 0x55)
		return dev_err_probe(dev, -ENODEV,
				     "unexpected chip ID 0x%02x\n", id);

	/* Set default voltages matching downstream init sequence */
	regmap_write(wl->regmap, WL2866D_REG_DVDD1_VOLT, 0x64); /* 1200 mV */
	regmap_write(wl->regmap, WL2866D_REG_DVDD2_VOLT, 0x4b); /* 1050 mV */
	regmap_write(wl->regmap, WL2866D_REG_AVDD1_VOLT, 0x80); /* 2800 mV */
	regmap_write(wl->regmap, WL2866D_REG_AVDD2_VOLT, 0x80); /* 2800 mV */

	cfg.dev = dev;
	cfg.driver_data = wl;

	for (i = 0; i < ARRAY_SIZE(wl2866d_regs); i++) {
		struct regulator_dev *rdev;

		rdev = devm_regulator_register(dev, &wl2866d_regs[i], &cfg);
		if (IS_ERR(rdev))
			return dev_err_probe(dev, PTR_ERR(rdev),
					     "failed to register %s\n",
					     wl2866d_regs[i].name);
	}

	i2c_set_clientdata(client, wl);
	dev_info(dev, "WL2866D registered (chip id 0x%02x)\n", id);

	return 0;
}

static const struct of_device_id __maybe_unused wl2866d_of_match[] = {
	{ .compatible = "ovti,wl2866d" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, wl2866d_of_match);

static const struct i2c_device_id wl2866d_id[] = {
	{ "wl2866d" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, wl2866d_id);

static struct i2c_driver wl2866d_regulator_driver = {
	.driver = {
		.name = "wl2866d-regulator",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
		.of_match_table = of_match_ptr(wl2866d_of_match),
	},
	.probe = wl2866d_probe,
	.id_table = wl2866d_id,
};
module_i2c_driver(wl2866d_regulator_driver);

MODULE_ALIAS("i2c:wl2866d");
MODULE_ALIAS("platform:wl2866d");
MODULE_DESCRIPTION("WL2866D camera PMIC regulator driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oscar Allegro <hsc100zz@gmail.com>");
