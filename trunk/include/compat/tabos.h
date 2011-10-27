
#define FileEndOfFile feof
#define FileFlush fflush

#define Print printf
#define FreeMemory free
#define Open open
#define Create creat
#define ThreadExit exit
#define ThreadSnooze sleepmsec
#define Close close
#define MemoryAllocation(__sz) calloc(1,__sz)
#define StringToInteger atol
#define LongSeek lseek
#define Write write
#define Read read
#define IoControl(__f, __t, __d) ioctl(__f, __t, __d, 0)
#define Status stat
#define Unlink unlink
#define GetCurrentWorkingDirectory getcwd

#define SendTo sendto
#define RecvFrom recvfrom
#define CloseSocket close
#define Socket socket

#define Sync sync

#define StringNumPrint snprintf
#define StringPrint snprintf
#define StringCat strcat
#define StringLength strlen
#define StringNumCompare strncmp
#define StringCompare strcmp
#define StringCopy strcpy
#define StringNumCopy strncmp

#define MemorySet memset
#define MemoryCopy memcpy
#define MemoryCompare memcmp

#define CountAllocation calloc

#define uint32 u_int32_t
#define sint32 int32_t

#define uint16 u_int16_t
#define sint16 int16_t

#define uint8 u_int8_t
#define sint8 int8_t


/*
static __inline void Error(const char *s)
{
    printf("Error: %s\n",s);
}
*/

#define Sinus sin
#define Power pow


#define IsHexDigit ishexdigit


#define StringError strerror


#define ESUCCESS 0
#define EFAIL -1
