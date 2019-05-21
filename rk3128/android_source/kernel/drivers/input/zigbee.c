#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

//#define power_io

struct zigbee_sensor_t{
	int zigbee_en_gpio;
	int zigbee_reset_gpio;
	int zigbee_en_flag;
	int zigbee_reset_flag;

};
static struct zigbee_sensor_t *zigbee_sensor = NULL;
 
extern int vcc_sd_enable(int value);
static const struct of_device_id of_rk_nix_sensor_match[] = {
	{ .compatible = "zigbee_h" },
	{ /* Sentinel */ }
};
void zigbee_power_io(int status)
{
	#ifdef power_io
		if ( gpio_direction_output(zigbee_sensor->zigbee_en_gpio,1) < 0 ){
			printk("zigbee_en_gpio output fail\n");
    		return ;	
		}
	#else
		
		vcc_sd_enable(status);
	#endif
}
void zigbee_reset_io(int status)
{

		if ( gpio_direction_output(zigbee_sensor->zigbee_reset_gpio,status) < 0 ){
			printk("zigbee_reset_gpio output fail\n");
    		return ;	
		}

}	
static ssize_t zigbee_reset_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	char cmd;
	
	sscanf(buf, "%c", &cmd);
	if (cmd == 'r'){
		zigbee_power_io(0);
		zigbee_reset_io(0);
		msleep(350);
		zigbee_power_io(1);
		msleep(50);
		zigbee_reset_io(1);
		
	} else if(cmd == 'h'){
		zigbee_power_io(1);
		zigbee_reset_io(1);
	}else if(cmd == 'l'){
		zigbee_power_io(0);
		zigbee_reset_io(0);
	}
	else
		printk("command error \n");

	return count;
}

static ssize_t zigbee_reset_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;//sprintf(buf, "power %d , reset %d.\n",gpio_get_value(zigbee_sensor->zigbee_en_gpio) ,gpio_get_value(zigbee_sensor->zigbee_reset_gpio));
}

static struct device_attribute zigbee_reset_attr[] = {
	__ATTR(zigbee_reset, 0664, zigbee_reset_read,zigbee_reset_store),
};


static int zigbee_sensor_probe(struct platform_device *pdev)
{
	enum of_gpio_flags flags;
    struct device_node *sensor_node = pdev->dev.of_node;
	
 
    printk("zigbee_sensor_probe start\n");
    zigbee_sensor = kzalloc(sizeof(struct zigbee_sensor_t), GFP_KERNEL);
    if (!zigbee_sensor)
    {
		printk("zigbee kzalloc fail");
        goto failed;
    }
#ifdef power_io
    zigbee_sensor->zigbee_en_gpio = of_get_named_gpio_flags(sensor_node,"zigbee-en-gpio", 0,&flags);
    if (!gpio_is_valid(zigbee_sensor->zigbee_en_gpio)){
    	printk("zigbee-en-gpio: %d  fail \n",zigbee_sensor->zigbee_en_gpio);
    	return -1;
    }
	zigbee_sensor->zigbee_en_flag = (flags & OF_GPIO_ACTIVE_LOW)? 1:0;
#endif
	
	zigbee_sensor->zigbee_reset_gpio = of_get_named_gpio_flags(sensor_node,"zigbee-reset-gpio", 0,&flags);
    if (!gpio_is_valid(zigbee_sensor->zigbee_reset_gpio)){
    	printk("zigbee_reset_gpio: %d  fail \n",zigbee_sensor->zigbee_reset_gpio);
    	return -1;
    }
	zigbee_sensor->zigbee_reset_flag = (flags & OF_GPIO_ACTIVE_LOW)? 0:1;
#ifdef power_io	
    if ( gpio_request(zigbee_sensor->zigbee_en_gpio, "zigbee_en_gpio") < 0 ){

		printk("zigbee_en_gpio request  fail\n");
    	gpio_free(zigbee_sensor->zigbee_en_gpio);
    	goto failed;	
    }
#endif
    if ( gpio_request(zigbee_sensor->zigbee_reset_gpio, "zigbee_reset_gpio") < 0 ){

		printk("zigbee_reset_gpio request fail\n");
    	gpio_free(zigbee_sensor->zigbee_reset_gpio);
    	goto failed;	
    }

//	zigbee_power_io(0);
	zigbee_reset_io(0);
//	msleep(350);
//	zigbee_power_io(1);
	msleep(100);
	zigbee_reset_io(1);

	
	if (sysfs_create_file(&pdev->dev.kobj,&zigbee_reset_attr[0].attr))
			printk("zigbee sysfs_create_file fail\n");

	return 0;
failed:
    return -1;
}

static int zigbee_sensor_remove(struct platform_device *pdev)
{	
	if(zigbee_sensor->zigbee_reset_gpio != 0)
		gpio_free(zigbee_sensor->zigbee_reset_gpio);
#ifdef power_io	
	if(zigbee_sensor->zigbee_en_gpio != 0)
		gpio_free(zigbee_sensor->zigbee_en_gpio);
#endif
    return 0;
}
static struct platform_driver zigbee_sensor_driver = {
	.probe		= zigbee_sensor_probe,
	.remove		= zigbee_sensor_remove,
	.driver		= {
	.name	= "nix-sensor",
	.owner	= THIS_MODULE,
	.of_match_table	= of_rk_nix_sensor_match,
	},

};

static int __init zigbee_sensor_init(void)
{
    printk(KERN_INFO "Enter hyman %s\n", __FUNCTION__);
    return platform_driver_register(&zigbee_sensor_driver);
}

static void __exit zigbee_sensor_exit(void)
{
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);	
    platform_driver_unregister(&zigbee_sensor_driver);
}

subsys_initcall(zigbee_sensor_init);
module_exit(zigbee_sensor_exit);

MODULE_DESCRIPTION("zigbee sensor driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:zigbee");
