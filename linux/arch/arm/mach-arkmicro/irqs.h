/*
 * Copyright 2018-2019 Arkmicro, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef __MACH_IRQS_H
#define __MACH_IRQS_H

#ifdef CONFIG_SOC_ARK1668
/* IRQ definitions */
/* VIC 1 */
#define IRQ_VIC_END				64

/* GPIO pins virtual irqs */
#define VIRTUAL_IRQS				128
#define NR_IRQS					(IRQ_VIC_END + VIRTUAL_IRQS)
#endif

#endif /* __MACH_IRQS_H */
