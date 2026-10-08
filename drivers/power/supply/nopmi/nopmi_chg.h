#if !defined(__NOPMI_CHG_H__)
#define __NOPMI_CHG_H__

#include "nopmi_chg_jeita.h"

#define STEP_TABLE_MAX 2
#define STEP_DOWN_CURR_MA 150
#define CV_BATT_VOLT_HYSTERESIS 10

#define CC_CV_STEP_VOTER		"CC_CV_STEP_VOTER"
#define NOPMI_DEFAULT_VOTER		"NOPMI_DEFAULT_VOTER"
#define NOPMI_TYPE_VOTER		"NOPMI_TYPE_VOTER"

/* Fallbacks used when an election has no active votes. They match the
 * downstream bq2589x DT (charge-current 2000mA) and the nopmi DT
 * (qcom,fv-max-uv 4400000) for these boards. */
#define NOPMI_DEFAULT_FCC_MA		2000
#define NOPMI_DEFAULT_ICL_MA		2000
#define NOPMI_DEFAULT_FV_MV		4400

struct step_config {
	int volt_lim;
	int curr_lim;
};

struct nopmi_dt_props {
	int	usb_icl_ua;
	int	chg_inhibit_thr_mv;
	bool	no_battery;
	bool	hvdcp_disable;
	bool	hvdcp_autonomous;
	bool	adc_based_aicl;
	int	sec_charger_config;
	int	auto_recharge_soc;
	int	auto_recharge_vbat_mv;
	int	wd_bark_time;
	int	wd_snarl_time_cfg;
	int	batt_profile_fcc_ua;
	int	batt_profile_fv_uv;
	int	pump_min_uv;
	int	pump_max_uv;
	int	pump_min_ua;
	int	pump_max_soc;
};


struct nopmi_chg {
	struct platform_device *pdev;
	struct device *dev;

	/* Downstream kept MTK charger_device / Richtek tcpc handles here,
	 * but nothing ever used them; mainline uses different frameworks.
	 * Cable state is polled from the charger + pump power supplies. */

	struct power_supply *main_psy;
	struct power_supply *master_psy;
	struct power_supply *slave_psy;
	struct power_supply *batt_psy;
	struct power_supply *usb_psy;
	struct power_supply *bms_psy;
	struct power_supply *bbc_psy;
	struct power_supply *sc_psy;
	struct power_supply *pd_psy;

	struct votable		*fcc_votable;
	struct votable		*fv_votable;
	struct votable		*usb_icl_votable;
	struct nopmi_dt_props	dt;
	struct delayed_work nopmi_chg_work;
	struct delayed_work 	cvstep_monitor_work;
	int pd_active;
	int real_type;
	int pd_min_vol;
	int pd_max_vol;
	int pd_cur_max;
	int pd_usb_suspend;
	int pd_in_hard_reset;
	int usb_online;
	int batt_health;
	int input_suspend;
	bool pump_enabled;
	/*jeita config*/
	struct nopmi_chg_jeita_st jeita_ctl;

	/* thermal */
	int *thermal_mitigation;
	int thermal_levels;
	int system_temp_level;
};

int get_prop_battery_charging_enabled(struct votable *usb_icl_votable,
				      union power_supply_propval *val);
int set_prop_battery_charging_enabled(struct votable *usb_icl_votable,
				      const union power_supply_propval *val);

#endif
