#ifndef _ASM_EMERGENCY_RESTART_H
#define _ASM_EMERGENCY_RESTART_H

#if defined(CONFIG_MACH_OMAP3621_EVT1A) || defined(CONFIG_MACH_OMAP3621_GOSSAMER)
extern void machine_emergency_restart(void);
#else
#include <asm-generic/emergency-restart.h>
#endif
#endif /* _ASM_EMERGENCY_RESTART_H */
