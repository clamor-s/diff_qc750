/*
 * include/linux/interrupt_keys.h
 *
 * Key driver for keys directly connected to intrrupt lines.
 *
 * Copyright (c) 2011, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _INTERRUPT_BQ27X00_H
#define _INTERRUPT_BQ27X00_H
#include <mach/irqs.h>
#include <linux/irq.h>
#include <linux/mfd/max77663-core.h>
#define TEGRA_MAX77663_TO_IRQ(irq) (TEGRA_NR_IRQS + (irq))

struct interrupt_plug_ac {
	/* Configuration parameters */
	int irq;
	int active_low;
};

struct interrupt_bq27x00_platform_data {
	struct interrupt_plug_ac *int_plug_ac;
	int nIrqs;
	int mHasAc;
};

#endif
