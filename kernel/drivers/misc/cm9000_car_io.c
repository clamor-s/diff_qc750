#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/irq.h>
#include <linux/ioport.h>
#include <mach/gpio.h>
#include <linux/switch.h>

#include <linux/idr.h>
#include <linux/slab.h>
#include <asm/unaligned.h>

#include <linux/interrupt.h>
#include<linux/cm9000_car_io.h>

#define _debug_is 1
#if _debug_is
#define debug_print(...) pr_err(__VA_ARGS__)
#else
#define debug_print(...) 
#endif

struct keenhi_hd_vehicle_data {
	struct switch_dev *handbrake_switch;
	struct switch_dev *dock_switch;
	struct plat_vehicle_port *pdata;
	unsigned int	handbrake_irq;
	unsigned int	dock_irq;
	int           pre_dock_state;
	int           pre_handbrake_state;
};
static irqreturn_t vehicle_irq_func(int irq, void *data)
{
	struct keenhi_hd_vehicle_data * hb_data;
	int val;

	hb_data =(struct keenhi_hd_vehicle_data*)data;
	if(!hb_data||!hb_data->pdata)return IRQ_HANDLED;
	debug_print("%s:===============>dock_gpio=%d handbrake_gpio=%dEXE will change\n",__func__,hb_data->pdata->dock_gpio,hb_data->pdata->handbrake_gpio);
	if(hb_data->pdata->dock_gpio>0){
		val = gpio_get_value(hb_data->pdata->dock_gpio);
		if(hb_data->pre_dock_state!=val){
			hb_data->pre_dock_state=val;

			if(val ==1){
				switch_set_state(hb_data->dock_switch,0);
			}
			else{
				switch_set_state(hb_data->dock_switch,2);
			}

			printk("dock state change to %d\n",val);
		}
	}
	if(hb_data->pdata->handbrake_gpio>0){
		val = gpio_get_value(hb_data->pdata->handbrake_gpio);
		if(hb_data->pre_handbrake_state!=val){
			hb_data->pre_handbrake_state=val;

			if(val ==1){
				switch_set_state(hb_data->handbrake_switch,2);
			}
			else{
				switch_set_state(hb_data->handbrake_switch,1);
			}

			printk("handbrake state change to %d\n",val);
		}
	}
	return IRQ_HANDLED;
}

static int __init vehicle_probe(struct platform_device *pdev)
{
        int err = 0;
	struct keenhi_hd_vehicle_data * hb_data;
	struct plat_vehicle_port *pdata;
	struct switch_dev switch_dev;
	struct switch_dev dc_switch_dev;

	printk("vehicle_probe");
	
	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "%s:No platform data supplied\n",__func__);
		return -EINVAL;
	}
	if(pdata->handbrake_gpio<0&&pdata->dock_gpio<0){
		dev_err(&pdev->dev, "%s:Can't get hand brake and dock gpio\n",__func__);
		return -EINVAL;
	}
	hb_data = kzalloc(sizeof(struct keenhi_hd_vehicle_data), GFP_KERNEL);
	if (!hb_data){
		dev_err(&pdev->dev, "%s:Can't allocate keenhi_hd_vehicle_data struct\n",__func__);
		return -ENOMEM;
	}

	printk("vehicle_probe handbrake_gpio=%d\n",pdata->handbrake_gpio);
	printk("vehicle_probe dock_gpio=%d\n",pdata->dock_gpio);
	
	
	hb_data->pdata = pdata;
	if(pdata->handbrake_gpio>0){
		hb_data->handbrake_irq = TEGRA_GPIO_TO_IRQ(pdata->handbrake_gpio);
		err = gpio_request(pdata->handbrake_gpio, "handbrak");		
		if(err <0){			
			dev_err(&pdev->dev, "%s: gpio_request(%d) failed\n",				
				__func__,pdata->handbrake_gpio);			
			goto err_request_gpio;		
		}		
		err= gpio_direction_input(pdata->handbrake_gpio);		
		if (err < 0) {			
			dev_err(&pdev->dev, "%s: gpio_direction_input(%d) failed\n",				
				__func__,pdata->handbrake_gpio);					
			goto err_set_gpio_direction;		
		}
		
		hb_data->pre_handbrake_state = gpio_get_value(pdata->handbrake_gpio);
		hb_data->handbrake_switch = kzalloc(sizeof(struct switch_dev), GFP_KERNEL);
		hb_data->handbrake_switch->name =HB_DEV_NAME;
		hb_data->handbrake_switch->state =hb_data->pre_handbrake_state;
		err = request_threaded_irq(hb_data->handbrake_irq, NULL,
			vehicle_irq_func,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			"handbrak_irq", hb_data);
		if (err < 0) {
			dev_err(&pdev->dev,
				"%s: request_irq failed(%d)\n", __func__, err);
			goto err_set_gpio_direction;
		}
		
		err = switch_dev_register(hb_data->handbrake_switch);
		if (err < 0){
			dev_err(&pdev->dev, "%s:register switch1 failed!\n",__func__);
			goto err_request_switch;
		}
		if(hb_data->pre_handbrake_state ==1){
				switch_set_state(hb_data->handbrake_switch,2);
			}
			else{
				switch_set_state(hb_data->handbrake_switch,1);
		}
	}
	if(pdata->dock_gpio>0){
		hb_data->dock_irq = TEGRA_GPIO_TO_IRQ(pdata->dock_gpio);
		err = gpio_request(pdata->dock_gpio, "dock");		
		if(err <0){			
			dev_err(&pdev->dev, "%s: gpio_request(%d) failed\n",				
				__func__,pdata->dock_gpio);			
			goto err_request_gpio;		
		}		
		err= gpio_direction_input(pdata->dock_gpio);		
		if (err < 0) {			
			dev_err(&pdev->dev, "%s: gpio_direction_input(%d) failed\n",				
				__func__,pdata->dock_gpio);						
			goto err_set_gpio_direction;		
		}	

		hb_data->pre_dock_state = gpio_get_value(pdata->dock_gpio);
		hb_data->dock_switch = kzalloc(sizeof(struct switch_dev), GFP_KERNEL);
		hb_data->dock_switch->name =DC_DEV_NAME;
		hb_data->dock_switch->state = hb_data->pre_dock_state;
		err = request_threaded_irq(hb_data->dock_irq, NULL,
			vehicle_irq_func,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			"dock_irq", hb_data);
		if (err < 0) {
			dev_err(&pdev->dev,
				"%s: request_irq failed(%d)\n", __func__, err);
			goto err_set_gpio_direction;
		}
		
		err = switch_dev_register(hb_data->dock_switch);
		if (err < 0){
			dev_err(&pdev->dev, "%s:register switch2 failed!\n",__func__);
			goto err_request_switch;
		}
		if(hb_data->pre_dock_state ==1){
				switch_set_state(hb_data->dock_switch,0);
			}
			else{
				switch_set_state(hb_data->dock_switch,2);
		}
	}
	platform_set_drvdata(pdev, hb_data);
	debug_print("%s: ===========>successfully!\n");
	return 0;
err_request_switch:
	if(hb_data->handbrake_irq>0)
		free_irq(hb_data->handbrake_irq, hb_data);
	if(hb_data->dock_irq>0)
		free_irq(hb_data->dock_irq, hb_data);
err_set_gpio_direction:
	if(pdata->handbrake_gpio>0)
		gpio_free(pdata->handbrake_gpio);
	if(pdata->dock_gpio>0)
		gpio_free(pdata->dock_gpio);
err_request_gpio:
	kfree(hb_data);
        return err;
}

static int vehicle_remove(struct platform_device *pdev)
{
	struct keenhi_hd_vehicle_data * hb_data;
	hb_data = platform_get_drvdata(pdev);
		
	if(hb_data->pdata->handbrake_gpio>0){
		free_irq(hb_data->handbrake_irq, hb_data);
		gpio_free(hb_data->pdata->handbrake_gpio);
		switch_set_state(hb_data->handbrake_switch, 2);
		switch_dev_unregister(hb_data->handbrake_switch);
	}
	if(hb_data->pdata->dock_gpio>0){
		free_irq(hb_data->dock_irq, hb_data);
		gpio_free(hb_data->pdata->dock_gpio);
		switch_set_state(hb_data->dock_switch, 0);
		switch_dev_unregister(hb_data->dock_switch);
	}
	
	kfree(hb_data);
        return 0;
}

static struct platform_driver vehicle_driver = {
        .probe                = vehicle_probe,
        .remove                = vehicle_remove,
        .driver                = {
                .name        = CM9000_CAR_DEV_NAME,
                .owner        = THIS_MODULE,
        },
};

static int __devinit vehicle_init(void)
{       
        return platform_driver_register(&vehicle_driver);
}

static void __exit vehicle_exit(void)
{
        platform_driver_unregister(&vehicle_driver);
}

module_init(vehicle_init);
module_exit(vehicle_exit);

MODULE_AUTHOR("jimmy.tang");
MODULE_DESCRIPTION("Module car");
MODULE_LICENSE("GPL");
MODULE_ALIAS("jimmy");
