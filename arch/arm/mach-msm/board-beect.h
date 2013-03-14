/* linux/arch/arm/mach-msm/board-beect.h
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_BEECT_H
#define __ARCH_ARM_MACH_MSM_BOARD_BEECT_H

#include <mach/board.h>

#define MSM_LINUX_BASE1                 0x02E00000
#define MSM_LINUX_SIZE1                 0x0D200000
#define MSM_LINUX_BASE2                 0x20000000
#define MSM_LINUX_SIZE2                 0x06F00000

#define MSM_PMEM_MDP_BASE               0x00000000
#define MSM_PMEM_MDP_SIZE               0x00800000

#define MSM_FB_BASE                     0x26F00000
#define MSM_FB_SIZE                     0x9b000

#define MSM_RAM_CONSOLE_BASE            MSM_FB_BASE + MSM_FB_SIZE
#define MSM_RAM_CONSOLE_SIZE            128 * SZ_1K

#define MSM_PMEM_ADSP_BASE              0x27000000
#define MSM_PMEM_ADSP_SIZE              0x00800000

#define MSM_PMEM_CAMERA_BASE            0x27800000
#define MSM_PMEM_CAMERA_SIZE            0x00800000

#define BEECT_GPIO_USB_ID_PIN          (19)

/* key */
#define BEECT_GPIO_POWER_KEY         (20)
#define BEECT_GPIO_Q_KP_MKOUT0       (35)
#define BEECT_GPIO_Q_KP_MKOUT1       (34)
#define BEECT_GPIO_Q_KP_MKOUT2       (33)
#define BEECT_GPIO_Q_KP_MKIN0_1      (42)
#define BEECT_GPIO_Q_KP_MKIN1_1      (41)
#define BEECT_GPIO_Q_KP_MKIN2_1      (40)
#define BEECT_GPIO_OJ_ACTION         (83)
/*
#define BEECT_GPIO_BALL_UP     (94)
#define BEECT_GPIO_BALL_LEFT   (17)
#define BEECT_GPIO_BALL_DOWN   (83)
#define BEECT_GPIO_BALL_RIGHT  (18)
*/
#define BEECT_GPIO_WIFI_IRQ1         (29)
#define BEECT_GPIO_WIFI_EN           (116)
#define BEECT_GPIO_SDMC_CD_N         (38)
#define BEECT_GPIO_UP_INT            (27)
#define BEECT_GPIO_UART3_RX          (86)
#define BEECT_GPIO_UART3_TX          (87)
#define BEECT_GPIO_VCM_PWDN          (31)
#define BEECT_GPIO_UP_RESET_N        (76)
#define BEECT_GPIO_PS_HOLD           (25)
/* flashlight */
#define BEECT_GPIO_FL_TORCH	(84)
#define BEECT_GPIO_FL_FLASH	(85)

/* 35mm headset */
#define BEECT_GPIO_35MM_HEADSET_DET	(112)
#define BEECT_GPIO_35MM_KEY_INT_SHUTDOWN	(93)

/* AP Key Led turn on*/
#define BEECT_AP_KEY_LED_EN			(119)

/* Compass */
#define BEECT_GPIO_COMPASS_INT_N		(37)
#define BEECT_LAYOUTS		{ \
		{ { 0,  1, 0}, {  1,  0, 0}, {0, 0, -1} }, \
		{ { 0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
		{ { 1,  0, 0}, {  0, -1, 0}, {0, 0, -1} }, \
		{ { 1,  0, 0}, {  0,  0, 1}, {0, 1,  0} }  \
					}

/* Proximity  */
#define BEECT_GPIO_PROXIMITY_INT     114 // PS_VOUT
#define BEECT_GPIO_PROXIMITY_EN      115 // PS_SHDN

#define BEECT_GPIO_LS_EN             111
#define BEECT_PS_2V85_EN             117

/* BT */
#define BEECT_GPIO_BT_UART1_RTS      (43)
#define BEECT_GPIO_BT_UART1_CTS      (44)
#define BEECT_GPIO_BT_UART1_RX       (45)
#define BEECT_GPIO_BT_UART1_TX       (46)
#define BEECT_GPIO_BT_RESET_N        (131)
#define BEECT_GPIO_BT_HOST_WAKE      (21)
#define BEECT_GPIO_BT_CHIP_WAKE      (91)
#define BEECT_GPIO_BT_SHUTDOWN_N     (113)

/* Touch Panel */
#define BEECT_TP_3V_EN                 (108)
#define BEECT_GPIO_TP_ATT_N            (92)
#define BEECT_GPIO_TP_RST              (107)

/* Display */
#define BEECT_LCD_ID1		   (23)
#define BEECT_LCD_ID0		   (32)
#define BEECT_LCM_2V85_EN		   (78)
#define BEECT_LCMIO_2V6_EN	   (79)
#define BEECT_MDDI_RSTz              (82)

/* Battery */
#define BEECT_GPIO_CHG_INT	    (39)

int beect_init_mmc(unsigned int);
int __init beect_init_keypad(void);
int __init beect_panel_init(void);

//struct sys_device;
unsigned int beect_get_hwid(void);
unsigned int beect_get_skuid(void);
unsigned beect_engineerid(void);
int beect_is_3M_camera(void);

#endif /* GUARD */
