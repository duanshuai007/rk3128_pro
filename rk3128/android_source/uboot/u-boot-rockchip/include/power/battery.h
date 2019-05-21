/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __POWER_BATTERY_H_
#define __POWER_BATTERY_H_

struct battery {
	unsigned int version;
	unsigned int state_of_chrg;
	unsigned int time_to_empty;
	unsigned int capacity;
	unsigned int voltage_uV;

	unsigned int state;
    //add by duanshuai start
    unsigned int isexistbat;
    //add by duanshuai end
};

int power_bat_init(unsigned char bus);
#endif /* __POWER_BATTERY_H_ */
