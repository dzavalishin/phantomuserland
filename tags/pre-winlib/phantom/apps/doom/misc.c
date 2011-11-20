#include <unistd.h>


int abs(int i) { return i < 0 ? -i : i; }

int fclose( int i ) { return close(i); }

long ftell( int i ) { return lseek(i, 0, SEEK_END); }


long fseek( int i, off_t pos, int wh ) { return lseek(i, pos, wh ); }

