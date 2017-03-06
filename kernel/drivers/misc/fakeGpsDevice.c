
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <linux/irq.h>
#include <linux/ioport.h>
#include <mach/gpio.h>
#include <linux/switch.h>
#include <linux/poll.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/freezer.h>

struct switch_dev mDev;
int mHasData;
char mData[100];
char mDataSize;
char mDataIndex;

struct timer_list	mUpdateTimer;
#define GPS_UPDATE_PERIOD 5000

static DECLARE_WAIT_QUEUE_HEAD(mWait);

static int gps_open(struct inode *inode, struct file *file){
	return 0;
}

static int gps_release(struct inode *inode, struct file *file){
	return 0;
}

static unsigned int gps_poll(struct file *file, poll_table *wait)
{
	unsigned int mask =0;

	printk("gps_poll in...\n");

	poll_wait(file, &mWait, wait);

	if(mHasData){
		mask |= POLLIN | POLLRDNORM;
	}

	printk("gps_poll out mak=0x%x\n",mask);

	return mask;
}

static ssize_t gps_read(struct file *file, char __user * buf, size_t count,
			   loff_t * offset)
{
	int ret;
	//char * data ="$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,19.7,M,,,,0000*1F\n";
	char * data ="$GPGGA,092204.999,2225.5600,N,11412.8280,E,1,04,24.4,19.7,M,,,,0000*1F\n";

	mDataSize =strlen(data);

	if(mHasData==0){
		printk("gps no data...");
		return -1;
	}

	int read =mDataSize -mDataIndex;

	if(read >count){
		read = count;
	}

	printk("gps read count =%d\n",read);
	
	ret = copy_to_user(buf, &data[mDataIndex], read);

	mDataIndex +=read;

	if(mDataSize ==mDataIndex){
		mHasData =0;
	}
	
	return read;
}

static const struct file_operations mGps_fops = {
	.owner = THIS_MODULE,
	.read = gps_read,
	.poll =gps_poll,
};

static struct miscdevice mGps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fakeGps",
	.fops = &mGps_fops,
	.mode =0777,
};

static void gps_update_func(unsigned long data){

	if(mHasData==0){
		printk("reset index\n");
		mDataIndex =0;
		mHasData =1;
		wake_up(&mWait);
	}

	printk("gps_update_func...\n");
	
	mod_timer(&mUpdateTimer,
		jiffies + msecs_to_jiffies(GPS_UPDATE_PERIOD));
}

static int __init test_probe(struct platform_device *pdev)
{
       int err = 0;
	mHasData =0;
	mHasData =1;

	err = misc_register(&mGps_device);

	init_waitqueue_head(&mWait);

	setup_timer(&mUpdateTimer,
		gps_update_func, 0);
	mod_timer(&mUpdateTimer,
		jiffies + msecs_to_jiffies(GPS_UPDATE_PERIOD));
		
        return err;
}

static int test_remove(struct platform_device *pdev)
{
	del_timer_sync(&mUpdateTimer);
	misc_deregister(&mGps_device);
	return 0;
}


static struct platform_device test_device = {
        .name = "test_ts",
        .id = -1,
};

static struct platform_driver test_driver = {
        .probe                = test_probe,
        .remove                = test_remove,
        .driver                = {
                .name        = "test_ts",
                .owner        = THIS_MODULE,
        },
};

static int __devinit test_init(void)
{
        platform_device_register(&test_device);        
        return platform_driver_register(&test_driver);
}

static void __exit test_exit(void)
{
        platform_device_unregister(&test_device);
        platform_driver_unregister(&test_driver);
}

module_init(test_init);
module_exit(test_exit);

MODULE_AUTHOR("zwolf");
MODULE_DESCRIPTION("Module test");
MODULE_LICENSE("GPL");
MODULE_ALIAS("test");

