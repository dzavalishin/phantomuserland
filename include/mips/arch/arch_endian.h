// This stuff doesnt work :( - set endianness manually
#if defined(__LITTLE_ENDIAN__) || defined(TARGET_LITTLE_ENDIAN)
#  define BYTE_ORDER LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__) || defined(TARGET_BIG_ENDIAN)
#  define BYTE_ORDER BIG_ENDIAN
#else

//#warning Compiler is silent about endianness
#define BYTE_ORDER BIG_ENDIAN

#endif
