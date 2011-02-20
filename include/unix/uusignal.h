#ifndef UUSIGNAL_H
#define UUSIGNAL_H

#include <signal.h>

errno_t sig_deliver( struct trap_state *st, int nsig, void *handler );
void sig_init(signal_handling_t *sh);
void sig_exec(signal_handling_t *sh, struct trap_state *st);



#endif // UUSIGNAL_H
