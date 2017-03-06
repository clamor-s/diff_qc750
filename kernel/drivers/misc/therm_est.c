/*
 * drivers/misc/therm_est.c
 *
 * Copyright (C) 2010-2012 NVIDIA Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/therm_est.h>
#ifdef    CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
#ifdef CONFIG_CHARGER_BQ24160

#include <linux/bq24160_charger.h>
#endif

#include <linux/mfd/max77663-core.h>
#include <asm/mach-types.h>

static bool disable_charge_system=0;
static bool disable_backlight_modify=0;
extern void modify_backlight_reduce(void);
extern void modify_backlight_add(void);
int therm_est_get_temp(struct therm_estimator *est, long *temp)
{
	*temp = est->cur_temp;
	return 0;
}

int therm_est_set_limits(struct therm_estimator *est,
				long lo_limit,
				long hi_limit)
{
	est->therm_est_lo_limit = lo_limit;
	est->therm_est_hi_limit = hi_limit;
	return 0;
}

int therm_est_set_alert(struct therm_estimator *est,
			void (*cb)(void *),
			void *cb_data)
{
	if ((!cb) || est->callback)
		BUG();

	est->callback = cb;
	est->callback_data = cb_data;

	return 0;
}

static void therm_est_work_func(struct work_struct *work)
{
	int i, j, index, sum = 0;
	long temp;
	static int run_count = 0;
	int skip_count = 30;
	struct delayed_work *dwork = container_of (work,
					struct delayed_work, work);
	struct therm_estimator *est = container_of(
					dwork,
					struct therm_estimator,
					therm_est_work);
	
	for (i = 0; i < est->ndevs; i++) {
		if (est->devs[i]->get_temp(est->devs[i]->dev_data, &temp))
			continue;
		est->devs[i]->hist[(est->ntemp % HIST_LEN)] = temp;
		if(run_count % skip_count == 0){
			printk(KERN_INFO "%s: i=%d, "
					"est->devs[%d]->coeffs[0]=%d, "
					"est->devs[%d]->temp=%d\n",
					 __func__, i, i,
					est->devs[i]->coeffs[0], i, temp);
		}
			
	}
	run_count++;
	for (i = 0; i < est->ndevs; i++) {
		for (j = 0; j < HIST_LEN; j++) {
			index = (est->ntemp - j + HIST_LEN) % HIST_LEN;
			sum += est->devs[i]->hist[index] *
				est->devs[i]->coeffs[j];
		}
	}

	est->cur_temp = sum / 100 + est->toffset;
//	printk("est->cur_temp=therm_est_work_func=%ld\n",est->cur_temp);
	if(machine_is_birch()&& max77663_acok_state())
	{
		if(((est->cur_temp)/1000>41)){
			modify_backlight_reduce();
		}
		else
		       modify_backlight_add();
	}	
	else{
		   modify_backlight_add();
	}
	if(machine_is_birch()&& max77663_acok_state())
	{
		if(((est->cur_temp)/1000>40))
		{
	
		if(disable_charge_system==0)
			{
				disable_charge_system=1;
				disable_charge_system_temperature();
			}
		}
		else if(disable_charge_system==1)
		{
			disable_charge_system=0;
			enable_charge_system_temperature();
		}
	}


	
	est->ntemp++;

	if (est->callback && ((est->cur_temp >= est->therm_est_hi_limit) ||
			 (est->cur_temp <= est->therm_est_lo_limit)))
		est->callback(est->callback_data);
	
	queue_delayed_work(est->workqueue, &est->therm_est_work,
				msecs_to_jiffies(est->polling_period));
}
#if 0
#ifdef    CONFIG_HAS_EARLYSUSPEND
void therm_earlysuspend_suspend(struct early_suspend *h)
{
	struct therm_estimator *est = container_of(h, struct therm_estimator, early_suspend);
	//printk("*************%s\n",__func__);
	cancel_delayed_work(&est->therm_est_work);
if(disable_charge_system==1)
{
	disable_charge_system=0;
	enable_charge_system_temperature_suspend();

}
	return;
}

void therm_earlysuspend_resume(struct early_suspend *h)
{
	struct therm_estimator *est = container_of(h, struct therm_estimator, early_suspend);
	//printk("************%s\n",__func__);
	queue_delayed_work(est->workqueue, &est->therm_est_work,
				msecs_to_jiffies(est->polling_period));

	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */
#endif
struct therm_estimator *therm_est_register(
			struct therm_est_subdevice **devs,
			int ndevs,
			long toffset,
			long polling_period)
{
	int i, j;
	long temp;
	struct therm_estimator *est;
	struct therm_est_subdevice *dev;

	disable_charge_system=0;
	disable_backlight_modify=0;

	est = kzalloc(sizeof(struct therm_estimator), GFP_KERNEL);
	if (IS_ERR_OR_NULL(est))
		return ERR_PTR(-ENOMEM);

	est->devs = devs;
	est->ndevs = ndevs;
	est->toffset = toffset;
	est->polling_period = polling_period;

	/* initialize history */
	for (i = 0; i < ndevs; i++) {
		dev = est->devs[i];

		if (dev->get_temp(dev->dev_data, &temp)) {
			kfree(est);
			return ERR_PTR(-EINVAL);
		}

		for (j = 0; j < HIST_LEN; j++) {
			dev->hist[j] = temp;
		}
	}

	est->workqueue = alloc_workqueue("therm_est",
				    WQ_HIGHPRI | WQ_UNBOUND | WQ_RESCUER, 1);
	INIT_DELAYED_WORK(&est->therm_est_work, therm_est_work_func);

	queue_delayed_work(est->workqueue,
				&est->therm_est_work,
				msecs_to_jiffies(est->polling_period));
#if 0
#ifdef    CONFIG_HAS_EARLYSUSPEND
	/* The higher the level, the earlier it resume, and the later it suspend */
	est->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 80;
	est->early_suspend.suspend = therm_earlysuspend_suspend;
	est->early_suspend.resume = therm_earlysuspend_resume;
	register_early_suspend(&est->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
#endif
	return est;
}
