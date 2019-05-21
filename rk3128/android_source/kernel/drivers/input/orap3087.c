/* **************************************************************************
 *   AP3087B - Linux kernel module sample code for				Ver 3.0
 * 	CRYSTOTECH AP3087B ambient light sensor
 ****************************************************************************/


#include <linux/module.h>	/* for module creation */
#include <linux/init.h>	/* for initialization */
#include <linux/slab.h>	/* for memory allocation, such as function kzalloc().*/
#include <linux/i2c.h>
#include <linux/mutex.h>	/* blocking mutal exclusion locks */
#include <linux/delay.h>	/* delay routines */
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>

#define AP3087B_DRV_NAME	"AP3087B"
#define DRIVER_VERSION	"1.0"

#define AP3087B_ADDR				0x44
#define	AP3087B_ALL_REG_COUNT		0x10
/* registers */
#define	AP3087B_INTERNAL_REG		0x00
#define	AP3087B_CONFIGURE_REG		0x01
#define	AP3087B_INTERRUPT_REG		0x02
#define	AP3087B_PROX_LT_REG		0x03
#define	AP3087B_PROX_HT_REG		0x04
#define	AP3087B_ALSIR_TH1_REG		0x05
#define	AP3087B_ALSIR_TH2_REG		0x06
#define	AP3087B_ALSIR_TH3_REG		0x07
#define	AP3087B_PROX_DATA_REG		0x08
#define	AP3087B_ALSIR_DT1_REG		0x09
#define	AP3087B_ALSIR_DT2_REG		0x0a
#define	AP3087B_ALS_RNG_REG		0x0b
#define	AP3087B_PRST_CNT_REG		0x0d
#define	AP3087B_TEST1_REG			0x0e
#define	AP3087B_TEST2_REG			0x0f

/* relevant bits */
	static union{
		unsigned short dirty_addr_buf[2];
		const unsigned short normal_i2c[2];
	}u_i2c_addr = {{0x00},};
#define	AP3087B_PROX_EN_SHIFT		0x07
#define	AP3087B_PROX_EN_MASK		0x01
#define	AP3087B_PROX_SLP_SHIFT		0x04
#define	AP3087B_PROX_SLP_MASK		0x07
#define	AP3087B_PROX_DR_SHIFT		0x03
#define	AP3087B_PROX_DR_MASK		0x01
#define	AP3087B_ALS_EN_SHIFT		0x02
#define	AP3087B_ALS_EN_MASK		0x01
#define	AP3087B_ALSIR_MODE_SHIF	0x00
#define	AP3087B_ALSIR_MODE_MASK	0x01

#define	AP3087B_PROX_FLAG_SHIFT	0x07
#define	AP3087B_PROX_FLAG_MASK		0x01
#define	AP3087B_PROX_PRST_SHIFT	0x05
#define	AP3087B_PROX_PRST_MASK		0x03
#define	AP3087B_ALS_FLAG_SHIFT		0x03
#define	AP3087B_ALS_FLAG_MASK		0x01
#define	AP3087B_ALS_PRST_SHIFT		0x01
#define	AP3087B_ALS_PRST_MASK		0x03
#define	AP3087B_INT_CTRL_SHIFT		0x00
#define	AP3087B_INT_CTRL_MASK		0x01

#define	AP3087B_ALSIR_HT_SHIFT		0x04
#define	AP3087B_ALSIR_HT_MASK		0x0f
#define	AP3087B_ALSIR_LT_SHIFT		0x00
#define	AP3087B_ALSIR_LT_MASK		0x0f

#define	AP3087B_ALSIR_DT2_SHIFT	0x00
#define	AP3087B_ALSIR_DT2_MASK		0x0f

#define	AP3087B_ALS_RANGE_SHIFT	0x00
#define	AP3087B_ALS_RANGE_MASK		0x07
#define	AP3087B_ALS_PRST_CNT_SHIFT	0x04
#define	AP3087B_ALS_PRST_CNT_MASK	0x0f
#define	AP3087B_PROX_PRST_CNT_SHIFT	0x00
#define	AP3087B_PROX_PRST_CNT_MASK	0x0f
#define AUDIO_ON					0

#define SENSOR_ON	1
#define SENSOR_OFF	0
#define LIGHTSENSOR_IOCTL_MAGIC 'l'
#define LIGHTSENSOR_IOCTL_GET_ENABLED	 _IOR(LIGHTSENSOR_IOCTL_MAGIC, 1, int *) 
#define LIGHTSENSOR_IOCTL_ENABLE	 _IOW(LIGHTSENSOR_IOCTL_MAGIC, 2, int *) 
#define LIGHTSENSOR_IOCTL_DISABLE        _IOW(LIGHTSENSOR_IOCTL_MAGIC, 3, int *)

static struct i2c_adapter *adapter;

 
struct work_struct ls_work_wq;
struct i2c_client *ls_client;

struct AP3087B_data *ls_data;
struct Orap_ts_priv {
	struct i2c_client *client;
	struct work_struct  ls_work;
	struct hrtimer timer;
	ktime_t light_poll_delay;
	struct workqueue_struct *ls_init_wq;
	struct input_dev *input_dev;
	int ls_power;
	int pwr_pin;
	bool pwr_flags;
	int statue;
};

static struct Orap_ts_priv *light;

static void orap_resume_events(struct work_struct *work);
static void orap_init_events(struct work_struct *work);
struct workqueue_struct *orap_resume_wq;
struct workqueue_struct *orap_init_wq;
static DECLARE_WORK(orap_resume_work, orap_resume_events);
static DECLARE_WORK(orap_init_work, orap_init_events);
static void orap_value_report(struct input_dev *input, int data);
	
struct AP3087B_data {
	struct i2c_client *client;
	struct mutex lock;
	u8 reg_buf[AP3087B_ALL_REG_COUNT + 1];	
/* when sending AP3087B WRITE command, reg_buf[0] will always be register offest */
/* Please notice that AP3087B Read command consists of two parts: 		*/
/* 1. Write register offset first, 						*/
/* 2. read any data bytes you want starting from offest register 		*/
};

static void orap_resume_events(struct work_struct *work){


}
static void orap_init_events(struct work_struct *work){

	struct Orap_ts_priv *Orap_priv = container_of(work,struct Orap_ts_priv,ls_work);
	hrtimer_start(&Orap_priv->timer, Orap_priv->light_poll_delay, HRTIMER_MODE_REL);
}



/*
 * write data to register(s)
 * struct AP3087B_data->reg_buf should be prepared before calling
 * parameter is the real data byte count, excluding starting offset byte
 * 1. for WRITE operation:
 *    struct msg->len should be one more than the real data byte count 
 *     because of the first byte for starting register offset
 *    That is, AP3087B->reg_buf[0] should always be starting register offset
 * 2. for READ operation:
 *    send just one byte for starting register offset. count should be 0.
 */

static int AP3087B_write_register(struct i2c_client *client, int reg, int count)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct AP3087B_data *data = i2c_get_clientdata(client);
	struct i2c_msg msg;

	msg.addr = client->addr;		
	/* AP3087B I2C slave address is 0x44 */
	msg.flags = 0;				
	/* write operation, defintion in i2c.h */ 
	msg.len = count + 1; 			
	/* one byte more besides real data bytes, i.e. register-offest byte */
	msg.buf = (__u8 *) data->reg_buf;
	/* just point to register data buffer in struct AP3087B_data */
	msg.scl_rate=200 * 1000;
	i2c_transfer(adapter, &msg, 1);
 	/* write some byte from I2C slave according to struc i2c_msg */
	
	return 0;
} 
 /*
 * read data from register(s)
 * all request return information are stored in strcut AP3087B_data
 */

static int AP3087B_read_register(struct i2c_client *client, int reg, int count)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct AP3087B_data *data = i2c_get_clientdata(client);
	struct i2c_msg msg;

	/* 1. write starting register offset to I2C slave 	*/

	data->reg_buf[0] = (u8) reg;
	AP3087B_write_register(client, reg, 0); 
	/* count == 0 means there is only register offset byte */

	/* 2. read registers data	*/

	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = count;
	msg.buf = (__u8 *) data->reg_buf;
	msg.scl_rate=200 * 1000;
	i2c_transfer(adapter, &msg, 1); 	
	/* just start I2C read cycles */
	
	return 0;
}

/*
 * When initialization, set AP3087B 7 registers 
 * based on the driver private data, i.e. data->reg_buf[]
 * no other data structure initialization
 */
#if 0 
static int AP3087B_init_client(struct i2c_client *client)
{
	int i;
	struct AP3087B_data *data = i2c_get_clientdata(client);
	
	mutex_lock(&data->lock);

	i = 0;
	data->reg_buf[i++] = AP3087B_CONFIGURE_REG;
	data->reg_buf[i++] = 0xb4;
	data->reg_buf[i++] = 0x03;
	data->reg_buf[i++] = 0x40;
	data->reg_buf[i++] = 0x80;
	data->reg_buf[i++] = 0x00;
	data->reg_buf[i++] = 0xf1;
	data->reg_buf[i++] = 0xff;

	AP3087B_write_register(client, AP3087B_CONFIGURE_REG, 4);

	mutex_unlock(&data->lock);

	return 0;
}
#endif 
/*
 * sysfs layer. That is, attribute files for user application access.
 * each "attribute" has two functions: 1. read (show), 2. write (store)
 */


/*
 * AP3087B lux reading
 * input parameter, buf, is area for sysfs
 */

static ssize_t AP3087B_lux_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	u16	lux;
	struct i2c_client *client = to_i2c_client(dev);		
 	/* just find out the starting address of struct i2c_client 	*/
	/*  according to struct device 					*/	
	/* struct device is in struct device 				*/
	struct AP3087B_data *data = i2c_get_clientdata(client);
	/* get private data which is set by i2c_set_clientdata() */

	mutex_lock(&data->lock);	
	/* exlcusively lock this private data structure, struct AP3087B_data 	*/

	AP3087B_read_register(client, AP3087B_ALSIR_DT1_REG, 2);	
	/* read 2 bytes with register offset AP3087B_ALSIR_DT1_REG 	*/
	lux = data->reg_buf[1] & AP3087B_ALSIR_DT2_MASK;		
	/* get lower 4 bits of high bytes	*/
	lux <<= 8;						
	/* shift left 8 bits	*/
	lux |= data->reg_buf[0];				
	/* OR low byte		*/

	mutex_unlock(&data->lock);
	/* release struct AP3087B_data 	*/

	return sprintf(buf, "%d\n", lux);
	/* print result to file buffer in ASCII format, i.e. a string	*/
}

static DEVICE_ATTR(AP3087B_lux, S_IRUGO, AP3087B_lux_show, NULL);
	/* __ATTR(name, mode, show, store) in device.h, line 306	*/
	/* __ATTR define in sysfs.h, line 48				*/
	/* define __ATTR(_name, _mode, _show, _store) { \		*/
	/* .attr = {.name = __stringify(_name), .mode = _mode}, \	*/
	/*	    .show = _show,				     \	*/
	/* 	    .store= _store,				     \	*/
	/* }								*/
	/* mode == S_IRUGO : Readable for User, Group & Others		*/ 

/*AP3087B proximity reading	*/

static ssize_t AP3087B_prox_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	u16	prox;
	struct i2c_client *client = to_i2c_client(dev);
	struct AP3087B_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock); /* lock data everytime when accessing it */

	AP3087B_read_register(client, AP3087B_PROX_DATA_REG, 1);
	prox = data->reg_buf[0];

	mutex_unlock(&data->lock);

	return sprintf(buf, "%d\n", prox);
}
static DEVICE_ATTR(AP3087B_prox, S_IRUGO, AP3087B_prox_show, NULL);


/*
 * AP3087B read the content of all 11 registers into a single string with
 *  22 ASCII characters
 * For example, the string content might look like "b403408000f1ff3208"
 */
static ssize_t AP3087B_all_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{   	u8	*ccc;
	int 	nnn;
	struct i2c_client *client = to_i2c_client(dev);
	struct AP3087B_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock); /* lock data everytime when accessing it */

	AP3087B_read_register(client, AP3087B_CONFIGURE_REG, 11);
	ccc = data->reg_buf;
	nnn = sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	 ccc[0], ccc[1], ccc[2], ccc[3], ccc[4], ccc[5], ccc[6], ccc[7], ccc[8], ccc[9], ccc[10]);

	mutex_unlock(&data->lock);

	return nnn;
}
static DEVICE_ATTR(AP3087B_all, S_IRUGO, AP3087B_all_show, NULL);

/*AP3087B  configure register reading	 */
#if 0
static ssize_t AP3087B_configure_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	u16	conf;
	struct i2c_client *client = to_i2c_client(dev);
	struct AP3087B_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock); /* lock data everytime when accessing it */

	AP3087B_read_register(client, AP3087B_CONFIGURE_REG, 1);
	conf = (u16) data->reg_buf[0];

	mutex_unlock(&data->lock);

	return sprintf(buf, "%x\n", conf);
}

/*
 * AP3087B  configure register writing (store)
 */

static ssize_t AP3087B_configure_store(struct device *dev,
				 struct device_attribute *attr, char *buf, size_t count)
{
	unsigned long val;

	struct i2c_client *client = to_i2c_client(dev);
	struct AP3087B_data *data = i2c_get_clientdata(client);
	
	if (strict_strtoul(buf, 16, &val) < 0) return -EINVAL;

	mutex_lock(&data->lock); /* lock data everytime when accessing it */
	data->reg_buf[0] = AP3087B_CONFIGURE_REG;
	data->reg_buf[1] = (u8) val;

	AP3087B_write_register(client, AP3087B_CONFIGURE_REG, 1);

	mutex_unlock(&data->lock);

	return count;
}
#endif
//static DEVICE_ATTR(AP3087B_configure, S_IRUGO | S_IWUGO, AP3087B_configure_show, AP3087B_configure_store);
	/* mode == S_IWUGO: Writable for User, Group & Others */

static struct attribute *AP3087B_attributes[] = {
	&dev_attr_AP3087B_lux.attr,
	&dev_attr_AP3087B_prox.attr,
	//&dev_attr_AP3087B_configure.attr,
	&dev_attr_AP3087B_all.attr,
	NULL
};


static const struct attribute_group AP3087B_attr_group = { 
	/* all related attributes form a group, this will be	*/
	/*  a subdirectory name in sysfs			*/
	.name = "AP3087B_attributes",
	.attrs = AP3087B_attributes,
};


int ReadRegister(struct i2c_client *client,uint8_t reg,unsigned char *buf, int ByteLen)
{
//	unsigned char buf[4];
	struct i2c_msg msg[2];
	int ret;

//	memset(buf, 0xFF, sizeof(buf));
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;
	msg[0].scl_rate=200 * 1000;
	
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = ByteLen;
	msg[1].buf = buf;
	msg[1].scl_rate=200 * 1000;
	ret = i2c_transfer(client->adapter, msg, 2);

	if(ret<0){
	
		return 0; 
	}else{
	
		return 1; 
	}

}


int WriteRegister(struct i2c_client *client,uint8_t *buf, int ByteLen)
{	
	//	unsigned char buf[4];
		struct i2c_msg msg;
		int ret;
	
	//	memset(buf, 0xFF, sizeof(buf));
		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = ByteLen;
		msg.buf = buf;
		msg.scl_rate=200 * 1000;
		ret = i2c_transfer(client->adapter, &msg, 1);

	
		if(ret<0){

			printk("i2c write err\n");
			return ret;	
		}else{

			return ret;	
		}
}


void ls_work(struct work_struct *work)
	{	
		char prox;
	//	struct i2c_msg msg[2];
		__u32 data;
		int ret;
		uint8_t als_date1;
		uint8_t als_date2;
		struct Orap_ts_priv *Orap_priv = container_of(work,struct Orap_ts_priv,ls_work);
		als_date1 = 0x09;
		als_date2 = 0x0A;
//while(1)
#if AUDIO_ON
		if(gpio_get_value(Orap_priv->audio_det.gpio) == 0){
			
			if(gpio_get_value(Orap_priv->audio_pa_ctrl.gpio) == 1){

			//	printk("ctrl = 1\n");
				__gpio_set_value(Orap_priv->audio_pa_ctrl.gpio, 0);
			}
			//printk("audio_pa_ctrl == 0 \n");
		}else{

			if(gpio_get_value(Orap_priv->audio_pa_ctrl.gpio) == 0){

				//printk("ctrl = 0\n");
				__gpio_set_value(Orap_priv->audio_pa_ctrl.gpio, 1);
			}
			//printk("audio_pa_ctrl == 1 \n");
		}
#endif		
		{
			ret = ReadRegister(Orap_priv->client,als_date1,&prox, 1);
			if(ret == 1){
			//	printk("prox  als_data1 = %d \n",prox);
				data = prox;
				prox = 0;
			
			}else{
				
			//	prox = 0;
			//	data = 0;
				//continue 1;
			}
			ret = ReadRegister(Orap_priv->client,als_date2,&prox, 1);
			if(ret == 1){
			//	printk("prox  als_date2 = %d \n",prox);
				data +=  (prox << 8);
				//input_report_abs(Orap_priv->input_dev, ABS_MISC, data);
				//input_sync(Orap_priv->input_dev);
				orap_value_report(Orap_priv->input_dev,data);
			//	printk("prox = %d \n",data);
				prox = 0;
				data = 0;
			}else{

				//continue 1;
				prox = 0;
				data = 0;
			}
			
			//msleep(1000);
		}
	}


static void orap_value_report(struct input_dev *input, int data)
{
	unsigned char index = 0;
	if(data <= 30){
		index = 0;goto report;
	}
	else if(data <= 160){
		index = 1;goto report;
	}
	else if(data <= 225){
		index = 2;goto report;
	}
	else if(data <= 320){
		index = 3;goto report;
	}
	else if(data <= 640){
		index = 4;goto report;
	}
	else if(data <= 1280){
		index = 5;goto report;
	}
	else if(data <= 10240){
		index = 6;goto report;
	}
	else{
		index = 7;goto report;
	}
report:
//	printk("orap report index = %d data=%d\n",index,data);
	input_report_abs(input, ABS_MISC, index);
	input_sync(input);
	return;
}	
	
	
static enum hrtimer_restart jsa_als_timer_func(struct hrtimer *timer)
{
	struct Orap_ts_priv *Orap_priv = container_of(timer, struct Orap_ts_priv, timer);
//	printk("time ===============\n");
//	if(light->statue == SENSOR_OFF)
//		return HRTIMER_RESTART;
	if(Orap_priv->ls_init_wq == NULL || &Orap_priv->ls_work == NULL)
		return HRTIMER_RESTART;
	queue_work(Orap_priv->ls_init_wq,&Orap_priv->ls_work);
	hrtimer_forward_now(&Orap_priv->timer, Orap_priv->light_poll_delay);
	return HRTIMER_RESTART;
}

static int orap_start(struct Orap_ts_priv *data)
{
//	struct Orap_ts_priv *orap = data;
	light->statue = SENSOR_ON;
	hrtimer_start(&light->timer, light->light_poll_delay, HRTIMER_MODE_REL);
////////////////////////////////////////////
#if 0
	struct Orap_ts_priv *orap = data;
	int ret = 0;
	char txData[2] = {0x01,0x80}; 
	
	printk("%s.....%d...\n",__FUNCTION__,__LINE__);
	if(orap->statue)
		return 0;
	ret = us5151_tx_data(orap->client,txData,sizeof(txData));
	if (ret == 0)
	{
		orap->statue = SENSOR_ON;
		orap->timer.expires  = jiffies + 3*HZ;
		add_timer(&orap->timer);
	}
	else
	{
		orap->statue = 0;
		printk("%s......%d ret=%d\n",__FUNCTION__,__LINE__,ret);
	}
#endif		
	printk("========== orap light sensor start ==========\n");

	return 0;
}

static int orap_stop(struct Orap_ts_priv *data)
{
//	struct Orap_ts_priv *orap = data;
	light->statue = SENSOR_OFF;
	hrtimer_cancel(&light->timer);
	cancel_work_sync(&light->ls_work);
#if 0
	struct Orap_ts_priv *orap = data;
	int ret = 0;
	char txData[2] = {0x01,0x00}; 

	printk("%s.....%d...\n",__FUNCTION__,__LINE__);
	if(orap->statue == 0)
		return 0;
	
	ret = us5151_tx_data(orap->client,txData,sizeof(txData));
	if (ret == 0)
	{
		orap->statue = SENSOR_OFF;
		del_timer(&orap->timer);
	}
	else
	{
		orap->statue = 0;
		printk("%s......%d ret=%d\n",__FUNCTION__,__LINE__,ret);
	}
#endif		
	printk("========== orap light sensor stop ==========\n");

	return 0;
}

static int orap_open(struct inode *indoe, struct file *file)
{
	//printk("%s.....%d\n",__FUNCTION__,__LINE__);
	return 0;
}

static int orap_release(struct inode *inode, struct file *file)
{
	//printk("%s.....%d\n",__FUNCTION__,__LINE__);
	return 0;
}
static char start_flag = 0;
static long orap_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long *argp = (unsigned long *)arg;

	//printk("%s.....%d\n",__FUNCTION__,__LINE__);
	switch(cmd){
		case LIGHTSENSOR_IOCTL_GET_ENABLED:
			*argp = light->statue;
			break;
		case LIGHTSENSOR_IOCTL_ENABLE:
			if(*argp)
			{
				start_flag = 1;
				orap_start(light);
			}
			else
			{
				start_flag = 0;
				orap_stop(light);
			}
			break;
		default:break;
	}
	return 0;
}

static struct file_operations orap_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = orap_ioctl,
	.open = orap_open,
	.release = orap_release,
};

static struct miscdevice orap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lightsensor",
	.fops = &orap_fops,
};


/*
 * I2C layer
 */

/*
 *  I2C is not a plug-and-play device.
 *  We actually has to resisgter this I2C "SLAVE" device in an I2C "MASTER"
 *  driver so that I2C master will scan whether some I2C slave exists or not
 */

/* static struct i2c_board_info USBIOX_hwmon_info = {  	*/
/*	I2C_BOARD_INFO("AP3087B", AP3087B_I2C_ADDR),	*/
/*	};						*/
/* 							*/
/* The above structure just show all of the I2C Salve	*/
/* devices attached to this I2C "MASTER" board		*/ 

/*
 * probe function will be called if bus probe, device probe or driver probe 
 */




static int AP3087B_probe(struct i2c_client *client,
				    const struct i2c_device_id *id)
 {
	//struct i2c_adapter *adapter
	/* struct i2c_client {				\	*/
	/* 	unsigned short flags;			\	*/
	/* 	unsigned short addr;			\	*/
	/* 	char name[I2C_NAME_SIZE]		\	*/
	/* 	struct i2c_adapter	*adapter;	\	*/
	/* 	struct i2c_driver	*driver;	\	*/
	/* 	struct device dev;			\	*/
	/* }							*/
	/* ref. i2c.h						*/ 
	struct AP3087B_data *data;
	struct Orap_ts_priv *Orap_priv;
	int ret,err;
	enum of_gpio_flags flags;
	u8 init_data[2]={0x0, 0x0};
	u8 init_data1[2]={0x0, 0x0};
	struct device_node *np = client->dev.of_node;
	printk("++++++AP3087B_probe +++++++\n");
	Orap_priv = kzalloc(sizeof(*Orap_priv), GFP_KERNEL);
	
	if (!Orap_priv){
		
		printk("	AP3087B_probe	: kzalloc err!\n");
		return -ENODEV;
	}
	else{
		
		printk("	AP3087B_probe	: kzalloc OK!\n");
	}
	light = Orap_priv;
	Orap_priv->client = client;
	i2c_set_clientdata(client, Orap_priv);
///////////////////////////////////////////
	Orap_priv->pwr_pin = of_get_named_gpio_flags(np, "power-gpio", 0, &flags);
	gpio_request(Orap_priv->pwr_pin, "Orap-power");
	Orap_priv->pwr_flags = (flags & OF_GPIO_ACTIVE_LOW)? 1:0;
	gpio_direction_output(Orap_priv->pwr_pin,!Orap_priv->pwr_flags);
	msleep(200);
	
	init_data[0]= 0x01;
	init_data[1]= 0x04;
	err = WriteRegister(client,init_data,2);
	if(err < 0)
		goto exit_kfree;
	
	init_data1[0]= 0x0B;
	init_data1[1]= 0x04;
	err = WriteRegister(client,init_data1,2);
	if(err < 0)
		goto exit_kfree;
////////////////////////////////////////////
	Orap_priv->input_dev = input_allocate_device();	
	if (!Orap_priv->input_dev) {	
			printk("%s: could not allocate input device\n", __func__);	
			return -ENOMEM;
	}	
	//input_set_drvdata(input_dev, jsa);
	Orap_priv->input_dev->name = "lightsensor-level";	
	input_set_capability(Orap_priv->input_dev, EV_ABS, ABS_MISC);	

	
	input_set_abs_params(Orap_priv->input_dev, ABS_MISC, 0, 0x1ff, 0, 0);
	
	
	ret = input_register_device(Orap_priv->input_dev);
	if (ret < 0) {	
			printk("%s: could not register input device\n", __func__);
			input_free_device(Orap_priv->input_dev);	
			return -ENOMEM;	
	}

	input_set_drvdata(Orap_priv->input_dev, Orap_priv);
	
	ret = misc_register(&orap_device);
	if (ret < 0) {
		printk("orap_device_probe: lightsensor_device register failed\n");
	//	goto exit_misc_register_fail;
	}
	
	Orap_priv->ls_init_wq = create_singlethread_workqueue("ls_work_wq");
	if (IS_ERR(Orap_priv->ls_init_wq)) 
	{
		printk("Creat ls workqueue failed.\n");
		return -ENOMEM;
		
	}

	///atomic_set(&Orap_priv->orap3087_suspend, 0);
	


	INIT_WORK(&Orap_priv->ls_work, ls_work);
	

	orap_resume_wq = create_singlethread_workqueue("orap_resume_wq");
	if (orap_resume_wq == NULL) {
			printk("create orap_resume_wq fail!\n");	
			return -ENOMEM;
	}	
	orap_init_wq = create_singlethread_workqueue("orap_init_wq");
	if (orap_init_wq == NULL) {
			printk("create orap_init_wq fail!\n");
			return -ENOMEM;	
	}

    /* ==================light sensor====================== */	
    /* hrtimer settings.  we poll for light values using a timer. */
	
	hrtimer_init(&Orap_priv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	Orap_priv->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	Orap_priv->timer.function = jsa_als_timer_func;	


	//queue_work(orap_init_wq,&orap_init_work);
/*	
	init_data[0]= 0x01;
	init_data[1]= 0x04;
	err = WriteRegister(client,init_data,2);
	if(err < 0)
		goto exit_kfree;
	
	init_data1[0]= 0x0B;
	init_data1[1]= 0x04;
	err = WriteRegister(client,init_data1,2);
	if(err < 0)
		goto exit_kfree;
*/
//	hrtimer_start(&Orap_priv->timer, Orap_priv->light_poll_delay, HRTIMER_MODE_REL);
////////////////////////////////////////////

	data = kzalloc(sizeof(struct AP3087B_data), GFP_KERNEL);	
	/* allocate kernel memory for AP3087B private data	*/

	if (!data)
		return -ENOMEM;
	
//	data->client = client;
//	i2c_set_clientdata(client, data);				
 	/* private data will be link to struct client for later use,		*/
	/* link .data to client->dev->dev					*/
	/* do not forget to free this memory block by kfree before remove 	*/
	mutex_init(&data->lock);
	/* lock struct AP3087B_data	*/

	/* initialize AP3087B */
//	err = AP3087B_init_client(client);


	/* register sysfs hooks */
//	err = sysfs_create_group(&client->dev.kobj, &AP3087B_attr_group);
	/* create sysfs for user access	*/
//	if (err) goto exit_kfree;
	//printk("=======AP3087B_probe  err == %d\n", err);
	//light->statue = SENSOR_ON;
	//hrtimer_start(&light->timer, light->light_poll_delay, HRTIMER_MODE_REL);
	/* show enabled message on screen	*/
	return 0;
exit_kfree:
	kfree(Orap_priv);
	return err;
}

static int AP3087B_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &AP3087B_attr_group);	
	/* delete the sysfs	
	*/
	misc_deregister(&orap_device);
	kfree(i2c_get_clientdata(client));	
	/* free the private data. an counter function of kzalloc 	*/
	return 0;
}

static const struct i2c_device_id AP3087B_id[] = {
	{ "AP3087B", 0 },		
	/* NAME[I2C_NAME_SIZE], driver_data	*/
	{}
};
MODULE_DEVICE_TABLE(i2c, AP3087B_id);	
	/* in module.h, line 139, MODULE_DEVICE_TABLE(type, name)	*/
	/* MODULE_GENERIC_TABLE(type##_device, name)			*/
	/* register which device will be supported by i2c bus		*/
#if 1
static int AP3087B_suspend(struct i2c_client *client, pm_message_t mesg){

	struct Orap_ts_priv *Orap_priv = i2c_get_clientdata(client);
#if 1
	u8 init_data[2]={0x01, 0x00};
	int ret = WriteRegister(client,init_data,2);
	if(ret < 0){
		printk("i2c  orap3087_suspend err\n");
		return ret;
	}
	//orap_stop(Orap_priv);
#endif
	gpio_direction_output(Orap_priv->pwr_pin,Orap_priv->pwr_flags);
	printk("orap3087_suspend================================\n");
	return 0;
	//printk("orap3087_suspend================================\n");
}


static int AP3087B_resume(struct i2c_client *client){

	struct Orap_ts_priv *Orap_priv = i2c_get_clientdata(client);
	//atomic_set(&Orap_priv->orap3087_suspend, 0);
#if 1
	int ret;
	u8 init_data[2]={0x01, 0x04};
	gpio_direction_output(Orap_priv->pwr_pin,!Orap_priv->pwr_flags);
	msleep(100);
	ret = WriteRegister(client,init_data,2);
	if(ret < 0){
		printk("i2c  orap3087_resume err\n");	
	}
	init_data[0]= 0x0B;
	init_data[1]= 0x04;
	ret = WriteRegister(client,init_data,2);
	if(ret < 0){
		printk("i2c  orap3087_resume err\n");	
	}
#endif
	//orap_start(Orap_priv);
	printk("orap3087_resume = =========================\n");
	return 0;
}
#endif
static struct of_device_id AP3087B_ids[] = {
    { .compatible = "AP3087B" },
    { }
};
static struct i2c_driver AP3087B_driver = {
	.driver = {
		.name	= "lightsensor",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(AP3087B_ids),
	},
	.probe	= AP3087B_probe,			
	/* called when I2C client is matching I2C driver */
	.remove	= AP3087B_remove,
  	/* called when driver module is unloaded */
	.id_table = AP3087B_id,	
#if 1	
	.suspend =	AP3087B_suspend,
	.resume =	AP3087B_resume,
#endif    
	/* match */
//	.address_list	= u_i2c_addr.normal_i2c,
	
};
/*
int ctp_detect1(struct i2c_client *client, struct i2c_board_info *info)
{
	adapter = client->adapter;
	

        printk("=========ctp_detect======\n");
   if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
		printk("check   err \n");
	   return -ENODEV;
	   }



    if(5 == adapter->nr)
	    {
					printk("%s: Detected chip %s at adapter %d, address 0x%02x\n",
					 __func__, AP3087B_DRV_NAME, i2c_adapter_id(adapter), client->addr);

					strlcpy(info->type, AP3087B_DRV_NAME, I2C_NAME_SIZE);
					return 0;
	    }

	
		  return -ENODEV; 	

}
*/
static const struct  i2c_board_info __initdata i2c_orap3087 = {I2C_BOARD_INFO(AP3087B_DRV_NAME,0x44)};




static int __init AP3087B_init(void)		
/* after kernel rn module_init function, all resource will be released	*/
/* actually, loadable module does not need to declare __init		*/
/* in this case, just add driver	*/
{	
	int ret;
	u_i2c_addr.dirty_addr_buf[0] = AP3087B_ADDR;
	u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
	//ret=i2c_register_board_info(1,&i2c_orap3087,1);
//	AP3087B_driver.detect = ctp_detect1;
	ret = i2c_add_driver(&AP3087B_driver);
	if(ret) printk("		: i2c_add_driver Error! \n");
	else    printk("		: i2c_add_driver OK! ret = %d \n",ret);
	return 0;
}

static void __exit AP3087B_exit(void)
{
	i2c_del_driver(&AP3087B_driver);
}

MODULE_AUTHOR(" ");
MODULE_DESCRIPTION("AP3087B ambient light sensor driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
module_init(AP3087B_init);
module_exit(AP3087B_exit);


