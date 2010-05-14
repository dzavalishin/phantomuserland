#ifndef KMAPI_H
#define KMAPI_H

/**
 *
 * General c lib
 *
**/


void *          malloc( size_t size );
void            free( void *mem );

void            printf( const char *fmt, ... );

#include <sys/syslog.h>
//void		syslog(int, const char *, ...) __printflike(2, 3);
//void    	vsyslog(int pri, const char *fmt, va_list ap) __printflike(2, 0);

/**
 *
 * Interrupts
 *
**/


void		km_cli();
void		km_sti();
int		km_is_sti(); // returns true if ints enabled
int		km_save_cli(); // cli and ret 1 if was enabled


/**
 *
 * Physmem/addr space
 *
**/


errno_t        	km_alloc_phys_pages(physaddr_t  *result, int npages); // alloc and not map
void		km_free_phys_pages(physaddr_t  page, int npages); // alloc and not map - WILL PANIC if page is mapped!

int		km_alloc_vaddress(void **result, int num); // alloc address of a page, but not memory
void		km_free_vaddress(void *addr, int num);

// Allocate physmem, address space for it, and map. Panics if out of anything.
void		km_pv_alloc( physaddr_t *pa, void **va, int size_bytes );
// Unmap, free addr space and physmem
void		km_pv_free( physaddr_t pa, void *va, int size_bytes );

// Low ( < 1M ) mem. Identically mapped all the time!
errno_t		km_alloc_phys_pages_low(physaddr_t *result, int npages);
void		km_free_phys_pages_low(physaddr_t  paddr, int npages);

void		km_pages_control( physaddr_t  p, void *page_start_addr, int n_pages, page_mapped_t mapped, page_access_t access );
void		km_pages_control_etc( physaddr_t  pa, void *va, int n_pages, page_mapped_t mapped, page_access_t access, u_int32_t flags );


/**
 *
 * Threads
 *
**/

int		km_start_thread(void (*thread)(void *arg), void *arg, int flags);
void		km_exit_thread(void);


void		km_sleep_msec( int miliseconds );


#endif // KMAPI_H
