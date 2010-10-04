#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

int dbg_init(void);
void kernel_debugger(void);


int dbg_add_command(void (*func)(int, char **), const char *name, const char *desc);



#endif // KERNEL_DEBUG_H
