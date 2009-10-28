
#define PAGE_SHIFT	12
#define PAGE_SIZE 	4096

#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))
