#include <errno.h>
#include <sys/types.h>

void 		unicode_to_utf8_char(u_int32_t c, char **out);
errno_t		unicode_to_utf8(const char  *src, int32_t *srcLen, char *dst, int32_t *dstLen);
