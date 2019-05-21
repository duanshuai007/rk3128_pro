/*
 * An I2C driver for the Philips PCF8563 RTC
 * Copyright 2005-06 Tower Technologies
 *
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 * Maintainers: http://www.nslu2-linux.org/
 *
 * based on the other drivers in this same directory.
 *
 * http://www.semiconductors.philips.com/acrobat/datasheets/PCF8563-04.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <linux/delay.h>

#define DRV_VERSION "0.4.3"

#define PCF8563_REG_ST1		0x00 /* status */
#define PCF8563_REG_ST2		0x01

#define PCF8563_REG_SC		0x02 /* datetime */
#define PCF8563_REG_MN		0x03
#define PCF8563_REG_HR		0x04
#define PCF8563_REG_DM		0x05
#define PCF8563_REG_DW		0x06
#define PCF8563_REG_MO		0x07
#define PCF8563_REG_YR		0x08

#define PCF8563_REG_AMN		0x09 /* alarm */
#define PCF8563_REG_AHR		0x0A
#define PCF8563_REG_ADM		0x0B
#define PCF8563_REG_ADW		0x0C

#define PCF8563_REG_CLKO	0x0D /* clock out */
#define PCF8563_REG_TMRC	0x0E /* timer control */
#define PCF8563_REG_TMR		0x0F /* timer */

#define PCF8563_SC_LV		0x80 /* low voltage */
#define PCF8563_MO_C		0x80 /* century */


#define	PCF_SET_TIME	1
#define	PCF_RED_TIME	7
#define	PCF_EN_ALARM	3
#define	PCF_DIS_ALARM	4
#define	PCF_SET_ONALARM	5
#define	PCF_RED_ALARM	6
#define	PCF_SET_OFALARM	8

#define PCF8563_IRQ	1

static struct workqueue_struct *pcf8563_wq;


static struct i2c_driver pcf8563_driver;

struct pcf8563 {
	struct rtc_device *rtc;
	/*
	 * The meaning of MO_C bit varies by the chip type.
	 * From PCF8563 datasheet: this bit is toggled when the years
	 * register overflows from 99 to 00
	 *   0 indicates the century is 20xx
	 *   1 indicates the century is 19xx
	 * From RTC8564 datasheet: this bit indicates change of
	 * century. When the year digit data overflows from 99 to 00,
	 * this bit is set. By presetting it to 0 while still in the
	 * 20th century, it will be set in year 2000, ...
	 * There seems no reliable way to know how the system use this
	 * bit.  So let's do it heuristically, assuming we are live in
	 * 1970...2069.
	 */
	int c_polarity;	/* 0: MO_C=1 means 19xx, otherwise MO_C=1 means 20xx */
	int voltage_low; /* incicates if a low_voltage was detected */
	struct i2c_client *pcf8563_client;
	struct rtc_time onalarm;
	int status;
	struct workqueue_struct *pcf_init_wq;
	struct work_struct  pcf_work;
#if PCF8563_IRQ		
	int irq_pin;
	int irq;
	int irq_flags;
//	struct hrtimer timer;
//	ktime_t poll_delay;
	int irq_is_disable;
	spinlock_t irq_lock;
#endif	
};
struct pcf8563 *pcf8563;


/*
 * In the routines that deal directly with the pcf8563 hardware, we use
 * rtc_time -- month 0-11, hour 0-23, yr = calendar year-epoch.
 */
static int pcf8563_get_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	struct pcf8563 *pcf8563 = i2c_get_clientdata(client);
	unsigned char buf[13] = { PCF8563_REG_ST1 };

	struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 13,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(client->adapter, msgs, 2)) != 2) {
		dev_err(&client->dev, "%s: read error\n", __func__);
		return -EIO;
	}

	if (buf[PCF8563_REG_SC] & PCF8563_SC_LV) {
		pcf8563->voltage_low = 1;
		dev_info(&client->dev,
			"low voltage detected, date/time is not reliable.\n");
	}

	dev_dbg(&client->dev,
		"%s: raw data is st1=%02x, st2=%02x, sec=%02x, min=%02x, hr=%02x, "
		"mday=%02x, wday=%02x, mon=%02x, year=%02x\n",
		__func__,
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7],
		buf[8]);


	tm->tm_sec = bcd2bin(buf[PCF8563_REG_SC] & 0x7F);
	tm->tm_min = bcd2bin(buf[PCF8563_REG_MN] & 0x7F);
	tm->tm_hour = bcd2bin(buf[PCF8563_REG_HR] & 0x3F); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(buf[PCF8563_REG_DM] & 0x3F);
	tm->tm_wday = buf[PCF8563_REG_DW] & 0x07;
	tm->tm_mon = bcd2bin(buf[PCF8563_REG_MO] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = bcd2bin(buf[PCF8563_REG_YR]);
	if (tm->tm_year < 70)
		tm->tm_year += 100;	/* assume we are in 1970...2069 */
	/* detect the polarity heuristically. see note above. */
	pcf8563->c_polarity = (buf[PCF8563_REG_MO] & PCF8563_MO_C) ?
		(tm->tm_year >= 100) : (tm->tm_year < 100);

	dev_dbg(&client->dev, "%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0)
		dev_err(&client->dev, "retrieved date/time is not valid.\n");

	return 0;
}

static int pcf8563_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	struct pcf8563 *pcf8563 = i2c_get_clientdata(client);
	int i, err;
	unsigned char buf[9];

	dev_dbg(&client->dev, "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* hours, minutes and seconds */
	buf[PCF8563_REG_SC] = bin2bcd(tm->tm_sec);
	buf[PCF8563_REG_MN] = bin2bcd(tm->tm_min);
	buf[PCF8563_REG_HR] = bin2bcd(tm->tm_hour);

	buf[PCF8563_REG_DM] = bin2bcd(tm->tm_mday);

	/* month, 1 - 12 */
	buf[PCF8563_REG_MO] = bin2bcd(tm->tm_mon + 1);

	/* year and century */
	buf[PCF8563_REG_YR] = bin2bcd(tm->tm_year % 100);
	if (pcf8563->c_polarity ? (tm->tm_year >= 100) : (tm->tm_year < 100))
		buf[PCF8563_REG_MO] |= PCF8563_MO_C;

	buf[PCF8563_REG_DW] = tm->tm_wday & 0x07;

	/* write register's data */
	for (i = 0; i < 7; i++) {
		unsigned char data[2] = { PCF8563_REG_SC + i,
						buf[PCF8563_REG_SC + i] };

		err = i2c_master_send(client, data, sizeof(data));
		if (err != sizeof(data)) {
			dev_err(&client->dev,
				"%s: err=%d addr=%02x, data=%02x\n",
				__func__, err, data[0], data[1]);
			return -EIO;
		}
	}

	return 0;
}

#ifdef CONFIG_RTC_INTF_DEV
static int pcf8563_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct pcf8563 *pcf8563 = i2c_get_clientdata(to_i2c_client(dev));
	struct rtc_time tm;

	switch (cmd) {
	case RTC_VL_READ:
		if (pcf8563->voltage_low)
			dev_info(dev, "low voltage detected, date/time is not reliable.\n");

		if (copy_to_user((void __user *)arg, &pcf8563->voltage_low,
					sizeof(int)))
			return -EFAULT;
		return 0;
	case RTC_VL_CLR:
		/*
		 * Clear the VL bit in the seconds register in case
		 * the time has not been set already (which would
		 * have cleared it). This does not really matter
		 * because of the cached voltage_low value but do it
		 * anyway for consistency.
		 */
		if (pcf8563_get_datetime(to_i2c_client(dev), &tm))
			pcf8563_set_datetime(to_i2c_client(dev), &tm);

		/* Clear the cached value. */
		pcf8563->voltage_low = 0;

		return 0;
	default:
		return -ENOIOCTLCMD;
	}
}
#else
#define pcf8563_rtc_ioctl NULL
#endif

static int pcf8563_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return pcf8563_get_datetime(to_i2c_client(dev), tm);
}

static int pcf8563_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return pcf8563_set_datetime(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops pcf8563_rtc_ops = {
	.ioctl		= pcf8563_rtc_ioctl,
	.read_time	= pcf8563_rtc_read_time,
	.set_time	= pcf8563_rtc_set_time,
};


static int pcf8563_set_datetime_hyman(struct rtc_time *tm)
{
	
	int i, err;
	unsigned char buf[9];

	printk( "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* hours, minutes and seconds */
	buf[PCF8563_REG_SC] = bin2bcd(tm->tm_sec);
	buf[PCF8563_REG_MN] = bin2bcd(tm->tm_min);
	buf[PCF8563_REG_HR] = bin2bcd(tm->tm_hour);

	buf[PCF8563_REG_DM] = bin2bcd(tm->tm_mday);

	/* month, 1 - 12 */
	buf[PCF8563_REG_MO] = bin2bcd(tm->tm_mon + 1);

	/* year and century */
	buf[PCF8563_REG_YR] = bin2bcd(tm->tm_year % 100);
	if (pcf8563->c_polarity ? (tm->tm_year >= 100) : (tm->tm_year < 100))
		buf[PCF8563_REG_MO] |= PCF8563_MO_C;

	buf[PCF8563_REG_DW] = tm->tm_wday & 0x07;

	/* write register's data */
	for (i = 0; i < 7; i++) {
		unsigned char data[2] = { PCF8563_REG_SC + i,
						buf[PCF8563_REG_SC + i] };

		err = i2c_master_send(pcf8563->pcf8563_client, data, sizeof(data));
		if (err != sizeof(data)) {
			printk("%s: err=%d addr=%02x, data=%02x\n",__func__, err, data[0], data[1]);
			return -EIO;
		}
	}

	return 0;
}

void pcf8563_read_datetime_hyman(struct rtc_time *tm)
{
	unsigned char buf[13] = { PCF8563_REG_ST1 };

	struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = pcf8563->pcf8563_client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = pcf8563->pcf8563_client->addr,
			.flags = I2C_M_RD,
			.len = 13,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(pcf8563->pcf8563_client->adapter, msgs, 2)) != 2) {
		printk( "%s: read error\n", __func__);
		return -EIO;
	}

	if (buf[PCF8563_REG_SC] & PCF8563_SC_LV) {
		pcf8563->voltage_low = 1;
		printk(
			"low voltage detected, date/time is not reliable.\n");
	}

	printk(
		"%s: raw data is st1=%02x, st2=%02x, sec=%02x, min=%02x, hr=%02x, "
		"mday=%02x, wday=%02x, mon=%02x, year=%02x\n",
		__func__,
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7],
		buf[8]);


	tm->tm_sec = bcd2bin(buf[PCF8563_REG_SC] & 0x7F);
	tm->tm_min = bcd2bin(buf[PCF8563_REG_MN] & 0x7F);
	tm->tm_hour = bcd2bin(buf[PCF8563_REG_HR] & 0x3F); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(buf[PCF8563_REG_DM] & 0x3F);
	tm->tm_wday = buf[PCF8563_REG_DW] & 0x07;
	tm->tm_mon = bcd2bin(buf[PCF8563_REG_MO] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = bcd2bin(buf[PCF8563_REG_YR]);
	if (tm->tm_year < 70)
		tm->tm_year += 100;	/* assume we are in 1970...2069 */
	/* detect the polarity heuristically. see note above. */
	pcf8563->c_polarity = (buf[PCF8563_REG_MO] & PCF8563_MO_C) ?
		(tm->tm_year >= 100) : (tm->tm_year < 100);

	printk( "%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0)
		printk( "retrieved date/time is not valid.\n");
}


void pcf8563_read_alarm_hyman(struct rtc_time *tm)
{
	unsigned char buf[13] = { PCF8563_REG_ST1 };

	struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = pcf8563->pcf8563_client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = pcf8563->pcf8563_client->addr,
			.flags = I2C_M_RD,
			.len = 13,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(pcf8563->pcf8563_client->adapter, msgs, 2)) != 2) {
		printk( "%s: read error\n", __func__);
		return -EIO;
	}

	if (buf[PCF8563_REG_SC] & PCF8563_SC_LV) {
		pcf8563->voltage_low = 1;
		printk(
			"low voltage detected, date/time is not reliable.\n");
	}

	printk(
		"%s: raw data is st1=%02x, st2=%02x, sec=%02x, min=%02x, hr=%02x, "
		"mday=%02x, wday=%02x, mon=%02x, year=%02x\n",
		__func__,
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7],
		buf[8]);


	
	tm->tm_min = bcd2bin(buf[PCF8563_REG_AMN] & 0x7F);
	tm->tm_hour = bcd2bin(buf[PCF8563_REG_AHR] & 0x3F); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(buf[PCF8563_REG_ADM] & 0x3F);
	tm->tm_wday = buf[PCF8563_REG_ADW] & 0x07;

	printk( "%s: tm is, mins=%d, hours=%d, "
		"mday=%d,, wday=%d\n",
		__func__,
		 tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_wday);



}

void pcf8563_set_onalarm_hyman(struct rtc_time *tm)
{
	int i, err;
	unsigned char buf[9];

	printk( "%s:  mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		 tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* hours, minutes and seconds */
#if 0	
	buf[PCF8563_REG_AMN - 9] = bin2bcd(tm->tm_min);
	buf[PCF8563_REG_AHR - 9] = bin2bcd(tm->tm_hour);

	buf[PCF8563_REG_ADM - 9] = bin2bcd(tm->tm_mday);

	/* WEEK */
	buf[PCF8563_REG_ADW - 9] = tm->tm_wday & 0x07;



	/* write register's data */
	for (i = 0; i < 4; i++) {
		unsigned char data[2] = { PCF8563_REG_AMN + i,
						buf[PCF8563_REG_AMN - 9  + i] };

		err = i2c_master_send(pcf8563->pcf8563_client, data, sizeof(data));
		if (err != sizeof(data)) {
			printk("%s: err=%d addr=%02x, data=%02x\n",__func__, err, data[0], data[1]);
			return -EIO;
		}
	}
#endif
	pcf8563->onalarm.tm_hour =  tm->tm_hour;
	pcf8563->onalarm.tm_min  =  tm->tm_min;
	pcf8563->onalarm.tm_wday =  tm->tm_wday;
	pcf8563->onalarm.tm_mday =  tm->tm_mday;
	pcf8563->status = 1;
	return 0;

}

void pcf8563_set_offalarm_hyman(struct rtc_time *tm)
{
	printk("pcf8563_set_offalarm_hyman\n");
	int i, err;
	unsigned char buf[9];

	printk( "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* hours, minutes and seconds */
#if 1	
	buf[PCF8563_REG_AMN - 9] = bin2bcd(tm->tm_min);
	buf[PCF8563_REG_AHR - 9] = bin2bcd(tm->tm_hour);

	buf[PCF8563_REG_ADM - 9] = bin2bcd(tm->tm_mday);

	/* WEEK */
	buf[PCF8563_REG_ADW - 9] = tm->tm_wday & 0x07;



	/* write register's data */
	for (i = 0; i < 4; i++) {
		unsigned char data[2] = { PCF8563_REG_AMN + i,
						buf[PCF8563_REG_AMN - 9  + i] };

		err = i2c_master_send(pcf8563->pcf8563_client, data, sizeof(data));
		if (err != sizeof(data)) {
			printk("%s: err=%d addr=%02x, data=%02x\n",__func__, err, data[0], data[1]);
			return -EIO;
		}
	}
#else
		
	
#endif
}

void pcf8563_en_alarm_hyman(int i)
{
	printk("pcf8563_en_alarm  i = %d \n",i);
	if(i)
	{
		unsigned char data[2] = { PCF8563_REG_ST2,0x2};
		i2c_master_send(pcf8563->pcf8563_client, data, sizeof(data));
	
	}
	else{
		unsigned char data[2] = { PCF8563_REG_ST2,0x0};
		i2c_master_send(pcf8563->pcf8563_client, data, sizeof(data));

	}
}


static long pcf8563_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg )
{
	struct rtc_time tm;
	copy_from_user(&tm,arg,sizeof(tm));
	printk("%d:%d week = %d cmd = %d \n",tm.tm_hour,tm.tm_min,tm.tm_wday,cmd);
	switch (cmd) {
	case PCF_SET_TIME:
	 	pcf8563_set_datetime_hyman(&tm);

		return 0;
	case PCF_SET_OFALARM:
		pcf8563_set_offalarm_hyman(&tm);
		return 0;
	case PCF_SET_ONALARM:
		pcf8563_set_onalarm_hyman(&tm);

		return 0;
	case PCF_EN_ALARM:
		pcf8563_en_alarm_hyman(1);
		return 0;
	case PCF_DIS_ALARM:
		pcf8563_en_alarm_hyman(0);
		return 0;
	case PCF_RED_ALARM:
		pcf8563_read_alarm_hyman(&tm);
		copy_to_user(arg,&tm,sizeof(tm));
		return 0;
	default:
		return -ENOIOCTLCMD;
	}

}

static const struct file_operations pcf8563_misc_fops = {
	.unlocked_ioctl = pcf8563_misc_ioctl,
};

static struct miscdevice pcf8563_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pcf8563",
	.fops = &pcf8563_misc_fops,
};

extern void (*arm_pm_restart)(char str, const char *cmd);
#if PCF8563_IRQ	

void pcf8563_irq_disable()
{
    unsigned long irqflags;

    spin_lock_irqsave(&pcf8563->irq_lock, irqflags);
    if (!pcf8563->irq_is_disable)
    {
        pcf8563->irq_is_disable = 1; 
        disable_irq_nosync(pcf8563->pcf8563_client->irq);
    }
    spin_unlock_irqrestore(&pcf8563->irq_lock, irqflags);
}

void pcf8563_irq_enable()
{
    unsigned long irqflags = 0;
    
    spin_lock_irqsave(&pcf8563->irq_lock, irqflags);
    if (pcf8563->irq_is_disable) 
    {
        enable_irq(pcf8563->pcf8563_client->irq);
        pcf8563->irq_is_disable = 0; 
    }
    spin_unlock_irqrestore(&pcf8563->irq_lock, irqflags);
}


static irqreturn_t pcf8563_ts_irq_handler(int irq, void *dev_id)
{
#if 0
		unsigned char buf[1] = { PCF8563_REG_ST2 };
		struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = pcf8563->pcf8563_client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = pcf8563->pcf8563_client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(pcf8563->pcf8563_client->adapter, msgs, 2)) != 2) {
		printk( "%s: read error\n", __func__);
		return -EIO;
	}
	if(!(buf[0] && (1<<3))){
	printk("pcf8563_ts_irq_handler fail %d \n",buf[0]);
	return 0;
	}
	printk("pcf8563_ts_irq_handler %d pcf8563->status + %d  \n",buf[0],pcf8563->status);
	//mdelay(600);
	//pcf8563_en_alarm_hyman(1);
	
	hrtimer_start(&pcf8563->timer, pcf8563->poll_delay, HRTIMER_MODE_REL);
	//if(pcf8563->status == 0)
		{
		//pcf8563_set_offalarm_hyman(&pcf8563->onalarm);
		//pcf8563_en_alarm_hyman(1);
//		struct rtc_time tm;
		//pcf8563_read_datetime_hyman(&tm);
		
	//	printk("pcf8563_ts_irq_handler ok \n");
		
		 //arm_pm_restart('r', "reboot");
		//rk818_shutdown_pcf8563();
	}
#endif		
	pcf8563_irq_disable();
	pcf8563_en_alarm_hyman(1);
	printk(" pcf8563_ts_irq_handler     \n");
	//hrtimer_start(&pcf8563->timer, pcf8563->poll_delay, HRTIMER_MODE_REL);
	queue_work(pcf8563->pcf_init_wq,&pcf8563->pcf_work);
	return IRQ_HANDLED;
}
#if 0
static enum hrtimer_restart jsa_als_timer_func(struct hrtimer *timer)
{
	unsigned char buf[1] = { PCF8563_REG_ST2 };
	struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = pcf8563->pcf8563_client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = pcf8563->pcf8563_client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(pcf8563->pcf8563_client->adapter, msgs, 2)) != 2) {
		printk( "%s: read error\n", __func__);
		return -EIO;
	}
	if(!(buf[0] && (1<<3))){
	printk("pcf8563_ts_irq_handler fail %d \n",buf[0]);
	return 0;
	}
	printk("pcf8563 jsa_als_timer_func start %d pcf8563->status  = %d \n",buf[0],pcf8563->status );
	pcf8563_irq_enable();
	pcf8563_en_alarm_hyman(1);
	if(pcf8563->status == 1)
	{
		printk("pcf8563 shutdown\n");
		pcf8563_set_offalarm_hyman(&pcf8563->onalarm);
		
		//arm_pm_restart('r', "shutdown");//hyman
	}else{
		printk("irq reboot \n");
		//arm_pm_restart('r', "reboot");//hyman
	}
	printk("pcf8563 jsa_als_timer_func end %d \n",buf[0]);
	return HRTIMER_RESTART;
}
#endif
extern void (*pm_power_off)(void);

void pcf_work(struct work_struct *work)
{
	unsigned char buf[1] = { PCF8563_REG_ST2 };
	struct i2c_msg msgs[] = {
		{/* setup read ptr */
			.addr = pcf8563->pcf8563_client->addr,
			.len = 1,
			.buf = buf
		},
		{/* read status + date */
			.addr = pcf8563->pcf8563_client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf
		},
	};

	/* read registers */
	if ((i2c_transfer(pcf8563->pcf8563_client->adapter, msgs, 2)) != 2) {
		printk( "%s: read error\n", __func__);
		return -EIO;
	}
	if(!(buf[0] && (1<<3))){
	printk("pcf8563_ts_irq_handler fail %d \n",buf[0]);
	return 0;
	}
	printk("pcf8563 jsa_als_timer_func start %d pcf8563->status  = %d \n",buf[0],pcf8563->status );
	pcf8563_irq_enable();
	pcf8563_en_alarm_hyman(1);
	if(pcf8563->status == 1)
	{
		printk("pcf8563 shutdown\n");
		pcf8563_set_offalarm_hyman(&pcf8563->onalarm);
		msleep(1500);
		//arm_pm_restart('h', "charge");//hyman
		pm_power_off();
	}else{
		printk("irq reboot \n");
		msleep(1500);
		arm_pm_restart('r', "reboot");//hyman
	}
	printk("pcf8563 jsa_als_timer_func end %d \n",buf[0]);
	return HRTIMER_RESTART;
}



#endif
static int pcf8563_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{

	dev_dbg(&client->dev, "%s\n", __func__);
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	pcf8563 = devm_kzalloc(&client->dev, sizeof(struct pcf8563),
				GFP_KERNEL);
	if (!pcf8563)
		return -ENOMEM;

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");

	//i2c_set_clientdata(client, pcf8563);

	//pcf8563->rtc = devm_rtc_device_register(&client->dev,
		//		pcf8563_driver.driver.name,
		//		&pcf8563_rtc_ops, THIS_MODULE);
		
	pcf8563->pcf8563_client = client;
	
	struct rtc_time tm;
	pcf8563_read_datetime_hyman(&tm);
	printk("pcf8563 probe time  %d:%d %d-%d \n",tm.tm_hour,tm.tm_min,tm.tm_mday,tm.tm_wday);
	pcf8563_read_alarm_hyman(&tm);
	printk("pcf8563 probe alarm %d:%d %d-%d \n",tm.tm_hour,tm.tm_min,tm.tm_mday,tm.tm_wday);
	misc_register(&pcf8563_misc);
#if PCF8563_IRQ	

	struct device_node *np = client->dev.of_node;
	pcf8563->irq_pin = of_get_named_gpio_flags(np, "irq-gpio", 0, (enum of_gpio_flags *)(&pcf8563->irq_flags));
	
	spin_lock_init(&pcf8563->irq_lock); 
    const u8 irq_table[] = {IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING, IRQ_TYPE_LEVEL_LOW, IRQ_TYPE_LEVEL_HIGH};
    int ret;
    pcf8563->irq=gpio_to_irq(pcf8563->irq_pin);       //If not defined in client
    if (pcf8563->irq)
    {
        pcf8563->pcf8563_client->irq = pcf8563->irq;
        ret = devm_request_threaded_irq(&(pcf8563->pcf8563_client->dev), pcf8563->irq, NULL, 
            pcf8563_ts_irq_handler, IRQ_TYPE_LEVEL_LOW | IRQF_ONESHOT /*irq_table[ts->int_trigger_type]*/, 
            pcf8563->pcf8563_client->name, pcf8563);
        if (ret != 0) {
            printk("devm_request_threaded_irq fail \n");
        }
        //gtp_irq_disable(ts->irq);
        printk("  <%s>_%d     ts->irq=%d   ret = %d\n", __func__, __LINE__, pcf8563->irq, ret);
    }else{
        printk("   ts->irq  error \n");

    }
//	hrtimer_init(&pcf8563->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
//	pcf8563->poll_delay = ns_to_ktime(50 * NSEC_PER_MSEC);
//	pcf8563->timer.function = jsa_als_timer_func;	

	pcf8563->pcf_init_wq = create_singlethread_workqueue("pcf_work_wq");
	INIT_WORK(&pcf8563->pcf_work, pcf_work);

	pcf8563_irq_enable();
#endif	
	pcf8563_en_alarm_hyman(1);
	if (IS_ERR(pcf8563->rtc))
		return PTR_ERR(pcf8563->rtc);

	return 0;
}

static int pcf8563_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id pcf8563_id[] = {
	{ "pcf8563", 0 },
	{ "rtc8564", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pcf8563_id);

#ifdef CONFIG_OF
static const struct of_device_id pcf8563_of_match[] = {
	{ .compatible = "nxp,pcf8563" },
	{}
};
MODULE_DEVICE_TABLE(of, pcf8563_of_match);
#endif

static struct i2c_driver pcf8563_driver = {
	.driver		= {
		.name	= "rtc-pcf8563",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(pcf8563_of_match),
	},
	.probe		= pcf8563_probe,
	.remove		= pcf8563_remove,
	.id_table	= pcf8563_id,
};

module_i2c_driver(pcf8563_driver);

MODULE_AUTHOR("Alessandro Zummo <a.zummo@towertech.it>");
MODULE_DESCRIPTION("Philips PCF8563/Epson RTC8564 RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
