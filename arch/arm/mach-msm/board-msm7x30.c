/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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
========================================================================================
when          who    what, where, why                      	comment tag
=========================================================================================
*/


#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/io.h>
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <linux/msm_ssbi.h>
#include <linux/mfd/pmic8058.h>
#include <linux/leds.h>
#include <linux/mfd/marimba.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/smsc911x.h>
#include <linux/ofn_atlab.h>
#include <linux/power_supply.h>
#include <linux/i2c/isa1200.h>
#include <linux/input/kp_flip_switch.h>
#include <linux/input/cy8c_ts.h>
#include <linux/msm_adc.h>
#include <linux/dma-mapping.h>
#include <linux/regulator/consumer.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_spi.h>
#include <mach/qdsp5v2/msm_lpa.h>
#include <mach/dma.h>
#include <linux/android_pmem.h>
#include <linux/input/msm_ts.h>
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include <mach/msm_memtypes.h>
#include <linux/cyttsp.h>

#include <asm/mach/mmc.h>
#include <asm/mach/flash.h>
#include <mach/vreg.h>

#include "devices.h"
#include "timer.h"
#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android.h>
#include <mach/usbdiag.h>
#endif
#include "pm.h"
#include "pm-boot.h"
#include "spm.h"
#include "acpuclock.h"
#include <mach/dal_axi.h>
#include <mach/msm_serial_hs.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/sdio_al.h>
#include "smd_private.h"

#include "board-msm7x30-regulator.h"
#include "pm.h"

#include <linux/lis302dl.h> 
#include <linux/pmic8058-othc.h>  
#include <mach/zte_memlog.h>
#include <linux/proc_fs.h>
void msm7x30_ts_init(void); 


//#define MSM_PMEM_SF_SIZE	0x1F00000
#define MSM_PMEM_SF_SIZE	(40*1024*1024)

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_SIZE            0x780000
#else
#define MSM_FB_SIZE            0x500000
#endif
#define MSM_PMEM_ADSP_SIZE      0x0 // 0x3200000//PMEM ->5M
#define MSM_FLUID_PMEM_ADSP_SIZE	0x0 // 0x2800000
#define PMEM_KERNEL_EBI0_SIZE   0x600000
#define MSM_PMEM_AUDIO_SIZE     0x0 // 0x200000

#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((544 * 960 * 3 * 2), 4096)
#define PMIC_GPIO_INT		27
#define PMIC_VREG_WLAN_LEVEL	2900
#define PMIC_GPIO_SD_DET	36
#define PMIC_GPIO_SDC4_EN_N	17  /* PMIC GPIO Number 18 */
#define PMIC_GPIO_HDMI_5V_EN_V3 32  /* PMIC GPIO for V3 H/W */
#define PMIC_GPIO_HDMI_5V_EN_V2 39 /* PMIC GPIO for V2 H/W */

#define ADV7520_I2C_ADDR	0x39

#define FPGA_SDCC_STATUS       0x8E0001A8

#define FPGA_OPTNAV_GPIO_ADDR	0x8E000026
#define OPTNAV_I2C_SLAVE_ADDR	(0xB0 >> 1)
#define OPTNAV_IRQ		20
#define OPTNAV_CHIP_SELECT	19


/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)
#define PM8058_MPP_BASE			   PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS)
#define PM8058_MPP_PM_TO_SYS(pm_gpio)	   (pm_gpio + PM8058_MPP_BASE)

#define PMIC_GPIO_FLASH_BOOST_ENABLE	15	/* PMIC GPIO Number 16 */
#define PMIC_GPIO_HAP_ENABLE   16  /* PMIC GPIO Number 17 */

#define PMIC_GPIO_WLAN_EXT_POR  22 /* PMIC GPIO NUMBER 23 */


#define HAP_LVL_SHFT_MSM_GPIO 24

#define PMIC_GPIO_QUICKVX_CLK 37 /* PMIC GPIO 38 */

#define	PM_FLIP_MPP 5 /* PMIC MPP 06 */

#define DDR1_BANK_BASE 0X20000000
#define DDR2_BANK_BASE 0X40000000

static unsigned int phys_add = DDR2_BANK_BASE;
unsigned long ebi1_phys_offset = DDR2_BANK_BASE;
EXPORT_SYMBOL(ebi1_phys_offset);

struct pm8xxx_gpio_init_info {
	unsigned			gpio;
	struct pm_gpio			config;
};

#if defined(CONFIG_ZTE_PLATFORM)
static smem_global *global;

static int g_zte_ftm_flag_fixup;
static int g_zte_board_id_type;
int get_zte_board_id_type(void);
extern void sync_sysfs_board_id(int flag);

#if defined(CONFIG_F3_LOG)
#define SDLOG_MEM_BASE    0x8D00000
#endif

#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define MSM_RAM_CONSOLE_PHYS  0x3D00000 
#define MSM_RAM_CONSOLE_SIZE  SZ_1M
static struct resource ram_console_resource[] = {
	{
		.start	= MSM_RAM_CONSOLE_PHYS,
		.end	= MSM_RAM_CONSOLE_PHYS + MSM_RAM_CONSOLE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resource),
	.resource       = ram_console_resource,
};
#endif

static int pm8058_gpios_init(void)
{
	int rc;

	struct pm8xxx_gpio_init_info sdc4_en = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_EN_N),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_L5,
			.function       = PM_GPIO_FUNC_NORMAL,
			.inv_int_pol    = 0,
			.out_strength   = PM_GPIO_STRENGTH_LOW,
			.output_value   = 0,
		},
	};


	struct pm8xxx_gpio_init_info haptics_enable = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.pull           = PM_GPIO_PULL_NO,
			.out_strength   = PM_GPIO_STRENGTH_HIGH,
			.function       = PM_GPIO_FUNC_NORMAL,
			.inv_int_pol    = 0,
			.vin_sel        = 2,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 0,
		},
	};

	struct pm8xxx_gpio_init_info hdmi_5V_en = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_5V_EN_V3),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_VPH,
			.function       = PM_GPIO_FUNC_NORMAL,
			.out_strength   = PM_GPIO_STRENGTH_LOW,
			.output_value   = 0,
		},
	};

	struct pm8xxx_gpio_init_info flash_boost_enable = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_FLASH_BOOST_ENABLE),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
			.out_strength   = PM_GPIO_STRENGTH_HIGH,
			.function        = PM_GPIO_FUNC_2,
		},
	};

	struct pm8xxx_gpio_init_info gpio23 = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_WLAN_EXT_POR),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = 2,
			.out_strength   = PM_GPIO_STRENGTH_LOW,
			.function       = PM_GPIO_FUNC_NORMAL,
		}
	};


#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	struct pm8xxx_gpio_init_info sdcc_det = {
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SD_DET - 1),
		{
			.direction      = PM_GPIO_DIR_IN,
			.pull           = PM_GPIO_PULL_UP_1P5,
			.vin_sel        = 2,
			.function       = PM_GPIO_FUNC_NORMAL,
			.inv_int_pol    = 0,
		},
	};

	if (machine_is_msm7x30_fluid())
		sdcc_det.config.inv_int_pol = 1;

	rc = pm8xxx_gpio_config(sdcc_det.gpio, &sdcc_det.config);
	if (rc) {
		pr_err("%s PMIC_GPIO_SD_DET config failed\n", __func__);
		return rc;
	}
#endif

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa() ||
						machine_is_msm7x30_fluid())
		hdmi_5V_en.gpio = PMIC_GPIO_HDMI_5V_EN_V2;
	else
		hdmi_5V_en.gpio = PMIC_GPIO_HDMI_5V_EN_V3;

	hdmi_5V_en.gpio = PM8058_GPIO_PM_TO_SYS(hdmi_5V_en.gpio);

	rc = pm8xxx_gpio_config(hdmi_5V_en.gpio, &hdmi_5V_en.config);
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_5V_EN config failed\n", __func__);
		return rc;
	}

	/* Deassert GPIO#23 (source for Ext_POR on WLAN-Volans) */
	rc = pm8xxx_gpio_config(gpio23.gpio, &gpio23.config);
	if (rc) {
		pr_err("%s PMIC_GPIO_WLAN_EXT_POR config failed\n", __func__);
		return rc;
	}

	if (machine_is_msm7x30_fluid()) {
		/* Haptics gpio */
		rc = pm8xxx_gpio_config(haptics_enable.gpio,
						&haptics_enable.config);
		if (rc) {
			pr_err("%s: PMIC GPIO %d write failed\n", __func__,
							haptics_enable.gpio);
			return rc;
		}
		/* Flash boost gpio */
		rc = pm8xxx_gpio_config(flash_boost_enable.gpio,
						&flash_boost_enable.config);
		if (rc) {
			pr_err("%s: PMIC GPIO %d write failed\n", __func__,
						flash_boost_enable.gpio);
			return rc;
		}
		/* SCD4 gpio */
		rc = pm8xxx_gpio_config(sdc4_en.gpio, &sdc4_en.config);
		if (rc) {
			pr_err("%s PMIC_GPIO_SDC4_EN_N config failed\n",
								 __func__);
			return rc;
		}
		rc = gpio_request(sdc4_en.gpio, "sdc4_en");
		if (rc) {
			pr_err("%s PMIC_GPIO_SDC4_EN_N gpio_request failed\n",
				__func__);
			return rc;
		}
		gpio_set_value_cansleep(sdc4_en.gpio, 0);
	}


	return 0;
}

/* Regulator API support */

#ifdef CONFIG_MSM_PROC_COMM_REGULATOR
static struct platform_device msm_proccomm_regulator_dev = {
	.name = PROCCOMM_REGULATOR_DEV_NAME,
	.id   = -1,
	.dev  = {
		.platform_data = &msm7x30_proccomm_regulator_data
	}
};
#endif

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM8058_GPIO_VIN_S3,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};
	int	rc = -EINVAL;
	int	id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(id - 1),
							&pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8xxx_gpio_config(%d): rc=%d\n",
				       __func__, id, rc);
		}
		break;

	case 3:
		id = PM_PWM_LED_KPD;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	case 4:
		id = PM_PWM_LED_0;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 40;
		break;

	case 5:
		id = PM_PWM_LED_2;
		mode = PM_PWM_CONF_PWM2;
		max_mA = 40;
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	default:
		break;
	}

	if (ch >= 3 && ch <= 6) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}

	return rc;
}

static int pm8058_pwm_enable(struct pwm_device *pwm, int ch, int on)
{
	int	rc;

	switch (ch) {
	case 7:
		rc = pm8058_pwm_set_dtest(pwm, on);
		if (rc)
			pr_err("%s: pwm_set_dtest(%d): rc=%d\n",
			       __func__, on, rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static const unsigned int surf_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN), 
	KEY(0, 2, KEY_RESERVED),
	KEY(0, 3, KEY_RESERVED),
	KEY(0, 4, KEY_RESERVED),
	KEY(0, 5, KEY_RESERVED),
	KEY(0, 6, KEY_RESERVED),
	KEY(0, 7, KEY_RESERVED),

	KEY(1, 0, KEY_OK),		//first camera
	KEY(1, 1, KEY_RESERVED),
	KEY(1, 2, KEY_CAMERA),	//second camrea
	KEY(1, 3, KEY_RESERVED),
	KEY(1, 4, KEY_RESERVED),
	KEY(1, 5, KEY_RESERVED),
	KEY(1, 6, KEY_RESERVED),
	KEY(1, 7, KEY_RESERVED),
};

static struct matrix_keymap_data surf_keymap_data = {
	.keymap_size    = ARRAY_SIZE(surf_keymap),
	.keymap		= surf_keymap,
};

static struct pm8xxx_keypad_platform_data surf_keypad_data = {
	.input_name		= "arthur_keypad",
	.input_phys_device	= "arthur_keypad/input0",
	.num_rows		= 12,
	.num_cols		= 8,
	.rows_gpio_start	= PM8058_GPIO_PM_TO_SYS(8),
	.cols_gpio_start	= PM8058_GPIO_PM_TO_SYS(0),
	.debounce_ms		= 15,
	.scan_delay_ms		= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
	.keymap_data		= &surf_keymap_data,
};

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config         = pm8058_pwm_config,
	.enable         = pm8058_pwm_enable,
};


#ifdef CONFIG_LEDS_GPIO	

static struct gpio_led android_led_list[] = {
	{
		.name = "button-backlight",
		.gpio = 125,
	},
	#ifdef CONFIG_ZTE_SPOTLIGHTS_LEDS_GPIO//yintianci	
	{
		//.name = "spotlight",flashlight
		.name = "flashlight",
		.gpio = 127,
	},
	#endif
};

static struct gpio_led_platform_data android_leds_data = {
	.num_leds	= ARRAY_SIZE(android_led_list),
	.leds		= android_led_list,
};

static struct platform_device android_leds = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &android_leds_data,
	},
};

#endif


#ifdef CONFIG_LEDS_MSM_PMIC 
static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds-status",
	.id = -1,
};
#endif

static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata = {
	.irq_base		= PMIC8058_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PMIC_GPIO_INT),
	.irq_trigger_flag       = IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata = {
	.gpio_base		= PM8058_GPIO_PM_TO_SYS(0),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata = {
	.mpp_base	= PM8058_MPP_PM_TO_SYS(0),
};

static struct pm8058_platform_data pm8058_7x30_data = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.pwm_pdata		= &pm8058_pwm_data,
};

#ifdef CONFIG_MSM_SSBI
static struct msm_ssbi_platform_data msm7x30_ssbi_pm8058_pdata = {
	.rsl_id = "D:PMIC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI2,
	.slave	= {
		.name			= "pm8058-core",
		.platform_data		= &pm8058_7x30_data,
	},
};
#endif

static struct lis302dl_platform_data gsensor = {
	.gpio_intr1 =  37,
	.gpio_intr2 =  49,
	.scale      =  2 ,
	.int_active_low = 1,
};

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {

#ifdef CONFIG_SENSORS_AKM8962
	{
		I2C_BOARD_INFO("akm8962", 0x0c),
	},
#endif
#ifdef CONFIG_SENSORS_LIS302DL
	{
		.type = "lis302dl",
		.addr = 0x1c,
		.platform_data = &gsensor,
	},
#endif
#ifdef CONFIG_SENSORS_ADXL34X
	{
		.type = "adxl34x",
		.addr = 0x53,
	},
#endif

#if defined(CONFIG_MACH_ARTHUR)
#ifdef CONFIG_OV5640
    /*
     *
     * Refer to drivers/media/video/msm/ov5640.c
     * For OV5640: 5.0Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     *
     * Attention: I2C device is initialized in sensor's driver if "CONFIG_SENSOR_ADAPTER"
     *            is defined
     */
#if !defined(CONFIG_SENSOR_ADAPTER)
    {
        I2C_BOARD_INFO("ov5640", 0x78 >> 1),
    },
#else
    //Do nothing
#endif
#endif
#endif 
};

#ifdef CONFIG_MSM_CAMERA
#define	CAM_STNDBY	143
static uint32_t camera_off_vcm_gpio_table[] = {
GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_off_gpio_table[] = {

    GPIO_CFG(4,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <0> */
    GPIO_CFG(5,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <1> */
    GPIO_CFG(6,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <2> */
    GPIO_CFG(7,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <3> */
    GPIO_CFG(8,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <4> */
    GPIO_CFG(9,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <5> */
    GPIO_CFG(10, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <6> */
    GPIO_CFG(11, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_DATA <7> */
    GPIO_CFG(12, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_PCLK */
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_HSYNC */
    GPIO_CFG(14, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_VSYNC */
    GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),  /* CIF_MCLK */
};

static uint32_t camera_on_vcm_gpio_table[] = {
GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_on_gpio_table[] = {

    GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <0> */
    GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <1> */
    GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <2> */
    GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <3> */
    GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <4> */
    GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <5> */
    GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <6> */
    GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_DATA <7> */
    GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),  /* CIF_PCLK */
    GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_HSYNC */
    GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),   /* CIF_VSYNC */
    GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), /* CIF_MCLK */

};

static uint32_t camera_off_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: CAMIF_STANDBY */
	GPIO_CFG(CAM_STNDBY, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
};

static uint32_t camera_on_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: CAMIF_STANDBY */
	GPIO_CFG(CAM_STNDBY, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
static int config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_on_vcm_gpio_table,
			ARRAY_SIZE(camera_on_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_on_gpio_fluid_table,
			ARRAY_SIZE(camera_on_gpio_fluid_table));
		/* FLUID: turn on 5V booster */
		gpio_set_value(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_FLASH_BOOST_ENABLE), 1);
		/* FLUID: drive high to put secondary sensor to STANDBY */
		gpio_set_value(CAM_STNDBY, 1);
	}
	return 0;
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_off_vcm_gpio_table,
			ARRAY_SIZE(camera_off_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_off_gpio_fluid_table,
			ARRAY_SIZE(camera_off_gpio_fluid_table));
		/* FLUID: turn off 5V booster */
		gpio_set_value(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_FLASH_BOOST_ENABLE), 0);
	}
}

#if defined(CONFIG_MACH_ARTHUR)
#define MSM_CAMERA_POWER_BACKEND_DVDD_VAL       (1800)
#define MSM_CAMERA_POWER_BACKEND_IOVDD_VAL      (1800)
#define MSM_CAMERA_POWER_BACKEND_AVDD_VAL       (2800)
#define MSM_CAMERA_POWER_BACKEND_MOTOR_VAL       (2800)
int32_t msm_camera_power_backend(enum msm_camera_pwr_mode_t pwr_mode)
{
    struct vreg *vreg_cam_dvdd  = NULL;
    struct vreg *vreg_cam_avdd  = NULL;
    struct vreg *vreg_cam_iovdd = NULL;
    struct vreg *vreg_cam_motor = NULL;
    int32_t rc_cam_dvdd, rc_cam_avdd, rc_cam_iovdd,rc_cam_motor;

    CDBG("%s: entry\n", __func__);

    /*
      * Power-up Sequence according to datasheet of sensor:
      *
      * VREG_CAM_DVDD1V8  = VREG_LVSW1
      * VREG_CAM_IOVDD2V8 = VREG_GP7
      * VREG_CAM_AVDD2V6  = VREG_GP2
      * VREG_CAM_MOTOR    = VREG_CAM_AVDD2V6 = VREG_GP3
      */
    vreg_cam_dvdd  = vreg_get(0, "lvsw1");
    vreg_cam_iovdd = vreg_get(0, "gp7");
    vreg_cam_avdd  = vreg_get(0, "gp2");
    vreg_cam_motor  = vreg_get(0, "gp10");
    if ((!vreg_cam_dvdd) || (!vreg_cam_iovdd) || (!vreg_cam_avdd) || (!vreg_cam_motor))
    {
        pr_err("%s: vreg_get failed!\n", __func__);
        return -EIO;
    }

    switch (pwr_mode)
    {
        case MSM_CAMERA_PWRUP_MODE:
        {

            rc_cam_dvdd = vreg_enable(vreg_cam_dvdd);
            if (rc_cam_dvdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            rc_cam_iovdd = vreg_set_level(vreg_cam_iovdd, MSM_CAMERA_POWER_BACKEND_IOVDD_VAL);
            if (rc_cam_iovdd)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_iovdd = vreg_enable(vreg_cam_iovdd);
            if (rc_cam_iovdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(2);

            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, MSM_CAMERA_POWER_BACKEND_AVDD_VAL);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

			rc_cam_motor = vreg_set_level(vreg_cam_motor, MSM_CAMERA_POWER_BACKEND_MOTOR_VAL);
            if (rc_cam_motor)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_motor = vreg_enable(vreg_cam_motor);
            if (rc_cam_motor)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }
            mdelay(10);

            break;
        }
        case MSM_CAMERA_STANDBY_MODE:
        {
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

            break;
        }
        case MSM_CAMERA_NORMAL_MODE:
        {
            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, MSM_CAMERA_POWER_BACKEND_AVDD_VAL);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            break;
        }
        case MSM_CAMERA_PWRDWN_MODE:
        {
            /*
               * Attention: DVDD, IOVDD, or AVDD may be used by other devices
               */
            rc_cam_dvdd  = vreg_disable(vreg_cam_dvdd);
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if ((rc_cam_dvdd) || (rc_cam_avdd))
            {
                pr_err("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

			rc_cam_motor  = vreg_disable(vreg_cam_motor);
            if (rc_cam_motor)
            {
                pr_err("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            } 
            break;
        }
        default:
        {
            pr_err("%s: parameter not supported!\n", __func__);
            return -EIO;
        }
    }

    return 0;
}

/*
 *
 * Camera power setting for frontend sensor, i.e., MT9V113
 * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
 */
#define MSM_CAMERA_POWER_FRONTEND_DVDD_VAL       (1800)
#define MSM_CAMERA_POWER_FRONTEND_IOVDD_VAL      (1800)
#define MSM_CAMERA_POWER_FRONTEND_AVDD_VAL       (2600)
int32_t msm_camera_power_frontend(enum msm_camera_pwr_mode_t pwr_mode)
{
    struct vreg *vreg_cam_dvdd  = NULL;
    struct vreg *vreg_cam_avdd  = NULL;
    struct vreg *vreg_cam_iovdd = NULL;
    int32_t rc_cam_dvdd, rc_cam_avdd, rc_cam_iovdd;

    CDBG("%s: entry\n", __func__);

    /*
      * Power-up Sequence according to datasheet of sensor:
      *
      * VREG_CAM_DVDD1V8 = VREG_LVSW1
      * VREG_CAM30_2V8   = VREG_GP7
      * VREG_CAM_AVDD2V6 = VREG_GP2
      */
    vreg_cam_dvdd  = vreg_get(0, "lvsw1");
    vreg_cam_iovdd = vreg_get(0, "gp7");
    vreg_cam_avdd  = vreg_get(0, "gp2");
    if ((!vreg_cam_dvdd) || (!vreg_cam_iovdd) || (!vreg_cam_avdd))
    {
        pr_err("%s: vreg_get failed!\n", __func__);
        return -EIO;
    }

    switch (pwr_mode)
    {
        case MSM_CAMERA_PWRUP_MODE:
        {

            rc_cam_dvdd = vreg_enable(vreg_cam_dvdd);
            if (rc_cam_dvdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            rc_cam_iovdd = vreg_set_level(vreg_cam_iovdd, MSM_CAMERA_POWER_FRONTEND_IOVDD_VAL);
            if (rc_cam_iovdd)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_iovdd = vreg_enable(vreg_cam_iovdd);
            if (rc_cam_iovdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(2);

            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, MSM_CAMERA_POWER_FRONTEND_AVDD_VAL);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                pr_err("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(500);

            break;
        }
        case MSM_CAMERA_STANDBY_MODE:
        {
            pr_err("%s: MSM_CAMERA_STANDBY_MODE not supported!\n", __func__);
            return -EIO;
        }
        case MSM_CAMERA_NORMAL_MODE:
        {
            pr_err("%s: MSM_CAMERA_NORMAL_MODE not supported!\n", __func__);
            return -EIO;
        }
        case MSM_CAMERA_PWRDWN_MODE:
        {
            rc_cam_dvdd  = vreg_disable(vreg_cam_dvdd);
            rc_cam_iovdd = vreg_disable(vreg_cam_iovdd);
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if ((rc_cam_dvdd) || (rc_cam_iovdd) || (rc_cam_avdd))
            {
                pr_err("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

            break;
        }
        default:
        {
            pr_err("%s: parameter not supported!\n", __func__);
            return -EIO;
        }
    }

    return 0;
}

/*
 *
 * Camera clock switch for both frontend and backend sensors, i.e., MT9V113 and MT9P111
 *
 * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
 * For MT9P111: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
 *
 * switch_val: 0, to switch clock to frontend sensor, i.e., MT9V113, or
 *             1, to switch clock to backend sensor, i.e., MT9P111
 */
int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                  uint32_t gpio_switch,
                                  uint32_t switch_val)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(gpio_switch, data->sensor_name);
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(gpio_switch, switch_val);

        /* time delay */
        mdelay(1);
    }

    gpio_free(gpio_switch);

    return rc;
}
#endif 
struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.flags  = IORESOURCE_DMA,
	}
};

struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 147456000,
};

#if defined(CONFIG_OV5640)
static struct msm_camera_sensor_flash_src msm_flash_src_pwm = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PWM,
	._fsrc.pwm_src.freq  = 1000,
	._fsrc.pwm_src.max_load = 300,
	._fsrc.pwm_src.low_load = 30,
	._fsrc.pwm_src.high_load = 100,
	._fsrc.pwm_src.channel = 7,
};
#endif

#if defined(CONFIG_MACH_ARTHUR)
#ifdef CONFIG_OV5640
/*
 *
 * Refer to drivers/media/video/msm/ov5640.c
 * For OV5640: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
 *
 * ".vcm_pwd" are DISUSED
 */

/* CHG_CAM_20100401
 * merge 5320 for camera flash.
 */ 
static struct msm_camera_sensor_flash_data flash_ov5640 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};
 
static struct msm_camera_sensor_info msm_camera_sensor_ov5640_data = {
	.sensor_name    = "ov5640",
	.sensor_reset   = 0,
	.sensor_pwd     = 1,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_ov5640,
	.csi_if         = 0

};

static struct platform_device msm_camera_sensor_ov5640 = {
	.name      = "msm_camera_ov5640",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov5640_data,
	},
};
#endif
#endif 
#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0xA3A00000,
		.end    = 0xA3A00000 + 0x0150 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0xAD200000,
		.end	= 0xAD200000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
       .name = "msm_vpe",
       .id   = 0,
       .num_resources = ARRAY_SIZE(msm_vpe_resources),
       .resource = msm_vpe_resources,
};
#endif

#endif /*CONFIG_MSM_CAMERA*/

#ifdef CONFIG_MSM7KV2_AUDIO
static uint32_t audio_pamp_gpio_config =
   GPIO_CFG(82, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

static uint32_t audio_fluid_icodec_tx_config =
  GPIO_CFG(85, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

static int __init snddev_poweramp_gpio_init(void)
{
	int rc;

	pr_info("snddev_poweramp_gpio_init \n");
	rc = gpio_tlmm_config(audio_pamp_gpio_config, GPIO_CFG_ENABLE);

	if (rc) {
		printk(KERN_ERR
			"%s: gpio_tlmm_config(%#x)=%d\n",
			__func__, audio_pamp_gpio_config, rc);
	}
	return rc;
}

void msm_snddev_tx_route_config(void)
{
	int rc;

	pr_debug("%s()\n", __func__);

	if (machine_is_msm7x30_fluid()) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		} else
			gpio_set_value(85, 0);
	}
}

void msm_snddev_tx_route_deconfig(void)
{
	int rc;

	pr_debug("%s()\n", __func__);

	if (machine_is_msm7x30_fluid()) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_CFG_DISABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		}
	}
}

void msm_snddev_poweramp_on(void)
{
	gpio_set_value(82, 1);	/* enable spkr poweramp */
	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off(void)
{
	gpio_set_value(82, 0);	/* disable spkr poweramp */
	pr_info("%s: power off amplifier\n", __func__);
}

static struct regulator_bulk_data snddev_regs[] = {
	{ .supply = "gp4", .min_uV = 2600000, .max_uV = 2600000 },
	{ .supply = "ncp", .min_uV = 1800000, .max_uV = 1800000 },
};

static int __init snddev_hsed_voltage_init(void)
{
	int rc;

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(snddev_regs), snddev_regs);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(snddev_regs), snddev_regs);

	if (rc) {
		pr_err("%s: could not set regulator voltages: %d\n",
				__func__, rc);
		goto regs_free;
	}

	return 0;

regs_free:
	regulator_bulk_free(ARRAY_SIZE(snddev_regs), snddev_regs);
out:
	return rc;
}


void msm_snddev_hsed_voltage_on(void)
{
	int rc = regulator_bulk_enable(ARRAY_SIZE(snddev_regs), snddev_regs);

	if (rc)
		pr_err("%s: could not enable regulators: %d\n", __func__, rc);
}

void msm_snddev_hsed_voltage_off(void)
{
	int rc = regulator_bulk_disable(ARRAY_SIZE(snddev_regs), snddev_regs);

	if (rc) {
		pr_err("%s: could not disable regulators: %d\n", __func__, rc);
	}
}

static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(138, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DOUT */
	GPIO_CFG(139, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DIN  */
	GPIO_CFG(140, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_SYNC */
	GPIO_CFG(141, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_CLK  */
};

static int __init aux_pcm_gpio_init(void)
{
	int pin, rc;

	pr_info("aux_pcm_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(aux_pcm_gpio_on); pin++) {
		rc = gpio_tlmm_config(aux_pcm_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, aux_pcm_gpio_on[pin], rc);
		}
	}
	return rc;
}

static struct msm_gpio mi2s_clk_gpios[] = {
	{ GPIO_CFG(145, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_SCLK"},
	{ GPIO_CFG(144, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_WS"},
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_MCLK_A"},
};

static struct msm_gpio mi2s_rx_data_lines_gpios[] = {
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD0_A"},
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD1_A"},
	{ GPIO_CFG(123, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD2_A"},
	{ GPIO_CFG(146, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

static struct msm_gpio mi2s_tx_data_lines_gpios[] = {
	{ GPIO_CFG(146, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

int mi2s_config_clk_gpio(void)
{
	int rc = 0;

	rc = msm_gpios_request_enable(mi2s_clk_gpios,
			ARRAY_SIZE(mi2s_clk_gpios));
	if (rc) {
		pr_err("%s: enable mi2s clk gpios  failed\n",
					__func__);
		return rc;
	}
	return 0;
}

int  mi2s_unconfig_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i, rc = 0;
	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		msm_gpios_disable_free(mi2s_tx_data_lines_gpios, 1);
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask) {
			if (sd_line_mask & 0x1)
				msm_gpios_disable_free(
					mi2s_rx_data_lines_gpios + i , 1);
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
						__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_config_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i , rc = 0;
	u8 sd_config_done_mask = 0;

	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		if ((sd_line_mask & MI2S_SD_0) || (sd_line_mask & MI2S_SD_1) ||
		   (sd_line_mask & MI2S_SD_2) || !(sd_line_mask & MI2S_SD_3)) {
			pr_err("%s: can not use SD0 or SD1 or SD2 for TX"
				".only can use SD3. sd_line_mask = 0x%x\n",
				__func__ , sd_line_mask);
			rc = -EINVAL;
		} else {
			rc = msm_gpios_request_enable(mi2s_tx_data_lines_gpios,
							 1);
			if (rc)
				pr_err("%s: enable mi2s gpios for TX failed\n",
					   __func__);
		}
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask && (rc == 0)) {
			if (sd_line_mask & 0x1) {
				rc = msm_gpios_request_enable(
					mi2s_rx_data_lines_gpios + i , 1);
				if (rc) {
					pr_err("%s: enable mi2s gpios for"
					 "RX failed.  SD line = %s\n",
					 __func__,
					 (mi2s_rx_data_lines_gpios + i)->label);
					mi2s_unconfig_data_gpio(DIR_RX,
						sd_config_done_mask);
				} else
					sd_config_done_mask |= (1 << i);
			}
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
			__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_unconfig_clk_gpio(void)
{
	msm_gpios_disable_free(mi2s_clk_gpios, ARRAY_SIZE(mi2s_clk_gpios));
	return 0;
}

#endif /* CONFIG_MSM7KV2_AUDIO */

static int __init buses_init(void)
{
	if (gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_INT, 1, GPIO_CFG_INPUT,
				  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config (gpio=%d) failed\n",
		       __func__, PMIC_GPIO_INT);

		pm8058_7x30_data.keypad_pdata = &surf_keypad_data;

	return 0;
}

#define TIMPANI_RESET_GPIO	86 

struct bahama_config_register{
	u8 reg;
	u8 value;
	u8 mask;
};

enum version{
	VER_1_0,
	VER_2_0,
	VER_UNSUPPORTED = 0xFF
};

static struct regulator *vreg_marimba_1;
static struct regulator *vreg_marimba_2;
static struct regulator *vreg_bahama;

#if 1
static struct msm_gpio timpani_reset_gpio_cfg[] = {
{ GPIO_CFG(TIMPANI_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
	GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "timpani_reset"} };
#else
static struct msm_gpio timpani_reset_gpio_cfg[] = {
{ GPIO_CFG(TIMPANI_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "timpani_reset"} };
#endif

static u8 read_bahama_ver(void)
{
	int rc;
	struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };
	u8 bahama_version;

	rc = marimba_read_bit_mask(&config, 0x00,  &bahama_version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			 "%s: version read failed: %d\n",
			__func__, rc);
			return rc;
	} else {
		printk(KERN_INFO
		"%s: version read got: 0x%x\n",
		__func__, bahama_version);
	}

	switch (bahama_version) {
	case 0x08: /* varient of bahama v1 */
	case 0x10:
	case 0x00:
		return VER_1_0;
	case 0x09: /* variant of bahama v2 */
		return VER_2_0;
	default:
		return VER_UNSUPPORTED;
	}
}

static int config_timpani_reset(void)
{
	int rc;

	rc = msm_gpios_request_enable(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
	if (rc < 0) {
		printk(KERN_ERR
			"%s: msm_gpios_request_enable failed (%d)\n",
				__func__, rc);
	}

	printk(KERN_ERR "%s: [YXS]reset TIMPANI Codec\n",	__func__);
	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 0);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
	}

	return rc;
}

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	rc = config_timpani_reset();
	if (rc < 0)
		goto out;

	rc = regulator_enable(vreg_marimba_1);
	if (rc) {
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);
		goto out;
	}

	rc = regulator_enable(vreg_marimba_2);
	if (rc) {
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);
		goto disable_marimba_1;
	}

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 1);
	mdelay(1);
	if (rc < 0) {
		pr_err("%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
		msm_gpios_free(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
		goto disable_marimba_2;
	}

	return 0;

disable_marimba_2:
	regulator_disable(vreg_marimba_2);
disable_marimba_1:
	regulator_disable(vreg_marimba_1);
out:
	return rc;
};

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = regulator_disable(vreg_marimba_2);
	if (rc)
		pr_err("%s: regulator_disable failed (%d)\n", __func__, rc);

	rc = regulator_disable(vreg_marimba_1);
	if (rc)
		pr_err("%s: regulator_disable failed (%d)\n", __func__, rc);

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 0);
	if (rc < 0)
		pr_err("%s: gpio_direction_output failed (%d)\n",
				__func__, rc);

	msm_gpios_free(timpani_reset_gpio_cfg,
				   ARRAY_SIZE(timpani_reset_gpio_cfg));
};

static unsigned int msm_bahama_core_config(int type)
{
	int rc = 0;

	if (type == BAHAMA_ID) {

		int i;
		struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };

		const struct bahama_config_register v20_init[] = {
			/* reg, value, mask */
			{ 0xF4, 0x84, 0xFF }, /* AREG */
			{ 0xF0, 0x04, 0xFF } /* DREG */
		};

		if (read_bahama_ver() == VER_2_0) {
			for (i = 0; i < ARRAY_SIZE(v20_init); i++) {
				u8 value = v20_init[i].value;
				rc = marimba_write_bit_mask(&config,
					v20_init[i].reg,
					&value,
					sizeof(v20_init[i].value),
					v20_init[i].mask);
				if (rc < 0) {
					printk(KERN_ERR
						"%s: reg %d write failed: %d\n",
						__func__, v20_init[i].reg, rc);
					return rc;
				}
				printk(KERN_INFO "%s: reg 0x%02x value 0x%02x"
					" mask 0x%02x\n",
					__func__, v20_init[i].reg,
					v20_init[i].value, v20_init[i].mask);
			}
		}
	}
	printk(KERN_INFO "core type: %d\n", type);

	return rc;
}

static unsigned int msm_bahama_setup_power(void)
{
	int rc = regulator_enable(vreg_bahama);

	if (rc)
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);

	return rc;
};

static unsigned int msm_bahama_shutdown_power(int value)
{
	int rc = 0;

	if (value != BAHAMA_ID) {
		rc = regulator_disable(vreg_bahama);

		if (rc)
			pr_err("%s: regulator_disable failed (%d)\n",
					__func__, rc);
	}

	return rc;
};

static struct msm_gpio marimba_svlte_config_clock[] = {
	{ GPIO_CFG(34, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"MARIMBA_SVLTE_CLOCK_ENABLE" },
};

static unsigned int msm_marimba_gpio_config_svlte(int gpio_cfg_marimba)
{
	if (machine_is_msm8x55_svlte_surf() ||
		machine_is_msm8x55_svlte_ffa()) {
		if (gpio_cfg_marimba)
			gpio_set_value(GPIO_PIN
				(marimba_svlte_config_clock->gpio_cfg), 1);
		else
			gpio_set_value(GPIO_PIN
				(marimba_svlte_config_clock->gpio_cfg), 0);
	}

	return 0;
};

static unsigned int msm_marimba_setup_power(void)
{
	int rc;

	rc = regulator_enable(vreg_marimba_1);
	if (rc) {
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);
		goto out;
	}

	rc = regulator_enable(vreg_marimba_2);
	if (rc) {
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);
		goto disable_marimba_1;
	}

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = msm_gpios_request_enable(marimba_svlte_config_clock,
				ARRAY_SIZE(marimba_svlte_config_clock));
		if (rc < 0) {
			pr_err("%s: msm_gpios_request_enable failed (%d)\n",
					__func__, rc);
			goto disable_marimba_2;
		}

		rc = gpio_direction_output(GPIO_PIN
			(marimba_svlte_config_clock->gpio_cfg), 0);
		if (rc < 0) {
			pr_err("%s: gpio_direction_output failed (%d)\n",
					__func__, rc);
			goto disable_marimba_2;
		}
	}

	return 0;

disable_marimba_2:
	regulator_disable(vreg_marimba_2);
disable_marimba_1:
	regulator_disable(vreg_marimba_1);
out:
	return rc;
};

static void msm_marimba_shutdown_power(void)
{
	int rc;

	rc = regulator_disable(vreg_marimba_2);
	if (rc)
		pr_err("%s: regulator_disable failed (%d)\n", __func__, rc);

	rc = regulator_disable(vreg_marimba_1);
	if (rc)
		pr_err("%s: regulator_disable failed (%d)\n", __func__, rc);
};

static int bahama_present(void)
{
	int id;
	switch (id = adie_get_detected_connectivity_type()) {
	case BAHAMA_ID:
		return 1;

	case MARIMBA_ID:
		return 0;

	case TIMPANI_ID:
	default:
	printk(KERN_ERR "%s: unexpected adie connectivity type: %d\n",
			__func__, id);
	return -ENODEV;
	}
}

struct regulator *fm_regulator;
static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc, voltage;
	uint32_t irqcfg;
	const char *id = "FMPW";

	int bahama_not_marimba = bahama_present();

	if (bahama_not_marimba < 0) {
		pr_warn("%s: bahama_present: %d\n",
				__func__, bahama_not_marimba);
		rc = -ENODEV;
		goto out;
	}
	if (bahama_not_marimba) {
		fm_regulator = regulator_get(NULL, "s3");
		voltage = 1800000;
	} else {
		fm_regulator = regulator_get(NULL, "s2");
		voltage = 1300000;
	}

	if (IS_ERR(fm_regulator)) {
		rc = PTR_ERR(fm_regulator);
		pr_err("%s: regulator_get failed (%d)\n", __func__, rc);
		goto out;
	}

	rc = regulator_set_voltage(fm_regulator, voltage, voltage);

	if (rc) {
		pr_err("%s: regulator_set_voltage failed (%d)\n", __func__, rc);
		goto regulator_free;
	}

	rc = regulator_enable(fm_regulator);

	if (rc) {
		pr_err("%s: regulator_enable failed (%d)\n", __func__, rc);
		goto regulator_free;
	}

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO, PMAPP_CLOCK_VOTE_ON);

	if (rc < 0) {
		pr_err("%s: clock vote failed (%d)\n", __func__, rc);
		goto regulator_disable;
	}

	/*Request the Clock Using GPIO34/AP2MDM_MRMBCK_EN in case
	of svlte*/
	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = marimba_gpio_config(1);
		if (rc < 0) {
			pr_err("%s: clock enable for svlte : %d\n",
					__func__, rc);
			goto clock_devote;
		}
	}
	irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__, irqcfg, rc);
		rc = -EIO;
		goto gpio_deconfig;

	}
	return 0;

gpio_deconfig:
	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa())
		marimba_gpio_config(0);
clock_devote:
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO, PMAPP_CLOCK_VOTE_OFF);
regulator_disable:
	regulator_disable(fm_regulator);
regulator_free:
	regulator_put(fm_regulator);
	fm_regulator = NULL;
out:
	return rc;
};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc;
	const char *id = "FMPW";
	uint32_t irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA);

	int bahama_not_marimba = bahama_present();
	if (bahama_not_marimba == -1) {
		pr_warn("%s: bahama_present: %d\n",
				__func__, bahama_not_marimba);
		return;
	}

	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__, irqcfg, rc);
	}
	if (!IS_ERR_OR_NULL(fm_regulator)) {
		rc = regulator_disable(fm_regulator);

		if (rc)
			pr_err("%s: return val: %d\n", __func__, rc);

		regulator_put(fm_regulator);
		fm_regulator = NULL;
	}
	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0)
		pr_err("%s: clock_vote return val: %d\n", __func__, rc);

	/*Disable the Clock Using GPIO34/AP2MDM_MRMBCK_EN in case
	of svlte*/
	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = marimba_gpio_config(0);
		if (rc < 0)
			pr_err("%s: clock disable for svlte : %d\n",
					__func__, rc);
	}
}

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = MSM_GPIO_TO_INT(147),
	.vreg_s2 = NULL,
	.vreg_xo_out = NULL,
	.is_fm_soc_i2s_master = false,
	.config_i2s_gpio = NULL,
};


/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define MARIMBA_SLAVE_ID_FM_ADDR	0x2A
#define MARIMBA_SLAVE_ID_CDC_ADDR	0x77
#define MARIMBA_SLAVE_ID_QMEMBIST_ADDR	0X66

#define BAHAMA_SLAVE_ID_FM_ADDR         0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B

static const char *tsadc_id = "MADC";

static struct regulator_bulk_data regs_tsadc_marimba[] = {
	{ .supply = "gp12", .min_uV = 2200000, .max_uV = 2200000 },
	{ .supply = "s2",   .min_uV = 1300000, .max_uV = 1300000 },
};

static struct regulator_bulk_data regs_tsadc_timpani[] = {
	{ .supply = "s3",   .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "gp12", .min_uV = 2200000, .max_uV = 2200000 },
	{ .supply = "gp16", .min_uV = 1200000, .max_uV = 1200000 },
};

static struct regulator_bulk_data *regs_tsadc;
static int regs_tsadc_count;

static int marimba_tsadc_power(int vreg_on)
{
	int rc = 0;
	int tsadc_adie_type = adie_get_detected_codec_type();

	switch (tsadc_adie_type) {
	case TIMPANI_ID:
		rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_D1,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
		if (rc)	{
			pr_err("%s: unable to %svote for d1 clk\n",
				__func__, vreg_on ? "" : "de-");
			goto D1_vote_fail;
		}

		/* fall through */
	case MARIMBA_ID:
		rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
		if (rc)	{
			pr_err("%s: unable to %svote for d1 clk\n",
				__func__, vreg_on ? "" : "de-");
			goto D0_vote_fail;
		}

		WARN_ON(regs_tsadc_count == 0);

		rc = vreg_on ?
			regulator_bulk_enable(regs_tsadc_count, regs_tsadc) :
			regulator_bulk_disable(regs_tsadc_count, regs_tsadc);

		if (rc) {
			pr_err("%s: regulator %sable failed: %d\n",
					__func__, vreg_on ? "en" : "dis", rc);
			goto regulator_switch_fail;
		}

		break;
	default:
		pr_err("%s:Adie %d not supported\n",
				__func__, tsadc_adie_type);
		return -ENODEV;
	}

	msleep(5); /* ensure power is stable */

	return 0;

regulator_switch_fail:
	pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
		vreg_on ? PMAPP_CLOCK_VOTE_OFF : PMAPP_CLOCK_VOTE_ON);
D0_vote_fail:
	if (tsadc_adie_type == TIMPANI_ID)
		pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_D1,
			vreg_on ? PMAPP_CLOCK_VOTE_OFF : PMAPP_CLOCK_VOTE_ON);
D1_vote_fail:
	return rc;
}

static int marimba_tsadc_init(void)
{
	int rc = 0;
	int tsadc_adie_type = adie_get_detected_codec_type();

	switch (tsadc_adie_type) {
	case MARIMBA_ID:
		regs_tsadc = regs_tsadc_marimba;
		regs_tsadc_count = ARRAY_SIZE(regs_tsadc_marimba);
		break;
	case TIMPANI_ID:
		regs_tsadc = regs_tsadc_timpani;
		regs_tsadc_count = ARRAY_SIZE(regs_tsadc_timpani);
		break;
	default:
		pr_err("%s:Adie %d not supported\n",
				__func__, tsadc_adie_type);
		rc = -ENODEV;
		goto out;
	}

	rc = regulator_bulk_get(NULL, regs_tsadc_count, regs_tsadc);
	if (rc) {
		pr_err("%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(regs_tsadc_count, regs_tsadc);
	if (rc) {
		pr_err("%s: could not set regulator voltages: %d\n",
				__func__, rc);
		goto vreg_free;
	}

	return 0;

vreg_free:
	regulator_bulk_free(regs_tsadc_count, regs_tsadc);
out:
	regs_tsadc = NULL;
	regs_tsadc_count = 0;
	return rc;
}

static int marimba_tsadc_exit(void)
{
	regulator_bulk_free(regs_tsadc_count, regs_tsadc);
	regs_tsadc_count = 0;
	regs_tsadc = NULL;

	return 0;
}


static struct msm_ts_platform_data msm_ts_data = {
	.min_x          = 284,
	.max_x          = 3801,
	.min_y          = 155,
	.max_y          = 3929,
	.min_press      = 0,
	.max_press      = 255,
	.inv_x          = 4096,
	.inv_y          = 4096,
	.can_wakeup	= false,
};

static struct marimba_tsadc_platform_data marimba_tsadc_pdata = {
	.marimba_tsadc_power =  marimba_tsadc_power,
	.init		     =  marimba_tsadc_init,
	.exit		     =  marimba_tsadc_exit,
	.tsadc_prechg_en = true,
	.can_wakeup	= false,
	.setup = {
		.pen_irq_en	=	true,
		.tsadc_en	=	true,
	},
	.params2 = {
		.input_clk_khz		=	2400,
		.sample_prd		=	TSADC_CLK_3,
	},
	.params3 = {
		.prechg_time_nsecs	=	6400,
		.stable_time_nsecs	=	6400,
		.tsadc_test_mode	=	0,
	},
	.tssc_data = &msm_ts_data,
};

static struct regulator_bulk_data codec_regs[] = {
	{ .supply = "s4", .min_uV = 2200000, .max_uV = 2200000 },
};

static int __init msm_marimba_codec_init(void)
{
	int rc = regulator_bulk_get(NULL, ARRAY_SIZE(codec_regs), codec_regs);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(codec_regs), codec_regs);
	if (rc) {
		pr_err("%s: could not set regulator voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	return rc;

reg_free:
	regulator_bulk_free(ARRAY_SIZE(codec_regs), codec_regs);
out:
	return rc;
}

static int msm_marimba_codec_power(int vreg_on)
{
	int rc = vreg_on ?
		regulator_bulk_enable(ARRAY_SIZE(codec_regs), codec_regs) :
		regulator_bulk_disable(ARRAY_SIZE(codec_regs), codec_regs);

	if (rc) {
		pr_err("%s: could not %sable regulators: %d",
				__func__, vreg_on ? "en" : "dis", rc);
		return rc;
	}

	return 0;
}

static struct marimba_codec_platform_data mariba_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_MARIMBA_CODEC
	.snddev_profile_init = msm_snddev_init,
#endif
};

static struct marimba_platform_data marimba_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_FM]       = MARIMBA_SLAVE_ID_FM_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_CDC]	     = MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_FM]        = BAHAMA_SLAVE_ID_FM_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_QMEMBIST]  = BAHAMA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_marimba_setup_power,
	.marimba_shutdown = msm_marimba_shutdown_power,
	.bahama_setup = msm_bahama_setup_power,
	.bahama_shutdown = msm_bahama_shutdown_power,
	.marimba_gpio_config = msm_marimba_gpio_config_svlte,
	.bahama_core_config = msm_bahama_core_config,
	.fm = &marimba_fm_pdata,
	.codec = &mariba_codec_pdata,
	.tsadc_ssbi_adap = MARIMBA_SSBI_ADAP,
};

static void __init msm7x30_init_marimba(void)
{
	int rc;

	struct regulator_bulk_data regs[] = {
		{ .supply = "s3",   .min_uV = 1800000, .max_uV = 1800000 },
		{ .supply = "gp16", .min_uV = 1200000, .max_uV = 1200000 },
		{ .supply = "usb2", .min_uV = 1800000, .max_uV = 1800000 },
	};

	rc = msm_marimba_codec_init();

	if (rc) {
		pr_err("%s: msm_marimba_codec_init failed (%d)\n",
				__func__, rc);
		return;
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs), regs);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		return;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs), regs);

	if (rc) {
		pr_err("%s: could not set voltages: %d\n", __func__, rc);
		regulator_bulk_free(ARRAY_SIZE(regs), regs);
		return;
	}

	vreg_marimba_1 = regs[0].consumer;
	vreg_marimba_2 = regs[1].consumer;
	vreg_bahama    = regs[2].consumer;
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_TIMPANI_CODEC
	.snddev_profile_init = msm_snddev_init_timpani,
#endif
};

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
	.tsadc = &marimba_tsadc_pdata,
	.tsadc_ssbi_adap = MARIMBA_SSBI_ADAP,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_MSM7KV2_AUDIO
static struct resource msm_aictl_resources[] = {
	{
		.name = "aictl",
		.start = 0xa5000100,
		.end = 0xa5000100,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_mi2s_resources[] = {
	{
		.name = "hdmi",
		.start = 0xac900000,
		.end = 0xac900038,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_rx",
		.start = 0xac940040,
		.end = 0xac940078,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_tx",
		.start = 0xac980080,
		.end = 0xac9800B8,
		.flags = IORESOURCE_MEM,
	}

};

static struct msm_lpa_platform_data lpa_pdata = {
	.obuf_hlb_size = 0x2BFF8,
	.dsp_proc_id = 0,
	.app_proc_id = 2,
	.nosb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x3ff8,
		.sb_min_addr = 0,
		.sb_max_addr = 0,
	},
	.sb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x37f8,
		.sb_min_addr = 0x3800,
		.sb_max_addr = 0x3ff8,
	}
};

static struct resource msm_lpa_resources[] = {
	{
		.name = "lpa",
		.start = 0xa5000000,
		.end = 0xa50000a0,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name = "aux_codec_reg_addr",
		.start = 0xac9c00c0,
		.end = 0xac9c00c8,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 138,
		.end    = 138,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 139,
		.end    = 139,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 140,
		.end    = 140,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 141,
		.end    = 141,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

struct platform_device msm_aictl_device = {
	.name = "audio_interct",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_aictl_resources),
	.resource = msm_aictl_resources,
};

struct platform_device msm_mi2s_device = {
	.name = "mi2s",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_mi2s_resources),
	.resource = msm_mi2s_resources,
};

struct platform_device msm_lpa_device = {
	.name = "lpa",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_lpa_resources),
	.resource = msm_lpa_resources,
	.dev		= {
		.platform_data = &lpa_pdata,
	},
};
#endif /* CONFIG_MSM7KV2_AUDIO */

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	0,
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_MODE_LP)|
	(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 1 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	 /* Concurrency 2 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 3 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 4 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 5 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 6 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

#define DEC_INSTANCE(max_instance_same, max_instance_diff) { \
	.max_instances_same_dec = max_instance_same, \
	.max_instances_diff_dec = max_instance_diff}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
};

static struct dec_instance_table dec_instance_list[][MSM_MAX_DEC_CNT] = {
	/* Non Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 2), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 2), /* WMA */
		DEC_INSTANCE(3, 2), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(1, 1), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 1), /* WMAPRO */
	},
	/* Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 3), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 3), /* WMA */
		DEC_INSTANCE(4, 3), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(2, 3), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 2), /* WMAPRO */
	},
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
	.dec_instance_list = &dec_instance_list[0][0],
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct resource smc91x_resources[] = {
	[0] = {
		.start = 0x8A000300,
		.end = 0x8A0003ff,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start = MSM_GPIO_TO_INT(156),
		.end = MSM_GPIO_TO_INT(156),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};

static struct smsc911x_platform_config smsc911x_config = {
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_32BIT,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start		= 0x8D000000,
		.end		= 0x8D000100,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= MSM_GPIO_TO_INT(88),
		.end		= MSM_GPIO_TO_INT(88),
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

static struct msm_gpio smsc911x_gpios[] = {
    { GPIO_CFG(172, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr6" },
    { GPIO_CFG(173, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr5" },
    { GPIO_CFG(174, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr4" },
    { GPIO_CFG(175, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr3" },
    { GPIO_CFG(176, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr2" },
    { GPIO_CFG(177, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr1" },
    { GPIO_CFG(178, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr0" },
    { GPIO_CFG(88, 2, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "smsc911x_irq"  },
};

static void msm7x30_cfg_smsc911x(void)
{
	int rc;

	rc = msm_gpios_request_enable(smsc911x_gpios,
			ARRAY_SIZE(smsc911x_gpios));
	if (rc)
		pr_err("%s: unable to enable gpios\n", __func__);
}

#ifdef CONFIG_USB_G_ANDROID
static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

static struct msm_gpio optnav_config_data[] = {
	{ GPIO_CFG(OPTNAV_CHIP_SELECT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
	"optnav_chip_select" },
};

static struct regulator_bulk_data optnav_regulators[] = {
	{ .supply = "gp7", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "gp4", .min_uV = 2600000, .max_uV = 2600000 },
	{ .supply = "gp9", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "usb", .min_uV = 3300000, .max_uV = 3300000 },
};

static void __iomem *virtual_optnav;

static int optnav_gpio_setup(void)
{
	int rc = -ENODEV;
	rc = msm_gpios_request_enable(optnav_config_data,
			ARRAY_SIZE(optnav_config_data));

	if (rc)
		return rc;

	/* Configure the FPGA for GPIOs */
	virtual_optnav = ioremap(FPGA_OPTNAV_GPIO_ADDR, 0x4);
	if (!virtual_optnav) {
		pr_err("%s:Could not ioremap region\n", __func__);
		return -ENOMEM;
	}
	/*
	 * Configure the FPGA to set GPIO 19 as
	 * normal, active(enabled), output(MSM to SURF)
	 */
	writew(0x311E, virtual_optnav);

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(optnav_regulators),
			optnav_regulators);
	if (rc)
		return rc;

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(optnav_regulators),
			optnav_regulators);

	if (rc)
		goto regulator_put;

	return rc;

regulator_put:
	regulator_bulk_free(ARRAY_SIZE(optnav_regulators), optnav_regulators);
	return rc;
}

static void optnav_gpio_release(void)
{
	msm_gpios_disable_free(optnav_config_data,
		ARRAY_SIZE(optnav_config_data));
	iounmap(virtual_optnav);
	regulator_bulk_free(ARRAY_SIZE(optnav_regulators), optnav_regulators);
}

static int optnav_enable(void)
{
	int rc;
	/*
	 * Enable the VREGs L8(gp7), L10(gp4), L12(gp9), L6(usb)
	 * for I2C communication with keyboard.
	 */

	rc = regulator_bulk_enable(ARRAY_SIZE(optnav_regulators),
			optnav_regulators);

	if (rc)
		return rc;

	/* Enable the chip select GPIO */
	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
	gpio_set_value(OPTNAV_CHIP_SELECT, 0);

	return 0;
}

static void optnav_disable(void)
{
	regulator_bulk_disable(ARRAY_SIZE(optnav_regulators),
			optnav_regulators);

	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
}

static struct ofn_atlab_platform_data optnav_data = {
	.gpio_setup    = optnav_gpio_setup,
	.gpio_release  = optnav_gpio_release,
	.optnav_on     = optnav_enable,
	.optnav_off    = optnav_disable,
	.rotate_xy     = 0,
	.function1 = {
		.no_motion1_en		= true,
		.touch_sensor_en	= true,
		.ofn_en			= true,
		.clock_select_khz	= 1500,
		.cpi_selection		= 1200,
	},
	.function2 =  {
		.invert_y		= false,
		.invert_x		= true,
		.swap_x_y		= false,
		.hold_a_b_en		= true,
		.motion_filter_en       = true,
	},
};

static int hdmi_comm_power(int on, int show);
static int hdmi_init_irq(void);
static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static bool hdmi_check_hdcp_hw_support(void);

static struct msm_hdmi_platform_data adv7520_hdmi_data = {
	.irq = MSM_GPIO_TO_INT(18),
	.comm_power = hdmi_comm_power,
	.init_irq = hdmi_init_irq,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.check_hdcp_hw_support = hdmi_check_hdcp_hw_support,
};



static struct i2c_board_info msm_i2c_board_info[] = {
	{
		.type = "taos", 
		.addr = 0x39,
	}, 
	{
		I2C_BOARD_INFO("m33c01", OPTNAV_I2C_SLAVE_ADDR),
		.irq		= MSM_GPIO_TO_INT(OPTNAV_IRQ),
		.platform_data = &optnav_data,
	},
	{
		I2C_BOARD_INFO("adv7520", ADV7520_I2C_ADDR),
		.platform_data = &adv7520_hdmi_data,
	},

};

static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};


static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 8594,
		.residency = 23740,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 4594,
		.residency = 23740,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE] = {
#ifdef CONFIG_MSM_STANDALONE_POWER_COLLAPSE
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 0,
#else /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
		.idle_supported = 0,
		.suspend_supported = 0,
		.idle_enabled = 0,
		.suspend_enabled = 0,
#endif /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
		.latency = 500,
		.residency = 6000,
	},
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 1,
		.latency = 443,
		.residency = 1098,
	},
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 2,
		.residency = 0,
	},
};

u32 msm7x30_power_collapse_latency(enum msm_pm_sleep_mode mode)
{
	switch (mode) {
	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE:
		return msm_pm_data
		[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency;
	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN:
		return msm_pm_data
		[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency;
	case MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT:
		return msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	case MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT:
		return msm_pm_data
		[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
	default:
	return 0;
	}
}

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_RESET_VECTOR_VIRT,
	.v_addr = (uint32_t *)PAGE_OFFSET,
};

static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA8000000,
		.end	= 0xA8000000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

#define AMDH0_BASE_PHYS		0xAC200000
#define ADMH0_GP_CTL		(ct_adm_base + 0x3D8)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_adm_base = 0;
	u32 spi_mux = 0;
	int ret = 0;

	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
		return -ENOMEM;
	}

	spi_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;

	qsd_spi_resources[4].start  = DMOV_USB_CHAN;

	switch (spi_mux) {
	case (1):
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -ENOENT;
	}

	iounmap(ct_adm_base);

	return ret;
}

static struct platform_device qsd_device_spi = {
	.name		= "spi_qsd",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

#ifdef CONFIG_SPI_QSD
static struct spi_board_info lcdc_sharp_spi_board_info[] __initdata = {
	{
		.modalias	= "lcdc_sharp_ls038y7dx01",
		.mode		= SPI_MODE_1,
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 26331429,
	}
};
static struct spi_board_info lcdc_toshiba_spi_board_info[] __initdata = {
	{
		.modalias       = "lcdc_toshiba_ltm030dd40",
		.mode           = SPI_MODE_3|SPI_CS_HIGH,
		.bus_num        = 0,
		.chip_select    = 0,
		.max_speed_hz   = 9963243,
	}
};
#endif

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(45, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "spi_mosi" },
	{ GPIO_CFG(48, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static int msm_qsd_spi_gpio_config(void)
{
	return msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 26331429,
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
        int rc;
        static int vbus_is_on;
	struct pm8xxx_gpio_init_info usb_vbus = {
		PM8058_GPIO_PM_TO_SYS(36),
		{
			.direction      = PM_GPIO_DIR_OUT,
			.pull           = PM_GPIO_PULL_NO,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 1,
			.vin_sel        = 2,
			.out_strength   = PM_GPIO_STRENGTH_MED,
			.function       = PM_GPIO_FUNC_NORMAL,
			.inv_int_pol    = 0,
		},
	};

        /* If VBUS is already on (or off), do nothing. */
        if (unlikely(on == vbus_is_on))
                return;

        if (on) {
		rc = pm8xxx_gpio_config(usb_vbus.gpio, &usb_vbus.config);
		if (rc) {
                        pr_err("%s PMIC GPIO 36 write failed\n", __func__);
                        return;
                }
	} else {
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(36), 0);
	}

        vbus_is_on = on;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
        .phy_info   = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
        .vbus_power = msm_hsusb_vbus_power,
        .power_budget   = 180,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct regulator *vreg_3p3;
static int msm_hsusb_ldo_init(int init)
{
	uint32_t version = 0;
	int def_vol = 3400000;

	version = socinfo_get_version();

	if (SOCINFO_VERSION_MAJOR(version) >= 2 &&
			SOCINFO_VERSION_MINOR(version) >= 1) {
		def_vol = 3075000;
		pr_debug("%s: default voltage:%d\n", __func__, def_vol);
	}

	if (init) {
		vreg_3p3 = regulator_get(NULL, "usb");
		if (IS_ERR(vreg_3p3))
			return PTR_ERR(vreg_3p3);
		regulator_set_voltage(vreg_3p3, def_vol, def_vol);
	} else
		regulator_put(vreg_3p3);

	return 0;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	if (enable)
		return regulator_enable(vreg_3p3);
	else
		return regulator_disable(vreg_3p3);
}

static int msm_hsusb_ldo_set_voltage(int mV)
{
	static int cur_voltage;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (cur_voltage == mV)
		return 0;

	cur_voltage = mV;

	pr_debug("%s: (%d)\n", __func__, mV);

	return regulator_set_voltage(vreg_3p3, mV*1000, mV*1000);
}
#endif

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init);
#endif
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,

#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init         = msm_hsusb_pmic_notif_init,
#else
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
	.chg_connected		 = hsusb_chg_connected,
	.chg_init		 = hsusb_chg_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_set_voltage	 = msm_hsusb_ldo_set_voltage,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif
#ifndef CONFIG_USB_EHCI_MSM_72K
typedef void (*notify_vbus_state) (int);
notify_vbus_state notify_vbus_state_func_ptr;
int vbus_on_irq;
static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	pr_info("%s: vbus notification from pmic\n", __func__);

	(*notify_vbus_state_func_ptr) (1);

	return IRQ_HANDLED;
}
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret;

	if (init) {
		if (!callback)
			return -ENODEV;

		notify_vbus_state_func_ptr = callback;
		vbus_on_irq = platform_get_irq_byname(&msm_device_otg,
			"vbus_on");
		if (vbus_on_irq <= 0) {
			pr_err("%s: unable to get vbus on irq\n", __func__);
			return -ENODEV;
		}

		ret = request_any_context_irq(vbus_on_irq, pmic_vbus_on_irq,
			IRQF_TRIGGER_RISING, "msm_otg_vbus_on", NULL);
		if (ret < 0) {
			pr_info("%s: request_irq for vbus_on"
				"interrupt failed\n", __func__);
			return ret;
		}
		msm_otg_pdata.pmic_vbus_irq = vbus_on_irq;
		return 0;
	} else {
		free_irq(vbus_on_irq, 0);
		notify_vbus_state_func_ptr = NULL;
		return 0;
	}
}
#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

//#ifndef CONFIG_SPI_QSD 
static int lcdc_gpio_array_num[] = {
				2, /* lcd_bkl_drv */				
				120, /* lcd_id */
				121,/* lcd_sdo */
				122,/* lcd_cs */
				123,/* lcd_sdi */
				124,/* lcd_clk */
				180,/* lcd_rst */
				};

static struct msm_gpio lcdc_gpio_config_data[] = {
	{ GPIO_CFG(2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_bkl_drv" },
	{ GPIO_CFG(120, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_ic_id" },
	{ GPIO_CFG(180, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_rst" },
	{ GPIO_CFG(121, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_spi_sdo" },
	{ GPIO_CFG(122, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_spi_cs" },
	{ GPIO_CFG(123, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_spi_sdi" },
	{ GPIO_CFG(124, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcd_spi_clk" },
};

static void lcdc_config_gpios(int enable)
{
	if (enable) {
		msm_gpios_request_enable(lcdc_gpio_config_data,
					      ARRAY_SIZE(
						      lcdc_gpio_config_data));
	} else
		msm_gpios_disable_free(lcdc_gpio_config_data,
					    ARRAY_SIZE(
						    lcdc_gpio_config_data));
}

static struct msm_panel_common_pdata lcdc_panel_data = {
	.panel_config_gpio = lcdc_config_gpios,
	.gpio_num          = lcdc_gpio_array_num,
};

static struct platform_device lcdc_panel_device = {

	.name   = "lcdc_panel_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};

static struct msm_gpio dtv_panel_irq_gpios[] = {
	{ GPIO_CFG(228, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"hdmi_int" },
};

static struct regulator_bulk_data hdmi_core_regs[] = {
	{ .supply = "ldo8",  .min_uV = 1800000, .max_uV = 1800000 },
};

static struct regulator_bulk_data hdmi_comm_regs[] = {
	{ .supply = "ldo8",  .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "ldo10", .min_uV = 2600000, .max_uV = 2600000 },
};

static struct regulator_bulk_data hdmi_cec_regs[] = {
	{ .supply = "ldo17", .min_uV = 2600000, .max_uV = 2600000 },
};

static int __init hdmi_init_regs(void)
{
	int rc;

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(hdmi_core_regs),
			hdmi_core_regs);

	if (rc) {
		pr_err("%s: could not get %s regulators: %d\n",
				__func__, "core", rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(hdmi_core_regs),
			hdmi_core_regs);

	if (rc) {
		pr_err("%s: could not set %s voltages: %d\n",
				__func__, "core", rc);
		goto free_core;
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(hdmi_comm_regs),
			hdmi_comm_regs);

	if (rc) {
		pr_err("%s: could not get %s regulators: %d\n",
				__func__, "comm", rc);
		goto free_core;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(hdmi_comm_regs),
			hdmi_comm_regs);

	if (rc) {
		pr_err("%s: could not set %s voltages: %d\n",
				__func__, "comm", rc);
		goto free_comm;
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(hdmi_cec_regs),
			hdmi_cec_regs);

	if (rc) {
		pr_err("%s: could not get %s regulators: %d\n",
				__func__, "cec", rc);
		goto free_comm;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(hdmi_cec_regs),
			hdmi_cec_regs);

	if (rc) {
		pr_err("%s: could not set %s voltages: %d\n",
				__func__, "cec", rc);
		goto free_cec;
	}

	return 0;

free_cec:
	regulator_bulk_free(ARRAY_SIZE(hdmi_cec_regs), hdmi_cec_regs);
free_comm:
	regulator_bulk_free(ARRAY_SIZE(hdmi_comm_regs), hdmi_comm_regs);
free_core:
	regulator_bulk_free(ARRAY_SIZE(hdmi_core_regs), hdmi_core_regs);
out:
	return rc;
}

static int hdmi_init_irq(void)
{
	int rc = msm_gpios_enable(dtv_panel_irq_gpios,
			ARRAY_SIZE(dtv_panel_irq_gpios));
	if (rc < 0) {
		pr_err("%s: gpio enable failed: %d\n", __func__, rc);
		return rc;
	}
	pr_info("%s\n", __func__);

	return 0;
}

static int hdmi_enable_5v(int on)
{
	int pmic_gpio_hdmi_5v_en ;

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa() ||
						machine_is_msm7x30_fluid())
		pmic_gpio_hdmi_5v_en = PMIC_GPIO_HDMI_5V_EN_V2 ;
	else
		pmic_gpio_hdmi_5v_en = PMIC_GPIO_HDMI_5V_EN_V3 ;

	pr_info("%s: %d\n", __func__, on);
	if (on) {
		int rc;
		rc = gpio_request(PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en),
			"hdmi_5V_en");
		if (rc) {
			pr_err("%s PMIC_GPIO_HDMI_5V_EN gpio_request failed\n",
				__func__);
			return rc;
		}
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en), 1);
	} else {
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en), 0);
		gpio_free(PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en));
	}
	return 0;
}

static int hdmi_comm_power(int on, int show)
{
	if (show)
		pr_info("%s: i2c comm: %d <LDO8+LDO10>\n", __func__, on);
	return on ?
		regulator_bulk_enable(ARRAY_SIZE(hdmi_comm_regs),
				hdmi_comm_regs) :
		regulator_bulk_disable(ARRAY_SIZE(hdmi_comm_regs),
				hdmi_comm_regs);
}

static int hdmi_core_power(int on, int show)
{
	if (show)
		pr_info("%s: %d <LDO8>\n", __func__, on);
	return on ?
		regulator_bulk_enable(ARRAY_SIZE(hdmi_core_regs),
				hdmi_core_regs) :
		regulator_bulk_disable(ARRAY_SIZE(hdmi_core_regs),
				hdmi_core_regs);
}

static int hdmi_cec_power(int on)
{
	pr_info("%s: %d <LDO17>\n", __func__, on);
	return on ? regulator_bulk_enable(ARRAY_SIZE(hdmi_cec_regs),
				hdmi_cec_regs) :
		regulator_bulk_disable(ARRAY_SIZE(hdmi_cec_regs),
				hdmi_cec_regs);
}

static bool hdmi_check_hdcp_hw_support(void)
{
	if (machine_is_msm7x30_fluid())
		return false;
	else
		return true;
}

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm7x30_fluid()) {
		if (!strcmp(name, "lcdc_sharp_wvga_pt"))
			return 0;
	} else {
		if (!strncmp(name, "mddi_toshiba_wvga_pt", 20))
			return -EPERM;
		else if (!strncmp(name, "lcdc_toshiba_wvga_pt", 20))
			return 0;
		else if (!strcmp(name, "mddi_orise"))
			return -EPERM;
		else if (!strcmp(name, "mddi_quickvx"))
			return -EPERM;
	}
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

static struct platform_device msm_migrate_pages_device = {
	.name   = "msm_migrate_pages",
	.id     = -1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
       .name = "pmem_adsp",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
	.memory_type = MEMTYPE_EBI0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
       .name = "pmem_audio",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
	.memory_type = MEMTYPE_EBI0,
};

static struct platform_device android_pmem_adsp_device = {
       .name = "android_pmem",
       .id = 2,
       .dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
       .name = "android_pmem",
       .id = 4,
       .dev = { .platform_data = &android_pmem_audio_pdata },
};


/**start******20120511***************
 **modify*******solve blue screen***************/
int mdp_core_clk_rate_table[] = {
	192000000, 		//122880000
	192000000,		//122880000
	192000000,
	192000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.hw_revision_addr = 0xac001270,
	.gpio = 30,
	.mdp_core_clk_rate = 192000000,		//122880000
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
	.mdp_rev = MDP_REV_40,
	.mem_hid = MEMTYPE_EBI0,
};
/**end**/

static struct msm_gpio lcd_panel_gpios[] = {
/* Workaround, since HDMI_INT is using the same GPIO line (18), and is used as
 * input.  if there is a hardware revision; we should reassign this GPIO to a
 * new open line; and removing it will just ensure that this will be missed in
 * the future.
	{ GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn0" },
 */

	{ GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn1" },
	{ GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu0" },
	{ GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu1" },
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu2" },
	{ GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red0" },
	{ GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red1" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red2" },
#if defined(CONFIG_MACH_ARTHUR)
#ifndef CONFIG_SPI_QSD
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
#endif
#endif 

	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red7" },
};


static int lcdc_panel_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo20;	
	int i;
	struct msm_gpio *gp;
	
	vreg_ldo20 = vreg_get(NULL, "gp13");	
	if (!vreg_ldo20) {		
	pr_err("%s: VREG L20 get failed\n", __func__);	
	}
	rc = vreg_set_level(vreg_ldo20, 2800);		
	if (rc) {			
	pr_err("%s: VREG L20 set failed\n", __func__);	
	}
	if (on){
		rc = vreg_enable(vreg_ldo20);		
		if (rc)
		{			
			pr_err("%s: VREG L20 enable failed\n", __func__);			
		}
		rc = msm_gpios_enable(lcd_panel_gpios,
				ARRAY_SIZE(lcd_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
					__func__, rc);
		}

	
	}
	else
	{
	gp = lcd_panel_gpios;
	for (i = 0; i < ARRAY_SIZE(lcd_panel_gpios); i++) {
	/* ouput low */
		gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
		gp++;
		}
	rc = vreg_disable(vreg_ldo20);		
	if (rc) 
	{			
		pr_err("%s: VREG L20 disable failed\n", __func__);			
	}

	}

	return 0;
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save   = lcdc_panel_power,
};

static struct regulator *atv_s4, *atv_ldo9;

static int __init atv_dac_power_init(void)
{
	int rc;
	struct regulator_bulk_data regs[] = {
		{ .supply = "smps4", .min_uV = 2200000, .max_uV = 2200000 },
		{ .supply = "ldo9",  .min_uV = 2050000, .max_uV = 2050000 },
	};

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs), regs);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		goto bail;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs), regs);

	if (rc) {
		pr_err("%s: could not set voltages: %d\n", __func__, rc);
		goto reg_free;
	}

	atv_s4   = regs[0].consumer;
	atv_ldo9 = regs[1].consumer;

reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs), regs);
bail:
	return rc;
}

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", &lcdc_pdata);
}

#ifdef CONFIG_MSM_BT 
enum {
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(134, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Tx" }
};

static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(134, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Tx" }
};

static u8 bahama_version;

static struct regulator_bulk_data regs_bt_marimba[] = {
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "smps2", .min_uV = 1300000, .max_uV = 1300000 },
	{ .supply = "ldo24", .min_uV = 1200000, .max_uV = 1200000 },
	{ .supply = "ldo13", .min_uV = 2900000, .max_uV = 3050000 },
};

static struct regulator_bulk_data regs_bt_bahama_v1[] = {
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "ldo7",  .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "smps2", .min_uV = 1300000, .max_uV = 1300000 },
	{ .supply = "ldo13", .min_uV = 2900000, .max_uV = 3050000 },
};

static struct regulator_bulk_data regs_bt_bahama_v2[] = {
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "ldo7",  .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "ldo13", .min_uV = 2900000, .max_uV = 3050000 },
};

static struct regulator_bulk_data *regs_bt;
static int regs_bt_count;

static int marimba_bt(int on)
{
	int rc;
	int i;
	struct marimba config = { .mod_id = MARIMBA_SLAVE_ID_MARIMBA };

	struct marimba_config_register {
		u8 reg;
		u8 value;
		u8 mask;
	};

	struct marimba_variant_register {
		const size_t size;
		const struct marimba_config_register *set;
	};

	const struct marimba_config_register *p;

	u8 version;

	const struct marimba_config_register v10_bt_on[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x02, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v10_bt_off[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x08, 0x0F },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v201_bt_on[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v201_bt_off[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v210_bt_on[] = {
		{ 0xE9, 0x01, 0x01 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v210_bt_off[] = {
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE9, 0x00, 0x01 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_variant_register bt_marimba[2][4] = {
		{
			{ ARRAY_SIZE(v10_bt_off), v10_bt_off },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_off), v201_bt_off },
			{ ARRAY_SIZE(v210_bt_off), v210_bt_off }
		},
		{
			{ ARRAY_SIZE(v10_bt_on), v10_bt_on },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_on), v201_bt_on },
			{ ARRAY_SIZE(v210_bt_on), v210_bt_on }
		}
	};

	on = on ? 1 : 0;

	rc = marimba_read_bit_mask(&config, 0x11,  &version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: version read failed: %d\n",
			__func__, rc);
		return rc;
	}

	if ((version >= ARRAY_SIZE(bt_marimba[on])) ||
	    (bt_marimba[on][version].size == 0)) {
		printk(KERN_ERR
			"%s: unsupported version\n",
			__func__);
		return -EIO;
	}

	p = bt_marimba[on][version].set;

	printk(KERN_INFO "%s: found version %d\n", __func__, version);

	for (i = 0; i < bt_marimba[on][version].size; i++) {
		u8 value = (p+i)->value;
		rc = marimba_write_bit_mask(&config,
			(p+i)->reg,
			&value,
			sizeof((p+i)->value),
			(p+i)->mask);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: reg %d write failed: %d\n",
				__func__, (p+i)->reg, rc);
			return rc;
		}
		printk(KERN_INFO "%s: reg 0x%02x value 0x%02x mask 0x%02x\n",
				__func__, (p+i)->reg,
				value, (p+i)->mask);
	}
	return 0;
}

static int bahama_bt(int on)
{
	int rc;
	int i;
	struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };

	struct bahama_variant_register {
		const size_t size;
		const struct bahama_config_register *set;
	};

	const struct bahama_config_register *p;


	const struct bahama_config_register v10_bt_on[] = {
		{ 0xE9, 0x00, 0xFF },
		{ 0xF4, 0x80, 0xFF },
		{ 0xF0, 0x06, 0xFF },
		{ 0xE4, 0x00, 0xFF },
		{ 0xE5, 0x00, 0x0F },
#ifdef CONFIG_WLAN
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0x11, 0x13, 0xFF },
		{ 0xE9, 0x21, 0xFF },
		{ 0x01, 0x0C, 0x1F },
		{ 0x01, 0x08, 0x1F },
	};

	const struct bahama_config_register v20_bt_on_fm_off[] = {
		{ 0x11, 0x0C, 0xFF },
		{ 0x13, 0x01, 0xFF },
		{ 0xF4, 0x80, 0xFF },
		{ 0xF0, 0x00, 0xFF },
		{ 0xE9, 0x00, 0xFF },
#ifdef CONFIG_WLAN
		{ 0x81, 0x00, 0xFF },
		{ 0x82, 0x00, 0xFF },
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0xE9, 0x21, 0xFF }
	};

	const struct bahama_config_register v20_bt_on_fm_on[] = {
		{ 0x11, 0x0C, 0xFF },
		{ 0x13, 0x01, 0xFF },
		{ 0xF4, 0x86, 0xFF },
		{ 0xF0, 0x06, 0xFF },
		{ 0xE9, 0x00, 0xFF },
#ifdef CONFIG_WLAN
		{ 0x81, 0x00, 0xFF },
		{ 0x82, 0x00, 0xFF },
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0xE9, 0x21, 0xFF }
	};

	const struct bahama_config_register v10_bt_off[] = {
		{ 0xE9, 0x00, 0xFF },
	};

	const struct bahama_config_register v20_bt_off_fm_off[] = {
		{ 0xF4, 0x84, 0xFF },
		{ 0xF0, 0x04, 0xFF },
		{ 0xE9, 0x00, 0xFF }
	};

	const struct bahama_config_register v20_bt_off_fm_on[] = {
		{ 0xF4, 0x86, 0xFF },
		{ 0xF0, 0x06, 0xFF },
		{ 0xE9, 0x00, 0xFF }
	};

	const struct bahama_variant_register bt_bahama[2][3] = {
		{
			{ ARRAY_SIZE(v10_bt_off), v10_bt_off },
			{ ARRAY_SIZE(v20_bt_off_fm_off), v20_bt_off_fm_off },
			{ ARRAY_SIZE(v20_bt_off_fm_on), v20_bt_off_fm_on }
		},
		{
			{ ARRAY_SIZE(v10_bt_on), v10_bt_on },
			{ ARRAY_SIZE(v20_bt_on_fm_off), v20_bt_on_fm_off },
			{ ARRAY_SIZE(v20_bt_on_fm_on), v20_bt_on_fm_on }
		}
	};

	u8 offset = 0; /* index into bahama configs */

	on = on ? 1 : 0;


	if (bahama_version == VER_2_0) {
		if (marimba_get_fm_status(&config))
			offset = 0x01;
	}

	p = bt_bahama[on][bahama_version + offset].set;

	dev_info(&msm_bt_power_device.dev,
		"%s: found version %d\n", __func__, bahama_version);

	for (i = 0; i < bt_bahama[on][bahama_version + offset].size; i++) {
		u8 value = (p+i)->value;
		rc = marimba_write_bit_mask(&config,
			(p+i)->reg,
			&value,
			sizeof((p+i)->value),
			(p+i)->mask);
		if (rc < 0) {
			dev_err(&msm_bt_power_device.dev,
				"%s: reg %d write failed: %d\n",
				__func__, (p+i)->reg, rc);
			return rc;
		}
		dev_info(&msm_bt_power_device.dev,
			"%s: reg 0x%02x write value 0x%02x mask 0x%02x\n",
				__func__, (p+i)->reg,
				value, (p+i)->mask);
	}
	/* Update BT status */
	if (on)
		marimba_set_bt_status(&config, true);
	else
		marimba_set_bt_status(&config, false);

	return 0;
}

static int bluetooth_regs_init(int bahama_not_marimba)
{
	int rc = 0;
	struct device *const dev = &msm_bt_power_device.dev;

	if (bahama_not_marimba) {
		bahama_version = read_bahama_ver();

		switch (bahama_version) {
		case VER_1_0:
			regs_bt = regs_bt_bahama_v1;
			regs_bt_count = ARRAY_SIZE(regs_bt_bahama_v1);
			break;
		case VER_2_0:
			regs_bt = regs_bt_bahama_v2;
			regs_bt_count = ARRAY_SIZE(regs_bt_bahama_v2);
			break;
		case VER_UNSUPPORTED:
		default:
			dev_err(dev,
				"%s: i2c failure or unsupported version: %d\n",
				__func__, bahama_version);
			rc = -EIO;
			goto out;
		}
	} else {
		regs_bt = regs_bt_marimba;
		regs_bt_count = ARRAY_SIZE(regs_bt_marimba);
	}

	rc = regulator_bulk_get(&msm_bt_power_device.dev,
			regs_bt_count, regs_bt);
	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(regs_bt_count, regs_bt);
	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	return 0;

reg_free:
	regulator_bulk_free(regs_bt_count, regs_bt);
out:
	regs_bt_count = 0;
	regs_bt = NULL;
	return rc;
}

static int bluetooth_power(int on)
{
	int rc;
	const char *id = "BTPW";

	int bahama_not_marimba = bahama_present();

	if (bahama_not_marimba == -1) {
		printk(KERN_WARNING "%s: bahama_present: %d\n",
				__func__, bahama_not_marimba);
		return -ENODEV;
	}

	if (unlikely(regs_bt_count == 0)) {
		rc = bluetooth_regs_init(bahama_not_marimba);
		if (rc)
			return rc;
	}

	if (on) {
		rc = regulator_bulk_enable(regs_bt_count, regs_bt);
		if (rc)
			return rc;

		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_ON);
		if (rc < 0)
			return -EIO;

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa()) {
					rc = marimba_gpio_config(1);
					if (rc < 0)
						return -EIO;
		}

		rc = (bahama_not_marimba ? bahama_bt(on) : marimba_bt(on));
		if (rc < 0)
			return -EIO;

		msleep(10);

		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_PIN_CTRL);
		if (rc < 0)
			return -EIO;

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa()) {
					rc = marimba_gpio_config(0);
					if (rc < 0)
						return -EIO;
		}

		rc = msm_gpios_enable(bt_config_power_on,
			ARRAY_SIZE(bt_config_power_on));

		if (rc < 0)
			return rc;

	} else {
		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0)
			return rc;

		/* check for initial RFKILL block (power off) */
		if (platform_get_drvdata(&msm_bt_power_device) == NULL)
			goto out;

		rc = (bahama_not_marimba ? bahama_bt(on) : marimba_bt(on));
		if (rc < 0)
			return -EIO;

		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
		if (rc < 0)
			return -EIO;

		rc = regulator_bulk_disable(regs_bt_count, regs_bt);
		if (rc)
			return rc;

	}

out:
	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}
static void __init bt_power_init(void)
{
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#endif
#ifdef CONFIG_BCM_BT

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(134, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Tx" },
	{ GPIO_CFG(39, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"BT_WAKE" },
	{ GPIO_CFG(40, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"BT_WAKE_INT" }
};

static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(134, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Tx" },
	{ GPIO_CFG(39, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"BT_WAKE" },
	{ GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"BT_WAKE_INT" }
};
static int bcm_power(int on)
{

   int rc;

   printk(KERN_DEBUG "%s\n", __func__);

   if (on) {
	  
		rc = msm_gpios_enable(bt_config_power_on,
			ARRAY_SIZE(bt_config_power_on));

		
	   

	//rc = gpio_tlmm_config(GPIO_CFG(39, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR"%s: Could not configure 39 gpio %d\n",__func__, 39);
			 return -EIO;
		   }
		

	  rc =gpio_request(39, "bt_wake");	  
	  if(!rc)
		   gpio_direction_output(39, 1);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 39);	
		gpio_free(39);	
	
		
		rc =gpio_request(38, "bt_deep_sleep");
		if(!rc)
		   gpio_direction_output(38, 0);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 38);		
		gpio_free(38);
			
	   rc =gpio_request(88, "bt_reset");
	   if(!rc)
		   gpio_direction_output(88, 0);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 88);	  
		gpio_free(88);
		
		msleep(20);		
		
		rc =gpio_request(38, "bt_deep_sleep");
		if(!rc)
		   gpio_direction_output(38, 1);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 38);		
		gpio_free(38);
			
	   rc =gpio_request(88, "bt_reset");
	   if(!rc)
		   gpio_direction_output(88, 1);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 88);		
		gpio_free(88);
				
	} else {
	  
		rc = msm_gpios_enable(bt_config_power_off,
			ARRAY_SIZE(bt_config_power_off));
		
  
		rc = gpio_request(88, "bt_reset");
		if(!rc)
		   gpio_direction_output(88, 0);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 41);
		gpio_free(88);		
		
			rc = gpio_request(38, "bt_deep_sleep");
		if(!rc)
		   gpio_direction_output(38, 0);
		else
			printk(KERN_ERR "gpio_request: %d failed!\n", 38);
		gpio_free(38);		
	}
	return 0;
}
static struct platform_device msm_bcm_power_device = {
	.name = "bcm_power",
};


static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 40,
		.end	= 40,//83
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 39,   // emmc mipi to 116
		.end	= 39,     //emmc mipi to 116
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(40),
		.end	= MSM_GPIO_TO_INT(40),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bcmsleep_device = {
	.name = "bcmsleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};
 int __init bt_power_init(void)
{
	
  int rc = 0;
	msm_bcm_power_device.dev.platform_data = &bcm_power;
	return rc;
}
#endif
#if 0

#endif
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 2800,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
};

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

static char *msm_adc_fluid_device_names[] = {
	"LTC_ADC1",
	"LTC_ADC2",
	"LTC_ADC3",
};

static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata;

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#ifdef CONFIG_MSM_SDIO_AL
static struct msm_gpio mdm2ap_status = {
	GPIO_CFG(77, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	"mdm2ap_status"
};


static int configure_mdm2ap_status(int on)
{
	if (on)
		return msm_gpios_request_enable(&mdm2ap_status, 1);
	else {
		msm_gpios_disable_free(&mdm2ap_status, 1);
		return 0;
	}
}

static int get_mdm2ap_status(void)
{
	return gpio_get_value(GPIO_PIN(mdm2ap_status.gpio_cfg));
}

static struct sdio_al_platform_data sdio_al_pdata = {
	.config_mdm2ap_status = configure_mdm2ap_status,
	.get_mdm2ap_status = get_mdm2ap_status,
	.allow_sdioc_version_major_2 = 1,
	.peer_sdioc_version_minor = 0x0001,
	.peer_sdioc_version_major = 0x0003,
	.peer_sdioc_boot_version_minor = 0x0001,
	.peer_sdioc_boot_version_major = 0x0003,
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.platform_data	= &sdio_al_pdata,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */

static struct platform_device *devices[] __initdata = {
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart2,
#endif
#ifdef CONFIG_MSM_PROC_COMM_REGULATOR
	&msm_proccomm_regulator_dev,
#endif
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
#if defined (CONFIG_SND_MSM_MVS_DAI_SOC)
	&asoc_msm_mvs,
	&asoc_mvs_dai0,
	&asoc_mvs_dai1,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	&smc91x_device,
	&smsc911x_device,
	&msm_device_nand,
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_G_ANDROID
	&android_usb_device,
#endif
	&qsd_device_spi,

#ifdef CONFIG_MSM_SSBI
	&msm_device_ssbi_pmic1,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&lcdc_panel_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
	&qup_device_i2c,

#ifdef CONFIG_LEDS_GPIO	
	&android_leds,
#endif

#ifdef CONFIG_LEDS_MSM_PMIC
	&msm_device_pmic_leds,
#endif

	&msm_kgsl_3d0,
	&msm_kgsl_2d0,

#if defined(CONFIG_MACH_ARTHUR)
#ifdef CONFIG_OV5640
    /*
     * Commented by zh.shj
     *
     * Refer to drivers/media/video/msm/ov5640.c
     * For OV5640: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     */
    &msm_camera_sensor_ov5640,
#endif
#endif // #if defined(CONFIG_MACH_ARTHUR)

	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif

	&msm_batt_device,
	&msm_adc_device,
	&msm_ebi0_thermal,
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
	&msm_ebi1_thermal,
#ifdef CONFIG_BCM_BT
	&msm_bcm_power_device, 
	&msm_bcmsleep_device,
#endif
};

int32_t msm_sensors_power_supply(void)
{
    struct vreg *vreg_l8_1p8  = NULL;
    struct vreg *vreg_l10_2p6  = NULL;

    int32_t rc_vdd, rc_iovdd;

    CDBG("%s: entry\n", __func__);
	
    vreg_l10_2p6   = vreg_get(0, "gp4");
    vreg_l8_1p8 = vreg_get(0, "gp7");


    if ((!vreg_l10_2p6) || (!vreg_l8_1p8))
    {
        printk(KERN_ERR"%s: vreg_get failed!\n", __func__);
        return -EIO;
    }

    rc_iovdd = vreg_set_level(vreg_l8_1p8, 1800);
    if (rc_iovdd)
    {
		printk(KERN_ERR"%s: vreg_set_level failed!\n", __func__);
		return -EIO;
	}

    rc_iovdd = vreg_enable(vreg_l8_1p8);
    if (rc_iovdd)
    {
        printk(KERN_ERR"%s: vreg_enable failed!\n", __func__);
        return -EIO;
    }

    mdelay(10);

	rc_vdd = vreg_set_level(vreg_l10_2p6, 2600);
    if (rc_vdd)
    {
		printk(KERN_ERR"%s: vreg_set_level failed!\n", __func__);
		return -EIO;
	}

    rc_vdd = vreg_enable(vreg_l10_2p6);
    if (rc_vdd)
    {
        printk(KERN_ERR"%s: vreg_enable failed!\n", __func__);
        return -EIO;
    }

    mdelay(10);
  
    return 0;
}
static struct msm_gpio msm_i2c_gpios_hw[] = {
	{ GPIO_CFG(70, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio msm_i2c_gpios_io[] = {
	{ GPIO_CFG(70, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(16, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};
static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(16, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};

static void
msm_i2c_gpio_config(int adap_id, int config_type)
{
	struct msm_gpio *msm_i2c_table;

	/* Each adapter gets 2 lines from the table */
	if (adap_id > 0)
		return;
	if (config_type)
		msm_i2c_table = &msm_i2c_gpios_hw[adap_id*2];
	else
		msm_i2c_table = &msm_i2c_gpios_io[adap_id*2];
	msm_gpios_enable(msm_i2c_table, 2);
}
/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
static struct regulator *qup_vreg;
#endif
static void
qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc = 0;
	struct msm_gpio *qup_i2c_table;
	/* Each adapter gets 2 lines from the table */
	if (adap_id != 4)
		return;
	if (config_type)
		qup_i2c_table = qup_i2c_gpios_hw;
	else
		qup_i2c_table = qup_i2c_gpios_io;
	rc = msm_gpios_enable(qup_i2c_table, 2);
	if (rc < 0)
		printk(KERN_ERR "QUP GPIO enable failed: %d\n", rc);
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	if (!IS_ERR_OR_NULL(qup_vreg)) {
		rc = regulator_enable(qup_vreg);
		if (rc) {
			pr_err("%s: regulator_enable failed: %d\n",
			__func__, rc);
		}
	}
#endif
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.pri_clk = 70,
	.pri_dat = 71,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000021",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (msm_gpios_request(msm_i2c_gpios_hw, ARRAY_SIZE(msm_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static struct msm_i2c_platform_data msm_i2c_2_pdata = {
	.clk_freq = 100000,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000022",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_2_init(void)
{
	msm_device_i2c_2.dev.platform_data = &msm_i2c_2_pdata;
}

static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 384000,
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};

static void __init qup_device_i2c_init(void)
{
	if (msm_gpios_request(qup_i2c_gpios_hw, ARRAY_SIZE(qup_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	qup_device_i2c.dev.platform_data = &qup_i2c_pdata;
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	qup_vreg = regulator_get(&qup_device_i2c.dev, "lvsw1");
	if (IS_ERR(qup_vreg)) {
		dev_err(&qup_device_i2c.dev,
			"%s: regulator_get failed: %ld\n",
			__func__, PTR_ERR(qup_vreg));
	}
#endif
}

#ifdef CONFIG_I2C_SSBI
static struct msm_i2c_ssbi_platform_data msm_i2c_ssbi7_pdata = {
	.rsl_id = "D:CODEC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

static struct msm_gpio msm_nand_ebi2_cfg_data[] = {
	{GPIO_CFG(86, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_cs1"},
	{GPIO_CFG(115, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_busy1"},
};

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};
#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
static struct msm_gpio sdc1_lvlshft_cfg_data[] = {
	{GPIO_CFG(35, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), "sdc1_lvlshft"},
};
#endif
static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(38, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc1_clk"},
	{GPIO_CFG(39, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_cmd"},
	{GPIO_CFG(40, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_0"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(64, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc2_clk"},
	{GPIO_CFG(65, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_cmd"},
	{GPIO_CFG(66, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_3"},
	{GPIO_CFG(67, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_2"},
	{GPIO_CFG(68, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_1"},
	{GPIO_CFG(69, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_0"},

#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{GPIO_CFG(115, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_4"},
	{GPIO_CFG(114, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_5"},
	{GPIO_CFG(113, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_6"},
	{GPIO_CFG(112, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_7"},
#endif
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc3_clk"},
	{GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_0"},
};

static struct msm_gpio sdc3_sleep_cfg_data[] = {
	{GPIO_CFG(110, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_clk"},
	{GPIO_CFG(111, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_cmd"},
	{GPIO_CFG(116, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_3"},
	{GPIO_CFG(117, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_2"},
	{GPIO_CFG(118, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_1"},
	{GPIO_CFG(119, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_0"},
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(58, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc4_clk"},
	{GPIO_CFG(59, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_cmd"},
	{GPIO_CFG(60, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_0"},
};

static struct msm_gpio sdc4_sleep_cfg_data[] = {
	{GPIO_CFG(58, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_clk"},
	{GPIO_CFG(59, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_cmd"},
	{GPIO_CFG(60, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_0"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
		.sleep_cfg_data = sdc3_sleep_cfg_data,
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
		.sleep_cfg_data = sdc4_sleep_cfg_data,
	},
};

static struct regulator *sdcc_vreg_data[ARRAY_SIZE(sdcc_cfg_data)];

static unsigned long vreg_sts, gpio_sts;

static uint32_t msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
		} else {
			msm_gpios_disable_free(curr->cfg_data, curr->size);
		}
	}

	return rc;
}

static uint32_t msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct regulator *curr = sdcc_vreg_data[dev_id - 1];
	static int enabled_once[] = {0, 0, 0, 0};

	if (test_bit(dev_id, &vreg_sts) == enable)
		return rc;

	if (!enable || enabled_once[dev_id - 1])
			return 0;
	
	if (!curr)
		return -ENODEV;

	if (IS_ERR(curr))
		return PTR_ERR(curr);

	if (enable) {
		
		set_bit(dev_id, &vreg_sts);

		rc = regulator_enable(curr);
		if (rc)
			pr_err("%s: could not enable regulator: %d\n",
					__func__, rc);
		enabled_once[dev_id - 1] = 1;
	} else {
		clear_bit(dev_id, &vreg_sts);

		rc = regulator_disable(curr);
		if (rc)
			pr_err("%s: could not disable regulator: %d\n",
					__func__, rc);
		
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_gpio(pdev->id, (vdd ? 1 : 0));
	if (rc)
		goto out;

	if (pdev->id == 4) {/* S3 is always ON and cannot be disabled */
		rc = msm_sdcc_setup_vreg(pdev->id, (vdd ? 1 : 0));

	 }
out:
	return rc;
}

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT) && \
	defined(CONFIG_CSDIO_VENDOR_ID) && \
	defined(CONFIG_CSDIO_DEVICE_ID) && \
	(CONFIG_CSDIO_VENDOR_ID == 0x70 && CONFIG_CSDIO_DEVICE_ID == 0x1117)

#define MBP_ON  1
#define MBP_OFF 0

#define MBP_RESET_N \
	GPIO_CFG(44, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA)
#define MBP_INT0 \
	GPIO_CFG(46, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA)

#define MBP_MODE_CTRL_0 \
	GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define MBP_MODE_CTRL_1 \
	GPIO_CFG(36, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define MBP_MODE_CTRL_2 \
	GPIO_CFG(34, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)

static struct msm_gpio mbp_cfg_data[] = {
	{GPIO_CFG(44, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"mbp_reset"},
	{GPIO_CFG(85, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"mbp_io_voltage"},
};

static int mbp_config_gpios_pre_init(int enable)
{
	int rc = 0;

	if (enable) {
		rc = msm_gpios_request_enable(mbp_cfg_data,
			ARRAY_SIZE(mbp_cfg_data));
		if (rc) {
			printk(KERN_ERR
				"%s: Failed to turnon GPIOs for mbp chip(%d)\n",
				__func__, rc);
		}
	} else
		msm_gpios_disable_free(mbp_cfg_data, ARRAY_SIZE(mbp_cfg_data));
	return rc;
}

static struct regulator_bulk_data mbp_regs_io[2];
static struct regulator_bulk_data mbp_regs_rf[2];
static struct regulator_bulk_data mbp_regs_adc[1];
static struct regulator_bulk_data mbp_regs_core[1];

static int mbp_init_regs(struct device *dev)
{
	struct regulator_bulk_data regs[] = {
		/* Analog and I/O regs */
		{ .supply = "gp4",  .min_uV = 2600000, .max_uV = 2600000 },
		{ .supply = "s3",   .min_uV = 1800000, .max_uV = 1800000 },
		/* RF regs */
		{ .supply = "s2",   .min_uV = 1300000, .max_uV = 1300000 },
		{ .supply = "rf",   .min_uV = 2600000, .max_uV = 2600000 },
		/* ADC regs */
		{ .supply = "s4",   .min_uV = 2200000, .max_uV = 2200000 },
		/* Core regs */
		{ .supply = "gp16", .min_uV = 1200000, .max_uV = 1200000 },
	};

	struct regulator_bulk_data *regptr = regs;
	int rc;

	rc = regulator_bulk_get(dev, ARRAY_SIZE(regs), regs);

	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs), regs);

	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	memcpy(mbp_regs_io, regptr, sizeof(mbp_regs_io));
	regptr += ARRAY_SIZE(mbp_regs_io);

	memcpy(mbp_regs_rf, regptr, sizeof(mbp_regs_rf));
	regptr += ARRAY_SIZE(mbp_regs_rf);

	memcpy(mbp_regs_adc, regptr, sizeof(mbp_regs_adc));
	regptr += ARRAY_SIZE(mbp_regs_adc);

	memcpy(mbp_regs_core, regptr, sizeof(mbp_regs_core));

	return 0;

reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs), regs);
out:
	return rc;
}

static int mbp_setup_rf_vregs(int state)
{
	return state ?
		regulator_bulk_enable(ARRAY_SIZE(mbp_regs_rf), mbp_regs_rf) :
		regulator_bulk_disable(ARRAY_SIZE(mbp_regs_rf), mbp_regs_rf);
}

static int mbp_setup_vregs(int state)
{
	return state ?
		regulator_bulk_enable(ARRAY_SIZE(mbp_regs_io), mbp_regs_io) :
		regulator_bulk_disable(ARRAY_SIZE(mbp_regs_io), mbp_regs_io);
}

static int mbp_set_tcxo_en(int enable)
{
	int rc;
	const char *id = "UBMC";
	struct vreg *vreg_analog = NULL;

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A1,
		enable ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0) {
		printk(KERN_ERR "%s: unable to %svote for a1 clk\n",
			__func__, enable ? "" : "de-");
		return -EIO;
	}
	return rc;
}

static void mbp_set_freeze_io(int state)
{
	if (state)
		gpio_set_value(85, 0);
	else
		gpio_set_value(85, 1);
}

static int mbp_set_core_voltage_en(int enable)
{
	static bool is_enabled;
	int rc = 0;

	if (enable && !is_enabled) {
		rc = regulator_bulk_enable(ARRAY_SIZE(mbp_regs_core),
				mbp_regs_core);
		if (rc) {
			pr_err("%s: could not enable regulators: %d\n",
					__func__, rc);
		} else {
			is_enabled = true;
		}
	}

	return rc;
}

static void mbp_set_reset(int state)
{
	if (state)
		gpio_set_value(GPIO_PIN(MBP_RESET_N), 0);
	else
		gpio_set_value(GPIO_PIN(MBP_RESET_N), 1);
}

static int mbp_config_interface_mode(int state)
{
	if (state) {
		gpio_tlmm_config(MBP_MODE_CTRL_0, GPIO_CFG_ENABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_1, GPIO_CFG_ENABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_2, GPIO_CFG_ENABLE);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_0), 0);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_1), 1);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_2), 0);
	} else {
		gpio_tlmm_config(MBP_MODE_CTRL_0, GPIO_CFG_DISABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_1, GPIO_CFG_DISABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_2, GPIO_CFG_DISABLE);
	}
	return 0;
}

static int mbp_setup_adc_vregs(int state)
{
	return state ?
		regulator_bulk_enable(ARRAY_SIZE(mbp_regs_adc), mbp_regs_adc) :
		regulator_bulk_disable(ARRAY_SIZE(mbp_regs_adc), mbp_regs_adc);
}

static int mbp_power_up(void)
{
	int rc;

	rc = mbp_config_gpios_pre_init(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_config_gpios_pre_init() done\n", __func__);

	rc = mbp_setup_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: gp4 (2.6) and s3 (1.8) done\n", __func__);

	rc = mbp_set_tcxo_en(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: tcxo clock done\n", __func__);

	mbp_set_freeze_io(MBP_OFF);
	pr_debug("%s: set gpio 85 to 1 done\n", __func__);

	udelay(100);
	mbp_set_reset(MBP_ON);

	udelay(300);
	rc = mbp_config_interface_mode(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_config_interface_mode() done\n", __func__);

	udelay(100 + mbp_set_core_voltage_en(MBP_ON));
	pr_debug("%s: power gp16 1.2V done\n", __func__);

	mbp_set_freeze_io(MBP_ON);
	pr_debug("%s: set gpio 85 to 0 done\n", __func__);

	udelay(100);

	rc = mbp_setup_rf_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: s2 1.3V and rf 2.6V done\n", __func__);

	rc = mbp_setup_adc_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: s4 2.2V  done\n", __func__);

	udelay(200);

	mbp_set_reset(MBP_OFF);
	pr_debug("%s: close gpio 44 done\n", __func__);

	msleep(20);
exit:
	return rc;
}

static int mbp_power_down(void)
{
	int rc;

	mbp_set_reset(MBP_ON);
	pr_debug("%s: mbp_set_reset(MBP_ON) done\n", __func__);

	udelay(100);

	rc = mbp_setup_adc_vregs(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: vreg_disable(vreg_adc) done\n", __func__);

	udelay(5);

	rc = mbp_setup_rf_vregs(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_setup_rf_vregs(MBP_OFF) done\n", __func__);

	udelay(5);

	mbp_set_freeze_io(MBP_OFF);
	pr_debug("%s: mbp_set_freeze_io(MBP_OFF) done\n", __func__);

	udelay(100);
	rc = mbp_set_core_voltage_en(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_set_core_voltage_en(MBP_OFF) done\n", __func__);

	rc = mbp_set_tcxo_en(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_set_tcxo_en(MBP_OFF) done\n", __func__);

	rc = mbp_setup_vregs(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_setup_vregs(MBP_OFF) done\n", __func__);

	rc = mbp_config_gpios_pre_init(MBP_OFF);
	if (rc)
		goto exit;
exit:
	return rc;
}

static void (*mbp_status_notify_cb)(int card_present, void *dev_id);
static void *mbp_status_notify_cb_devid;
static int mbp_power_status;
static int mbp_power_init_done;

static uint32_t mbp_setup_power(struct device *dv,
	unsigned int power_status)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);

	if (power_status == mbp_power_status)
		goto exit;
	if (power_status) {
		pr_debug("turn on power of mbp slot");
		rc = mbp_power_up();
		mbp_power_status = 1;
	} else {
		pr_debug("turn off power of mbp slot");
		rc = mbp_power_down();
		mbp_power_status = 0;
	}
exit:
	return rc;
};

int mbp_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	mbp_status_notify_cb = callback;
	mbp_status_notify_cb_devid = dev_id;
	return 0;
}

static unsigned int mbp_status(struct device *dev)
{
	return mbp_power_status;
}

static uint32_t msm_sdcc_setup_power_mbp(struct device *dv, unsigned int vdd)
{
	struct platform_device *pdev;
	uint32_t rc = 0;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_power(dv, vdd);
	if (rc) {
		pr_err("%s: Failed to setup power (%d)\n",
			__func__, rc);
		goto out;
	}
	if (!mbp_power_init_done) {
		rc = mbp_init_regs(dv);
		if (rc) {
			dev_err(dv, "%s: regulator init failed: %d\n",
					__func__, rc);
			goto out;
		}
		mbp_setup_power(dv, 1);
		mbp_setup_power(dv, 0);
		mbp_power_init_done = 1;
	}
	if (vdd >= 0x8000) {
		rc = mbp_setup_power(dv, (0x8000 == vdd) ? 0 : 1);
		if (rc) {
			pr_err("%s: Failed to config mbp chip power (%d)\n",
				__func__, rc);
			goto out;
		}
		if (mbp_status_notify_cb) {
			mbp_status_notify_cb(mbp_power_status,
				mbp_status_notify_cb_devid);
		}
	}
out:
	/* should return 0 only */
	return 0;
}

#endif

#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm7x30_sdcc_slot_status(struct device *dev)
{ 
	return (unsigned int)
		!(gpio_get_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SD_DET - 1)));
}
#endif

static int msm_sdcc_get_wpswitch(struct device *dv)
{
	void __iomem *wp_addr = 0;
	uint32_t ret = 0;
	struct platform_device *pdev;

	if (!(machine_is_msm7x30_surf()))
		return -1;
	pdev = container_of(dv, struct platform_device, dev);

	wp_addr = ioremap(FPGA_SDCC_STATUS, 4);
	if (!wp_addr) {
		pr_err("%s: Could not remap %x\n", __func__, FPGA_SDCC_STATUS);
		return -ENOMEM;
	}

	ret = (((readl(wp_addr) >> 4) >> (pdev->id-1)) & 0x01);
	pr_info("%s: WP Status for Slot %d = 0x%x \n", __func__,
							pdev->id, ret);
	iounmap(wp_addr);

	return ret;
}
#endif

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
#if defined(CONFIG_CSDIO_VENDOR_ID) && \
	defined(CONFIG_CSDIO_DEVICE_ID) && \
	(CONFIG_CSDIO_VENDOR_ID == 0x70 && CONFIG_CSDIO_DEVICE_ID == 0x1117)
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power_mbp,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.status	        = mbp_status,
	.register_status_notify = mbp_register_status_notify,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 24576000,
	.nonremovable	= 0,
};
#else
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif
#endif



int zte_wifi_status_register(
                        void (*callback)(int card_present, void *dev_id),
                        void *dev_id);

unsigned int zte_wifi_status(struct device *dev);

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x30_sdc2_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x30_sdc3_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
#ifdef CONFIG_ATH_WIFI
#ifdef CONFIG_WOW_BY_SDIO_DATA1
	.sdiowakeup_irq = MSM_GPIO_TO_INT(118),
#endif
#endif
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
	.status                 = zte_wifi_status,
    .register_status_notify = zte_wifi_status_register,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data msm7x30_sdc4_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x30_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, PMIC_GPIO_SD_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.wpswitch    = msm_sdcc_get_wpswitch,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static int msm_sdc1_lvlshft_enable(void)
{
	static struct regulator *ldo5;
	int rc;

	/* Enable LDO5, an input to the FET that powers slot 1 */

	ldo5 = regulator_get(NULL, "ldo5");

	if (IS_ERR(ldo5)) {
		rc = PTR_ERR(ldo5);
		pr_err("%s: could not get ldo5: %d\n", __func__, rc);
		goto out;
	}

	rc = regulator_set_voltage(ldo5, 2850000, 2850000);
	if (rc) {
		pr_err("%s: could not set ldo5 voltage: %d\n", __func__, rc);
		goto ldo5_free;
	}

	rc = regulator_enable(ldo5);
	if (rc) {
		pr_err("%s: could not enable ldo5: %d\n", __func__, rc);
		goto ldo5_free;
	}

	/* Enable GPIO 35, to turn on the FET that powers slot 1 */
	rc = msm_gpios_request_enable(sdc1_lvlshft_cfg_data,
				ARRAY_SIZE(sdc1_lvlshft_cfg_data));
	if (rc)
		printk(KERN_ERR "%s: Failed to enable GPIO 35\n", __func__);

	rc = gpio_direction_output(GPIO_PIN(sdc1_lvlshft_cfg_data[0].gpio_cfg),
				1);
	if (rc)
		printk(KERN_ERR "%s: Failed to turn on GPIO 35\n", __func__);

	return 0;

ldo5_free:
	regulator_put(ldo5);
out:
	ldo5 = NULL;
	return rc;
}
#endif

static int mmc_regulator_init(int sdcc_no, const char *supply, int uV)
{
	int rc;

	BUG_ON(sdcc_no < 1 || sdcc_no > 4);

	sdcc_no--;

	sdcc_vreg_data[sdcc_no] = regulator_get(NULL, supply);

	if (IS_ERR(sdcc_vreg_data[sdcc_no])) {
		rc = PTR_ERR(sdcc_vreg_data[sdcc_no]);
		pr_err("%s: could not get regulator \"%s\": %d\n",
				__func__, supply, rc);
		goto out;
	}

	rc = regulator_set_voltage(sdcc_vreg_data[sdcc_no], uV, uV);

	if (rc) {
		pr_err("%s: could not set voltage for \"%s\" to %d uV: %d\n",
				__func__, supply, uV, rc);
		goto reg_free;
	}

	return rc;

reg_free:
	regulator_put(sdcc_vreg_data[sdcc_no]);
out:
	sdcc_vreg_data[sdcc_no] = NULL;
	return rc;
}

static void __init msm7x30_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (mmc_regulator_init(1, "s3", 1800000))
		goto out1;
	msm7x30_sdc1_data.swfi_latency = msm7x30_power_collapse_latency(
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT);

	if (machine_is_msm7x30_fluid()) {
		msm7x30_sdc1_data.ocr_mask =  MMC_VDD_27_28 | MMC_VDD_28_29;
		if (msm_sdc1_lvlshft_enable()) {
			pr_err("%s: could not enable level shift\n");
			goto out1;
		}
	}

	msm_add_sdcc(1, &msm7x30_sdc1_data);
out1:
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (mmc_regulator_init(2, "s3", 1800000))
		goto out2;
	msm7x30_sdc2_data.swfi_latency = msm7x30_power_collapse_latency(
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT);

	if (machine_is_msm8x55_svlte_surf())
		msm7x30_sdc2_data.msmsdcc_fmax =  24576000;
	if (machine_is_msm8x55_svlte_surf() ||
			machine_is_msm8x55_svlte_ffa()) {
		msm7x30_sdc2_data.sdiowakeup_irq = MSM_GPIO_TO_INT(68);
		msm7x30_sdc2_data.is_sdio_al_client = 1;
	}

	msm_add_sdcc(2, &msm7x30_sdc2_data);
out2:
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	if (mmc_regulator_init(3, "s3", 1800000))
		goto out3;
	msm7x30_sdc3_data.swfi_latency = msm7x30_power_collapse_latency(
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT);

	msm_sdcc_setup_gpio(3, 1);
	msm_add_sdcc(3, &msm7x30_sdc3_data);
out3:
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	if (mmc_regulator_init(4, "mmc", 2850000))
		return;
	msm7x30_sdc4_data.swfi_latency = msm7x30_power_collapse_latency(
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT);

	msm_add_sdcc(4, &msm7x30_sdc4_data);
#endif

}

static void __init msm7x30_init_nand(void)
{
	char *build_id;
	struct flash_platform_data *plat_data;

	build_id = socinfo_get_build_id();
	if (build_id == NULL) {
		pr_err("%s: Build ID not available from socinfo\n", __func__);
		return;
	}

	if (build_id[8] == 'C' &&
			!msm_gpios_request_enable(msm_nand_ebi2_cfg_data,
			ARRAY_SIZE(msm_nand_ebi2_cfg_data))) {
		plat_data = msm_device_nand.dev.platform_data;
		plat_data->interleave = 1;
		printk(KERN_INFO "%s: Interleave mode Build ID found\n",
			__func__);
	}
}

#ifdef CONFIG_SERIAL_MSM_CONSOLE
static struct msm_gpio uart2_config_data[] = {
	{ GPIO_CFG(49, 2, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_RFR"},
	{ GPIO_CFG(50, 2, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_CTS"},
	{ GPIO_CFG(51, 2, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Rx"},
	{ GPIO_CFG(52, 2, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Tx"},
};

static void msm7x30_init_uart2(void)
{
	msm_gpios_request_enable(uart2_config_data,
			ARRAY_SIZE(uart2_config_data));

}
#endif

static struct msm_spm_platform_data msm_spm_data __initdata = {
	.reg_base_addr = MSM_SAW_BASE,

	.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x05,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x18,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x00006666,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFF000666,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x03,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

	.awake_vlevel = 0xF2,
	.retention_vlevel = 0xE0,
	.collapse_vlevel = 0x72,
	.retention_mid_vlevel = 0xE0,
	.collapse_mid_vlevel = 0xE0,

	.vctl_timeout_us = 50,
};

static struct regulator_bulk_data regs_isa1200[] = {
	{ .supply = "gp7",  .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "gp10", .min_uV = 2600000, .max_uV = 2600000 },
};

static int isa1200_power(int vreg_on)
{
	int rc = 0;

	rc = vreg_on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_isa1200), regs_isa1200) :
		regulator_bulk_disable(ARRAY_SIZE(regs_isa1200), regs_isa1200);

	if (rc) {
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, vreg_on ? "en" : "dis", rc);
		goto out;
	}

	/* vote for DO buffer */
	rc = pmapp_clock_vote("VIBR", PMAPP_CLOCK_ID_DO,
		vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	if (rc)	{
		pr_err("%s: unable to %svote for d0 clk\n",
			__func__, vreg_on ? "" : "de-");
		goto vreg_fail;
	}

	return 0;

vreg_fail:
	if (vreg_on)
		regulator_bulk_disable(ARRAY_SIZE(regs_isa1200), regs_isa1200);
	else
		regulator_bulk_enable(ARRAY_SIZE(regs_isa1200), regs_isa1200);
out:
	return rc;
}

static int isa1200_dev_setup(bool enable)
{
	int rc;

	if (enable == true) {
		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_isa1200),
				regs_isa1200);

		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto out;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_isa1200),
				regs_isa1200);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto reg_free;
		}

		rc = gpio_tlmm_config(GPIO_CFG(HAP_LVL_SHFT_MSM_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: Could not configure gpio %d\n",
					__func__, HAP_LVL_SHFT_MSM_GPIO);
			goto reg_free;
		}

		rc = gpio_request(HAP_LVL_SHFT_MSM_GPIO, "haptics_shft_lvl_oe");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, HAP_LVL_SHFT_MSM_GPIO, rc);
			goto reg_free;
		}

		gpio_set_value(HAP_LVL_SHFT_MSM_GPIO, 1);
	} else {
		regulator_bulk_free(ARRAY_SIZE(regs_isa1200), regs_isa1200);
		gpio_free(HAP_LVL_SHFT_MSM_GPIO);
	}

	return 0;

reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs_isa1200), regs_isa1200);
out:
	return rc;
}
static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.power_on = isa1200_power,
	.dev_setup = isa1200_dev_setup,
	.pwm_ch_id = 1, /*channel id*/
	/*gpio to enable haptic*/
	.hap_en_gpio = PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
	.hap_len_gpio = -1,
	.max_timeout = 15000,
	.mode_ctrl = PWM_GEN_MODE,
	.pwm_fd = {
		.pwm_div = 256,
	},
	.is_erm = false,
	.smart_en = true,
	.ext_clk_en = true,
	.chip_en = 1,
};

static struct i2c_board_info msm_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};


static int kp_flip_mpp_config(void)
{
	struct pm8xxx_mpp_config_data kp_flip_mpp = {
		.type = PM8XXX_MPP_TYPE_D_INPUT,
		.level = PM8018_MPP_DIG_LEVEL_S3,
		.control = PM8XXX_MPP_DIN_TO_INT,
	};

	return pm8xxx_mpp_config(PM8058_MPP_PM_TO_SYS(PM_FLIP_MPP),
						&kp_flip_mpp);
}

static struct flip_switch_pdata flip_switch_data = {
	.name = "kp_flip_switch",
	.flip_gpio = PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS) + PM_FLIP_MPP,
	.left_key = KEY_OPEN,
	.right_key = KEY_CLOSE,
	.active_low = 0,
	.wakeup = 1,
	.flip_mpp_config = kp_flip_mpp_config,
};

static struct platform_device flip_switch_device = {
	.name   = "kp_flip_switch",
	.id	= -1,
	.dev    = {
		.platform_data = &flip_switch_data,
	}
};


static ssize_t debug_global_read(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
  ssize_t size;

  size = sizeof(smem_global);

	if (pos >= size)
		return 0;

	count = min(len, (size_t)(size - pos));
	if (copy_to_user(buf, (char *)global + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static struct file_operations debug_global_file_ops = {
	.owner = THIS_MODULE,
	.read = debug_global_read,
};

extern void __init msm_init_pmic_vibrator(void); 


#ifdef CONFIG_ZTE_PLATFORM

static void set_zte_board_id_type(void)
{
       smem_global *global_tmp;
	global_tmp = ioremap(SMEM_LOG_GLOBAL_BASE, sizeof(smem_global));
	g_zte_board_id_type = global_tmp->mboard_id;
	
	printk(KERN_DEBUG "g_zte_board_id_type: %d\n",g_zte_board_id_type);

	sync_sysfs_board_id(g_zte_board_id_type);	
}

int get_zte_board_id_type(void)
{
	return g_zte_board_id_type;
}

EXPORT_SYMBOL(get_zte_board_id_type);

#endif

int wlan_init_power(void);

static void __init msm7x30_init(void)
{
	int rc;
	unsigned smem_size;
	uint32_t cam2_reset_gpio_cfg_value=0;
		
#ifdef CONFIG_ZTE_PLATFORM
  struct proc_dir_entry *entry;
#endif
	uint32_t usb_hub_gpio_cfg_value = GPIO_CFG(56,
						0,
						GPIO_CFG_OUTPUT,
						GPIO_CFG_NO_PULL,
						GPIO_CFG_2MA);
	uint32_t soc_version = 0;

	soc_version = socinfo_get_version();
	
	cam2_reset_gpio_cfg_value = GPIO_CFG(3,
						0,
						GPIO_CFG_OUTPUT,
						GPIO_CFG_PULL_DOWN,
						GPIO_CFG_2MA);	
		rc = gpio_tlmm_config(cam2_reset_gpio_cfg_value, GPIO_CFG_ENABLE);
		if (rc)
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, cam2_reset_gpio_cfg_value, rc);
	
#ifdef CONFIG_ZTE_PLATFORM
	zte_ftm_set_value(g_zte_ftm_flag_fixup);
       set_zte_board_id_type();
#endif
#ifdef CONFIG_BCM_BT
	bt_power_init();
#endif

	msm_clock_init(&msm7x30_clock_init_data);
#ifdef CONFIG_SERIAL_MSM_CONSOLE
	msm7x30_init_uart2();
#endif
	msm_spm_init(&msm_spm_data, 1);
	acpuclk_init(&acpuclk_7x30_soc_data);
	if (machine_is_msm7x30_surf() || machine_is_msm7x30_fluid())
		msm7x30_cfg_smsc911x();

#ifdef CONFIG_USB_MSM_OTG_72K
	if (SOCINFO_VERSION_MAJOR(soc_version) >= 2 &&
			SOCINFO_VERSION_MINOR(soc_version) >= 1) {
		pr_debug("%s: SOC Version:2.(1 or more)\n", __func__);
		msm_otg_pdata.ldo_set_voltage = 0;
	}

	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET
	msm_otg_pdata.swfi_latency =
 	msm_pm_data
 	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(136);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;

	if (machine_is_msm7x30_fluid()) {
		msm_adc_pdata.dev_names = msm_adc_fluid_device_names;
		msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_fluid_device_names);
	} else {
		msm_adc_pdata.dev_names = msm_adc_surf_device_names;
		msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_surf_device_names);
	}


	buses_init();

#ifdef CONFIG_MSM_SSBI
	msm_device_ssbi_pmic1.dev.platform_data =
				&msm7x30_ssbi_pm8058_pdata;
#endif

	platform_add_devices(msm_footswitch_devices,
			     msm_num_footswitch_devices);
	platform_add_devices(devices, ARRAY_SIZE(devices));
#ifdef CONFIG_USB_EHCI_MSM_72K
	msm_add_host(0, &msm_usb_host_pdata);
#endif
	wlan_init_power(); 

	msm_init_pmic_vibrator();

	msm7x30_init_mmc();
	msm7x30_init_nand();
	msm_qsd_spi_init();

#ifdef CONFIG_SPI_QSD
	if (machine_is_msm7x30_fluid())
		spi_register_board_info(lcdc_sharp_spi_board_info,
			ARRAY_SIZE(lcdc_sharp_spi_board_info));
	else
		spi_register_board_info(lcdc_toshiba_spi_board_info,
			ARRAY_SIZE(lcdc_toshiba_spi_board_info));
#endif

	atv_dac_power_init();
	hdmi_init_regs();
	msm_fb_add_devices();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	msm_device_i2c_init();
	msm_device_i2c_2_init();
	qup_device_i2c_init();
	msm7x30_init_marimba();
#ifdef CONFIG_MSM7KV2_AUDIO
	snddev_poweramp_gpio_init();
	snddev_hsed_voltage_init();
	aux_pcm_gpio_init();
#endif

	i2c_register_board_info(0, msm_i2c_board_info,
			ARRAY_SIZE(msm_i2c_board_info));

#ifdef CONFIG_INPUT_TOUCHSCREEN
	msm7x30_ts_init();
#endif

	if (!machine_is_msm8x55_svlte_ffa() && !machine_is_msm7x30_fluid())
		marimba_pdata.tsadc = &marimba_tsadc_pdata;

	i2c_register_board_info(2, msm_marimba_board_info,
			ARRAY_SIZE(msm_marimba_board_info));

	i2c_register_board_info(2, msm_i2c_gsbi7_timpani_info,
			ARRAY_SIZE(msm_i2c_gsbi7_timpani_info));

	i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo,
				ARRAY_SIZE(msm_camera_boardinfo));

	//bt_power_init();
	msm_sensors_power_supply(); 
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi7.dev.platform_data = &msm_i2c_ssbi7_pdata;
#endif
	if (machine_is_msm7x30_fluid())
		i2c_register_board_info(0, msm_isa1200_board_info,
			ARRAY_SIZE(msm_isa1200_board_info));

	if (machine_is_msm7x30_surf())
		platform_device_register(&flip_switch_device);

	pm8058_gpios_init();

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = gpio_tlmm_config(usb_hub_gpio_cfg_value, GPIO_CFG_ENABLE);
		if (rc)
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, usb_hub_gpio_cfg_value, rc);
	}

#ifdef CONFIG_ZTE_PLATFORM
	global = ioremap(SMEM_LOG_GLOBAL_BASE, sizeof(smem_global));
	if (!global) {
		printk(KERN_ERR "ioremap failed with SCL_SMEM_LOG_RAM_BASE\n");
		return;
	}
	entry = create_proc_entry("smem_global", S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		printk(KERN_ERR "smem_global: failed to create proc entry\n");
		return;
	}
	entry->proc_fops = &debug_global_file_ops;
	entry->size = sizeof(smem_global);
#endif
	boot_reason = *(unsigned int *)
		(smem_get_entry(SMEM_POWER_ON_STATUS_INFO, &smem_size));
	printk(KERN_NOTICE "Boot Reason = 0x%02x\n", boot_reason);
}

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}
early_param("fb_size", fb_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned fluid_pmem_adsp_size = MSM_FLUID_PMEM_ADSP_SIZE;
static int __init fluid_pmem_adsp_size_setup(char *p)
{
	fluid_pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("fluid_pmem_adsp_size", fluid_pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static unsigned pmem_kernel_ebi0_size = PMEM_KERNEL_EBI0_SIZE;
static int __init pmem_kernel_ebi0_size_setup(char *p)
{
	pmem_kernel_ebi0_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi0_size", pmem_kernel_ebi0_size_setup);

static struct memtype_reserve msm7x30_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
	unsigned long size;

	if machine_is_msm7x30_fluid()
		size = fluid_pmem_adsp_size;
	else
		size = pmem_adsp_size;
	android_pmem_adsp_pdata.size = size;
	android_pmem_audio_pdata.size = pmem_audio_size;
	android_pmem_pdata.size = pmem_sf_size;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7x30_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_audio_pdata);
	reserve_memory_for(&android_pmem_pdata);
	msm7x30_reserve_table[MEMTYPE_EBI0].size += pmem_kernel_ebi0_size;
#endif
}
static void __init reserve_mdp_memory(void)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	msm7x30_reserve_table[mdp_pdata.mem_hid].size += mdp_pdata.ov0_wb_size;
}

static void __init msm7x30_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_mdp_memory();
}

static int msm7x30_paddr_to_memtype(unsigned int paddr)
{
	if (paddr < phys_add)
		return MEMTYPE_EBI0;
	if (paddr >= phys_add && paddr < 0x80000000)
		return MEMTYPE_EBI1;
	return MEMTYPE_NONE;
}

static struct reserve_info msm7x30_reserve_info __initdata = {
	.memtype_reserve_table = msm7x30_reserve_table,
	.calculate_reserve_sizes = msm7x30_calculate_reserve_sizes,
	.paddr_to_memtype = msm7x30_paddr_to_memtype,
};

static void __init msm7x30_reserve(void)
{
	reserve_info = &msm7x30_reserve_info;
	msm_reserve();
}

static void __init msm7x30_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));
}

static void __init msm7x30_map_io(void)
{
	msm_shared_ram_phys = 0x00100000;
	msm_map_msm7x30_io();
	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);
}

static void __init msm7x30_init_early(void)
{
	msm7x30_allocate_memory_regions();
}

#if defined(CONFIG_ZTE_PLATFORM) && defined(CONFIG_F3_LOG)
  unsigned int len = 0;
  smem_global *global_tmp = (smem_global *)(MSM_RAM_LOG_BASE + PAGE_SIZE) ;

  len = global_tmp->f3log;

  pr_info("length for f3 log is %d ++ \n", len);

  if (len > 12)
  {
    len = 12;
  }
  else
  {
    len = len/2*2;
  }
    
  pr_info("length = %d -- \n", len);
  size = len;
    
  if (size)
  {
    reserve_bootmem(SDLOG_MEM_BASE, size * 0x100000, BOOTMEM_DEFAULT);
  }

  addr = phys_to_virt(SDLOG_MEM_BASE);
  pr_info("allocating %lu M at %p (%lx physical) for F3\n",size, addr, __pa(addr));

#endif

#ifdef CONFIG_ZTE_PLATFORM
#define ATAG_ZTEFTM 0x5d53cd73
static int  parse_tag_zteftm(const struct tag *tags)
{
	int flag = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_ZTEFTM) {
			printk(KERN_DEBUG "find the zte ftm tag\n");
			find = 1;
			break;
		}
	}

	if (find)
		flag = t->u.revision.rev;
	printk(KERN_INFO "jiangfeng:parse_tag_zteftm: zte FTM %s !\n", flag?"enable":"disable");
	return flag;
}

static void __init zte_fixup(struct machine_desc *desc, struct tag *tags,
				  char **cmdline, struct meminfo *mi)
{
	g_zte_ftm_flag_fixup = parse_tag_zteftm((const struct tag *)tags);
}

int get_ftm_from_tag(void)
{
	return g_zte_ftm_flag_fixup;
}
EXPORT_SYMBOL(get_ftm_from_tag);

#endif

MACHINE_START(ARTHUR, "arthur")
	.boot_params = PLAT_PHYS_OFFSET + 0x100,
#ifdef CONFIG_ZTE_PLATFORM
		.fixup			= zte_fixup, 
#endif
	.map_io = msm7x30_map_io,
	.reserve = msm7x30_reserve,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
	.init_early = msm7x30_init_early,
	.handle_irq = vic_handle_irq,
MACHINE_END
