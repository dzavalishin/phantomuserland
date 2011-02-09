
#define PAGE_SHIFT	12
#define PAGE_SIZE 	4096

#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))
#define PAGE_ALIGNED(x) ( 0 == ( (x)  & (PAGE_SIZE-1) ))

// Align to address of page which contains addr x
#define PREV_PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))

#define ALIGN(x,___how) (((x) + ((___how)-1)) & ~((___how)-1))
