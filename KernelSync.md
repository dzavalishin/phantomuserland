# List #

  * Spinlocks - fast, CPU consuming
  * Interrupt disable - fast, protects nothing per se :)
  * Preemption disble - fast, only this CPU
  * Mutexes - slow
  * Conds - slow

Historically we have semaphores also, but shouldn't.


## Spinlocks ##

Spinlock is a fast, lightweit (takes one int) lock for a short pieces of code.

Techincally implemented as integer value, which is zero if spinlock is open and nonzero if locked. Lock is taken with atomic exchange operation:

```
int temp = 1;
atomically_exchange(temp, spinlock);
if(temp != 0)
  wait some more time, consuming CPU.
```

Spinlocks are good when you need to protect data against modification by other CPU. If interrupts/preemption is enabled, spinlock can hang due to modification attempt from other thread or interrupt hangler, so generally spinlock has to be taken with interrupts disabled and/or preemption disabled.

If spinlock is locked by CPU1 and CPU2 attempts to lock it too, CPU2 will wait (spin in a tight loop), hence the name, and it means that spinlocks are not for a long locking - just for a brief structure update, or something like that.

Todo: warn if spinlock is taken with interrupts open and preemption enabled.

```
hal_spinlock_t lock;

hal_spin_init(&lock);

hal_spin_lock(&lock);
...
hal_spin_unlock(&lock);
```

**NB!** Spinlocks can't be used around code which accesses object land. Such code can cause page fault and context switch as a result, which leads to panic in spinlock counter assert in context switch.

## Interrupt disable ##

Interrupt disable protects current CPU from being interrupted. It guarantees you that this CPU won't switch from your code to interrupt handler or other thread.

Interrupt disable + spinlock guarantees you that protected code segment will be executed only by this CPU without intervention.

It is not a good idea to disable interrupts for a long time.

(Don't forget about NMI, though.)

## Preemption disable ##

Preemtion disable makes sure that no thread switch will occure. Interrupts are still possible and interrupt handlers will run. Preemption disable + spinlock are good solution for code which can not be executed in interrupt or clash with interrupt executed code.

It's more or less OK to disable preemption for some 50 msec, for example.

## Mutex ##

```
#include <kernel/mutex.h>
```

Mutex is a high-level sync primitive. An attempt to take mutex which is locked by another thread will put current thread in a blocked state. Thread will continue when mutex is released by previous owner. Mutex is OK to take for a long time (seconds, days, years). Mutex protects against same or other CPU. Generally mutex must be used where possible.

Mutex can't be used:
  * In code which can't block. Driver interrupt handler is an example.
  * In places which do a lot of brief locking: mutex lock is a relatively long process.

## Cond ##

```
#include <kernel/cond.h>
```

Cond is similar to mutex in terms of usage, but does a different thing. Cond is used to signal other thread that it can continue running. Cond is used with mutex - nearly allways!

Here's why. Typical use of cond is, for example, signalling about new data available in buffer. Assume following code:

```
char getchar()
{
    while( buffer_is_empty(buf) )             // A
        hal_cond_wait( buffer_related_cond ); // B
    return buffer_get( buf );                 
}

void interrupt_handler()
{
    char c = read_char_from_device();
    buffer_put( buf, c );                  // C
    hal_cond_signal( buffer_related_cond );   // D
}
```

Now imagine that due to bad luck code was executed ln the following sequence: A, **interrupt**, C, D, **return from interrupt**, B. Getchar checked that buffer is empty, then interrupt code put data and signalled about it with cond - but nobody yet listened to signal, because line B was not executed. Interrupt is lost then - too bad.

Because of this cond is usually used with mutex, which makes sure that data modification and signalling are done atomically. Look:

```
char getchar()
{
    hal_mutex_lock( buffer_related_mutex );
    while( buffer_is_empty(buf) )             // A
        hal_cond_wait( buffer_related_cond, buffer_related_mutex ); // B
    hal_mutex_unlock( buffer_related_mutex );

    return buffer_get( buf );                 
}

void interrupt_handler()
{
    char c = read_char_from_device();

    hal_mutex_lock( buffer_related_mutex );
    buffer_put( buf, c );                     // C
    hal_cond_signal( buffer_related_cond );   // D
    hal_mutex_unlock( buffer_related_mutex );
}
```

Note that hal\_cond\_wait() unlocks given mutex **AFTER** blocking the thread and starting to wait for signal. It makes sure that data modification and cond signalling can't occure between lines A and B.

## Semaphore ##

```
#include <kernel/sem.h>
```

Semaphore can be used as mutex or cond.

Classical use pattern:

```
hal_sem_t sem;

void init()
{
    assert(!hal_sem_init( &sem, "some name for debug" );
}

char getchar()
{   
    while( buffer_is_empty(buf) )
        hal_sem_acquire( &sem );

    return buffer_get( buf );                 
}

void interrupt_handler()
{
    char c = read_char_from_device();

    buffer_put( buf, c );
    hal_sem_release( &sem );
}
```

Semaphore keeps acquire/release count and each `sem_release` will let just one `sem_acquire` to pass through. That's why semaphore needs no additional protection with mutex.

Bulk consumer use pattern:

```

void consumer_thread()
{   
    while(1)
    {
        // Wait for something to come
        hal_sem_acquire( &sem );
        // If we missed some releases, we'll process 'em all below
        hal_sem_zero( &sem );

        while( !buffer_is_empty(buf) )
            do_smthng( buffer_get( buf ) );
    }
}

void interrupt_handler()
{
    char c = read_char_from_device();

    buffer_put( buf, c );
    hal_sem_release( &sem );
}
```