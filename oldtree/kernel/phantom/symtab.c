#include <phantom_libc.h>
#include <multiboot.h>
#include <elf.h>
#include <assert.h>

#include "misc.h"


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




static void load_elf_symtab(
                            Elf32_Sym *symtab, int symsize,
                            int nsymbols,
                            char *strtab,
                            int text_section_header_index,
                            int text_section_load_addr
                           )
{

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

        //if(DEBUG > 2) printf("0x%X shndx=%d %s\n", value, sym->st_shndx, name);

        if(text_section_header_index == sym->st_shndx)
        {
            if( 0 == strcmp(".text", name) )
                continue;

            assert(space_left > 0);

            // TODO! Kernel load address hardcoded
            //sptr->address = (void*) (0x100000 + value);
            sptr->address = (void*) value;
            //sptr->address = (void*) (text_section_load_addr + value);
            sptr->name = name;

            if(DEBUG > 2) printf("0x%X  %s()\n", sptr->address, sptr->name);

            sptr++;
            n_symbols++;
            space_left--;

        }
    }

    if(space_left > 0)
        printf("Warning: extra space in symtab left");

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
                             "-- ENTRY type 0x%X, size %d, addr 0x%X\n",
                             header[i].sh_type,
                             header[i].sh_size,
                             header[i].sh_addr
                            );

        if(
           (header[i].sh_flags & SHF_EXECINSTR)
           &&
           (header[i].sh_type == SHT_PROGBITS)
          )
        {
            printf("exec is %d\n", i);
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
            //printf("Got sym table!\n");
            Elf32_Sym *symtab = (void*)(ep->sh_addr);
            int symsize = ep->sh_size;

            int str_tab_index = ep->sh_link;


            /*
            int sym_tab_size = ep->sh_info;
            printf("sh_info size = %d, calc'ed size = %d\n",
                   sym_tab_size,
                   symsize/ep->sh_entsize
                  );
            getchar();
            */

            char *strtab = (void *)(header[str_tab_index].sh_addr);
            //int strsize = header[str_tab_index].sh_size;

            printf("Got string table %d : '%s'\n", str_tab_index, strtab+1);
            if(DEBUG > 0) getchar();

            load_elf_symtab(
                            symtab, symsize,
                            symsize/ep->sh_entsize,
                            strtab,
                            text_section_header_index, text_section_load_addr
                           );
        }

        /*
         if(ep->sh_type == SHT_STRTAB)
         {
         //strtab = kphystov(ep->sh_addr);
         strtab = (void *)(ep->sh_addr);
         strsize = ep->sh_size;

         //printf("Got string table! '%s'\n", strtab+1);
         //getchar();
         if(symtab != 0)
         load_elf_symtab(symtab, symsize, strtab);
         }
         */

        //((void *)ep) += step;
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
