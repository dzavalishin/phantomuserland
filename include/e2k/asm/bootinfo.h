#ifndef _L_BOOTINFO_H_
#define _L_BOOTINFO_H_

#ifdef	__KERNEL__

/*
 * 0x0:
 * 0x1: extended command line
 */
#define BOOTBLOCK_VER			0x1

#define	KSTRMAX_SIZE			128
#define	KSTRMAX_SIZE_EX			512
#define	BIOS_INFO_SIGN_SIZE		8
#define KERNEL_ARGS_STRING_EX_SIGN_SIZE	22
#define	BOOT_VER_STR_SIZE		128
#define	BOOTBLOCK_SIZE			0x1000	/* 1 PAGE_SIZE */
#define	X86BOOT_SIGNATURE		0x8086
#define	ROMLOADER_SIGNATURE		0xe200
#define	BIOS_INFO_SIGNATURE		"E2KBIOS"
#define KERNEL_ARGS_STRING_EX_SIGNATURE "KERNEL_ARGS_STRING_EX"
#define BOOT_KERNEL_ARGS_STRING_EX_SIGNATURE	\
		boot_va_to_pa(KERNEL_ARGS_STRING_EX_SIGNATURE)

#define	L_MAX_NUMNODES		4	/* MAX_NUMNODES defined at */
					/* include/linux/numa.h */

/*
 * Below is boot information that comes out of the x86 code of Linux/E2K
 * loader proto.
 */

/* BOOT_MAX_NODE_PHYS_BANKS = 4 sometimes is not enough, so we increase it to
 * an arbitary value (8 now). The old BOOT_MAX_NODE_PHYS_BANKS we rename to
 * BOOT_MAX_NODE_PHYS_BANKS_FUSTY and take in mind for boot_info compatibility.
 *
 * BOOT_MAX_NODE_PHYS_BANKS_FUSTY and BOOT_MAX_MEM_NUMNODES describe max size of
 * array of memory banks on all nodes and should be in accordance with old value
 * of BOOT_MAX_PHYS_BANKS for compatibility with boot_info old structure (bank)
 * size, so BOOT_MAX_NODE_PHYS_BANKS_FUSTY * BOOT_MAX_MEM_NUMNODES should be
 * equal to 32.
 */
#define L_MAX_NODE_PHYS_BANKS		8	/* max number of memory banks */
						/* on one node */
#define L_MAX_NODE_PHYS_BANKS_FUSTY	4	/* fusty max number of memory */
						/* banks on one node */
#define L_MAX_PHYS_BANKS_EX		64	/* max number of memory banks */
						/* in banks_ex field of */
						/* boot_info */
#define L_MAX_MEM_NUMNODES		8	/* max number of nodes in the */
						/* list of memory banks on */
						/* each node */

#define	L_MAX_BUSY_AREAS	4	/* max number of busy areas */
					/* occupied by BIOS and should be */
					/* kept unchanged by kernel to */
					/* support recovery mode */
#define	L_MAX_CNT_POINTS	8	/* max number of control points */
					/* this value can be changed only */
					/* with max value of  */
					/* CONFIG_CNT_POINTS_NUM in the */
					/* of arch/e2k/Kconfig */

#ifndef	__ASSEMBLY__

typedef struct bank_info {
	u64	address;	/* start address of bank */
	u64	size;		/* size of bank in bytes */
} bank_info_t;

typedef struct node_banks {
	bank_info_t banks[L_MAX_NODE_PHYS_BANKS_FUSTY];	/* memory banks array */
							/* of a node */
} node_banks_t;

/*
 * Each NUMA node can have own memory and access to memory of other nodes/
 * Nodes use IP links to access another's memory. In common case node has
 * one IP link to access to one node, so in system with 4 nodes each node
 * has 3 IP links to access to other nodes. In some case system (machine)
 * can be based on smaller number of nodes where 1 or 2 nodes are absent.
 * In this case nodes can be connected by more than one IP link and the node's
 * memory accessible through sevral links. To support this feature address
 * space of node's memory is divided among links into appropriate number of
 * slabs. BIOS should tune memory router registers (rt_mlo, rt_mhi) and
 * pass slabs info for each node/
 */
#define	MAX_NODE_MEM_SLABS	(1 * (L_MAX_NUMNODES - 1))
				/* one IP link to each other 3 nodes */
typedef	bank_info_t	slab_info_t;
typedef struct node_slabs {
	int mlo_slabs_num;				/* number of slabs */
							/* into low memory */
							/* space of a node */
	int mhi_slabs_num;				/* number of slabs */
							/* into high memory */
							/* space of a node */
	slab_info_t mlo_slabs[MAX_NODE_MEM_SLABS];	/* low memory slabs */
							/* array */
	slab_info_t mhi_slabs[MAX_NODE_MEM_SLABS];	/* high memory slabs */
							/* array */
} node_slabs_t;

typedef	struct cntp_info {
	u64	kernel_base;	/* kernel image base address */
	u64	node_data;	/* physical address of memory nodes data */
				/* list */
	u64	nosave_areas;	/* do not save all areas from this array */
				/* physical address of array */
	u16	nosaves_num;	/* number of areas to do not save */
	u8	mem_valid;	/* control point is valid in memory */
	u8	disk_valid;	/* control point is valid on disk */
} cntp_info_t;

typedef struct dump_info {
	u64	pa_init_task;		/* phys address of init_task */
	u64	init_mm;		/* address of init_mm */
	u64	targ_KERNEL_BASE;	/* KERNEL_BASE value  */
	u64	mm_offset;		/* offset mm member in task_struct */
	u64	tasks_offset;		/* offset tasks member in task_struct */
	u64	thread_offset;		/* offset thread member in task_struct */
	/* kallsyms info to print procedure names */
	u64	kallsyms_addresses;
	u64	kallsyms_num_syms;
	u64	kallsyms_names;
	u64	kallsyms_token_table;
	u64	kallsyms_token_index;
	u64	kallsyms_markers;
	/* sections of original kernel image */
	u64	_start;
	u64	_end;
	u64	_stext;
	u64	_etext;
	//u64	_sextratext;	/* Obsolete field on 2.6.33 */
	//u64	_eextratext;	/* Obsolete field on 2.6.33 */
	u64	_sinittext;
	u64	_einittext;
	/* Trace buffer description of original kernel */
	u64	reserved1;
	u64	reserved2;
        //u64     freq;
} dump_info_t;

typedef struct boot_times {
	u64 arch;
	u64 unpack;
	u64 pci;
	u64 drivers1;
	u64 drivers2;
	u64 menu;
	u64 sm;
	u64 kernel;
	u64 reserved[8];
} boot_times_t;

typedef struct bios_info {
	u8	signature[BIOS_INFO_SIGN_SIZE];	 	/* signature, 'E2KBIOS' */
	u8	boot_ver[BOOT_VER_STR_SIZE];	 	/* boot version */
	u8	mb_type;			 	/* mother board type */
	u8	chipset_type;			 	/* chipset type */
	u8	cpu_type;			 	/* cpu type */
	u8	kernel_args_string_ex[KSTRMAX_SIZE_EX];	/* extended command line of */
							/* kernel used to pass */
							/* command line */
						  	/* from e2k BIOS */
	u8	reserved1;				/* reserved1 */
	u32	cache_lines_damaged;			/* number of damaged */
							/* cache lines */
	node_slabs_t nodes_mem_slabs[L_MAX_NUMNODES];	/* array of slabs */
							/* accessible memory */
							/* on each node */
	bank_info_t banks_ex[L_MAX_PHYS_BANKS_EX];	/* extended array of */
							/* descriptors of */
							/* banks of available */
							/* physical memory */
} bios_info_t;

typedef struct boot_info {
	u16	signature;	/* signature, 0x8086 */
	u16	cylinders;	/* hdd info */
	u8	heads;		/* hdd info */
	u8	sectors;	/* hdd info */
	u8	vga_mode;	/* vga mode */
	u8	num_of_banks;	/* number of available physical memory banks */
				/* see below bank array */
				/* total number on all nodes or 0 */
	u64	kernel_base;	/* base address to load kernel image */
				/* if 0 then BIOS can load at any address */
				/* but address should be large page size */
				/* aligned - 4 Mb */
	u64	kernel_size;	/* kernel image byte's size */
	u64	ramdisk_base;	/* base address to load RAM-disk */
				/* now not used */
	u64	ramdisk_size;	/* RAM-disk byte's size */

	u16	num_of_cpus;	/* number of started physical CPU(s) */
	u16	mach_flags;	/* machine identifacition flags */
				/* should be set by our romloader and BIOS */
	u16     num_of_busy;	/* number of busy areas occupied by BIOS */
				/* see below busy array */
	u16	num_of_nodes;	/* number of nodes on NUMA system */
	u64	mp_table_base;	/* MP-table base address */
	u64	serial_base;	/* base address of serial port for Am85c30 
				 * in e3s architecture */
				/* Used for debugging purpose */
	u64	nodes_map;	/* online nodes map */
	u64	mach_serialn;	/* serial number of the machine */
	u8	mac_addr[6];	/* base MAC address for ethernet cards */
	u16	reserved1;	/* reserved1 */

	char	kernel_args_string[KSTRMAX_SIZE]; /* command line of kernel */
						  /* used to pass command line */
						  /* from e2k BIOS */
	node_banks_t	nodes_mem[L_MAX_MEM_NUMNODES];	/* array of */
						/* descriptors of banks of */
						/* available physical memory */
						/* on each node */
	bank_info_t	busy[L_MAX_BUSY_AREAS];	/* descriptors of areas */
						/* occupied by BIOS, all this */
						/* shoud be kept in system */
						/* recovery mode */
	cntp_info_t	cntp_info[L_MAX_CNT_POINTS];	/* control points */
							/* info to save and */
							/* restore them state */
	dump_info_t	dmp;		/* Info for future work of dump analyzer */
	u64		reserved2[13];	/* reserved2 */
	u8		mb_name[16];	/* Motherboard product name */
	u32		reserved3;	/* reserved3 */
	u32		kernel_csum;	/* kernel image control sum */
	bios_info_t	bios;		/* extended BIOS info */
					/* SHOULD BE LAST ITEM into this */
					/* structure */
} boot_info_t;

typedef struct bootblock_struct {
	boot_info_t	info;			/* general kernel<->BIOS info */
	u8					/* zip area to make size of */
						/* bootblock struct - constant */
			gap[BOOTBLOCK_SIZE -
				sizeof (boot_info_t)  -
				sizeof (boot_times_t) -
				1 -		/* u8  : bootblock_ver */
				4 -		/* u32 : reserved1 */
				2 -		/* u16 : kernel_flags */
				1 -		/* u8  : reserved2 */
				5 -		/* u8  : number of cnt points */
						/* u8  : current # of cnt point */
						/* u8  : number of cnt points */
						/*	 ready in the memory */
						/* u8  : number of cnt points */
						/*	 saved on the disk */
						/* u8  : all control points */
						/*	 is created */
				8 -		/* u64 : dump sector */
				8 -		/* u64 : cnt point sector */
				2 -		/* u16 : dump device */
				2 -		/* u16 : cnt point device */
				2 -		/* u16 : boot_flags */
				2];		/* u16 : x86_marker */
	u8		bootblock_ver;		/* bootblock version number */
	u32		reserved1;		/* reserved1 */
	boot_times_t	boot_times;		/* boot load times */
	u16		kernel_flags;		/* kernel flags, boot should */
						/* not modify it */
	u8		reserved2;		/* reserved2 */
	u8		cnt_points_num;		/* number of control points */
						/* all memory will be devided */
						/* on this number of parts */
	u8		cur_cnt_point;		/* current # of active control */
						/* point (running part) */
	u8		mem_cnt_points;		/* number of started control */
						/* points (ready in the memory) */
	u8		disk_cnt_points;	/* number of control points */
						/* saved on the disk (ready  */
						/* to be loaded from disk) */
	u8		cnt_points_created;	/* all control points created */
						/* in the memory and on disk */
	u64		dump_sector;		/* start sector # to dump */
						/* physical memory */
	u64		cnt_point_sector;	/* start sector # to save */
						/* restore control points */
	u16		dump_dev;		/* disk # to dump memory */
	u16		cnt_point_dev;		/* disk # for save/restore */
						/* control point */
	u16		boot_flags;		/* boot flags: if non */
						/* zero then this structure */
						/* is recovery info */
						/* structure instead of boot */
						/* info structure */
	u16		x86_marker;		/* marker of the end of x86 */
						/* boot block (0xAA55) */
} bootblock_struct_t;

#endif /* ! __ASSEMBLY__ */

/*
 * Boot block flags to elaborate boot modes
 */

#define	RECOVERY_BB_FLAG	0x0001	/* recovery flag: if non zero then */
					/* this structure is recovery info */
					/* structure instead of boot info */
					/* structure */
					/* BIOS should not clear memory */
					/* and should keep current state of */
					/* physical memory */
#define	CNT_POINT_BB_FLAG	0x0002	/* kernel restarted in the mode of */
					/* control point creation */
					/* BIOS should read kernel image from */
					/* the disk to the specified area of */
					/* the memory and start kernel */
#define	NO_READ_IMAGE_BB_FLAG	0x0004	/* BIOS should not read kernel image */
					/* from disk and start current */
					/* image in the specified area of */
					/* the memory (this flag should be */
					/* with RECOVERY_BB_FLAG flag) */
#define	DUMP_ANALYZE_BB_FLAG	0x0008	/* This flag is used only by kernel */
					/* to indicate dump analyzer mode */
#define	MEMORY_DUMP_BB_FLAG	0x0010	/* BIOS should dump all physical */
					/* memory before start all other */
					/* actions */
#define	IMAGE_DUMP_BB_FLAG	0x0020	/* BIOS should save only kernel image */
					/* area on the disk */

/*
 * The machine identification flags
 */

#define	SIMULATOR_MACH_FLAG	0x0001	/* system is running on simulator */
#define	PROTOTYPE_MACH_FLAG	0x0002	/* machine is prototype */
#define	IOHUB_MACH_FLAG		0x0004	/* machine has IOHUB */
#define OLDMGA_MACH_FLAG	0x0008	/* MGA card has old firmware */
#define MULTILINK_MACH_FLAG	0x0010	/* some nodes are connected by */
					/* sevral IP links */
#define	MSI_MACH_FLAG		0x0020	/* boot inits right values in apic */
					/* to support MSI. Meanfull for e2k */
					/* only. For v9 it always true */




/*
 * The chipset types
 */

#define CHIPSET_TYPE_PIIX4	0x01	/* PIIX4 */
#define CHIPSET_TYPE_IOHUB	0x02	/* IOHUB */





/*
 * The chipset types names
 */

#define GET_CHIPSET_TYPE_NAME(type)			\
({							\
	char *name;					\
							\
	switch (type) {					\
		case CHIPSET_TYPE_PIIX4:		\
			name = "PIIX4";			\
			break;				\
		case CHIPSET_TYPE_IOHUB:		\
			name = "IOHUB";			\
			break;				\
		default:				\
			name = "?????";			\
	}						\
							\
	name;						\
})


extern char *mcst_mb_name;

#endif /* __KERNEL__ */

#endif /* _L_BOOTINFO_H_ */

