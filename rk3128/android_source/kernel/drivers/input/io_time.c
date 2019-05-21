#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>

#include <linux/wakelock.h>
#include <linux/suspend.h>

#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>
#include <linux/fs.h>
#include <asm/gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>

static unsigned long value_time;
typedef enum _RMC_STATE
{
    RMC_IDLE,
    RMC_PRELOAD,
    RMC_USERCODE,
    RMC_GETDATA,
    RMC_SEQUENCE
}eRMC_STATE;


struct RKxx_remotectl_platform_data {
	//struct rkxx_remotectl_button *buttons;
	int nbuttons;
	int rep;
	int gpio;
	int active_low;
	int timer;
	int wakeup;
	void (*set_iomux)(void);
};

struct rkxx_remotectl_suspend_data{
    int suspend_flag;
    int cnt;
    long scanTime[50];
};

struct rkxx_remotectl_drvdata {
    int state;
	int nbuttons;
	int result;
    unsigned long pre_time;
    unsigned long cur_time;
    long int pre_sec;
    long int cur_sec;
    long period;
    int scanData;
    int count;
    int keybdNum;
    int keycode;
    int press;
    int pre_press;
    int gpio;
    int wakeup;
    int rep;
    unsigned long value_time;
    struct input_dev *input;
    struct timer_list timer;
    struct tasklet_struct remote_tasklet;
    struct wake_lock remotectl_wake_lock;
    struct rkxx_remotectl_suspend_data remotectl_suspend_data;
};

static void remotectl_timer(unsigned long _data)
{
    struct rkxx_remotectl_drvdata *ddata =  (struct rkxx_remotectl_drvdata*)_data;
    
    printk("to\n");
    
    if(ddata->press != ddata->pre_press) {
        ddata->pre_press = ddata->press = 0;
        
				input_event(ddata->input, EV_KEY, ddata->keycode, 0);
        input_sync(ddata->input);
        //printk("5\n");
        //if (get_suspend_state()==0){
            //input_event(ddata->input, EV_KEY, ddata->keycode, 1);
            //input_sync(ddata->input);
            //input_event(ddata->input, EV_KEY, ddata->keycode, 0);
		    //input_sync(ddata->input);
        //}else if ((get_suspend_state())&&(ddata->keycode==KEY_POWER)){
            //input_event(ddata->input, EV_KEY, KEY_WAKEUP, 1);
            //input_sync(ddata->input);
            //input_event(ddata->input, EV_KEY, KEY_WAKEUP, 0);
            //input_sync(ddata->input);
        //}
    }
#ifdef CONFIG_PM
   // remotectl_wakeup(_data);
#endif
    ddata->state = RMC_PRELOAD;
}


static void remotectl_do_something(unsigned long  data)
{
	unsigned gpio;
    struct rkxx_remotectl_drvdata *ddata = (struct rkxx_remotectl_drvdata *)data;
	int ret = gpio_get_value(ddata->gpio);
		
	if(ret == 0){
		ddata->value_time = ddata->period;
		value_time= ddata->period;
		printk("hyman  11  remotectl_do_something ddata-> %d \n",ddata->period);
	}
	printk("hyman remotectl_do_something ddata-> %d \n",ddata->period);
}


static irqreturn_t remotectl_isr(int irq, void *dev_id)
{

    struct rkxx_remotectl_drvdata *ddata =  (struct rkxx_remotectl_drvdata*)dev_id;
    struct timeval  ts;
#if 1
    ddata->pre_time = ddata->cur_time;
    ddata->pre_sec = ddata->cur_sec;
    do_gettimeofday(&ts);
    ddata->cur_time = ts.tv_usec;
    ddata->cur_sec = ts.tv_sec;  
		if (likely(ddata->cur_sec == ddata->pre_sec)){
			ddata->period =  ddata->cur_time - ddata->pre_time;
			//printk("hyman remotectl_isr ddata-> %d         3 \n",ddata->period);
	  }else{
			//printk("hyman remotectl_isr ddata-> %d         4 \n",ddata->period);
				ddata->period =  1000000 - ddata->pre_time + ddata->cur_time;
		}
		
	//printk("hyman remotectl_isr ddata-> %d \n",ddata->period);
    tasklet_hi_schedule(&ddata->remote_tasklet); 
    //if ((ddata->state==RMC_PRELOAD)||(ddata->state==RMC_SEQUENCE))
    //mod_timer(&ddata->timer,jiffies + msecs_to_jiffies(130));

#endif
    return IRQ_HANDLED;
}


static uint32_t _show_on(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	
	unsigned long value;	
	value = value_time;
	value_time = 0;
	return 	sprintf(buf, "%d", value);	
}

static struct device_attribute dev_attr_gpio_time = __ATTR(gpio_time,0664,_show_on,NULL);
static struct attribute *dev_attrs[] = {
	&dev_attr_gpio_time.attr,
	NULL,
};
static struct attribute_group dev_attr_grp = {
		.attrs = dev_attrs,
};

static int remotectl_probe(struct platform_device *pdev)
{

    struct rkxx_remotectl_drvdata *ddata;
    struct input_dev *input;
    unsigned int gpio;
    int i, j;
    int irq, flag;
    int error = 0,ret;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	printk("hyman proble io time high start \n");	
	
	ddata = kzalloc(sizeof(struct rkxx_remotectl_drvdata),GFP_KERNEL);
    memset(ddata,0,sizeof(struct rkxx_remotectl_drvdata));
	gpio = of_get_named_gpio(node, "io_gpios", 0);
		if (!gpio_is_valid(gpio)) {
			printk("invalid gpio specified\n");
			error = -2;
			goto fail;
	}
	printk("hyman proble io time high start 1 \n");		
  error = devm_gpio_request(dev, gpio, "io_time");
	if (error < 0) {
		printk("gpio-keys: failed to request GPIO %d,"
		" error %d\n", gpio, error);
		goto fail0;
	}

	printk("hyman proble io time high start 2 \n");	
	error = gpio_direction_input(gpio);
	if (error < 0) {
		pr_err("gpio-keys: failed to configure input"
			" direction for GPIO %d, error %d\n",
		gpio, error);
		gpio_free(gpio);
		goto fail0;
	}
	printk("hyman proble io time high start 3 \n");	
    irq = gpio_to_irq(gpio);
	if (irq < 0) {
		error = irq;
		pr_err("gpio-keys: Unable to get irq number for GPIO %d, error %d\n",	gpio, error);
		gpio_free(gpio);
		goto fail1;
	}
	printk("hyman proble io time high start 4 \n");	
    ddata->gpio = gpio;
    ddata->wakeup = 1;

	error = request_irq(irq, remotectl_isr,	IRQF_TRIGGER_FALLING |  IRQF_TRIGGER_RISING , "io_high_irq", ddata);
	printk("hyman irq error = %d \n",error);
//    setup_timer(&ddata->timer,remotectl_timer, (unsigned long)ddata);
    
    tasklet_init(&ddata->remote_tasklet, remotectl_do_something, (unsigned long)ddata);
  
	sysfs_create_group(&dev->kobj, &dev_attr_grp);
	
	printk("hyman proble io time high end \n");	
	return 0;
fail2:
    pr_err("gpio-remotectl input_allocate_device fail\n");
	input_free_device(input);
	kfree(ddata);
fail1:
    pr_err("gpio-remotectl gpio irq request fail\n");
  //  free_irq(gpio_to_irq(gpio), ddata);
//    del_timer_sync(&ddata->timer);
 //   tasklet_kill(&ddata->remote_tasklet); 
 //   gpio_free(gpio);
fail0: 
    pr_err("gpio-remotectl input_register_device fail\n");
  //  platform_set_drvdata(pdev, NULL);
fail:
return -1;	
}

static int remotectl_remove(struct platform_device *pdev)
{

	return 0;
}


#ifdef CONFIG_PM
static int remotectl_suspend(struct device *dev)
{

	return 0;
}

static int remotectl_resume(struct device *dev)
{

	return 0;
}


static const struct dev_pm_ops remotectl_pm_ops = {
	.suspend	= remotectl_suspend,
	.resume		= remotectl_resume,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id of_rk_remotectl_match[] = {
	{ .compatible = "gpio-high-time" },
	{ /* Sentinel */ }
};
#endif

static struct platform_driver remotectl_device_driver = {
	.probe		= remotectl_probe,
	.remove		= remotectl_remove,
	.driver		= {
		.name	= "gpio-high-time",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
	    .pm	= &remotectl_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table	= of_rk_remotectl_match,
#endif
	},

};

static int  remotectl_init(void)
{
    printk("hyman _init\n");
	
    return platform_driver_register(&remotectl_device_driver);
}


static void  remotectl_exit(void)
{
	platform_driver_unregister(&remotectl_device_driver);
    printk(KERN_INFO "++++++++remotectl_init\n");
}

module_init(remotectl_init);
module_exit(remotectl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hyman");
MODULE_DESCRIPTION("driver for CPU GPIOs");
MODULE_ALIAS("platform:gpio-high-time");