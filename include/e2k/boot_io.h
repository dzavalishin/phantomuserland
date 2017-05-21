
#ifndef	_E2K_BOOT_IO_H_
#define	_E2K_BOOT_IO_H_

//#include <asm/e2k_api.h>
//#include <asm/types.h>
#include <asm/head.h>

/*
 * E2K I/O ports for BIOS
 */

#if	!defined(CONFIG_E2K_SIC)	/* E3M */
#define	PHYS_X86_IO_BASE		E3M_X86_IO_AREA_PHYS_BASE
#elif	defined(CONFIG_E2K_FULL_SIC)	/* E3S/ES2/E2S/E8C */
#define	PHYS_X86_IO_BASE		E2K_FULL_SIC_IO_AREA_PHYS_BASE
#elif	defined(CONFIG_E2K_LEGACY_SIC)	/* E1C+ */
#define	PHYS_X86_IO_BASE		E2K_LEGACY_SIC_IO_AREA_PHYS_BASE
#else
#error	"Undefined machine or SIC type"
#endif	/* ! CONFIG_E2K_SIC */

#ifdef	CONFIG_E2K_SIC
extern void boot_conf_outb(int domain, unsigned char bus, unsigned char byte,
			unsigned long port);
extern void boot_ioh_e3s_outb(int domain, unsigned char bus, unsigned char byte,
			unsigned long port);
extern void boot_conf_outw(int domain, unsigned char bus, u16 halfword,
			unsigned long port);
extern void boot_ioh_e3s_outw(int domain, unsigned char bus, u16 halfword,
			unsigned long port);
extern u16 boot_conf_inw(int domain, unsigned char bus, unsigned long port);
extern void boot_conf_outl(int domain, unsigned char bus, u32 word,
			unsigned long port);
extern u32 boot_conf_inl(int domain, unsigned char bus, unsigned long port);
extern void boot_ioh_e3s_outl(int domain, unsigned char bus, u32 word,
			unsigned long port);
extern unsigned char boot_conf_inb(int domain, unsigned char bus, 
							unsigned long port);
extern u8 boot_ioh_e3s_inb(int domain, unsigned char bus, unsigned long port);
extern u16 boot_ioh_e3s_inw(int domain, unsigned char bus, unsigned long port);
extern u32 boot_ioh_e3s_inl(int domain, unsigned char bus, unsigned long port);

#endif	/* CONFIG_E2K_SIC */

#endif	/* _E2K_BOOT_IO_H_ */
