#if !defined(__NOPMI_CHG_COMMON_H__)
#define __NOPMI_CHG_COMMON_H__

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pmic-voter.h>

/* Charger-IC selector (downstream kept this in power_supply.h).
 * SYV = bq2589x buck on these boards; MAXIM is compiled out until
 * max77729 is ported. */
typedef enum _NOPMI_CHARGER_IC_TYPE {
	NOPMI_CHARGER_IC_NONE,
	NOPMI_CHARGER_IC_MAXIM,
	NOPMI_CHARGER_IC_SYV,
	NOPMI_CHARGER_IC_SC,
	NOPMI_CHARGER_IC_MAX
} NOPMI_CHARGER_IC_TYPE;

enum quick_charge_type {
	QUICK_CHARGE_NORMAL = 0,
	QUICK_CHARGE_FAST,
	QUICK_CHARGE_FLASH,
	QUICK_CHARGE_TURBE,
	QUICK_CHARGE_MAX,
};

struct quick_charge {
	enum power_supply_type adap_type;
	enum quick_charge_type adap_cap;
};


int nopmi_chg_is_usb_present(struct power_supply *usb_psy);

char nopmi_set_charger_ic_type(NOPMI_CHARGER_IC_TYPE nopmi_type);
NOPMI_CHARGER_IC_TYPE nopmi_get_charger_ic_type(void);

/* Implemented in nopmi_chg.c: toggles the main buck (bq25890 HiZ via
 * POWER_SUPPLY_PROP_ONLINE). Replaces downstream main_set_charge_enable()
 * from the downstream bq2589x driver. */
int nopmi_set_charge_enable(bool en);

int nopmi_get_quick_charge_type(struct power_supply *psy);
#endif
