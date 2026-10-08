#define pr_fmt(fmt) "nopmi_chg_common %s: " fmt, __func__

#include "nopmi_chg_common.h"

static NOPMI_CHARGER_IC_TYPE nopmi_charger_ic = NOPMI_CHARGER_IC_NONE;

int nopmi_chg_is_usb_present(struct power_supply *usb_psy)
{
	union power_supply_propval prop = {0, };
	int ret = 0;
	int usb_present;

	if(!usb_psy)
	{
		usb_psy = power_supply_get_by_name("usb");
		if (!usb_psy) {
			pr_err("usb supply not found, defer probe\n");
			return -EINVAL;
		}
	}

	ret = power_supply_get_property(usb_psy,
				POWER_SUPPLY_PROP_ONLINE, &prop);
	if (ret < 0) {
		pr_err("couldn't read usb_present property, ret=%d\n", ret);
		return -EINVAL;
	}
	usb_present = prop.intval;

	return usb_present;

}

char nopmi_set_charger_ic_type(NOPMI_CHARGER_IC_TYPE nopmi_type)
{
	char ret = 0;

	if(NOPMI_CHARGER_IC_MAX > nopmi_type && NOPMI_CHARGER_IC_NONE < nopmi_type)
	{
		nopmi_charger_ic = nopmi_type;
	}
	else
	{
		ret = -ENODEV;
	}
	return ret;
}

NOPMI_CHARGER_IC_TYPE nopmi_get_charger_ic_type(void)
{
	return nopmi_charger_ic;
}

EXPORT_SYMBOL_GPL(nopmi_set_charger_ic_type);
EXPORT_SYMBOL_GPL(nopmi_get_charger_ic_type);

/* nopmi_set_charge_enable() itself lives in nopmi_chg.c now: it drives
 * the main buck through its power supply (bq25890 HiZ). The downstream
 * switch below dispatched to per-IC drivers that are not ported. */

struct quick_charge adapter_cap[10] = {
	{ POWER_SUPPLY_TYPE_USB,        QUICK_CHARGE_NORMAL },
	{ POWER_SUPPLY_TYPE_USB_DCP,    QUICK_CHARGE_NORMAL },
	{ POWER_SUPPLY_TYPE_USB_CDP,    QUICK_CHARGE_NORMAL },
	{ POWER_SUPPLY_TYPE_USB_ACA,    QUICK_CHARGE_NORMAL },
	{ POWER_SUPPLY_TYPE_USB_FLOAT,  QUICK_CHARGE_NORMAL },
	{ POWER_SUPPLY_TYPE_USB_PD,       QUICK_CHARGE_FAST },
	{ POWER_SUPPLY_TYPE_USB_HVDCP,    QUICK_CHARGE_FAST },
	{ POWER_SUPPLY_TYPE_USB_HVDCP_3,  QUICK_CHARGE_FAST },
	{ POWER_SUPPLY_TYPE_WIRELESS,     QUICK_CHARGE_FAST },
	{0, 0},
};

int nopmi_get_quick_charge_type(struct power_supply *usb_psy)
{
	union power_supply_propval prop = {0, };
	int i = 0;
	int ret = 0;
	enum power_supply_type chg_type;
	struct power_supply *batt_psy;
	int pd_verifed;
	static bool is_single_flash = false;

	if(!usb_psy)
	{
		usb_psy = power_supply_get_by_name("usb");
		if (!usb_psy) {
			pr_err("usb supply not found, defer probe\n");
			return -EINVAL;
		}
	}

	ret = power_supply_get_property(usb_psy,
				POWER_SUPPLY_PROP_REAL_TYPE, &prop);
	if (ret < 0) {
		pr_err("couldn't read usb real type, ret=%d\n", ret);
		return -EINVAL;
	}
	chg_type = prop.intval;

	if(!batt_psy)
	{
		batt_psy = power_supply_get_by_name("battery");
		if(!batt_psy){
			pr_err("battery supply not found, defer probe\n");
			return -EINVAL;
		}
	}

	ret = power_supply_get_property(batt_psy, POWER_SUPPLY_PROP_TEMP, &prop);
	if (ret < 0) {
		pr_err("couldn't read batt temp property, ret=%d\n", ret);
		return -EINVAL;
	}

	pr_debug("battery temp: %d", prop.intval);
	if (prop.intval < 50 || prop.intval >= 480) {
		if (usb_psy && !is_single_flash){
			pr_debug("battery temp is under 5 or above 48");
			power_supply_changed(usb_psy);
		}
		is_single_flash = true;
		return 0;
	} else {
		if (usb_psy && is_single_flash){
			pr_debug("battery temp returned to normal");
			power_supply_changed(usb_psy);
		}
		is_single_flash = false;
	}

	if (NOPMI_CHARGER_IC_SYV == nopmi_get_charger_ic_type())
		/* No PD-verified adapter glue on mainline yet (downstream
		 * adapter_dev_get_pd_verified has no counterpart); treat
		 * PD as unverified so quick-charge stays conservative. */
		pd_verifed = 0;
	else
		pd_verifed = 1;

	if ((chg_type == POWER_SUPPLY_TYPE_USB_PD) && pd_verifed) {
		return QUICK_CHARGE_TURBE;
	}

	while (adapter_cap[i].adap_type != 0) {
		if (chg_type == adapter_cap[i].adap_type) {
			return adapter_cap[i].adap_cap;
		}
		i++;
	}

	return 0;
}




