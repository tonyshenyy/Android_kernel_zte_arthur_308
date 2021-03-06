#if 0	 //zte'msm_battery.c for 8903 share_memory

/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
#define DEBUG  0

#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>

#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VER_1_1	0x00010001
#define BATTERY_RPC_VER_2_1	0x00020001
#define BATTERY_RPC_VER_4_1     0x00040001
#define BATTERY_RPC_VER_5_1     0x00050001

#define BATTERY_RPC_CB_PROG	(BATTERY_RPC_PROG | 0x01000000)

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VER_1_1		0x00010001
#define CHG_RPC_VER_1_3		0x00010003
#define CHG_RPC_VER_2_2		0x00020002
#define CHG_RPC_VER_3_1         0x00030001
#define CHG_RPC_VER_4_1         0x00040001

#define BATTERY_REGISTER_PROC				2
#define BATTERY_MODIFY_CLIENT_PROC			4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_READ_MV_PROC				12
#define BATTERY_ENABLE_DISABLE_FILTER_PROC		14

#define VBATT_FILTER			2

#define BATTERY_CB_TYPE_PROC		1
#define BATTERY_CB_ID_ALL_ACTIV		1
#define BATTERY_CB_ID_LOW_VOL		2

#define BATTERY_LOW		3200
#define BATTERY_HIGH		4300

#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC	12
#define ONCRPC_CHARGER_API_VERSIONS_PROC	0xffffffff

#define BATT_RPC_TIMEOUT    5000	/* 5 sec */

#define INVALID_BATT_HANDLE    -1

#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))

//merged by zhang.yu_1 from gb7x27 @111221
#define FEATURE_ZTE_APP_ENABLE_USB_CHARGING 
#define ZTE_PLATFORM_NOT_SHUTDOWN_WHILE_PERCENTAGE_0

#if DEBUG
#define DBG_LIMIT(x...) do {if (printk_ratelimit()) pr_debug(x); } while (0)
#else
#define DBG_LIMIT(x...) do {} while (0)
#endif

#define ABSM(a,b)	  ((a)>(b)?((a)-(b)):((b)-(a)))	/* zte-ccb-20120208-2 */

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN,
};

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
	/* The charger is good      */
	CHARGER_STATUS_GOOD,
	/* The charger is bad       */
	CHARGER_STATUS_BAD,
	/* The charger is weak      */
	CHARGER_STATUS_WEAK,
	/* Invalid charger status.  */
	CHARGER_STATUS_INVALID
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
	/* The charger is removed                 */
	CHARGER_TYPE_NONE,
	/* The charger is a regular wall charger   */
	CHARGER_TYPE_WALL,
	/* The charger is a PC USB                 */
	CHARGER_TYPE_USB_PC,
	/* The charger is a wall USB charger       */
	CHARGER_TYPE_USB_WALL,
	/* The charger is a USB carkit             */
	CHARGER_TYPE_USB_CARKIT,
	/* Invalid charger hardware status.        */
	CHARGER_TYPE_INVALID
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
	/* The battery is good        */
	BATTERY_STATUS_GOOD,
	/* The battery is cold/hot    */
	BATTERY_STATUS_BAD_TEMP,
	/* The battery is bad         */
	BATTERY_STATUS_BAD,
	/* The battery is removed     */
	BATTERY_STATUS_REMOVED,		/* on v2.2 only */
	BATTERY_STATUS_INVALID_v1 = BATTERY_STATUS_REMOVED,
	/* Invalid battery status.    */
	BATTERY_STATUS_INVALID
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
	/* The battery voltage is dead/very low (less than 3.2V) */
	BATTERY_LEVEL_DEAD,
	/* The battery voltage is weak/low (between 3.2V and 3.4V) */
	BATTERY_LEVEL_WEAK,
	/* The battery voltage is good/normal(between 3.4V and 4.2V) */
	BATTERY_LEVEL_GOOD,
	/* The battery voltage is up to full (close to 4.2V) */
	BATTERY_LEVEL_FULL,
	/* Invalid battery voltage level. */
	BATTERY_LEVEL_INVALID
};

#ifndef CONFIG_BATTERY_MSM_FAKE
struct rpc_reply_batt_chg_v1 {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32     battery_voltage;
	u32	battery_temp;
	
         #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for send battery temp and capacity when no charging @110813   	
	 u32 batt_capacity;
         #endif	
};

struct rpc_reply_batt_chg_v2 {
	struct rpc_reply_batt_chg_v1	v1;

	u32	is_charger_valid;
	u32	is_charging;
	u32	is_battery_valid;
	u32	ui_event;
};

union rpc_reply_batt_chg {
	struct rpc_reply_batt_chg_v1	v1;
	struct rpc_reply_batt_chg_v2	v2;
};

static union rpc_reply_batt_chg rep_batt_chg;
#endif

struct msm_battery_info {
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 chg_api_version;
	u32 batt_technology;
	u32 batt_api_version;

	u32 avail_chg_sources;
	u32 current_chg_source;

	u32 batt_status;
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
	u32 batt_capacity; /* in percentage */

	u32 charger_status;
	u32 charger_type;
	u32 battery_status;
	u32 battery_level;
	u32 battery_voltage; /* in millie volts */
	u32 battery_temp;  /* in celsius */

	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;
	struct power_supply *current_ps;

	struct msm_rpc_client *batt_client;
	struct msm_rpc_endpoint *chg_ep;

	wait_queue_head_t wait_q;

	u32 vbatt_modify_reply_avail;

	struct early_suspend early_suspend;
};

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
	.charger_status = CHARGER_STATUS_BAD,
	.charger_type = CHARGER_TYPE_INVALID,
	.battery_status = BATTERY_STATUS_GOOD,
	.battery_level = BATTERY_LEVEL_FULL,
	.battery_voltage = BATTERY_HIGH,
	.batt_capacity = 100,
	.batt_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.batt_health = POWER_SUPPLY_HEALTH_GOOD,
	.batt_valid  = 1,
	.battery_temp = 23,
	.vbatt_modify_reply_avail = 0,
};

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

#ifdef CONFIG_ZTE_PLATFORM//merged from 7x30 by zhang.yu_1 @111128
/*	zte-ccb-20120208-2
static char *charger_st[] = {
    "good", "bad", "weak", "invalid"
};*/

static char *charger_tp[] = {
    "no chg", "wall", "usb pc", "usb  wall", "usb carkit",
    "invalid"
};

static char *battery_st[] = {
    "good ", "bad temp", "bad", "removed", "invalid"
};

static char *batt_st[] ={
    "unkown ", "charging", "discharging", "not charing", "full"	
};

static char *battery_lvl[] = {
    "dead", "weak", "good", "full", "invalid"
};

#endif

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
		}
		if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
         #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for battery temperature @110906			
         POWER_SUPPLY_PROP_TEMP		 
         #endif
};

#ifdef ZTE_PLATFORM_NOT_SHUTDOWN_WHILE_PERCENTAGE_0
static int shutdown_percentage_zero_enable = 0;
#endif

#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
void msm_batt_force_update(void)
{
    // empty to be filled by batt comrades lator
}
#endif


static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = msm_batt_info.batt_capacity;
		break;
#ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for battery temperature @110906		
	case  POWER_SUPPLY_PROP_TEMP:
		val->intval = msm_batt_info.battery_temp * 10;
		break;
#endif	
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
};

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING			//merged by zhang.yu_1 from gb7x27 @111221	

#define USB_CHG_DISABLE 0
#define USB_CHG_ENABLE 1
static int usb_charger_enable = USB_CHG_ENABLE;	
static int usb_charger_enable_pre = USB_CHG_ENABLE;	

//#include <mach/msm_rpcrouter.h>
//#define CHG_RPC_PROG		0x3000001a
//#define CHG_RPC_VERS		0x00010003
#define BATTERY_ENABLE_DISABLE_USB_CHG_PROC 		6


/*--------------------------------------------------------------
msm_batt_handle_control_usb_charging() is added according msm_chg_usb_charger_connected() in rpc_hsusb.c
and the rpc is used to stop or resume usb charging
---------------------------------------------------------------*/
static int msm_batt_handle_control_usb_charging(u32 usb_enable_disable)
{
	int rc;

	struct batt_modify_client_req {
		struct rpc_request_hdr hdr;
		u32 usb_chg_enable;
	} req;

	pr_debug("%s:  msm_rpc_write usb switch enable/disable = %d start.\n",
				__func__,usb_enable_disable);

	req.usb_chg_enable = cpu_to_be32(usb_enable_disable);
    	rc=msm_rpc_call(msm_batt_info.chg_ep, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, &req,
                    sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, rc);
		return rc;
	}
		pr_debug("%s:  msm_rpc_write usb switch enable/disable end rc = %d.\n",
				__func__,rc);

	return 0;
}

static ssize_t zte_usb_chg_show_enable(struct device_driver *driver, char *buf)
{
    pr_debug("%s():line:%d %s usb to chager\n", __func__,__LINE__,usb_charger_enable?"enable":"forbid");
    return snprintf(buf, PAGE_SIZE, "%s\n", usb_charger_enable?"enable":"forbid");
}

static ssize_t zte_usb_chg_store_enable(struct device_driver *driver,
                                  const char *buf, size_t count)
{
    char *p = (char *)buf;
    unsigned int enable= 1;
    enable=(unsigned int)simple_strtol(p, NULL, 10);

    pr_debug("%s():enter,get usb_charger_enable:%d from \"%s\"\n", __func__,enable,buf);
    if(enable==0 ||enable==1)
    {
        usb_charger_enable=enable;
    }
    else
    {
        pr_err("%s():get err usb_charger_enable value:%d \n", __func__,enable);
    }
    //printk("%s(): exit,line:%d\n", __func__,__LINE__);
    return strnlen(buf, count);
}
static DRIVER_ATTR(usb_chg_enable, S_IRWXUGO, zte_usb_chg_show_enable, zte_usb_chg_store_enable);

#endif


#ifndef CONFIG_BATTERY_MSM_FAKE
struct msm_batt_get_volt_ret_data {
	u32 battery_voltage;
};

static int msm_batt_get_volt_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct msm_batt_get_volt_ret_data *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_get_volt_ret_data *)data;
	buf_ptr = (struct msm_batt_get_volt_ret_data *)buf;

	data_ptr->battery_voltage = be32_to_cpu(buf_ptr->battery_voltage);

	return 0;
}

static u32 msm_batt_get_vbatt_voltage(void)
{
	int rc;

	struct msm_batt_get_volt_ret_data rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_READ_MV_PROC,
			NULL, NULL,
			msm_batt_get_volt_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt get volt. rc=%d\n", __func__, rc);
		return 0;
	}

	return rep.battery_voltage;
}

#define	be32_to_cpu_self(v)	(v = be32_to_cpu(v))

static int msm_batt_get_batt_chg_status(void)
{
	int rc;

	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;
	struct rpc_reply_batt_chg_v1 *v1p;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	v1p = &rep_batt_chg.v1;
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		pr_err("%s: ERROR. msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(v1p->more_data)) {
		be32_to_cpu_self(v1p->charger_status);
		be32_to_cpu_self(v1p->charger_type);
		be32_to_cpu_self(v1p->battery_status);
		be32_to_cpu_self(v1p->battery_level);
		be32_to_cpu_self(v1p->battery_voltage);
		be32_to_cpu_self(v1p->battery_temp);
		
                 #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for send battery temp and capacity when no charging @110813   	
        	 	be32_to_cpu_self(v1p->batt_capacity);
                 #endif		
	} else {
		pr_err("%s: No battery/charger data in RPC reply\n", __func__);
		return -EIO;
	}

	return 0;
}

static void msm_batt_update_psy_status(void)
{
	static u32 unnecessary_event_count;
	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32  battery_voltage;
	u32	battery_temp;
	struct	power_supply	*supp;

	#ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for send battery temp and capacity when no charging @110813   	
	u32 battery_capacity;
         #endif
	
	if (msm_batt_get_batt_chg_status())
		return;

	charger_status = rep_batt_chg.v1.charger_status;
	charger_type = rep_batt_chg.v1.charger_type;
	battery_status = rep_batt_chg.v1.battery_status;
	battery_level = rep_batt_chg.v1.battery_level;
	battery_voltage = rep_batt_chg.v1.battery_voltage;
	battery_temp = rep_batt_chg.v1.battery_temp;

         #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for send battery temp and capacity when no charging @110813   	
	 battery_capacity = rep_batt_chg.v1.batt_capacity ;
         #endif	

	/* Make correction for battery status */
	if (battery_status == BATTERY_STATUS_INVALID_v1) {
		if (msm_batt_info.chg_api_version < CHG_RPC_VER_3_1)
			battery_status = BATTERY_STATUS_INVALID;
	}

	if (charger_status == msm_batt_info.charger_status &&
	    charger_type == msm_batt_info.charger_type &&
	    battery_status == msm_batt_info.battery_status &&
	    battery_level == msm_batt_info.battery_level &&
	    //battery_voltage == msm_batt_info.battery_voltage &&
	    (ABSM(battery_voltage,msm_batt_info.battery_voltage) < 8 )&&	//zte-ccb-20120208-2
             #ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING//merged by zhang.yu_1 from gb7x27 @111221	
	    (((charger_type == CHARGER_TYPE_USB_WALL || 
	         charger_type == CHARGER_TYPE_USB_PC) &&	   
                  usb_charger_enable_pre == usb_charger_enable ) ||
                  (charger_type != CHARGER_TYPE_USB_WALL && 
	          charger_type != CHARGER_TYPE_USB_PC))&&
             #endif            
	    battery_temp == msm_batt_info.battery_temp) {
		/* Got unnecessary event from Modem PMIC VBATT driver.
		 * Nothing changed in Battery or charger status.
		 */
		unnecessary_event_count++;
		if ((unnecessary_event_count % 20) == 1)
			DBG_LIMIT("BATT: same event count = %u\n",
				 unnecessary_event_count);
		
		return;
	}

	unnecessary_event_count = 0;

	DBG_LIMIT("BATT: rcvd: %d, %d, %d, %d; %d, %d\n",
		 charger_status, charger_type, battery_status,
		 battery_level, battery_voltage, battery_temp);

	if (battery_status == BATTERY_STATUS_INVALID &&
	    battery_level != BATTERY_LEVEL_INVALID) {
		DBG_LIMIT("BATT: change status(%d) to (%d) for level=%d\n",
			 battery_status, BATTERY_STATUS_GOOD, battery_level);
		battery_status = BATTERY_STATUS_GOOD;
	}

	if (msm_batt_info.charger_type != charger_type) {
		if (charger_type == CHARGER_TYPE_USB_WALL ||
		    charger_type == CHARGER_TYPE_USB_PC ||
		    charger_type == CHARGER_TYPE_USB_CARKIT) {
			DBG_LIMIT("BATT: USB charger plugged in\n");
			msm_batt_info.current_chg_source = USB_CHG;
			supp = &msm_psy_usb;
		} else if (charger_type == CHARGER_TYPE_WALL) {
			DBG_LIMIT("BATT: AC Wall changer plugged in\n");
			msm_batt_info.current_chg_source = AC_CHG;
			supp = &msm_psy_ac;
		} else {
			if (msm_batt_info.current_chg_source & AC_CHG)
				DBG_LIMIT("BATT: AC Wall charger removed\n");
			else if (msm_batt_info.current_chg_source & USB_CHG)
				DBG_LIMIT("BATT: USB charger removed\n");
			else
				DBG_LIMIT("BATT: No charger present\n");
			msm_batt_info.current_chg_source = 0;
			supp = &msm_psy_batt;

			#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING//merged by zhang.yu_1 from gb7x27 @111221	
			usb_charger_enable_pre = USB_CHG_ENABLE;
			pr_debug("USB remove: usb_charger_enable_pre set2 %d \n",usb_charger_enable_pre);
			#endif			

			/* Correct charger status */
			if (charger_status != CHARGER_STATUS_INVALID) {
				DBG_LIMIT("BATT: No charging!\n");
				charger_status = CHARGER_STATUS_INVALID;
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}
	} else
		supp = NULL;

	if (msm_batt_info.charger_status != charger_status) {
		if (charger_status == CHARGER_STATUS_GOOD ||
		    charger_status == CHARGER_STATUS_WEAK) {
			if (msm_batt_info.current_chg_source) {
				DBG_LIMIT("BATT: Charging.\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_CHARGING;

				/* Correct when supp==NULL */
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			}
		} else {
			DBG_LIMIT("BATT: No charging.\n");
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			supp = &msm_psy_batt;
		}
	} else {
		/* Correct charger status */
		if (charger_type != CHARGER_TYPE_INVALID &&
		    charger_status == CHARGER_STATUS_GOOD) {
			DBG_LIMIT("BATT: In charging\n");
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_CHARGING;
		}
	}

	/* Correct battery voltage and status */
	if (!battery_voltage) {
		if (charger_status == CHARGER_STATUS_INVALID) {
			DBG_LIMIT("BATT: Read VBATT\n");
			battery_voltage = msm_batt_get_vbatt_voltage();
		} else
			/* Use previous */
			battery_voltage = msm_batt_info.battery_voltage;
	}
	if (battery_status == BATTERY_STATUS_INVALID) {
		if (battery_voltage >= msm_batt_info.voltage_min_design &&
		    battery_voltage <= msm_batt_info.voltage_max_design) {
			DBG_LIMIT("BATT: Battery valid\n");
			msm_batt_info.batt_valid = 1;
			battery_status = BATTERY_STATUS_GOOD;
		}
	}

	if (msm_batt_info.battery_status != battery_status) {
		if (battery_status != BATTERY_STATUS_INVALID) {
			msm_batt_info.batt_valid = 1;

			if (battery_status == BATTERY_STATUS_BAD) {
				DBG_LIMIT("BATT: Battery bad.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_DEAD;
			} else if (battery_status == BATTERY_STATUS_BAD_TEMP) {
				DBG_LIMIT("BATT: Battery overheat.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_OVERHEAT;
			} else {
				DBG_LIMIT("BATT: Battery good.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_GOOD;
			}
		} else {
			msm_batt_info.batt_valid = 0;
			DBG_LIMIT("BATT: Battery invalid.\n");
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		}

		if (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_CHARGING) {
			if (battery_status == BATTERY_STATUS_INVALID) {
				DBG_LIMIT("BATT: Battery -> unknown\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_UNKNOWN;
			} else {
				DBG_LIMIT("BATT: Battery -> discharging\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_DISCHARGING;
			}
		}
		if (!supp) {
			if (msm_batt_info.current_chg_source) {
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			} else
				supp = &msm_psy_batt;
		}
	}

         #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for icon update when charged full @111124
         if(msm_batt_info.batt_status == POWER_SUPPLY_STATUS_CHARGING){
                  //if(battery_status == BATTERY_STATUS_BAD_TEMP || battery_status == BATTERY_STATUS_REMOVED){
                  if(battery_status == BATTERY_STATUS_BAD_TEMP || (battery_temp < 0 || battery_temp >45)){                  
			msm_batt_info.batt_status =	POWER_SUPPLY_STATUS_NOT_CHARGING;					   	
                  }
         	else if(battery_capacity == 100){
			msm_batt_info.batt_status =	POWER_SUPPLY_STATUS_FULL;
                  }
         }
	#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING//merged by zhang.yu_1 from gb7x27 @111221	
         if (charger_type == CHARGER_TYPE_USB_WALL || charger_type == CHARGER_TYPE_USB_PC){
		if(USB_CHG_DISABLE == usb_charger_enable)//if disabled usb charging, show  discharging
		{
	                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		}

		if((usb_charger_enable_pre != usb_charger_enable) /* &&
			((usb_charger_enable == USB_CHG_DISABLE && (msm_batt_info.batt_status == POWER_SUPPLY_STATUS_CHARGING))
			||(usb_charger_enable == USB_CHG_ENABLE && (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_CHARGING)))*/){
			pr_debug("Before RPC charging = %d, usb_charger_enable_pre = %d,usb_charger_enable = %d \n",
				msm_batt_info.batt_status,usb_charger_enable_pre,usb_charger_enable);
			
			usb_charger_enable_pre = usb_charger_enable;
			msm_batt_handle_control_usb_charging(usb_charger_enable);
		}
	}
	#endif
		 
	#endif	

         #ifdef CONFIG_ZTE_PLATFORM//added by zhang.yu_1 for send battery temp and capacity when no charging @110813   	
	pr_debug("[ZHY@msm-battery]:%s,battery_status=%s,charge=%s, level=%s, %dmV, %dC, %d%% ,%d\n",
		 charger_tp[charger_type], battery_st[battery_status],batt_st[msm_batt_info.batt_status],
		 battery_lvl[battery_level], battery_voltage, battery_temp, battery_capacity
		 #ifdef ZTE_PLATFORM_NOT_SHUTDOWN_WHILE_PERCENTAGE_0//merged by zhang.yu_1 from gb7x27 @111221
                   ,shutdown_percentage_zero_enable
                   #endif
		 );
	#endif

	msm_batt_info.charger_status 	= charger_status;
	msm_batt_info.charger_type 	= charger_type;
	msm_batt_info.battery_status 	= battery_status;
	msm_batt_info.battery_level 	= battery_level;
	msm_batt_info.battery_temp 	= battery_temp;

	if (msm_batt_info.battery_voltage != battery_voltage) {
		msm_batt_info.battery_voltage  	= battery_voltage;
		
                 #ifdef ZTE_PLATFORM_NOT_SHUTDOWN_WHILE_PERCENTAGE_0 //merged by zhang.yu_1 from gb7x27 @111221
                 if((0 == battery_capacity)&&(0 == shutdown_percentage_zero_enable))// percentage is 0% and not allow to shutdown,report 1% instead
                 		msm_batt_info.batt_capacity = 33;
                 else
		msm_batt_info.batt_capacity = battery_capacity;			
		#else			
		msm_batt_info.batt_capacity = msm_batt_info.calculate_capacity(battery_voltage);
		#endif
		
		DBG_LIMIT("BATT: voltage = %u mV [capacity = %d%%]\n",
			 battery_voltage, msm_batt_info.batt_capacity);

		if (!supp)
			supp = msm_batt_info.current_ps;
	}

	if (supp) {
		msm_batt_info.current_ps = supp;
		DBG_LIMIT("BATT: Supply = %s\n", supp->name);
		power_supply_changed(supp);
	}
}

#ifdef CONFIG_HAS_EARLYSUSPEND
struct batt_modify_client_req {

	u32 client_handle;

	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
};

struct batt_modify_client_rep {
	u32 result;
};

static int msm_batt_modify_client_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_modify_client_req *batt_modify_client_req =
		(struct batt_modify_client_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(batt_modify_client_req->client_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->desired_batt_voltage);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->voltage_direction);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->batt_cb_id);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->cb_data);
	size += sizeof(u32);

	return size;
}

static int msm_batt_modify_client_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct  batt_modify_client_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_modify_client_rep *)data;
	buf_ptr = (struct batt_modify_client_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);

	return 0;
}

static int msm_batt_modify_client(u32 client_handle, u32 desired_batt_voltage,
	     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	int rc;

	struct batt_modify_client_req  req;
	struct batt_modify_client_rep rep;

	req.client_handle = client_handle;
	req.desired_batt_voltage = desired_batt_voltage;
	req.voltage_direction = voltage_direction;
	req.batt_cb_id = batt_cb_id;
	req.cb_data = cb_data;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_MODIFY_CLIENT_PROC,
			msm_batt_modify_client_arg_func, &req,
			msm_batt_modify_client_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: ERROR. failed to modify  Vbatt client\n",
		       __func__);
		return rc;
	}

	if (rep.result != BATTERY_MODIFICATION_SUCCESSFUL) {
		pr_err("%s: ERROR. modify client failed. result = %u\n",
		       __func__, rep.result);
		return -EIO;
	}

	return 0;
}

#if 0//removed by zhang.yu_1 to suspend @120116
void msm_batt_early_suspend(struct early_suspend *h)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL, BATTERY_LOW);

		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client. rc=%d\n",
			       __func__, rc);
			return;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}

	pr_debug("%s: exit\n", __func__);
}
#endif

#if 0//removed by zhang.yu_1 to resume @120116
void msm_batt_late_resume(struct early_suspend *h)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client FAIL rc=%d\n",
			       __func__, rc);
			return;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}

	msm_batt_update_psy_status();
	pr_debug("%s: exit\n", __func__);

}
#endif
#endif

struct msm_batt_vbatt_filter_req {
	u32 batt_handle;
	u32 enable_filter;
	u32 vbatt_filter;
};

struct msm_batt_vbatt_filter_rep {
	u32 result;
};

static int msm_batt_filter_arg_func(struct msm_rpc_client *batt_client,

		void *buf, void *data)
{
	struct msm_batt_vbatt_filter_req *vbatt_filter_req =
		(struct msm_batt_vbatt_filter_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(vbatt_filter_req->batt_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->enable_filter);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->vbatt_filter);
	size += sizeof(u32);
	return size;
}

static int msm_batt_filter_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{

	struct msm_batt_vbatt_filter_rep *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_vbatt_filter_rep *)data;
	buf_ptr = (struct msm_batt_vbatt_filter_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);
	return 0;
}

static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct  msm_batt_vbatt_filter_req  vbatt_filter_req;
	struct  msm_batt_vbatt_filter_rep  vbatt_filter_rep;

	vbatt_filter_req.batt_handle = msm_batt_info.batt_handle;
	vbatt_filter_req.enable_filter = 1;
	vbatt_filter_req.vbatt_filter = vbatt_filter;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_ENABLE_DISABLE_FILTER_PROC,
			msm_batt_filter_arg_func, &vbatt_filter_req,
			msm_batt_filter_ret_func, &vbatt_filter_rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: enable vbatt filter. rc=%d\n",
		       __func__, rc);
		return rc;
	}

	if (vbatt_filter_rep.result != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: FAIL: enable vbatt filter: result=%d\n",
		       __func__, vbatt_filter_rep.result);
		return -EIO;
	}

	pr_debug("%s: enable vbatt filter: OK\n", __func__);
	return rc;
}

struct batt_client_registration_req {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_req_4_1 {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	u32 batt_handle;
};

struct batt_client_registration_rep_4_1 {
	u32 batt_handle;
	u32 more_data;
	u32 err;
};

static int msm_batt_register_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_req *batt_reg_req =
		(struct batt_client_registration_req *)data;

	u32 *req = (u32 *)buf;
	int size = 0;


	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	} else {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->more_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	}

}

static int msm_batt_register_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_rep *data_ptr, *buf_ptr;
	struct batt_client_registration_rep_4_1 *data_ptr_4_1, *buf_ptr_4_1;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		data_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)data;
		buf_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)buf;

		data_ptr_4_1->batt_handle
			= be32_to_cpu(buf_ptr_4_1->batt_handle);
		data_ptr_4_1->more_data
			= be32_to_cpu(buf_ptr_4_1->more_data);
		data_ptr_4_1->err = be32_to_cpu(buf_ptr_4_1->err);
		return 0;
	} else {
		data_ptr = (struct batt_client_registration_rep *)data;
		buf_ptr = (struct batt_client_registration_rep *)buf;

		data_ptr->batt_handle = be32_to_cpu(buf_ptr->batt_handle);
		return 0;
	}
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req batt_reg_req;
	struct batt_client_registration_req_4_1 batt_reg_req_4_1;
	struct batt_client_registration_rep batt_reg_rep;
	struct batt_client_registration_rep_4_1 batt_reg_rep_4_1;
	void *request;
	void *reply;
	int rc;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		batt_reg_req_4_1.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req_4_1.voltage_direction = voltage_direction;
		batt_reg_req_4_1.batt_cb_id = batt_cb_id;
		batt_reg_req_4_1.cb_data = cb_data;
		batt_reg_req_4_1.batt_error = 1;
		request = &batt_reg_req_4_1;
	} else {
		batt_reg_req.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req.voltage_direction = voltage_direction;
		batt_reg_req.batt_cb_id = batt_cb_id;
		batt_reg_req.cb_data = cb_data;
		batt_reg_req.more_data = 1;
		batt_reg_req.batt_error = 0;
		request = &batt_reg_req;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1)
		reply = &batt_reg_rep_4_1;
	else
		reply = &batt_reg_rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_REGISTER_PROC,
			msm_batt_register_arg_func, request,
			msm_batt_register_ret_func, reply,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt register. rc=%d\n", __func__, rc);
		return rc;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		if (batt_reg_rep_4_1.more_data != 0
			&& batt_reg_rep_4_1.err
				!= BATTERY_REGISTRATION_SUCCESSFUL) {
			pr_err("%s: vBatt Registration Failed proc_num=%d\n"
					, __func__, BATTERY_REGISTER_PROC);
			return -EIO;
		}
		msm_batt_info.batt_handle = batt_reg_rep_4_1.batt_handle;
	} else
		msm_batt_info.batt_handle = batt_reg_rep.batt_handle;

	return 0;
}

struct batt_client_deregister_req {
	u32 batt_handle;
};

struct batt_client_deregister_rep {
	u32 batt_error;
};

static int msm_batt_deregister_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_req *deregister_req =
		(struct  batt_client_deregister_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(deregister_req->batt_handle);
	size += sizeof(u32);

	return size;
}

static int msm_batt_deregister_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_client_deregister_rep *)data;
	buf_ptr = (struct batt_client_deregister_rep *)buf;

	data_ptr->batt_error = be32_to_cpu(buf_ptr->batt_error);

	return 0;
}

static int msm_batt_deregister(u32 batt_handle)
{
	int rc;
	struct batt_client_deregister_req req;
	struct batt_client_deregister_rep rep;

	req.batt_handle = batt_handle;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_DEREGISTER_CLIENT_PROC,
			msm_batt_deregister_arg_func, &req,
			msm_batt_deregister_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt deregister. rc=%d\n", __func__, rc);
		return rc;
	}

	if (rep.batt_error != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: vbatt deregistration FAIL. error=%d, handle=%d\n",
		       __func__, rep.batt_error, batt_handle);
		return -EIO;
	}

	return 0;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

static int msm_batt_cleanup(void)
{
	int rc = 0;

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			pr_err("%s: FAIL: msm_batt_deregister. rc=%d\n",
			       __func__, rc);
	}

	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.batt_client)
		msm_rpc_unregister_client(msm_batt_info.batt_client);
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.chg_ep) {
		rc = msm_rpc_close(msm_batt_info.chg_ep);
		if (rc < 0) {
			pr_err("%s: FAIL. msm_rpc_close(chg_ep). rc=%d\n",
			       __func__, rc);
		}
	}

#if 0//removed by zhang.yu_1 to resume @120116
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
#endif

#endif
	return rc;
}

static u32 msm_batt_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_batt_info.voltage_min_design;
	u32 high_voltage = msm_batt_info.voltage_max_design;

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

#ifndef CONFIG_BATTERY_MSM_FAKE
int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VER_1_1,
			  ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		pr_err("%s: FAIL: msm_rpc_write. proc=0x%08x, rc=%d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			pr_err("%s: LENGTH ERR: msm_rpc_read. rc=%d (<%d)\n",
			       __func__, rc, RPC_REQ_REPLY_COMMON_HEADER_SIZE);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {
			pr_err("%s: TYPE ERR: type=%d (!=%d)\n",
			       __func__, reply->type, RPC_TYPE_REQ);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {
			pr_err("%s: XID ERR: xid=%d (!=%d)\n", __func__,
			       reply->xid, req_chg_api_ver.hdr.xid);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		pr_debug("%s: num_of_chg_api_versions = %u. "
			"The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}

static int msm_batt_cb_func(struct msm_rpc_client *client,
			    void *buffer, int in_size)
{
	int rc = 0;
	struct rpc_request_hdr *req;
	u32 procedure;
	u32 accept_status;

	req = (struct rpc_request_hdr *)buffer;
	procedure = be32_to_cpu(req->procedure);

	switch (procedure) {
	case BATTERY_CB_TYPE_PROC:
		accept_status = RPC_ACCEPTSTAT_SUCCESS;
		break;

	default:
		accept_status = RPC_ACCEPTSTAT_PROC_UNAVAIL;
		pr_err("%s: ERROR. procedure (%d) not supported\n",
		       __func__, procedure);
		break;
	}

	msm_rpc_start_accepted_reply(msm_batt_info.batt_client,
			be32_to_cpu(req->xid), accept_status);

	rc = msm_rpc_send_accepted_reply(msm_batt_info.batt_client, 0);
	if (rc)
		pr_err("%s: FAIL: sending reply. rc=%d\n", __func__, rc);

	if (accept_status == RPC_ACCEPTSTAT_SUCCESS)
		msm_batt_update_psy_status();

	return rc;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

#ifdef CONFIG_ZTE_PLATFORM//modified by zhang.yu_1 for  @120116
static int msm_batt_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL, BATTERY_LOW);

		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client. rc=%d\n",
			       __func__, rc);
			return rc;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return 0;
	}

	pr_debug("%s: exit\n", __func__);
	
    	return 0;
}

static int msm_batt_resume(struct platform_device *pdev)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client FAIL rc=%d\n",
			       __func__, rc);
			return rc;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return 0;
	}

    	msm_batt_update_psy_status();

	pr_debug("%s: exit\n", __func__);
    
    	return 0;
}
#endif

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (pdata->avail_chg_sources & AC_CHG) {
#else
	{
#endif
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.batt_technology = pdata->batt_technology;
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;

	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

	if (!msm_batt_info.calculate_capacity)
		msm_batt_info.calculate_capacity = msm_batt_capacity;

	rc = power_supply_register(&pdev->dev, &msm_psy_batt);
	if (rc < 0) {
		dev_err(&pdev->dev, "%s: power_supply_register failed"
			" rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.msm_psy_batt = &msm_psy_batt;

#ifndef CONFIG_BATTERY_MSM_FAKE
	rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc = %d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	rc =  msm_batt_enable_filter(VBATT_FILTER);

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_enable_filter failed rc = %d\n",
			__func__, rc);
		msm_batt_cleanup();
		return rc;
	}

#if 0//removed by zhang.yu_1 to suspend @120116
#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 38;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
#endif

	msm_batt_update_psy_status();

#else
	power_supply_changed(&msm_psy_ac);
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
        #ifdef CONFIG_ZTE_PLATFORM//merged from 7x30 by zhang.yu_1 for  @110927
        .suspend = msm_batt_suspend,
        .resume = msm_batt_resume,
        #endif   
};

static int __devinit msm_batt_init_rpc(void)
{
	int rc;

#ifdef CONFIG_BATTERY_MSM_FAKE
	pr_info("Faking MSM battery\n");
#else

	msm_batt_info.chg_ep =
		msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VER_4_1, 0);
	msm_batt_info.chg_api_version =  CHG_RPC_VER_4_1;
	if (msm_batt_info.chg_ep == NULL) {
		pr_err("%s: rpc connect CHG_RPC_PROG = NULL\n", __func__);
		return -ENODEV;
	}

	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_3_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_3_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_3, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_3;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_2_2, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_2_2;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		rc = PTR_ERR(msm_batt_info.chg_ep);
		pr_err("%s: FAIL: rpc connect for CHG_RPC_PROG. rc=%d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}

	/* Get the real 1.x version */
	if (msm_batt_info.chg_api_version == CHG_RPC_VER_1_1)
		msm_batt_info.chg_api_version =
			msm_batt_get_charger_api_version();

	/* Fall back to 1.1 for default */
	if (msm_batt_info.chg_api_version < 0)
		msm_batt_info.chg_api_version = CHG_RPC_VER_1_1;
	msm_batt_info.batt_api_version =  BATTERY_RPC_VER_4_1;

	msm_batt_info.batt_client =
		msm_rpc_register_client("battery", BATTERY_RPC_PROG,
					BATTERY_RPC_VER_4_1,
					1, msm_batt_cb_func);

	if (msm_batt_info.batt_client == NULL) {
		pr_err("%s: FAIL: rpc_register_client. batt_client=NULL\n",
		       __func__);
		return -ENODEV;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_1_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_2_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_2_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_5_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_5_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		rc = PTR_ERR(msm_batt_info.batt_client);
		pr_err("%s: ERROR: rpc_register_client: rc = %d\n ",
		       __func__, rc);
		msm_batt_info.batt_client = NULL;
		return rc;
	}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0)
		pr_err("%s: FAIL: platform_driver_register. rc = %d\n",
		       __func__, rc);

	return rc;
}

static int __init msm_batt_init(void)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	rc = msm_batt_init_rpc();

	if (rc < 0) {
		pr_err("%s: FAIL: msm_batt_init_rpc.  rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING	//merged by zhang.yu_1 from gb7x27 @111221
	rc |= driver_create_file(&msm_batt_driver.driver, &driver_attr_usb_chg_enable);
	if (rc < 0)
		pr_debug("%s: driver_create_file failed for usb_chg_enable " "batt driver. rc = %d\n", __func__, rc);
#endif	

#if 0//def FEATURE_ZTE_APP_ENABLE_USB_CHARGING//merged by zhang.yu_1 from gb7x27 @111221
	msm_batt_info.chg_ep =
	    msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VERS, 0);

	if (msm_batt_info.chg_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.chg_ep)) {
		int rc = PTR_ERR(msm_batt_info.chg_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed for CHG_RPC_PROG. rc = %d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}
#endif

	pr_info("%s: Charger/Battery = 0x%08x/0x%08x (RPC version)\n",
		__func__, msm_batt_info.chg_api_version,
		msm_batt_info.batt_api_version);

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");

#else	//zte'msm_battery.c for 8903 share_memory

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
#define DEBUG  0

#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>

#include <mach/zte_memlog.h>	/* zte_ccb add */
#include <linux/io.h>

#define BATTERY_LOW		3200
#define BATTERY_HIGH		4300

#define FEATURE_ZTE_APP_ENABLE_USB_CHARGING

#if DEBUG
#define DBG_LIMIT(x...) do {if (printk_ratelimit()) pr_debug(x); } while (0)
#else
#define DBG_LIMIT(x...) do {} while (0)
#endif

#define ABSM(a,b)	  ((a)>(b)?((a)-(b)):((b)-(a)))	/* zte-ccb-20120208-2 */

#ifdef MSM_BATTERY_DEBUG
#define DEBUG_MSM_BATTERY(fmt, args...)\
    do\
    {\
        printk("%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__);\
        printk(fmt "\n", ## args);\
    }\
    while (0)
#else
#define DEBUG_MSM_BATTERY(fmt, args...) do{}while(0)
#endif

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
	/* The charger is good      */
	CHARGER_STATUS_GOOD,
	/* The charger is bad       */
	CHARGER_STATUS_BAD,
	/* The charger is weak      */
	CHARGER_STATUS_WEAK,
	/* Invalid charger status.  */
	CHARGER_STATUS_INVALID
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
	/* The charger is removed                 */
	CHARGER_TYPE_NONE,
	/* The charger is a regular wall charger   */
	CHARGER_TYPE_WALL,
	/* The charger is a PC USB                 */
	CHARGER_TYPE_USB_PC,
	/* The charger is a wall USB charger       */
	CHARGER_TYPE_USB_WALL,
	/* The charger is a USB carkit             */
	CHARGER_TYPE_USB_CARKIT,
	/* Invalid charger hardware status.        */
	CHARGER_TYPE_INVALID
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
	/* The battery is good        */
	BATTERY_STATUS_GOOD,
	/* The battery is cold/hot    */
	BATTERY_STATUS_BAD_TEMP,
	/* The battery is bad         */
	BATTERY_STATUS_BAD,
	/* The battery is removed     */
	BATTERY_STATUS_REMOVED,		/* on v2.2 only */
	BATTERY_STATUS_INVALID_v1 = BATTERY_STATUS_REMOVED,
	/* Invalid battery status.    */
	BATTERY_STATUS_INVALID
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
	/* The battery voltage is dead/very low (less than 3.2V) */
	BATTERY_LEVEL_DEAD,
	/* The battery voltage is weak/low (between 3.2V and 3.4V) */
	BATTERY_LEVEL_WEAK,
	/* The battery voltage is good/normal(between 3.4V and 4.2V) */
	BATTERY_LEVEL_GOOD,
	/* The battery voltage is up to full (close to 4.2V) */
	BATTERY_LEVEL_FULL,
	/* Invalid battery voltage level. */
	BATTERY_LEVEL_INVALID
};

struct msm_battery_info {
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 chg_api_version;
	u32 batt_technology;
	u32 batt_api_version;

	u32 avail_chg_sources;
	u32 current_chg_source;

	u32 batt_status;
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
        	u32 battery_capacity;
	
    	u8 charger_status;
    	u8 charger_type;
    	u8 charging;
    	u8 chg_fulled;

    	u8 battery_status;
    	u8 battery_level;
    	u16 battery_voltage;
    	s16 battery_temp;

	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

        spinlock_t lock;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;
	struct power_supply *current_ps;

        	struct workqueue_struct *msm_batt_wq;
	struct msm_rpc_client *batt_client;
	struct msm_rpc_endpoint *chg_ep;

	wait_queue_head_t wait_q;

	u32 vbatt_modify_reply_avail;

	struct early_suspend early_suspend;
	
        	atomic_t handle_event;
        	u32 any_event;
			
};

static struct smem_batt_chg_t rep_batt_chg;

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work);

static DECLARE_WORK(msm_batt_cb_work, msm_batt_wait_for_batt_chg_event);

static struct msm_battery_info msm_batt_info = {
	.batt_handle = -1,
	.charger_status = CHARGER_STATUS_BAD,
	.charger_type = CHARGER_TYPE_INVALID,
	.battery_status = BATTERY_STATUS_GOOD,
	.battery_level = BATTERY_LEVEL_FULL,
	.battery_voltage = BATTERY_HIGH,
	.battery_capacity = 100,
	.batt_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.batt_health = POWER_SUPPLY_HEALTH_GOOD,
	.batt_valid  = 1,
	.battery_temp = 23,
	.vbatt_modify_reply_avail = 0,
	.any_event = 0,
};

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

/*	zte-ccb-20120208-2
static char *charger_st[] = {
    "good", "bad", "weak", "invalid"
};*/

static char *charger_tp[] = {
    "no  chg", "   wall", "usb  pc", "usb wall", "usbcarkit",
    "invalid"
};

static char *battery_st[] = {
    "good ", "bad temp", "bad", "removed", "invalid"
};

static char *batt_st[] ={
    "unkown ", "charging", "discharging", "not charing", "full"	
};

static char *battery_lvl[] = {
    "dead", "weak", "good", "full", "invalid"
};

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
		}
		if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
         #ifdef CONFIG_ZTE_PLATFORM		
         POWER_SUPPLY_PROP_TEMP
         #endif
};

static int shutdown_percentage_zero_enable = 1;
module_param_named(zero_shutdown_enalbe, shutdown_percentage_zero_enable, int, S_IRUGO | S_IWUSR | S_IWGRP);


#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE

#include <linux/wakelock.h>
static struct wake_lock charger_wake_lock;
static int wl_initialized = 0;//The rpc occur anytime ,so ,we must make sure that the batt driver already initialized

void msm_batt_force_update(void)
{
	// empty to be filled by batt comrades lator
	if (wl_initialized)
	{
      		wake_lock_timeout(&charger_wake_lock, 3 * HZ);
	}
}
#endif


static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = msm_batt_info.battery_capacity;
		break;
#ifdef CONFIG_ZTE_PLATFORM	
	case  POWER_SUPPLY_PROP_TEMP:
		val->intval = msm_batt_info.battery_temp * 10;
		break;
#endif	
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
};

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING

#define USB_CHG_DISABLE 0
#define USB_CHG_ENABLE 1
static int usb_charger_enable = USB_CHG_ENABLE;	
static int usb_charger_enable_pre = USB_CHG_ENABLE;	

//#include <mach/msm_rpcrouter.h>
//#define CHG_RPC_PROG		0x3000001a
//#define CHG_RPC_VERS		0x00010003

//LHX_PM_20110601_01 change RPC prog and version for 8X55
#define CHG_RPC_PROG			0x3000001A
#define CHG_RPC_VERS			0x00040001 /* 0x00040001 */

#define BATTERY_ENABLE_DISABLE_USB_CHG_PROC 		6

module_param(usb_charger_enable, int, S_IRUGO | S_IWUSR | S_IWGRP);

/*--------------------------------------------------------------
msm_batt_handle_control_usb_charging() is added according msm_chg_usb_charger_connected() in rpc_hsusb.c
and the rpc is used to stop or resume usb charging
---------------------------------------------------------------*/
static int msm_batt_handle_control_usb_charging(u32 usb_enable_disable)
{
	int rc;

	struct batt_modify_client_req {
		struct rpc_request_hdr hdr;
		u32 usb_chg_enable;
	} req;

	pr_debug("%s:  msm_rpc_write usb switch enable/disable = %d start.\n",
				__func__,usb_enable_disable);

	req.usb_chg_enable = cpu_to_be32(usb_enable_disable);
    	rc=msm_rpc_call(msm_batt_info.chg_ep, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, &req,
                    sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, rc);
		return rc;
	}
		pr_debug("%s:  msm_rpc_write usb switch enable/disable end rc = %d.\n",
				__func__,rc);

	return 0;
}

#endif

static void msm_batt_update_psy_status(void)
{
	static smem_global *plog=(smem_global *)0;
    	struct smem_batt_chg_t *batt_chg_ptr;

	static unsigned int low_power_cnt=0;		//chenchongbao.2011.5.25

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));
	
	//batt_chg_ptr = ioremap(MSM_BAT_RAM_PHYS, MSM_BAT_RAM_SIZE);
	if(!plog)
	{
		plog = ioremap(SMEM_LOG_GLOBAL_BASE,  (63*PAGE_SIZE));//YINTIANCI_BAT_20101101
	}
	batt_chg_ptr=(struct smem_batt_chg_t *)(&(plog->batchginfo));

	if (batt_chg_ptr == NULL)
    	{
        		printk("%s: share memery read error!\n", __func__);
        		return;
    	}

	rep_batt_chg = *batt_chg_ptr;	/*get the battery information form SMEM*/


//chenchongbao.2011.5.25 : 用于处理连接USB 时手机大电流造成的掉电关机反复重启问题!

	if(  ( (rep_batt_chg.charger_type == CHARGER_TYPE_USB_PC) ||
		(rep_batt_chg.charger_type == CHARGER_TYPE_USB_WALL) )
		&& (rep_batt_chg.battery_capacity == 0) && (rep_batt_chg.battery_voltage < 3400) )
	{
		low_power_cnt ++;
		if(low_power_cnt > 10)	//10*2=20s
		{
			rep_batt_chg.charger_type = CHARGER_TYPE_NONE;	//capacity=0 & no charger , system will power down!
		}
	}
	else
	{
		low_power_cnt = 0;
	}

//chenchongbao.2011.5.25 : end


    if (msm_batt_info.charger_status == rep_batt_chg.charger_status &&
        msm_batt_info.charger_type == rep_batt_chg.charger_type &&
        msm_batt_info.battery_status == rep_batt_chg.battery_status &&
        msm_batt_info.battery_level == rep_batt_chg.battery_level &&
	/*if voltage not change,or changes within the voltage's range, return  */
	((msm_batt_info.battery_voltage == rep_batt_chg.battery_voltage) ||
		((msm_batt_info.battery_voltage != rep_batt_chg.battery_voltage) &&
			((msm_batt_info.voltage_min_design <= rep_batt_chg.battery_voltage) &&
			(msm_batt_info.voltage_max_design >= rep_batt_chg.battery_voltage)))) &&
        msm_batt_info.battery_capacity == rep_batt_chg.battery_capacity && 
        msm_batt_info.battery_temp == rep_batt_chg.battery_temp && 
#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
	(((rep_batt_chg.charger_type == CHARGER_TYPE_USB_WALL || 
	rep_batt_chg.charger_type == CHARGER_TYPE_USB_PC)&&	   
       usb_charger_enable_pre == usb_charger_enable ) ||
       (rep_batt_chg.charger_type != CHARGER_TYPE_USB_WALL && 
	rep_batt_chg.charger_type != CHARGER_TYPE_USB_PC))&&
#endif        
        msm_batt_info.chg_fulled == rep_batt_chg.chg_fulled &&  
        msm_batt_info.charging == rep_batt_chg.charging)
    {
        DEBUG_MSM_BATTERY(KERN_NOTICE "%s() : Got unnecessary event from Modem "
               "PMIC VBATT driver. Nothing changed in Battery or " "charger status\n", __func__);

        return;
    }

	pr_info("[ZHY@msm-battery]:%s,batt_st=%s,%s,level=%s,%dmV,%dC,%d%% ,full=%d,%d,%dmV,%dC,%d%%,%d.\n",
		 charger_tp[rep_batt_chg.charger_type], battery_st[rep_batt_chg.battery_status],batt_st[msm_batt_info.batt_status],
		 battery_lvl[rep_batt_chg.battery_level], rep_batt_chg.battery_voltage, rep_batt_chg.battery_temp, rep_batt_chg.battery_capacity,
                   rep_batt_chg.chg_fulled,shutdown_percentage_zero_enable,
                   rep_batt_chg.curr_voltage,rep_batt_chg.curr_temp,rep_batt_chg.curr_capacity,rep_batt_chg.temp_data);

    if (rep_batt_chg.battery_status != BATTERY_STATUS_INVALID)
    {
        msm_batt_info.batt_valid = 1;

        if (rep_batt_chg.battery_voltage > msm_batt_info.voltage_max_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if (rep_batt_chg.battery_voltage < msm_batt_info.voltage_min_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD)
        {
            printk("battery status bad\n");
            msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD_TEMP)
        {
            printk("battery status bad temp\n");
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;

            if (rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID)
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
            else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_GOOD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_WEAK)
                 && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

            if (rep_batt_chg.chg_fulled)
            {
                msm_batt_info.battery_capacity = 100;
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
            }
            else
	#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
	{
	         if ((rep_batt_chg.charger_type == CHARGER_TYPE_USB_WALL || 
			 	rep_batt_chg.charger_type == CHARGER_TYPE_USB_PC))
		{
				if(USB_CHG_DISABLE == usb_charger_enable)//if disabled usb charging, show  discharging
				{
			                	msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				}
				else
				{
			                	msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
				}
				
				#if 1		//LHX_PM_20110602_01 del charging to disable USB charger for 8903 charge.
				if((usb_charger_enable_pre != usb_charger_enable)&&
					((usb_charger_enable == USB_CHG_DISABLE)
					||(usb_charger_enable == USB_CHG_ENABLE)))
				#else	// need for 7540 charge
				if((usb_charger_enable_pre != usb_charger_enable)&&
					((usb_charger_enable == USB_CHG_DISABLE && rep_batt_chg.charging)
					||(usb_charger_enable == USB_CHG_ENABLE && !rep_batt_chg.charging)))
				
				#endif
				{
					printk(KERN_INFO "Before RPC charging = %d, usb_charger_enable_pre = %d,usb_charger_enable = %d \n",
						rep_batt_chg.charging,usb_charger_enable_pre,usb_charger_enable);
					usb_charger_enable_pre = usb_charger_enable;
					msm_batt_handle_control_usb_charging(usb_charger_enable);
				}
		}
		else if(rep_batt_chg.charger_type == CHARGER_TYPE_WALL)	//zte_ccb
		{
			if(rep_batt_chg.charging)	//zte_ccb
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
			else
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
		else
		{
			msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	}
	#else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
	#endif
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID) 
                  && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
        }

        //msm_batt_info.battery_capacity = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
    }
    else
    {
        printk("battery status invalid id\n");
        msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
        msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;
        msm_batt_info.batt_valid = 0;        
    }

    if (msm_batt_info.charger_type != rep_batt_chg.charger_type)/*charger's type changes,insert or pull out*/
    {
        msm_batt_info.charger_type = rep_batt_chg.charger_type;

        if (msm_batt_info.charger_type == CHARGER_TYPE_WALL)/*AC insert*/
        {
            msm_batt_info.current_chg_source &= ~USB_CHG;
            msm_batt_info.current_chg_source |= AC_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = WALL\n", __func__);

            power_supply_changed(&msm_psy_ac);

        }
        else if (msm_batt_info.charger_type ==
                 CHARGER_TYPE_USB_WALL || msm_batt_info.charger_type == CHARGER_TYPE_USB_PC)/*USB insert*/
        {
            msm_batt_info.current_chg_source &= ~AC_CHG;
            msm_batt_info.current_chg_source |= USB_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            power_supply_changed(&msm_psy_usb);
            DEBUG_MSM_BATTERY();
        }
        else	/*pull out charger */
        {
            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;

            if (msm_batt_info.current_chg_source & AC_CHG)	/*the previous  charger is AC*/
            {
                msm_batt_info.current_chg_source &= ~AC_CHG;

                DEBUG_MSM_BATTERY(KERN_INFO "%s() : AC WALL charger" " removed\n", __func__);

                power_supply_changed(&msm_psy_ac);

            }
            else if (msm_batt_info.current_chg_source & USB_CHG)/*the previous  charger is USB*/
            {
                msm_batt_info.current_chg_source &= ~USB_CHG;
                DEBUG_MSM_BATTERY(KERN_INFO "%s() : USB charger removed\n", __func__);

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
			usb_charger_enable_pre = USB_CHG_ENABLE;
			printk(KERN_INFO "USB remove: usb_charger_enable_pre set2 %d \n",usb_charger_enable_pre);
#endif        

                power_supply_changed(&msm_psy_usb);
            }
            else	/*? unnecessary?*/
                power_supply_changed(&msm_psy_batt);
        }
    }
    else
    {
               power_supply_changed(&msm_psy_batt);
    }
	
    msm_batt_info.charger_status = rep_batt_chg.charger_status;
    msm_batt_info.charger_type = rep_batt_chg.charger_type;
    msm_batt_info.battery_status = rep_batt_chg.battery_status;
    msm_batt_info.battery_level = rep_batt_chg.battery_level;
    msm_batt_info.battery_voltage = rep_batt_chg.battery_voltage;
    msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;
    msm_batt_info.battery_temp = rep_batt_chg.battery_temp;
    msm_batt_info.chg_fulled = rep_batt_chg.chg_fulled;
    msm_batt_info.charging = rep_batt_chg.charging;

}


static void msm_batt_send_event(void)
{
	unsigned long flags;
	
	spin_lock_irqsave(&msm_batt_info.lock, flags);

	msm_batt_info.any_event = 1;

	atomic_set(&msm_batt_info.handle_event, 1);
	wake_up(&msm_batt_info.wait_q);
	
	spin_unlock_irqrestore(&msm_batt_info.lock, flags);
}

static void msm_batt_handle_event(void)
{
	unsigned long flags;

	if (!atomic_read(&msm_batt_info.handle_event))
    	{
        		printk(KERN_ERR "%s(): batt call back thread while in "
               		"msm_rpc_read got signal. Signal is not from "
               		"early suspend or  from late resume or from Clean up " "thread.\n", __func__);
        		return;
    	}

	spin_lock_irqsave(&msm_batt_info.lock, flags);
	msm_batt_info.any_event = 0;
	spin_unlock_irqrestore(&msm_batt_info.lock, flags);

	msm_batt_update_psy_status();

	atomic_set(&msm_batt_info.handle_event, 0);

}

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work)
{
    int ret;

    msm_batt_update_psy_status();
    
    while (1)
    {
        ret = wait_event_interruptible_timeout(msm_batt_info.wait_q, msm_batt_info.any_event , 2*HZ);

        if (ret != 0)
        {
            msm_batt_handle_event();

        }
        else
        {
            msm_batt_update_psy_status();	//update info every 2s.
        }
    }

}

static void msm_batt_cleanup(void)
{
	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);
	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);
}

static u32 msm_batt_capacity(u32 current_voltage)
{
	return 55;
}

static int msm_batt_suspend(struct platform_device *pdev, pm_message_t state)
{
    	return 0;
}

static int msm_batt_resume(struct platform_device *pdev)
{
	msm_batt_send_event();    
    	return 0;
}

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

	if (pdata->avail_chg_sources & AC_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.batt_technology = pdata->batt_technology;
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;

	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

	if (!msm_batt_info.calculate_capacity)
		msm_batt_info.calculate_capacity = msm_batt_capacity;

	rc = power_supply_register(&pdev->dev, &msm_psy_batt);
	if (rc < 0) {
		dev_err(&pdev->dev, "%s: power_supply_register failed"
			" rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.msm_psy_batt = &msm_psy_batt;

	spin_lock_init(&msm_batt_info.lock);

    	init_waitqueue_head(&msm_batt_info.wait_q);

	msm_batt_info.msm_batt_wq = create_singlethread_workqueue("msm_battery");

	if (!msm_batt_info.msm_batt_wq)
    	{
		printk(KERN_ERR "%s: create workque failed \n", __func__);
		return -ENOMEM;
    	}

	atomic_set(&msm_batt_info.handle_event, 0);
    	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_cb_work);
	pr_info("msm_battery task queue start!\n");

	return 0;
	
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	if (msm_batt_info.msm_batt_wq)
	{
        		destroy_workqueue(msm_batt_info.msm_batt_wq);
	}
	
	msm_batt_cleanup();
	
	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
        .suspend = msm_batt_suspend,
        .resume = msm_batt_resume,
};


static int __init msm_batt_init(void)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0){
		pr_err("%s: FAIL: platform_driver_register. rc = %d\n",__func__, rc);
		return rc;
	}

#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
	wake_lock_init(&charger_wake_lock, WAKE_LOCK_SUSPEND, "chg_event");
	wl_initialized = 1;
#endif

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING

	msm_batt_info.chg_ep =
	    msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VERS, 0);

	if (msm_batt_info.chg_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.chg_ep)) {
		int rc = PTR_ERR(msm_batt_info.chg_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed for CHG_RPC_PROG. rc = %d\n",__func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}
	
#endif

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");


#endif
