/* arch/arm/mach-msm/board-beect.c
 * Copyright (C) 2010 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c-msm.h>
#include <linux/irq.h>
#include <linux/leds.h>
#include <linux/switch.h>
#include <linux/atmel_qt602240.h>
#include <linux/bma150.h>
#include <linux/capella_cm3602.h>
#include <linux/sysdev.h>
#include <linux/android_pmem.h>

#include <mach/board.h>
#include <mach/camera.h>

#include <linux/delay.h>
#include <linux/gpio.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/system.h>
#include <mach/system.h>
#include <mach/vreg.h>
#include <mach/msm_serial_debugger.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_microp.h>

#include <asm/io.h>
#include <asm/delay.h>
#include <asm/setup.h>

#include <linux/gpio_event.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/mmc.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/curcial_oj.h>
#include "board-beect.h"
#include "proc_comm.h"
#include "gpio_chip.h"

#include <mach/board_htc.h>
#include <mach/msm_serial_hs.h>

#include "devices.h"

#include <mach/atmega_microp.h>
#include <mach/msm_tssc.h>
#include <mach/htc_battery.h>

#include <mach/htc_pwrsink.h>
#include <mach/perflock.h>
#include <mach/drv_callback.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_iomap.h>
#include <mach/msm_flashlight.h>
#include <mach/msm_hsusb.h>
#include <mach/htc_usb.h>

void msm_init_irq(void);
void msm_init_gpio(void);
void msm_init_pmic_vibrator(void);
void config_beect_camera_on_gpios(void);
void config_beect_camera_off_gpios(void);
#ifdef CONFIG_MICROP_COMMON
void __init beect_microp_init(void);
#endif

static int beect_phy_init_seq[] = {0x2C, 0x31, 0x20, 0x32, 0x1, 0x0D, 0x1, 0x10, -1};
#ifdef CONFIG_USB_ANDROID
static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.phy_init_seq		= beect_phy_init_seq,
	.usb_id_pin_gpio =  BEECT_GPIO_USB_ID_PIN,
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "HTC",
	.product	= "Android Phone",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x0bb4,
	.product_id	= 0x0cbd,
	.version	= 0x0100,
	.product_name		= "Android Phone",
	.manufacturer_name	= "HTC",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
static void beect_add_usb_devices(void)
{
	android_usb_pdata.products[0].product_id =
		android_usb_pdata.product_id;
	android_usb_pdata.serial_number = board_serialno();
	msm_hsusb_pdata.serial_number = board_serialno();
	msm_device_hsusb.dev.platform_data = &msm_hsusb_pdata;
	platform_device_register(&msm_device_hsusb);
	platform_device_register(&usb_mass_storage_device);
	platform_device_register(&android_usb_device);
}
#endif

void config_beect_proximity_gpios(int on);

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = GUAGE_MODEM,
	.charger = SWITCH_CHARGER,
	.m2a_cable_detect = 1,
	.int_data.chg_int = MSM_GPIO_TO_INT(BEECT_GPIO_CHG_INT),
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev	= {
		.platform_data = &htc_battery_pdev_data,
	},
};

static int capella_cm3602_power(int pwr_device, uint8_t enable);
static struct microp_function_config microp_functions[] = {
	{
		.name   = "microp_intrrupt",
		.category = MICROP_FUNCTION_INTR,
	},
	{
		.name   = "reset-int",
		.category = MICROP_FUNCTION_RESET_INT,
		.int_pin = 1 << 8,
	},
	{
		.name   = "oj",
		.category = MICROP_FUNCTION_OJ,
		.int_pin = 1 << 12,
	},
};
static void curcial_oj_shutdown (int	enable)
{
/*
	uint8_t	cmd[3];
	memset(cmd, 0x00, sizeof(uint8_t)*3);

	cmd[2] = 0x80;
	if (enable)
		microp_i2c_write(0x91, cmd,	3);
	else
		microp_i2c_write(0x90, cmd,	3);
*/

}

static int curcial_oj_poweron(int	on)
{

	struct vreg	*oj_power = vreg_get(0, "wlan");
	if (IS_ERR(oj_power)) {
		printk(KERN_ERR"%s:Error power domain\n",__func__);
		return 0;
	}
	if (on) {
		vreg_set_level(oj_power, 2850);
		vreg_enable(oj_power);
		printk(KERN_ERR "%s:OJ	power	enable(%d)\n", __func__, on);
	} else {
		vreg_disable(oj_power);
		printk(KERN_ERR "%s:OJ	power	enable(%d)\n", __func__, on);
		}
	return 1;
}
static void curcial_oj_adjust_xy(uint8_t *data, int16_t *mSumDeltaX, int16_t *mSumDeltaY)
{
	int8_t 	deltaX;
	int8_t 	deltaY;


	if (data[2] == 0x80)
		data[2] = 0x81;
	if (data[1] == 0x80)
		data[1] = 0x81;
	if (0) {
		deltaX = (1)*((int8_t) data[2]); /*X=2*/
		deltaY = (-1)*((int8_t) data[1]); /*Y=1*/
	} else {
		deltaX = (1)*((int8_t) data[1]);
		deltaY = (1)*((int8_t) data[2]);
	}
	*mSumDeltaX += -((int16_t)deltaX);
	*mSumDeltaY += -((int16_t)deltaY);
}
#define BEECT_MICROP_VER	0x01

static struct curcial_oj_platform_data beect_oj_data = {
	.oj_poweron = curcial_oj_poweron,
	.oj_shutdown = curcial_oj_shutdown,
	.oj_adjust_xy = curcial_oj_adjust_xy,
	.microp_version = BEECT_MICROP_VER,
	.mdelay_time = 0,
	.normal_th = 10,
	.xy_ratio = 15,
	.interval = 0,
	.swap = 1,
	.x = 1,
	.y = -1,
	.share_power = false,
	.debugflag = 0,
	.ap_code = true,
	.sht_tbl = {0, 500, 750, 1000, 1250, 1500, 1750, 2000, 3000},
	.pxsum_tbl = {0, 0, 70, 80, 90, 100, 110, 120, 130},
	.degree = 9,
	.Xsteps = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 10, 10, 10, 10, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	.Ysteps = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 10, 10, 10, 10, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
	.irq = MSM_uP_TO_INT(12),
};

static struct platform_device beect_oj = {
	.name = CURCIAL_OJ_NAME,
	.id = -1,
	.dev = {
		.platform_data	= &beect_oj_data,
	}
};

/* HTC_HEADSET_GPIO Driver */
static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= BEECT_GPIO_35MM_HEADSET_DET,
	.key_enable_gpio	= BEECT_GPIO_35MM_KEY_INT_SHUTDOWN,
	.mic_select_gpio	= 0,
};

static struct platform_device htc_headset_gpio = {
	.name	= "HTC_HEADSET_GPIO",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_gpio_data,
	},
};

/* HTC_HEADSET_MICROP Driver */
static struct htc_headset_microp_platform_data htc_headset_microp_data = {
	.remote_int		= 1 << 5,
	.remote_irq		= MSM_uP_TO_INT(5),
	.remote_enable_pin	= 0,
	.adc_channel		= 0x01,
	.adc_remote		= {0, 27, 30, 100, 105, 200},
};

static struct platform_device htc_headset_microp = {
	.name	= "HTC_HEADSET_MICROP",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_microp_data,
	},
};

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_microp,
	&htc_headset_gpio,
	/* Please put the headset detection driver on the last */
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
};

static struct microp_function_config microp_lightsensor = {
	.name = "light_sensor",
	.category = MICROP_FUNCTION_LSENSOR,
	.levels = { 1, 3, 5, 32, 53, 365, 449, 533, 618, 0x3FF},
	.channel = 3,
	.int_pin = 1 << 9,
	.golden_adc = 0x159,
	.ls_power = capella_cm3602_power,
};

static struct lightsensor_platform_data lightsensor_data = {
	.config = &microp_lightsensor,
	.irq = MSM_uP_TO_INT(9),
};

static struct microp_led_config led_config[] = {
	{
		.name   = "amber",
		.type = LED_RGB,
	},
	{
		.name   = "green",
		.type = LED_RGB,
	},
	{
		.name = "button-backlight",
		.type = LED_PWM,
		.led_pin = 1 << 0,
		.init_value = 15,
		.fade_time = 5,
	},
};

static struct microp_led_platform_data microp_leds_data = {
	.num_leds	= ARRAY_SIZE(led_config),
	.led_config	= led_config,
};

static struct bma150_platform_data buzz_g_sensor_pdata = {
	.microp_new_cmd = 1,
	.layouts = BEECT_LAYOUTS,
};

static struct platform_device microp_devices[] = {
	{
		.name = "lightsensor_microp",
		.dev = {
			.platform_data = &lightsensor_data,
		},
	},
	{
		.name = "leds-microp",
		.id = -1,
		.dev = {
			.platform_data = &microp_leds_data,
		},
	},
	{
		.name = BMA150_G_SENSOR_NAME_REMOVE_ECOMPASS,
		.dev = {
			.platform_data = &buzz_g_sensor_pdata,
		},
	},
	{
		.name	= "HTC_HEADSET_MGR",
		.id	= -1,
		.dev	= {
			.platform_data	= &htc_headset_mgr_data,
		},
	},
};

static struct microp_i2c_platform_data microp_data = {
	.num_functions   = ARRAY_SIZE(microp_functions),
	.microp_function = microp_functions,
	.num_devices = ARRAY_SIZE(microp_devices),
	.microp_devices = microp_devices,
	.gpio_reset = BEECT_GPIO_UP_RESET_N,
	.spi_devices = SPI_OJ | SPI_GSENSOR,
};

static struct i2c_board_info i2c_microp_devices = {
	I2C_BOARD_INFO(MICROP_I2C_NAME, 0xCC >> 1),
	.platform_data = &microp_data,
	.irq = MSM_GPIO_TO_INT(BEECT_GPIO_UP_INT),
};

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_beect_camera_on_gpios,
	.camera_gpio_off = config_beect_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

static struct resource msm_camera_resources[] = {
	{
		.start	= MSM_VFE_PHYS,
		.end	= MSM_VFE_PHYS + MSM_VFE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

#if 0
static int flashlight_control(int mode)
{
	return 0;
}

static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
	.camera_flash		= flashlight_control,
	.num_flash_levels	= FLASHLIGHT_NUM,
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
};
#endif

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1gx_data = {
	.sensor_name    = "s5k4e1gx",
	.sensor_reset   = 118,
	.vcm_pwd        = BEECT_GPIO_VCM_PWDN,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_NONE,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	/*.flash_cfg	= &msm_camera_sensor_flash_cfg,*/
};

static struct platform_device msm_camera_sensor_s5k4e1gx = {
	.name      = "msm_camera_s5k4e1gx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k4e1gx_data,
	},
};

static int beect_ts_atmel_power(int on)
{
	printk(KERN_INFO "%s():\n", __func__);
	if (on) {
		gpio_set_value(BEECT_GPIO_TP_RST, 0);
		msleep(5);
		gpio_set_value(BEECT_TP_3V_EN, 1);
		msleep(5);
		gpio_set_value(BEECT_GPIO_TP_RST, 1);
		msleep(40);
	} else {
		gpio_set_value(BEECT_TP_3V_EN, 0);
		msleep(2);
	}
	return 0;
}

struct atmel_i2c_platform_data beect_ts_atmel_data[] = {
	{
		.version = 0x020,
		.abs_x_min = 5,
		.abs_x_max = 1020,
		.abs_y_min = 15,
		.abs_y_max = 940,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = BEECT_GPIO_TP_ATT_N,
		.power = beect_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {30, 15, 25},
		.config_T8 = {7, 0, 10, 10, 0, 0, 10, 39, 5, 170},
		.config_T9 = {3, 0, 0, 15, 11, 0, 16, 35, 4, 3, 0, 5, 5, 15, 3, 21, 12, 10, 0, 0, 0, 0, 241, 245, 20, 40, 150, 58, 160, 58, 20, 10},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T19 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 88, 27, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T27 = {0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 0, 8, 8, 60},
		.object_crc = {0x5A, 0x4A, 0xC1},
		.cal_tchthr = {52, 52},
		.cable_config = {42, 32, 8, 8},
		.GCAF_level = {20, 24, 28, 40, 63},
	},
	{
		.version = 0x016,
		.abs_x_min = 5,
		.abs_x_max = 1020,
		.abs_y_min = 15,
		.abs_y_max = 940,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = BEECT_GPIO_TP_ATT_N,
		.power = beect_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {30, 15, 25},
		.config_T8 = {8, 0, 5, 2, 0, 0, 5, 0},
		.config_T9 = {3, 0, 0, 15, 11, 0, 16, 40, 4, 3, 0, 5, 5, 15, 3, 20, 50, 10, 0, 0, 0, 0, 241, 0, 25, 27, 140, 50, 154, 50, 40},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T19 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {7, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T27 = {0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 0, 8, 16, 60},
		.cable_config_T9 = {3, 0, 0, 15, 11, 0, 16, 35, 2, 3, 0, 5, 5, 15, 3, 20, 50, 10, 0, 0, 0, 0, 241, 0, 25, 27, 140, 50, 154, 50, 40},
		.cable_config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 7, 18, 255, 255, 0},
		.cable_config_T28 = {0, 0, 0, 16, 32, 60},
	},
	{
		.version = 0x0015,
		.abs_x_min = 12,
		.abs_x_max = 1011,
		.abs_y_min = 8,
		.abs_y_max = 946,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = BEECT_GPIO_TP_ATT_N,
		.power = beect_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {50, 15, 25},
		.config_T8 = {8, 0, 20, 10, 0, 0, 10, 40},
		.config_T9 = {139, 0, 0, 15, 11, 0, 16, 35, 2, 3, 0, 2, 2, 15, 2, 10, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 140, 50, 153, 45},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T19 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 20, 0, 1, 10, 15, 20, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 88, 27, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T27 = {0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 0, 4, 8, 30},
	},
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO(ATMEL_QT602240_NAME, 0x94 >> 1),
		.platform_data = &beect_ts_atmel_data,
		.irq = MSM_GPIO_TO_INT(BEECT_GPIO_TP_ATT_N)
	},
	{
		/*I2C_BOARD_INFO("s5k4b2fx", 0x22 >> 1),*/
		I2C_BOARD_INFO("s5k4b2fx", 0x22),
		/* .irq = TROUT_GPIO_TO_INT(TROUT_GPIO_CAM_BTN_STEP1_N), */
	},
	{
		I2C_BOARD_INFO("s5k4e1gx", 0x20 >> 1),   /*5M bayer sensor*/
		.platform_data = &msm_camera_device_data,
	},
	{
		I2C_BOARD_INFO("tps65200", 0xD4 >> 1),
	},
};

static struct pwr_sink beect_pwrsink_table[] = {
	{
		.id     = PWRSINK_AUDIO,
		.ua_max = 100000,
	},
	{
		.id     = PWRSINK_BACKLIGHT,
		.ua_max = 125000,
	},
	{
		.id     = PWRSINK_LED_BUTTON,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_LED_KEYBOARD,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_GP_CLK,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_BLUETOOTH,
		.ua_max = 15000,
	},
	{
		.id     = PWRSINK_CAMERA,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_SDCARD,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_VIDEO,
		.ua_max = 0,
	},
	{
		.id     = PWRSINK_WIFI,
		.ua_max = 200000,
	},
	{
		.id     = PWRSINK_SYSTEM_LOAD,
		.ua_max = 100000,
		.percent_util = 38,
	},
};

static int beect_pwrsink_resume_early(struct platform_device *pdev)
{
	htc_pwrsink_set(PWRSINK_SYSTEM_LOAD, 7);
	return 0;
}

static void beect_pwrsink_resume_late(struct early_suspend *h)
{
	htc_pwrsink_set(PWRSINK_SYSTEM_LOAD, 38);
}

static void beect_pwrsink_suspend_early(struct early_suspend *h)
{
	htc_pwrsink_set(PWRSINK_SYSTEM_LOAD, 7);
}

static int beect_pwrsink_suspend_late(struct platform_device *pdev, pm_message_t state)
{
	htc_pwrsink_set(PWRSINK_SYSTEM_LOAD, 1);
	return 0;
}

static struct pwr_sink_platform_data beect_pwrsink_data = {
	.num_sinks      = ARRAY_SIZE(beect_pwrsink_table),
	.sinks          = beect_pwrsink_table,
	.suspend_late	= beect_pwrsink_suspend_late,
	.resume_early	= beect_pwrsink_resume_early,
	.suspend_early	= beect_pwrsink_suspend_early,
	.resume_late	= beect_pwrsink_resume_late,
};

static struct platform_device beect_pwr_sink = {
	.name = "htc_pwrsink",
	.id = -1,
	.dev    = {
		.platform_data = &beect_pwrsink_data,
	},
};

static struct msm_pmem_setting pmem_setting = {
	.pmem_start = MSM_PMEM_MDP_BASE,
	.pmem_size = MSM_PMEM_MDP_SIZE,
	.pmem_adsp_start = MSM_PMEM_ADSP_BASE,
	.pmem_adsp_size = MSM_PMEM_ADSP_SIZE,
	.pmem_camera_start = MSM_PMEM_CAMERA_BASE,
	.pmem_camera_size = MSM_PMEM_CAMERA_SIZE,
	.ram_console_start = MSM_RAM_CONSOLE_BASE,
	.ram_console_size = MSM_RAM_CONSOLE_SIZE,
};

static ssize_t beect_virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":17:350:42:55"
		":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":71:350:58:55"
		":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":172:350:55:55"
		":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":220:350:39:55"
		"\n");
}

static struct kobj_attribute beect_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.atmel-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &beect_virtual_keys_show,
};

static struct attribute *beect_properties_attrs[] = {
	&beect_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group beect_properties_attr_group = {
	.attrs = beect_properties_attrs,
};

static struct msm_i2c_device_platform_data msm_i2c_pdata = {
	.i2c_clock = 400000,
	.clock_strength = GPIO_8MA,
	.data_strength = GPIO_4MA,
};

static void __init msm_device_i2c_init(void)
{
	msm_i2c_gpio_init();
	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static struct platform_device beect_rfkill = {
	.name = "beect_rfkill",
	.id = -1,
};

static int __capella_cm3602_power(int on)
{
	printk(KERN_DEBUG "%s: Turn the capella_cm3602 power %s\n",
		__func__, (on) ? "on" : "off");
	if (on) {
		config_beect_proximity_gpios(1);
		gpio_direction_output(BEECT_GPIO_PROXIMITY_EN, 1);
		gpio_direction_output(BEECT_PS_2V85_EN, 1);
	} else {
		gpio_direction_output(BEECT_PS_2V85_EN, 0);
		gpio_direction_output(BEECT_GPIO_PROXIMITY_EN, 0);
		config_beect_proximity_gpios(0);
	}
	return 0;
}

static DEFINE_MUTEX(capella_cm3602_lock);
static unsigned int als_power_control;

static int capella_cm3602_power(int pwr_device, uint8_t enable)
{
	unsigned int old_status = 0;
	int ret = 0, on = 0;
	mutex_lock(&capella_cm3602_lock);

	old_status = als_power_control;
	if (enable)
		als_power_control |= pwr_device;
	else
		als_power_control &= ~pwr_device;

	on = als_power_control ? 1 : 0;
	if (old_status == 0 && on)
		ret = __capella_cm3602_power(1);
	else if (!on)
		ret = __capella_cm3602_power(0);

	mutex_unlock(&capella_cm3602_lock);
	return ret;
}

static struct capella_cm3602_platform_data capella_cm3602_pdata = {
	.p_out = BEECT_GPIO_PROXIMITY_INT,
	.p_en = BEECT_GPIO_PROXIMITY_EN,
	.power = capella_cm3602_power,
	.irq = MSM_GPIO_TO_INT(BEECT_GPIO_PROXIMITY_INT),
};

static struct platform_device capella_cm3602 = {
	.name = CAPELLA_CM3602,
	.dev = {
		.platform_data = &capella_cm3602_pdata
	}
};
/* End Proximity Sensor (Capella_CM3602)*/

static struct platform_device *devices[] __initdata = {
	&msm_device_smd,
	&msm_device_nand,
	&msm_device_i2c,
	&htc_battery_pdev,
	&msm_camera_sensor_s5k4e1gx,
	&beect_rfkill,
#ifdef CONFIG_HTC_PWRSINK
	&beect_pwr_sink,
#endif

#ifdef CONFIG_INPUT_CAPELLA_CM3602
	&capella_cm3602,
#endif
	&beect_oj,
};

extern struct sys_timer msm_timer;

static void __init beect_init_irq(void)
{
	printk("beect_init_irq()\n");
	msm_init_irq();
}

static uint cpld_iset;
static uint cpld_charger_en;
static uint cpld_usb_h2w_sw;
static uint opt_disable_uart3;
static char *keycaps = "";

module_param_named(iset, cpld_iset, uint, 0);
module_param_named(charger_en, cpld_charger_en, uint, 0);
module_param_named(usb_h2w_sw, cpld_usb_h2w_sw, uint, 0);
module_param_named(disable_uart3, opt_disable_uart3, uint, 0);
module_param_named(keycaps, keycaps, charp, 0);

static char bt_chip_id[10] = "bcm4329";
module_param_string(bt_chip_id, bt_chip_id, sizeof(bt_chip_id), S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(bt_chip_id, "BT's chip id");

static char bt_fw_version[10] = "v2.0.38";
module_param_string(bt_fw_version, bt_fw_version, sizeof(bt_fw_version), S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(bt_fw_version, "BT's fw version");

static void beect_reset(void)
{
	gpio_set_value(BEECT_GPIO_PS_HOLD, 0);
}

static uint32_t proximity_on_gpio_table[] = {
	PCOM_GPIO_CFG(BEECT_GPIO_PROXIMITY_INT,
		0, GPIO_INPUT, GPIO_NO_PULL, 0), /* PS_VOUT */
};

static uint32_t proximity_off_gpio_table[] = {
	PCOM_GPIO_CFG(BEECT_GPIO_PROXIMITY_INT,
		0, GPIO_INPUT, GPIO_PULL_DOWN, 0) /* PS_VOUT */
};

static uint32_t camera_off_gpio_table[] = {
	/* CAMERA */
	PCOM_GPIO_CFG(0, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT0 */
	PCOM_GPIO_CFG(1, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT1 */

	PCOM_GPIO_CFG(2, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT2 */
	PCOM_GPIO_CFG(3, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT3 */
	PCOM_GPIO_CFG(4, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT4 */
	PCOM_GPIO_CFG(5, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT5 */
	PCOM_GPIO_CFG(6, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT6 */
	PCOM_GPIO_CFG(7, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT7 */
	PCOM_GPIO_CFG(8, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT8 */
	PCOM_GPIO_CFG(9, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT9 */
	PCOM_GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT10 */
	PCOM_GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* DAT11 */
	PCOM_GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA), /* PCLK */
	PCOM_GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA),/*HSYNC_IN*/
	PCOM_GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_4MA),/*VSYNC_IN*/
	PCOM_GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_4MA),/*MCLK*/

/* PCOM_GPIO_CFG(61, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),//CAM_I2C_SDA */
/* PCOM_GPIO_CFG(60, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),//CAM_I2C_SCL */
};

static uint32_t camera_on_gpio_table[] = {
	/* CAMERA */
	PCOM_GPIO_CFG(2, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	PCOM_GPIO_CFG(3, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	PCOM_GPIO_CFG(4, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	PCOM_GPIO_CFG(5, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	PCOM_GPIO_CFG(6, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	PCOM_GPIO_CFG(7, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	PCOM_GPIO_CFG(8, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	PCOM_GPIO_CFG(9, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	PCOM_GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	PCOM_GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	PCOM_GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
	PCOM_GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	PCOM_GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	PCOM_GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */
};

void config_beect_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

void config_beect_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

/* for bcm */
static char bdaddress[20];
extern unsigned char *get_bt_bd_ram(void);

static void bt_export_bd_address(void)
{
	unsigned char cTemp[6];

	memcpy(cTemp, get_bt_bd_ram(), 6);
	sprintf(bdaddress, "%02x:%02x:%02x:%02x:%02x:%02x",
		cTemp[0], cTemp[1], cTemp[2], cTemp[3], cTemp[4], cTemp[5]);
	printk(KERN_INFO "YoYo--BD_ADDRESS=%s\n", bdaddress);
}

module_param_string(bdaddress, bdaddress, sizeof(bdaddress), S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(bdaddress, "BT MAC ADDRESS");

void config_beect_proximity_gpios(int on)
{
	if (on)
		config_gpio_table(proximity_on_gpio_table,
			ARRAY_SIZE(proximity_on_gpio_table));
	else
		config_gpio_table(proximity_off_gpio_table,
			ARRAY_SIZE(proximity_off_gpio_table));
}

static uint32_t beect_serial_debug_table[] = {
	/* config as serial debug uart */
	PCOM_GPIO_CFG(BEECT_GPIO_UART3_RX, 1,
			GPIO_INPUT, GPIO_PULL_UP, GPIO_8MA),	/* UART3 RX */
	PCOM_GPIO_CFG(BEECT_GPIO_UART3_TX, 1,
			GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),	/* UART3 TX */
};

static void beect_config_serial_debug_gpios(void)
{
	config_gpio_table(beect_serial_debug_table,
			ARRAY_SIZE(beect_serial_debug_table));
}


static void __init config_gpios(void)
{
	beect_config_serial_debug_gpios();
	config_beect_camera_off_gpios();
}

static struct msm_acpu_clock_platform_data beect_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.power_collapse_khz = 19200,
#if defined(CONFIG_TURBO_MODE)
	.wait_for_irq_khz = 176000,
#else
	.wait_for_irq_khz = 128000,
#endif
};

static unsigned beect_perf_acpu_table[] = {
	245760000,
	480000000,
	528000000,
};

static struct perflock_platform_data beect_perflock_data = {
	.perf_acpu_table = beect_perf_acpu_table,
	.table_size = ARRAY_SIZE(beect_perf_acpu_table),
};

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.rx_wakeup_irq = MSM_GPIO_TO_INT(BEECT_GPIO_BT_HOST_WAKE),
	.inject_rx_on_wakeup = 0,
	.cpu_lock_supported = 1,

	/* for bcm */
	.bt_wakeup_pin_supported = 1,
	.bt_wakeup_pin = BEECT_GPIO_BT_CHIP_WAKE,
	.host_wakeup_pin = BEECT_GPIO_BT_HOST_WAKE,

};
#endif

static void __init beect_init(void)
{
	int rc;
	struct kobject *properties_kobj;

	printk("beect_init() revision=%d\n", system_rev);
	printk(KERN_INFO "mfg_mode=%d\n", board_mfg_mode());

	/* for bcm */
	bt_export_bd_address();

	/*
	 * Setup common MSM GPIOS
	 */
	config_gpios();

	/* We need to set this pin to 0 only once on power-up; we will
	 * not actually enable the chip until we apply power to it via
	 * vreg.
	 */
	gpio_request(BEECT_GPIO_LS_EN, "ls_en");
	gpio_request(BEECT_PS_2V85_EN, "ps_pw_en");
	gpio_direction_output(BEECT_GPIO_LS_EN, 0);

	msm_hw_reset_hook = beect_reset;

	msm_acpu_clock_init(&beect_clock_data);
	perflock_init(&beect_perflock_data);
	/* adjust GPIOs based on bootloader request */

#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	if (!opt_disable_uart3)
		msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
			&msm_device_uart3.dev, 1,
				MSM_GPIO_TO_INT(BEECT_GPIO_UART3_RX));
#endif

#ifdef CONFIG_SERIAL_MSM_HS
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
	msm_device_uart_dm1.name = "msm_serial_hs_bcm";	/* for bcm */
	msm_add_serial_devices(3);
#else
	msm_add_serial_devices(0);
#endif

	msm_add_serial_devices(2);
#ifdef CONFIG_USB_FUNCTION
	msm_add_usb_id_pin_gpio(BEECT_GPIO_USB_ID_PIN);
	msm_add_usb_devices(NULL, NULL);
#endif
	msm_add_mem_devices(&pmem_setting);
	msm_init_pmic_vibrator();
#ifdef CONFIG_MICROP_COMMON
	beect_microp_init();
#endif

	rc = beect_init_mmc(system_rev);
	if (rc)
		printk(KERN_CRIT "%s: MMC init failure (%d)\n", __func__, rc);

	properties_kobj = kobject_create_and_add("board_properties", NULL);

	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
					 &beect_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("failed to create board_properties\n");

	msm_device_i2c_init();
	platform_add_devices(devices, ARRAY_SIZE(devices));

#ifdef CONFIG_USB_ANDROID
	beect_add_usb_devices();
#endif
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	i2c_register_board_info(0 , &i2c_microp_devices, 1);

	beect_panel_init();
	beect_init_keypad();
}

static void __init beect_fixup(struct machine_desc *desc, struct tag *tags,
				char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 2;
	mi->bank[0].start = MSM_LINUX_BASE1;
	mi->bank[0].node = PHYS_TO_NID(MSM_LINUX_BASE1);
	mi->bank[0].size = MSM_LINUX_SIZE1;
	mi->bank[1].start = MSM_LINUX_BASE2;
	mi->bank[1].node = PHYS_TO_NID(MSM_LINUX_BASE2);
	mi->bank[1].size = MSM_LINUX_SIZE2;
}

static void __init beect_map_io(void)
{
	printk("beect_init_map_io()\n");
	msm_map_common_io();
	msm_clock_init();
}

MACHINE_START(BEECT, "beect")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params    = 0x02E00100,
	.fixup          = beect_fixup,
	.map_io         = beect_map_io,
	.init_irq       = beect_init_irq,
	.init_machine   = beect_init,
	.timer          = &msm_timer,
MACHINE_END
