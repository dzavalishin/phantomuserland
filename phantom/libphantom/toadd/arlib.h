#ifndef ARLIB_H
#define ARLIB_H


int arread( int (*read_f)( void *data, int len ), void (*process_f)(const char *fname, void *data, int len) );





#endif // ARLIB_H

