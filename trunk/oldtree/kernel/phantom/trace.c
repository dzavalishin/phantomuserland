#include <threads.h>
#include <string.h>

struct trace_entry {
    int generation;
    void *dst_pc;
    void *src_pc;
    int cpu;
    phantom_thread_t *thread;
    void *stack[4];
};

/* must be power of 2 */
#define CONFIG_TRACE_BUF_SIZE 4096

struct trace_entry trace_buf[CONFIG_TRACE_BUF_SIZE];
static unsigned int trace_buf_wptr;

/* -finstrument-functions */

void __cyg_profile_func_enter(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site) __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
// No __sync_fetch_and_add on arm?
#if ARCH_arm
    (void) this_fn;
    (void) call_site;
#else
#if 0
    unsigned int generation = 1;
    asm volatile (
            "lock xadd %0, %1"
            : "+r"(generation), "+m"(trace_buf_wptr) : : "memory");
#else
    unsigned int generation = __sync_fetch_and_add(&trace_buf_wptr, 1);
#endif
    struct trace_entry *entry = trace_buf + (generation & (CONFIG_TRACE_BUF_SIZE - 1));
    void **frame = (void**)__builtin_frame_address(1);

    entry->dst_pc = this_fn;
    entry->src_pc = call_site;
    entry->cpu = GET_CPU_ID();
    entry->thread = get_current_thread();
    entry->generation = generation;
    if (frame) {
        memcpy(entry->stack, frame + 2, sizeof(entry->stack));
    } else {
        memset(entry->stack, 0xdc, sizeof(entry->stack));
    }
#endif // !ARCH_arm
}

void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    (void)this_fn;
    (void)call_site;
}
