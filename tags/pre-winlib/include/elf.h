/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Excerpt from 32-bit ELF definitions.
 *
 *
**/

#ifndef ELF_H
#define ELF_H


#define ELF_MAGIC "\x7f""ELF"
#define EI_MAG0	0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_NIDENT 16


#define EM_SPARC	2
#define EM_386		3
#define EM_68K		4
#define EM_MIPS		8
#define EM_SPARC32PLUS	18
#define EM_PPC		20
#define EM_PPC64	21
#define EM_ARM		40
#define EM_SH		42
#define EM_SPARCV9	43
#define EM_IA_64	50
#define EM_X86_64	62
#define EM_ALPHA	0x9026

#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4



/* 32-bit */
typedef unsigned long Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned long Elf32_Off;
typedef signed long Elf32_Sword;
typedef unsigned long Elf32_Word;


struct Elf32_Ehdr {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half		e_type;
	Elf32_Half		e_machine;
	Elf32_Word		e_version;
	Elf32_Addr		e_entry;
	Elf32_Off		e_phoff;
	Elf32_Off		e_shoff;
	Elf32_Word		e_flags;
	Elf32_Half		e_ehsize;
	Elf32_Half		e_phentsize;
	Elf32_Half		e_phnum;
	Elf32_Half		e_shentsize;
	Elf32_Half		e_shnum;
	Elf32_Half		e_shstrndx;
} ;


/* program header - page 5-2, figure 5-1 */

typedef struct
{
    Elf32_Word 		p_type;
    Elf32_Off 		p_offset;
    Elf32_Addr 		p_vaddr;
    Elf32_Addr 		p_paddr;
    Elf32_Word 		p_filesz;
    Elf32_Word 		p_memsz;
    Elf32_Word 		p_flags;
    Elf32_Word 		p_align;
}
Elf32_Phdr;

/* segment types - page 5-3, figure 5-2 */

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

/* segment permissions - page 5-6 */

#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKPROC	0xf0000000









/* section table - ? */
typedef struct
{
    Elf32_Word	sh_name;		/* Section name (string tbl index) */
    Elf32_Word	sh_type;		/* Section type */
    Elf32_Word	sh_flags;		/* Section flags */
    Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
    Elf32_Off	sh_offset;		/* Section file offset */
    Elf32_Word	sh_size;		/* Section size in bytes */
    Elf32_Word	sh_link;		/* Link to another section */
    Elf32_Word	sh_info;		/* Additional section information */
    Elf32_Word	sh_addralign;		/* Section alignment */
    Elf32_Word	sh_entsize;		/* Entry size if section holds table */
}
Elf32_Shdr;



/* sh_type */
#define SHT_NULL		0	/* inactive */
#define SHT_PROGBITS		1	/* program defined information */
#define SHT_SYMTAB		2	/* symbol table section */
#define SHT_STRTAB		3	/* string table section */
#define SHT_RELA		4	/* relocation section with addends */
#define SHT_HASH		5	/* symbol hash table section */
#define SHT_DYNAMIC		6	/* dynamic section */ 
#define SHT_NOTE		7	/* note section */
#define SHT_NOBITS		8	/* no space section */
#define SHT_REL			9	/* relocation section - no addends */
#define SHT_SHLIB		10	/* reserved - purpose unknown */
#define SHT_DYNSYM		11	/* dynamic symbol table section */ 
#define SHT_INIT_ARRAY		14	/* Initialization function pointers. */
#define SHT_FINI_ARRAY		15	/* Termination function pointers. */
#define SHT_PREINIT_ARRAY	16	/* Pre-initialization function ptrs. */
#define SHT_GROUP		17	/* Section group. */
#define SHT_SYMTAB_SHNDX	18	/* Section indexes (see SHN_XINDEX). */
#define SHT_LOOS		0x60000000	/* First of OS specific semantics */
#define SHT_LOSUNW		0x6ffffff4
#define SHT_SUNW_dof		0x6ffffff4
#define SHT_SUNW_cap		0x6ffffff5
#define SHT_SUNW_SIGNATURE	0x6ffffff6
#define SHT_SUNW_ANNOTATE	0x6ffffff7
#define SHT_SUNW_DEBUGSTR	0x6ffffff8
#define SHT_SUNW_DEBUG		0x6ffffff9
#define SHT_SUNW_move		0x6ffffffa
#define SHT_SUNW_COMDAT		0x6ffffffb
#define SHT_SUNW_syminfo	0x6ffffffc
#define SHT_SUNW_verdef		0x6ffffffd
#define SHT_GNU_verdef		0x6ffffffd	/* Symbol versions provided */
#define SHT_SUNW_verneed	0x6ffffffe
#define SHT_GNU_verneed		0x6ffffffe	/* Symbol versions required */
#define SHT_SUNW_versym		0x6fffffff
#define SHT_GNU_versym		0x6fffffff	/* Symbol version table */
#define SHT_HISUNW		0x6fffffff
#define SHT_HIOS		0x6fffffff	/* Last of OS specific semantics */
#define SHT_LOPROC		0x70000000	/* reserved range for processor */
#define SHT_AMD64_UNWIND	0x70000001	/* unwind information */
#define SHT_HIPROC		0x7fffffff	/* specific section header types */
#define SHT_LOUSER		0x80000000	/* reserved range for application */
#define SHT_HIUSER		0xffffffff	/* specific indexes */



/* Flags for sh_flags. */
#define SHF_WRITE		0x1	/* Section contains writable data. */
#define SHF_ALLOC		0x2	/* Section occupies memory. */
#define SHF_EXECINSTR		0x4	/* Section contains instructions. */
#define SHF_MERGE		0x10	/* Section may be merged. */
#define SHF_STRINGS		0x20	/* Section contains strings. */
#define SHF_INFO_LINK		0x40	/* sh_info holds section index. */
#define SHF_LINK_ORDER		0x80	/* Special ordering requirements. */
#define SHF_OS_NONCONFORMING	0x100	/* OS-specific processing required. */
#define SHF_GROUP		0x200	/* Member of section group. */
#define SHF_TLS			0x400	/* Section contains TLS data. */
#define SHF_MASKOS	0x0ff00000	/* OS-specific semantics. */
#define SHF_MASKPROC	0xf0000000	/* Processor-specific semantics. */




























/*
 * Symbol table entries.
 */

typedef struct {
    Elf32_Word		st_name;	/* String table index of name. */
    Elf32_Addr		st_value;	/* Symbol value. */
    Elf32_Word		st_size;	/* Size of associated object. */
    unsigned char	st_info;	/* Type and binding information. */
    unsigned char	st_other;	/* Reserved (not used). */
    Elf32_Half		st_shndx;	/* Section index of symbol. */
} Elf32_Sym;


/* Macros for accessing the fields of st_info. */
#define ELF32_ST_BIND(info)		((info) >> 4)
#define ELF32_ST_TYPE(info)		((info) & 0xf)

/* Macro for accessing the fields of st_other. */
#define ELF32_ST_VISIBILITY(oth)	((oth) & 0x3)





/* Structures used by Sun & GNU symbol versioning. */
typedef struct
{
    Elf32_Half	vd_version;
    Elf32_Half	vd_flags;
    Elf32_Half	vd_ndx;
    Elf32_Half	vd_cnt;
    Elf32_Word	vd_hash;
    Elf32_Word	vd_aux;
    Elf32_Word	vd_next;
} Elf32_Verdef;

typedef struct
{
	Elf32_Word	vda_name;
	Elf32_Word	vda_next;
} Elf32_Verdaux;

typedef struct
{
	Elf32_Half	vn_version;
	Elf32_Half	vn_cnt;
	Elf32_Word	vn_file;
	Elf32_Word	vn_aux;
	Elf32_Word	vn_next;
} Elf32_Verneed;

typedef struct
{
	Elf32_Word	vna_hash;
	Elf32_Half	vna_flags;
	Elf32_Half	vna_other;
	Elf32_Word	vna_name;
	Elf32_Word	vna_next;
} Elf32_Vernaux;

typedef Elf32_Half Elf32_Versym;

typedef struct {
	Elf32_Half	si_boundto;	/* direct bindings - symbol bound to */
	Elf32_Half	si_flags;	/* per symbol flags */
} Elf32_Syminfo;
















/* Symbol Binding - ELFNN_ST_BIND - st_info */
#define STB_LOCAL	0	/* Local symbol */
#define STB_GLOBAL	1	/* Global symbol */
#define STB_WEAK	2	/* like global - lower precedence */
#define STB_LOOS	10	/* Reserved range for operating system */
#define STB_HIOS	12	/*   specific semantics. */
#define STB_LOPROC	13	/* reserved range for processor */
#define STB_HIPROC	15	/*   specific semantics. */

/* Symbol type - ELFNN_ST_TYPE - st_info */
#define STT_NOTYPE	0	/* Unspecified type. */
#define STT_OBJECT	1	/* Data object. */
#define STT_FUNC	2	/* Function. */
#define STT_SECTION	3	/* Section. */
#define STT_FILE	4	/* Source file. */
#define STT_COMMON	5	/* Uninitialized common block. */
#define STT_TLS		6	/* TLS object. */
#define STT_NUM		7
#define STT_LOOS	10	/* Reserved range for operating system */
#define STT_HIOS	12	/*   specific semantics. */
#define STT_LOPROC	13	/* reserved range for processor */
#define STT_HIPROC	15	/*   specific semantics. */

/* Symbol visibility - ELFNN_ST_VISIBILITY - st_other */
#define STV_DEFAULT	0x0	/* Default visibility (see binding). */
#define STV_INTERNAL	0x1	/* Special meaning in relocatable objects. */
#define STV_HIDDEN	0x2	/* Not visible. */
#define STV_PROTECTED	0x3	/* Visible but not preemptible. */
#define STV_EXPORTED	0x4
#define STV_SINGLETON	0x5
#define STV_ELIMINATE	0x6

/* Special symbol table indexes. */
#define STN_UNDEF	0	/* Undefined symbol index. */

/* Symbol versioning flags. */
#define	VER_DEF_CURRENT	1
#define VER_DEF_IDX(x)	VER_NDX(x)

#define	VER_FLG_BASE	0x01
#define	VER_FLG_WEAK	0x02

#define	VER_NEED_CURRENT	1
#define VER_NEED_WEAK	(1u << 15)
#define VER_NEED_HIDDEN	VER_NDX_HIDDEN
#define VER_NEED_IDX(x)	VER_NDX(x)

#define	VER_NDX_LOCAL	0
#define	VER_NDX_GLOBAL	1
#define VER_NDX_GIVEN	2

#define VER_NDX_HIDDEN	(1u << 15)
#define VER_NDX(x)	((x) & ~(1u << 15))

#define	CA_SUNW_NULL	0
#define	CA_SUNW_HW_1	1		/* first hardware capabilities entry */
#define	CA_SUNW_SF_1	2		/* first software capabilities entry */

/*
 * Syminfo flag values
 */
#define	SYMINFO_FLG_DIRECT	0x0001	/* symbol ref has direct association */
					/*	to object containing defn. */
#define	SYMINFO_FLG_PASSTHRU	0x0002	/* ignored - see SYMINFO_FLG_FILTER */
#define	SYMINFO_FLG_COPY	0x0004	/* symbol is a copy-reloc */
#define	SYMINFO_FLG_LAZYLOAD	0x0008	/* object containing defn should be */
					/*	lazily-loaded */
#define	SYMINFO_FLG_DIRECTBIND	0x0010	/* ref should be bound directly to */
					/*	object containing defn. */
#define	SYMINFO_FLG_NOEXTDIRECT	0x0020	/* don't let an external reference */
					/*	directly bind to this symbol */
#define	SYMINFO_FLG_FILTER	0x0002	/* symbol ref is associated to a */
#define	SYMINFO_FLG_AUXILIARY	0x0040	/* 	standard or auxiliary filter */

/*
 * Syminfo.si_boundto values.
 */
#define	SYMINFO_BT_SELF		0xffff	/* symbol bound to self */
#define	SYMINFO_BT_PARENT	0xfffe	/* symbol bound to parent */
#define	SYMINFO_BT_NONE		0xfffd	/* no special symbol binding */
#define	SYMINFO_BT_EXTERN	0xfffc	/* symbol defined as external */
#define	SYMINFO_BT_LOWRESERVE	0xff00	/* beginning of reserved entries */

/*
 * Syminfo version values.
 */
#define	SYMINFO_NONE		0	/* Syminfo version */
#define	SYMINFO_CURRENT		1
#define	SYMINFO_NUM		2


























#endif // ELF_H

