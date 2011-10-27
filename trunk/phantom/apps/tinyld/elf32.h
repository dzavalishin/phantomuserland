/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _ELF32_H
#define _ELF32_H

#include <sys/types.h>

typedef u_int32_t Elf32_Addr;
typedef u_int16_t Elf32_Half;
typedef u_int32_t Elf32_Off;
typedef   int32_t Elf32_Sword;
typedef u_int32_t Elf32_Word;

#define ELF_MAGIC "\x7f""ELF"
#define EI_MAG0	0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
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

#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#define EV_CURRENT 1

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

struct Elf32_Shdr {
	Elf32_Word		sh_name;
	Elf32_Word		sh_type;
	Elf32_Word		sh_flags;
	Elf32_Addr		sh_addr;
	Elf32_Off		sh_offset;
	Elf32_Word		sh_size;
	Elf32_Word		sh_link;
	Elf32_Word		sh_info;
	Elf32_Word		sh_addralign;
	Elf32_Word		sh_entsize;
} ;

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

#define SHF_WRITE 1

struct Elf32_Phdr {
	Elf32_Word		p_type;
	Elf32_Off		p_offset;
	Elf32_Addr		p_vaddr;
	Elf32_Addr		p_paddr;
	Elf32_Word		p_filesz;
	Elf32_Word		p_memsz;
	Elf32_Word		p_flags;
	Elf32_Word		p_align;
} ;

#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKPROC	0xf0000000


#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

struct Elf32_Sym {
	Elf32_Word		st_name;
	Elf32_Addr		st_value;
	Elf32_Word		st_size;
	unsigned char	st_info;
	unsigned char 	st_other;
	Elf32_Half		st_shndx;
};

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STN_UNDEF 0

struct Elf32_Rel {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
};

struct Elf32_Rela {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;
};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char)(t))

/*
 * i386 relocation types
 */
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF 9
#define R_386_GOTPC 10

/*
 * sh4 relocation types
 */
#define R_SH_NONE 0
#define R_SH_DIR32 1
#define R_SH_REL32 2
#define R_SH_DIR8WPN 3
#define R_SH_IND12W 4
#define R_SH_DIR8WPL 5
#define R_SH_DIR8WPZ 6
#define R_SH_DIR8BP 7
#define R_SH_DIR8W 8
#define R_SH_DIR8L 9
#define R_SH_SWITCH16 25
#define R_SH_SWITCH32 26
#define R_SH_USES 27
#define R_SH_COUNT 28
#define R_SH_ALIGN 29

#define R_SH_CODE 30
#define R_SH_DATA 31
#define R_SH_LABEL 32
#define R_SH_SWITCH8 33
#define R_SH_GNU_VTINHERIT 34
#define R_SH_GNU_VTENTRY 35
#define R_SH_LOOP_START 36
#define R_SH_LOOP_END 37
#define R_SH_DIR5U 45
#define R_SH_DIR6U 46
#define R_SH_DIR6S 47
#define R_SH_DIR10S 48
#define R_SH_DIR10SW 49
#define R_SH_DIR10SL 50
#define R_SH_DIR10SQ 51
#define R_SH_GOT32 160
#define R_SH_PLT32 161
#define R_SH_COPY 162
#define R_SH_GLOB_DAT 163
#define R_SH_JMP_SLOT 164
#define R_SH_RELATIVE 165
#define R_SH_GOTOFF 166
#define R_SH_GOTPC 167
#define R_SH_GOTPLT32 168
#define R_SH_GOT_LOW16 169
#define R_SH_GOT_MEDLOW16 170
#define R_SH_GOT_MEDHI16 171
#define R_SH_GOT_HI16 172
#define R_SH_GOTPLT_LOW16 173
#define R_SH_GOTPLT_MEDLOW16 174
#define R_SH_GOTPLT_MEDHI16 175
#define R_SH_GOTPLT_HI16 176
#define R_SH_PLT_LOW16 177
#define R_SH_PLT_MEDLOW16 178
#define R_SH_PLT_MEDHI16 179
#define R_SH_PLT_HI16 180
#define R_SH_GOTOFF_LOW16 181
#define R_SH_GOTOFF_MEDLOW16 182
#define R_SH_GOTOFF_MEDHI16 183
#define R_SH_GOTOFF_HI16 184
#define R_SH_GOTPC_LOW16 185
#define R_SH_GOTPC_MEDLOW16 186
#define R_SH_GOTPC_MEDHI16 187
#define R_SH_GOTPC_HI16 188
#define R_SH_GOT10BY4 189
#define R_SH_GOTPLT10BY4 190
#define R_SH_GOT10BY8 191
#define R_SH_GOTPLT10BY8 192
#define R_SH_COPY64 193
#define R_SH_GLOB_DAT64 194
#define R_SH_JMP_SLOT64 195
#define R_SH_RELATIVE64 196
#define R_SH_SHMEDIA_CODE 242
#define R_SH_PT_16 243
#define R_SH_IMMS16 244
#define R_SH_IMMU16 245
#define R_SH_IMM_LOW16 246
#define R_SH_IMM_LOW16_PCREL 247
#define R_SH_IMM_MEDLOW16 248
#define R_SH_IMM_MEDLOW16_PCREL 249
#define R_SH_IMM_MEDHI16 250
#define R_SH_IMM_MEDHI16_PCREL 251
#define R_SH_IMM_HI16 252
#define R_SH_IMM_HI16_PCREL 253
#define R_SH_64 254
#define R_SH_64_PCREL 255

/* PowerPC specific declarations */

/*
 * ppc relocation types
 */
#define R_PPC_NONE      0
#define R_PPC_ADDR32        1   /* 32bit absolute address */
#define R_PPC_ADDR24        2   /* 26bit address, 2 bits ignored.  */
#define R_PPC_ADDR16        3   /* 16bit absolute address */
#define R_PPC_ADDR16_LO     4   /* lower 16bit of absolute address */
#define R_PPC_ADDR16_HI     5   /* high 16bit of absolute address */
#define R_PPC_ADDR16_HA     6   /* adjusted high 16bit */
#define R_PPC_ADDR14        7   /* 16bit address, 2 bits ignored */
#define R_PPC_ADDR14_BRTAKEN    8
#define R_PPC_ADDR14_BRNTAKEN   9
#define R_PPC_REL24     10  /* PC relative 26 bit */
#define R_PPC_REL14     11  /* PC relative 16 bit */
#define R_PPC_REL14_BRTAKEN 12
#define R_PPC_REL14_BRNTAKEN    13
#define R_PPC_GOT16     14
#define R_PPC_GOT16_LO      15
#define R_PPC_GOT16_HI      16
#define R_PPC_GOT16_HA      17
#define R_PPC_PLTREL24      18
#define R_PPC_COPY      19
#define R_PPC_GLOB_DAT      20
#define R_PPC_JMP_SLOT      21
#define R_PPC_RELATIVE      22
#define R_PPC_LOCAL24PC     23
#define R_PPC_UADDR32       24
#define R_PPC_UADDR16       25
#define R_PPC_REL32     26
#define R_PPC_PLT32     27
#define R_PPC_PLTREL32      28
#define R_PPC_PLT16_LO      29
#define R_PPC_PLT16_HI      30
#define R_PPC_PLT16_HA      31
#define R_PPC_SDAREL16      32
#define R_PPC_SECTOFF       33
#define R_PPC_SECTOFF_LO    34
#define R_PPC_SECTOFF_HI    35
#define R_PPC_SECTOFF_HA    36
#define R_PPC_NUM       37


struct Elf32_Dyn {
	Elf32_Sword d_tag;
	union {
		Elf32_Word d_val;
		Elf32_Addr d_ptr;
	} d_un;
};

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTREL 20
#define DT_DEBUG 21
#define DT_TEXTREL 22
#define DT_JMPREL 23
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff

#endif
