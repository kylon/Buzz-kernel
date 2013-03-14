/* arch/arm/mach-msm/board--bee.c
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
*/


#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>

#include "board-bee.h"

struct bee_axis_info {
	struct gpio_event_axis_info info;
	uint16_t in_state;
	uint16_t out_state;
	uint16_t temp_state;
};

static unsigned int bee_col_gpios[] = {
	BEE_GPIO_Q_KP_MKOUT0,
	BEE_GPIO_Q_KP_MKOUT1,
	BEE_GPIO_Q_KP_MKOUT2,
};

static unsigned int bee_row_gpios[] = {
	BEE_GPIO_Q_KP_MKIN0_1,
	BEE_GPIO_Q_KP_MKIN1_1,
	BEE_GPIO_Q_KP_MKIN2_1,
};

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(bee_row_gpios) + (row))

static unsigned short bee_keymap[ARRAY_SIZE(bee_col_gpios) *
					ARRAY_SIZE(bee_row_gpios)] = {
	[KEYMAP_INDEX(0, 0)] = KEY_RESERVED,
	[KEYMAP_INDEX(0, 1)] = KEY_RESERVED,
	[KEYMAP_INDEX(0, 2)] = KEY_VOLUMEUP,

	[KEYMAP_INDEX(1, 0)] = KEY_RESERVED,
	[KEYMAP_INDEX(1, 1)] = KEY_RESERVED,
	[KEYMAP_INDEX(1, 2)] = KEY_VOLUMEDOWN,

	[KEYMAP_INDEX(2, 0)] = KEY_RESERVED,
	[KEYMAP_INDEX(2, 1)] = KEY_RESERVED,
	[KEYMAP_INDEX(2, 2)] = KEY_RESERVED,
};

static struct gpio_event_matrix_info bee_keypad_matrix_info = {
	.info.func = gpio_event_matrix_func,
	.keymap = bee_keymap,
	.output_gpios = bee_col_gpios,
	.input_gpios = bee_row_gpios,
	.noutputs = ARRAY_SIZE(bee_col_gpios),
	.ninputs = ARRAY_SIZE(bee_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.debounce_delay.tv.nsec = 5 * NSEC_PER_MSEC,
	.flags = (GPIOKPF_LEVEL_TRIGGERED_IRQ |
		GPIOKPF_REMOVE_PHANTOM_KEYS |
		GPIOKPF_PRINT_UNMAPPED_KEYS /* |
		GPIOKPF_PRINT_MAPPED_KEYS */),
	.detect_phone_status = 1,
};

static struct gpio_event_direct_entry bee_keypad_dir_key_map[] = {
	{
		.gpio = BEE_GPIO_POWER_KEY,
		.code = KEY_POWER,
	},
	{
		.gpio = BEE_GPIO_OJ_ACTION,
		.code = BTN_MOUSE,
	},
};

static struct gpio_event_input_info bee_keypad_dir_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.info.oj_btn = true,
	.flags = GPIOEDF_PRINT_KEYS,
	.type = EV_KEY,
	.debounce_time.tv.nsec = 5 * NSEC_PER_MSEC,
	.keymap = bee_keypad_dir_key_map,
	.keymap_size = ARRAY_SIZE(bee_keypad_dir_key_map),
};

static struct gpio_event_info *bee_input_info[] = {
	&bee_keypad_matrix_info.info,
	&bee_keypad_dir_info.info,
};

int bee_gpio_event_power(const struct gpio_event_platform_data *pdata, bool on)
{
       return 0;
}

static struct gpio_event_platform_data bee_keypad_data = {
	.names = {
		"bee-keypad",
		NULL,
	},
	.info = bee_input_info,
	.info_count = ARRAY_SIZE(bee_input_info),
	.power = bee_gpio_event_power,
};

static struct platform_device bee_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &bee_keypad_data,
	},
};

static int bee_reset_keys_up[] = {
	KEY_VOLUMEUP,
	0
};

static struct keyreset_platform_data bee_reset_keys_pdata = {
	.keys_up = bee_reset_keys_up,
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		BTN_MOUSE,
		0
	},
};

struct platform_device bee_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &bee_reset_keys_pdata,
};

int __init bee_init_keypad(void)
{
	if (platform_device_register(&bee_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);

	return platform_device_register(&bee_keypad_device);
}

