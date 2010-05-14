#include "mmx.h"
#include <x86/phantom_page.h>

//! \brief Clear a page using MMX extensions.
void fast_clear_page( void *page )
{
    int i;
    unsigned int __cr0;
    __asm__ __volatile__ ("movl %%cr0, %0\n" : "=r"(__cr0));

    // Clear the task switch flag.
    __asm__ __volatile__ ("clts");

    // Clear the 64-bit mm0 register.
    __asm__ __volatile__ ("pxor %mm0, %mm0");

    // Fast clear the page.
    for( i=0; i<PAGE_SIZE/128; i++ )
    {
        __asm__ __volatile__ (
                              "movq %%mm0, (%0)\n"
                              "movq %%mm0, 8(%0)\n"
                              "movq %%mm0, 16(%0)\n"
                              "movq %%mm0, 24(%0)\n"
                              "movq %%mm0, 32(%0)\n"
                              "movq %%mm0, 40(%0)\n"
                              "movq %%mm0, 48(%0)\n"
                              "movq %%mm0, 56(%0)\n"
                              "movq %%mm0, 64(%0)\n"
                              "movq %%mm0, 72(%0)\n"
                              "movq %%mm0, 80(%0)\n"
                              "movq %%mm0, 88(%0)\n"
                              "movq %%mm0, 96(%0)\n"
                              "movq %%mm0, 104(%0)\n"
                              "movq %%mm0, 112(%0)\n"
                              "movq %%mm0, 120(%0)\n"
                              : : "r"(page) : "memory");
        page += 128;
    }

    // Restore task switch flag.
    __asm__ __volatile__ ("movl %0, %%cr0" : "=r"(__cr0));
}
