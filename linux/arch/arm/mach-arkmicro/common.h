#ifndef __MACH_ARK_COMMON_H__
#define __MACH_ARK_COMMON_H__

#ifdef CONFIG_SMP
extern char ark_secondary_trampoline;
extern char ark_secondary_trampoline_jump;
extern char ark_secondary_trampoline_end;
extern int ark_cpun_start(u32 address, int cpu);
extern const struct smp_operations ark_smp_ops;
#endif

extern void __iomem *ark_scu_base;

#endif
