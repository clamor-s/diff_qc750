/*
 * tegra_rt5631.c - Tegra machine ASoC driver for boards using rt5631 codec.
 *
 * Author: Stephen Warren <swarren@nvidia.com>
 * Copyright (C) 2010-2011 - NVIDIA, Inc.
 *
 * Based on code copyright/by:
 *
 * (c) 2009, 2010 Nvidia Graphics Pvt. Ltd.
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 * Author: Graeme Gregory
 *         graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <asm/mach-types.h>

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif

//#include <mach/tegra_rt5631_pdata.h>

#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "../codecs/rt5631.h"

#include "tegra_pcm.h"
#include "tegra_asoc_utils.h"

#include "mach/tegra_rt5631_pdata.h"


#ifdef CONFIG_ARCH_TEGRA_2x_SOC
#include "tegra20_das.h"
#endif

#define DRV_NAME "tegra-snd-rt5631"

//#define DEBUG
#ifdef DEBUG
#define SOCDBG(fmt, arg...)	printk(KERN_ERR "%s: %s() " fmt, "TEGRA-RT5631", __FUNCTION__, ##arg)
#else
#define SOCDBG(fmt, arg...)
#endif
#define SOCINF(fmt, args...)	printk(KERN_INFO "%s: " fmt, "TEGRA-RT5631",  ##args)
#define SOCERR(fmt, args...)	printk(KERN_ERR "%s: " fmt, "TEGRA-RT5631",  ##args)

#define I2S1_CLK 		11289600	//24576000	//12000000	//11289600
#define I2S2_CLK 		2000000


#define GPIO_SPKR_EN    BIT(0)
#define GPIO_HP_DET     BIT(1)
#define GPIO_HP_MUTE    BIT(2)
#define GPIO_AUDIO_DET     BIT(3)
#define GPIO_EXT_MIC_DET   BIT(4)
#define GPIO_INT_MIC_EN BIT(5)
#define GPIO_EXT_MIC_EN BIT(6)
#define GPIO_RESET_EN BIT(7)


struct tegra_rt5631 {
	struct tegra_asoc_utils_data util_data;
	struct tegra_rt5631_platform_data *pdata;
	struct regulator *spk_reg;
	struct regulator *dmic_reg;
	int gpio_requested;
#ifdef CONFIG_SWITCH
	int jack_status;
#endif
	enum snd_soc_bias_level bias_level;
	volatile int clock_enabled;
};

static int tegra_rt5631_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	int srate, mclk, i2s_daifmt;
	int err;
	mdelay(100);
	SOCDBG("---->srate1=%d",srate);
	srate = params_rate(params);
	SOCDBG("---->srate2=%d",srate);
	mclk = 256 * srate;
	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	i2s_daifmt = SND_SOC_DAIFMT_NB_NF |
		     SND_SOC_DAIFMT_CBS_CFS;

	i2s_daifmt |= SND_SOC_DAIFMT_I2S;

	err = snd_soc_dai_set_fmt(codec_dai, i2s_daifmt);
	if (err < 0) {
		dev_err(card->dev, "codec_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_fmt(cpu_dai, i2s_daifmt);
	if (err < 0) {
		dev_err(card->dev, "cpu_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					SND_SOC_CLOCK_IN);
	if (err < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return err;
	}



	SOCDBG("---->end\n");

	return 0;
}
#if 0
static int tegra_bt_sco_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	//struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	int srate, mclk, min_mclk,i2s_daifmt;
	int err;
	SOCDBG("---->srate=%d\n",srate);
	srate = params_rate(params);
	mclk=I2S1_CLK;
/*
	switch (srate) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		mclk = 11289600;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		mclk = 12288000;
		break;
	default:
		return -EINVAL;
	}*/
#if 0 	
	min_mclk = 64 * srate;

	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	err = snd_soc_dai_set_fmt(cpu_dai,
					SND_SOC_DAIFMT_DSP_A |
					SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS);
	if (err < 0) {
		dev_err(card->dev, "cpu_dai fmt not set\n");
		return err;
	}
#else
	i2s_daifmt=SND_SOC_DAIFMT_DSP_A | \
					SND_SOC_DAIFMT_NB_NF | \
					SND_SOC_DAIFMT_CBS_CFS;
	
	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}
	
	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	err = snd_soc_dai_set_fmt(cpu_dai,i2s_daifmt);
	if (err < 0) {
		pr_err("cpu_dai fmt not set \n");
		return err;
	}

#endif
#ifdef CONFIG_ARCH_TEGRA_2x_SOC
	err = tegra20_das_connect_dac_to_dap(TEGRA20_DAS_DAP_SEL_DAC2,TEGRA20_DAS_DAP_ID_4);
	if (err < 0) {
		dev_err(card->dev, "failed to set dac-dap path\n");
		return err;
	}

	err = tegra20_das_connect_dap_to_dac(TEGRA20_DAS_DAP_ID_4,TEGRA20_DAS_DAP_SEL_DAC2);
	if (err < 0) {
		dev_err(card->dev, "failed to set dac-dap path\n");
		return err;
	}
#endif
	return 0;
}
#endif

static int tegra_spdif_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	int srate, mclk, min_mclk;
	int err;
	SOCDBG("---->srate=%d\n",srate);
	srate = params_rate(params);
	switch (srate) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		mclk = 11289600;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		mclk = 12288000;
		break;
	default:
		return -EINVAL;
	}
	min_mclk = 128 * srate;

	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	return 0;
}

static int tegra_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(rtd->card);
	SOCDBG("--------->\n");
	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 0);

	return 0;
}

static struct snd_soc_ops tegra_rt5631_ops = {
	.hw_params = tegra_rt5631_hw_params,
	.hw_free = tegra_hw_free,
};
/*
static struct snd_soc_ops tegra_rt5631_bt_sco_ops = {
	.hw_params = tegra_bt_sco_hw_params,
	.hw_free = tegra_hw_free,
};
*/
static struct snd_soc_ops tegra_spdif_ops = {
	.hw_params = tegra_spdif_hw_params,
	.hw_free = tegra_hw_free,
};

static struct snd_soc_jack tegra_rt5631_hp_jack;
//static struct snd_soc_jack tegra_rt5631_mic_jack;

static struct snd_soc_jack_gpio tegra_rt5631_hp_jack_gpio = {
	.name = "headphone detect",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 150,
	.invert = 1,
};

#ifdef CONFIG_SWITCH
/* These values are copied from Android WiredAccessoryObserver */
enum headset_state {
	BIT_NO_HEADSET = 0,
	BIT_HEADSET = (1 << 0),
	BIT_HEADSET_NO_MIC = (1 << 1),
};

static struct switch_dev tegra_rt5631_headset_switch = {
	.name = "h2w",
};

static int tegra_rt5631_jack_notifier(struct notifier_block *self,
			      unsigned long action, void *dev)
{
	struct snd_soc_jack *jack = dev;
	struct snd_soc_codec *codec = jack->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;
	enum headset_state state = BIT_NO_HEADSET;
	unsigned char status_jack =0;
	SOCDBG("--------->\n");
	
	if (jack == &tegra_rt5631_hp_jack) {
#if 1

		if (action) {
			
			if (!strncmp(machine->pdata->codec_name, "rt5631", 6))
			status_jack = rt5631_headset_detect(codec, 1);
	
			machine->jack_status &= ~SND_JACK_HEADPHONE;
			machine->jack_status &= ~SND_JACK_MICROPHONE;
			if (status_jack == RT5631_HEADPHO_DET){
					machine->jack_status |=
							SND_JACK_HEADPHONE;
					SOCDBG("--------->RT5631_HEADPHO_DET\n");
			}
			else if (status_jack == RT5631_HEADSET_DET) {
					machine->jack_status |=
							SND_JACK_HEADPHONE;
					machine->jack_status |=
							SND_JACK_MICROPHONE;
					SOCDBG("--------->RT5631_HEADSET_DET\n");
			}

#else
		machine->jack_status &= ~SND_JACK_HEADPHONE;
		machine->jack_status |= (action & SND_JACK_HEADPHONE);
#endif
	} else {
#if 1
		if (!strncmp(machine->pdata->codec_name, "rt5631", 6))
				rt5631_headset_detect(codec, 0);
			machine->jack_status &= ~SND_JACK_HEADPHONE;
			machine->jack_status &= ~SND_JACK_MICROPHONE;
#else
		
		machine->jack_status &= ~SND_JACK_MICROPHONE;
		machine->jack_status |= (action & SND_JACK_MICROPHONE);
#endif
	}
	}
	switch (machine->jack_status) {
	case SND_JACK_HEADPHONE:
		state = BIT_HEADSET_NO_MIC;
		break;
	case SND_JACK_HEADSET:
		state = BIT_HEADSET;
		break;
	case SND_JACK_MICROPHONE:
		/* mic: would not report */
	default:
		state = BIT_NO_HEADSET;
	}
	if(state == BIT_HEADSET_NO_MIC  ||state ==BIT_HEADSET)
		gpio_direction_output(pdata->gpio_ext_mic_en, 0);
	else
		gpio_direction_output(pdata->gpio_ext_mic_en, 1);
		
#ifdef CONFIG_SWITCH
	
	

	switch_set_state(&tegra_rt5631_headset_switch, state);
#endif
	
	return NOTIFY_OK;
}

static struct notifier_block tegra_rt5631_jack_detect_nb = {
	.notifier_call = tegra_rt5631_jack_notifier,
};
#else
static struct snd_soc_jack_pin tegra_rt5631_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_pin tegra_rt5631_mic_jack_pins[] = {
	{
		.pin = "Ext Mic",
		.mask = SND_JACK_MICROPHONE,
	},
};
#endif

static int tegra_rt5631_event_int_spk(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	//struct snd_soc_dapm_context *dapm = w->dapm;
	//struct snd_soc_card *card = dapm->card;
	//struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	//struct tegra_rt5631_platform_data *pdata = machine->pdata;
	//SOCDBG("--------->\n");
	//SND_SOC_DAPM_EVENT_ON(event);
//	mdelay(10);
/*	if (machine->spk_reg) {
		if (SND_SOC_DAPM_EVENT_ON(event))
			regulator_enable(machine->spk_reg);
		else
			regulator_disable(machine->spk_reg);
	}

	if (!(machine->gpio_requested & GPIO_SPKR_EN))
		return 0;
	gpio_set_value_cansleep(pdata->gpio_spkr_en,SND_SOC_DAPM_EVENT_ON(event));
*/
	return 0;
}

static int tegra_rt5631_event_hp(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;
	SOCDBG("---------%d>\n",event);

	if (!(machine->gpio_requested & GPIO_HP_MUTE))
		return 0;
	gpio_set_value_cansleep(pdata->gpio_hp_mute,!SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}
#if 0
static int tegra_rt5631_event_int_mic(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;
	
	if (machine->dmic_reg) {
		if (SND_SOC_DAPM_EVENT_ON(event))
			regulator_enable(machine->dmic_reg);
		else
			regulator_disable(machine->dmic_reg);
	}

	if (!(machine->gpio_requested & GPIO_INT_MIC_EN))
		return 0;
	
	gpio_set_value_cansleep(pdata->gpio_int_mic_en,
				SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}

static int tegra_rt5631_event_ext_mic(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;
	SOCDBG("--------->\n");
	if (!(machine->gpio_requested & GPIO_EXT_MIC_EN))
		return 0;

	gpio_set_value_cansleep(pdata->gpio_ext_mic_en,!SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}
#endif

static const struct snd_soc_dapm_widget rt5631_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Int Spk", tegra_rt5631_event_int_spk),
	SND_SOC_DAPM_HP("Headphone Jack", tegra_rt5631_event_hp),
	SND_SOC_DAPM_MIC("Int Mic", NULL),//tegra_rt5631_event_int_mic),
//	SND_SOC_DAPM_HP("Headset Out", NULL),
	SND_SOC_DAPM_MIC("Ext Mic", NULL),//tegra_rt5631_event_ext_mic),
};

static const struct snd_soc_dapm_route rt5631_audio_map[] = {

	/* headphone connected to HPOL, HPOR */
	{"Headphone Jack", NULL, "HPOR"},
	{"Headphone Jack", NULL, "HPOL"},

	/* headset Jack  - in = MIC1, out = HPOL HPOR */
//	{"Headset Out", NULL, "HPOR"},
//	{"Headset Out", NULL, "HPOL"},
//	{"MIC1", NULL, "Headset In"},

	/* build-in speaker connected to SPOL SPOR */
	{"Int Spk", NULL, "SPOL"},
	{"Int Spk", NULL, "SPOR"},

	/* internal mic is mono */
	{"MIC2", NULL, "Int Mic"},

	/* external mic is mono */
	{"MIC1", NULL, "Ext Mic"},
};
static const struct snd_kcontrol_new rt5631_controls[] = {
	SOC_DAPM_PIN_SWITCH("Int Spk"),
	SOC_DAPM_PIN_SWITCH("Headphone Jack"),
	SOC_DAPM_PIN_SWITCH("Int Mic"),
//	SOC_DAPM_PIN_SWITCH("Headset Out"),
	SOC_DAPM_PIN_SWITCH("Ext Mic"),
};


static int tegra_rt5631_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = codec->card;
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;
	int ret;

	SOCDBG("--------->\n");
	if (gpio_is_valid(pdata->gpio_spkr_en)) {
		ret = gpio_request(pdata->gpio_spkr_en, "spkr_en");
		if (ret) {
			dev_err(card->dev, "cannot get spkr_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_SPKR_EN;

		gpio_direction_output(pdata->gpio_spkr_en, 0);
	}

	if (gpio_is_valid(pdata->gpio_hp_mute)) {
		ret = gpio_request(pdata->gpio_hp_mute, "hp_mute");
		if (ret) {
			dev_err(card->dev, "cannot get hp_mute gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_HP_MUTE;

		gpio_direction_output(pdata->gpio_hp_mute, 0);
	}

	if (gpio_is_valid(pdata->gpio_int_mic_en)) {
		ret = gpio_request(pdata->gpio_int_mic_en, "int_mic_en");
		if (ret) {
			dev_err(card->dev, "cannot get int_mic_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_INT_MIC_EN;

		/* Disable int mic; enable signal is active-high */
		gpio_direction_output(pdata->gpio_int_mic_en, 1);
	}

	if (gpio_is_valid(pdata->gpio_ext_mic_en)) {
		ret = gpio_request(pdata->gpio_ext_mic_en, "ext_mic_en");
		if (ret) {
			dev_err(card->dev, "cannot get ext_mic_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_EXT_MIC_EN;

		/* Enable ext mic; enable signal is active-low */
		gpio_direction_output(pdata->gpio_ext_mic_en,1);
	}

	if (gpio_is_valid(pdata->gpio_rt5631_reset)) {
		ret = gpio_request(pdata->gpio_rt5631_reset, "reset_en");
		if (ret) {
			dev_err(card->dev, "cannot get reset_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_RESET_EN;

		/* Enable reset; enable signal is active-low */
		gpio_direction_output(pdata->gpio_rt5631_reset, 1);
	}


	if (gpio_is_valid(pdata->gpio_hp_det))
	{

		tegra_rt5631_hp_jack_gpio.invert =1; 
		tegra_rt5631_hp_jack_gpio.gpio = pdata->gpio_hp_det;
		snd_soc_jack_new(codec, "Headphone Jack", SND_JACK_HEADPHONE,&tegra_rt5631_hp_jack);
#ifndef CONFIG_SWITCH
		snd_soc_jack_add_pins(&tegra_rt5631_hp_jack,ARRAY_SIZE(tegra_rt5631_hp_jack_pins),tegra_rt5631_hp_jack_pins);
#else
		snd_soc_jack_notifier_register(&tegra_rt5631_hp_jack,&tegra_rt5631_jack_detect_nb);
#endif
		snd_soc_jack_add_gpios(&tegra_rt5631_hp_jack,1,&tegra_rt5631_hp_jack_gpio);
		machine->gpio_requested |= GPIO_HP_DET;
	}
	ret = snd_soc_add_controls(codec, rt5631_controls,
				ARRAY_SIZE(rt5631_controls));
	if (ret < 0)
		return ret;

	snd_soc_dapm_new_controls(dapm, rt5631_dapm_widgets,
				ARRAY_SIZE(rt5631_dapm_widgets));
	
	snd_soc_dapm_add_routes(dapm, rt5631_audio_map,
				ARRAY_SIZE(rt5631_audio_map));
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
//	snd_soc_dapm_force_enable_pin(dapm, "MIC2");

	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_dai_link tegra_rt5631_dai[] = {
	{
		.name = "rt5631",
		.stream_name = "rt5631 PCM",
		.codec_name = "rt5631.4-001a",
		.platform_name = "tegra-pcm-audio",
		.cpu_dai_name = "tegra30-i2s.1",
		.codec_dai_name = "RT5631-HIFI",
		.init = tegra_rt5631_init,
		.ops = &tegra_rt5631_ops,
	},
	{
		.name = "SPDIF",
		.stream_name = "SPDIF PCM",
		.codec_name = "spdif-dit.0",
		.platform_name = "tegra-pcm-audio",
		.cpu_dai_name = "tegra30-spdif",
		.codec_dai_name = "dit-hifi",
		.ops = &tegra_spdif_ops,
	},
/*

	{
		.name = "BT-SCO",
		.stream_name = "BT SCO PCM",
		.codec_name = "spdif-dit.1",
		.platform_name = "tegra-pcm-audio",
		.cpu_dai_name = "tegra30-i2s.1",
		.codec_dai_name = "dit-hifi",
		.ops = &tegra_rt5631_bt_sco_ops,
	},*/

};
static int tegra_rt5631_resume_pre(struct snd_soc_card *card)
{
	int val;
	struct snd_soc_jack_gpio *gpio = &tegra_rt5631_hp_jack_gpio;

	if (gpio_is_valid(gpio->gpio)) {
		val = gpio_get_value(gpio->gpio);
		val = gpio->invert ? !val : val;
		snd_soc_jack_report(gpio->jack, val, gpio->report);
	}

	return 0;
}


static int tegra_rt5631_set_bias_level(struct snd_soc_card *card,
	struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level)
{
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
//	SOCDBG("--------->\n");
	if (machine->bias_level == SND_SOC_BIAS_OFF &&
		level != SND_SOC_BIAS_OFF && (!machine->clock_enabled)) {
		machine->clock_enabled = 1;
		tegra_asoc_utils_clk_enable(&machine->util_data);
		machine->bias_level = level;
	}

	return 0;
}

static int tegra_rt5631_set_bias_level_post(struct snd_soc_card *card,
	struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level)
{
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
//	SOCDBG("--------->\n");
	if (machine->bias_level != SND_SOC_BIAS_OFF &&
		level == SND_SOC_BIAS_OFF && machine->clock_enabled) {
		machine->clock_enabled = 0;
		tegra_asoc_utils_clk_disable(&machine->util_data);
	}

	machine->bias_level = level;

	return 0 ;
}

static struct snd_soc_card snd_soc_tegra_rt5631 = {
	.name = "tegra-rt5631",
	.dai_link = tegra_rt5631_dai,
	.num_links = ARRAY_SIZE(tegra_rt5631_dai),
	.resume_pre = tegra_rt5631_resume_pre,
	.set_bias_level = tegra_rt5631_set_bias_level,
	.set_bias_level_post = tegra_rt5631_set_bias_level_post,
};

static __devinit int tegra_rt5631_driver_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_tegra_rt5631;
	struct tegra_rt5631 *machine;
	struct tegra_rt5631_platform_data *pdata;
	int ret;

	SOCDBG("--------->\n");
	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		return -EINVAL;
	}

	if (pdata->codec_name)
		card->dai_link->codec_name = pdata->codec_name;
	if (pdata->codec_dai_name)
		card->dai_link->codec_dai_name = pdata->codec_dai_name;
	machine = kzalloc(sizeof(struct tegra_rt5631), GFP_KERNEL);
	if (!machine) {
		dev_err(&pdev->dev, "Can't allocate tegra_rt5631 struct\n");
		return -ENOMEM;
	}

	machine->pdata = pdata;

	ret = tegra_asoc_utils_init(&machine->util_data, &pdev->dev, card);
	if (ret)
		goto err_free_machine;//tmp


#ifdef CONFIG_SWITCH
	/* Addd h2w swith class support */
	ret = switch_dev_register(&tegra_rt5631_headset_switch);
	if (ret < 0)
		goto err_fini_utils;
#endif

	card->dev = &pdev->dev;
	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, machine);

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err_unregister_switch;
	}

	if (!card->instantiated) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err_unregister_card;
	}

	return 0;

err_unregister_card:
	snd_soc_unregister_card(card);
err_unregister_switch:
#ifdef CONFIG_SWITCH
	switch_dev_unregister(&tegra_rt5631_headset_switch);
#endif
err_fini_utils:
	tegra_asoc_utils_fini(&machine->util_data);
err_free_machine:
	kfree(machine);
	return ret;
}

static int __devexit tegra_rt5631_driver_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct tegra_rt5631 *machine = snd_soc_card_get_drvdata(card);
	struct tegra_rt5631_platform_data *pdata = machine->pdata;

	SOCDBG("--------->\n");
	if (machine->gpio_requested & GPIO_HP_DET)
		snd_soc_jack_free_gpios(&tegra_rt5631_hp_jack,
					1,
					&tegra_rt5631_hp_jack_gpio);
	if (machine->gpio_requested & GPIO_EXT_MIC_EN)
		gpio_free(pdata->gpio_ext_mic_en);
	if (machine->gpio_requested & GPIO_INT_MIC_EN)
		gpio_free(pdata->gpio_int_mic_en);
	if (machine->gpio_requested & GPIO_HP_MUTE)
		gpio_free(pdata->gpio_hp_mute);
	if (machine->gpio_requested & GPIO_SPKR_EN)
		gpio_free(pdata->gpio_spkr_en);
	if (machine->gpio_requested & GPIO_RESET_EN)
		gpio_free(pdata->gpio_rt5631_reset);
	machine->gpio_requested = 0;

	if (machine->spk_reg)
		regulator_put(machine->spk_reg);
	if (machine->dmic_reg)
		regulator_put(machine->dmic_reg);

	snd_soc_unregister_card(card);

	tegra_asoc_utils_fini(&machine->util_data);

#ifdef CONFIG_SWITCH
	switch_dev_unregister(&tegra_rt5631_headset_switch);
#endif
	kfree(machine);

	return 0;
}

static struct platform_driver tegra_rt5631_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
	.probe = tegra_rt5631_driver_probe,
	.remove = __devexit_p(tegra_rt5631_driver_remove),
};

static int __init tegra_rt5631_modinit(void)
{
	SOCDBG("--------->\n");
	return platform_driver_register(&tegra_rt5631_driver);
}
module_init(tegra_rt5631_modinit);

static void __exit tegra_rt5631_modexit(void)
{
	SOCDBG("--------->\n");
	platform_driver_unregister(&tegra_rt5631_driver);
}
module_exit(tegra_rt5631_modexit);

MODULE_AUTHOR("Stephen Warren <swarren@nvidia.com>");
MODULE_DESCRIPTION("Tegra+rt5631 machine ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
