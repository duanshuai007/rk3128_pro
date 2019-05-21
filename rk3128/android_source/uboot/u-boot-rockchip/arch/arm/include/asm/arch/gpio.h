/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _ASM_ARCH_GPIO_H
#define _ASM_ARCH_GPIO_H

struct rockchip_gpio_regs {
	u32 swport_dr;
	u32 swport_ddr;
	u32 reserved0[(0x30 - 0x08) / 4];
	u32 inten;
	u32 intmask;
	u32 inttype_level;
	u32 int_polarity;
	u32 int_status;
	u32 int_rawstatus;
	u32 debounce;
	u32 porta_eoi;
	u32 ext_port;
	u32 reserved1[(0x60 - 0x54) / 4];
	u32 ls_sync;
};
check_member(rockchip_gpio_regs, ls_sync, 0x60);




/*
 *  * This is Linux-specific flags. By default controllers' and Linux' mapping
 *   * match, but GPIO controllers are free to translate their own flags to
 *    * Linux-specific in their .xlate callback. Though, 1:1 mapping is recommended.
 *     */
enum of_gpio_flags {
    OF_GPIO_ACTIVE_LOW = 0x1,
};

#endif
