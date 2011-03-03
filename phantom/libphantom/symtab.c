#include <phantom_libc.h>
#include <multiboot.h>
#include <elf.h>
#include <assert.h>
#include <kernel/init.h>

//#include "misc.h"


#define DEBUG 0



typedef struct {
    void 	*address;
    char        *name;
} sym_t;

static sym_t    *symbols;
static int      n_symbols = 0;


static char *find_sym(void * addr)
{
    int i;
    for( i = 0; i < n_symbols-1; i++ )
    {
        if(
           ( (int)addr >= (int)(symbols[i].address) )
           &&
           ( (int)addr < (int)(symbols[i+1].address) )
          )
            return symbols[i].name;
    }

    return "?";
}


static int compare_symbols(const void *a, const void *b)
{
    return ((const sym_t*)a)->address - ((const sym_t*)b)->address;
}

static void load_elf_symtab(
                            Elf32_Sym *symtab, int symsize,
                            int nsymbols,
                            char *strtab,
                            int text_section_header_index,
                            int text_section_load_addr
                           )
{
	// TODO use text_section_load_addr or remove it
	(void) text_section_load_addr;
    Elf32_Sym *sym;

    int maxcount = nsymbols;

    symbols = calloc( nsymbols, sizeof(sym_t) );
    n_symbols = 0;

    sym_t *sptr = symbols;
    int space_left = nsymbols;

    for( sym = symtab; sym < symtab+symsize; sym++ )
    {
        if( maxcount-- <=0 ) break;

        //int type = ELF32_ST_TYPE(sym->st_info);
        //int bind = ELF32_ST_BIND(sym->st_info);

        int value = sym->st_value;
        char *name = strtab+sym->st_name;

        //if(DEBUG > 2) printf("0x%p shndx=%d %s\n", value, sym->st_shndx, name);

        if(text_section_header_index == sym->st_shndx)
        {
            if( 0 == strcmp(".text", name) )
                continue;

            assert(space_left > 0);

            sptr->address = (void*) value;
            sptr->name = name;

            if(DEBUG > 2) printf("0x%p  %s()\n", sptr->address, sptr->name);

            sptr++;
            n_symbols++;
            space_left--;

        }
    }

    if(space_left > 0)
        printf("Warning: extra space left in symtab\n");

    qsort(symbols, n_symbols, sizeof(*symbols), compare_symbols);

    phantom_symtab_getname = find_sym;
}



static void load_elf_hdr(Elf32_Shdr *header, int h_elements)
{
    int text_section_header_index = 0;
    int text_section_load_addr = 0;

    int i;
    for( i = 0; i < h_elements; i++ )
    {
        if(DEBUG > 2) printf(
                             "-- ENTRY type 0x%X, size %d, addr 0x%p\n",
                             (int)header[i].sh_type,
                             (int)header[i].sh_size,
                             (void *)header[i].sh_addr
                            );

        if(
           (header[i].sh_flags & SHF_EXECINSTR)
           &&
           (header[i].sh_type == SHT_PROGBITS)
          )
        {
            text_section_header_index = i;
            text_section_load_addr = header[i].sh_addr;
        }
    }


    Elf32_Shdr *ep = header;
    int neh = h_elements;

    while(neh--)
    {
        if(ep->sh_type == SHT_SYMTAB)
        {
            Elf32_Sym *symtab = (void*)(ep->sh_addr);
            int symsize = ep->sh_size;

            int str_tab_index = ep->sh_link;


            char *strtab = (void *)(header[str_tab_index].sh_addr);
            //int strsize = header[str_tab_index].sh_size;

            if(DEBUG > 9) printf("Got string table %d : '%s'\n", str_tab_index, strtab+1);

            load_elf_symtab(
                            symtab, symsize,
                            symsize/ep->sh_entsize,
                            strtab,
                            text_section_header_index, text_section_load_addr
                           );
        }
        ep++;
    }

    if(DEBUG > 0) getchar();

}

void init_multiboot_symbols(void)
{
    if(bootParameters.flags & MULTIBOOT_ELF_SHDR)
    {
        if(DEBUG > 0) printf("have multiboot ELF SHDR, %d entries, %d bytes, %d shndx\n",
                         bootParameters.syms.e.num,
                         bootParameters.syms.e.size,
                         bootParameters.syms.e.shndx
                        );

        Elf32_Shdr *header = (void *)bootParameters.syms.e.addr;
        load_elf_hdr(header, bootParameters.syms.e.num);
    }

    if(bootParameters.flags & MULTIBOOT_AOUT_SYMS)
    {
        if(DEBUG > 0) printf("have multiboot a.out symbols, %d bytes\n", bootParameters.syms.a.tabsize);
    }

}
