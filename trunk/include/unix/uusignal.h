#ifndef UUSIGNAL_H
#define UUSIGNAL_H

#include <signal.h>
//#include <unix/uuprocess.h>

errno_t sig_deliver( struct uuprocess *u, struct trap_state *st, int nsig, void *handler );

void sig_init( signal_handling_t *sh );

// Send a signal (turn bit on)
void sig_send(signal_handling_t *sh, int signal );

// Translate signals to user
void sig_exec( struct uuprocess *u, signal_handling_t *sh, struct trap_state *st);



#endif // UUSIGNAL_H
