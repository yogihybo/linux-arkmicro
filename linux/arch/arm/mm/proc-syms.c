/*
 *  linux/arch/arm/mm/proc-syms.c
 *
 *  Copyright (C) 2000-2002 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/mm.h>

#include <asm/cacheflush.h>
#include <asm/proc-fns.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

#ifndef MULTI_CPU
EXPORT_SYMBOL(cpu_dcache_clean_area);
#ifdef CONFIG_MMU
EXPORT_SYMBOL(cpu_set_pte_ext);
#endif
#else
EXPORT_SYMBOL(processor);
#endif

#ifndef MULTI_CACHE
EXPORT_SYMBOL(__cpuc_flush_kern_all);
EXPORT_SYMBOL(__cpuc_flush_user_all);
EXPORT_SYMBOL(__cpuc_flush_user_range);
EXPORT_SYMBOL(__cpuc_coherent_kern_range);
EXPORT_SYMBOL(__cpuc_flush_dcache_area);
/* Vivante's galcore driver does its own DMA cache maintenance and calls
 * these "private to the dma-mapping API" functions directly -- Vivante's
 * own gc_hal_kernel_os.c documents this exact patch as required "in case
 * cache API is not exported". Without it, modprobe fails with unknown
 * symbol v7_dma_map_area/v7_dma_unmap_area/v7_dma_flush_range on any
 * single-cache-type (non-MULTI_CACHE) build such as this CPU_V7-only one.
 * These are assembly-only symbols (arch/arm/mm/cache-v7.S) with no C
 * prototype visible here, so EXPORT_SYMBOL needs an explicit extern first.
 */
extern void __glue(_CACHE,_dma_map_area)(const void *, size_t, int);
extern void __glue(_CACHE,_dma_unmap_area)(const void *, size_t, int);
extern void __glue(_CACHE,_dma_flush_range)(const void *, const void *);
EXPORT_SYMBOL(__glue(_CACHE,_dma_map_area));
EXPORT_SYMBOL(__glue(_CACHE,_dma_unmap_area));
EXPORT_SYMBOL(__glue(_CACHE,_dma_flush_range));
#else
EXPORT_SYMBOL(cpu_cache);
#endif

#ifdef CONFIG_MMU
#ifndef MULTI_USER
EXPORT_SYMBOL(__cpu_clear_user_highpage);
EXPORT_SYMBOL(__cpu_copy_user_highpage);
#else
EXPORT_SYMBOL(cpu_user);
#endif
#endif

/*
 * No module should need to touch the TLB (and currently
 * no modules do.  We export this for "loadkernel" support
 * (booting a new kernel from within a running kernel.)
 */
#ifdef MULTI_TLB
EXPORT_SYMBOL(cpu_tlb);
#endif
