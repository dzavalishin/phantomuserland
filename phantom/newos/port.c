/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Ports. Based on NewOS ports code.
 *
**/

/*
 ** Copyright 2001-2004, Mark-Jan Bastian. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#define DEBUG_MSG_PREFIX "port"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <newos/compat.h>
#include <newos/port.h>
#include <newos/cbuf.h>

#include <kernel/debug.h>
#include <kernel/sem.h>

#include <string.h>
#include <stdlib.h>

#include <hal.h>
#include <threads.h>

#if CONF_NEW_PORTS
#include <hashfunc.h>
#include <kernel/khash.h>
#include <kernel/pool.h>


static pool_t *port_pool;
static void   *port_hash; // name to pool handle

/*
struct port_hash_entry
{
    struct port_entry	*hash_next;
    char                *name;
    pool_handle_t        h;
};
*/

#else

static port_id next_port = 0;
static struct port_entry *ports = NULL;

#endif // CONF_NEW_PORTS


struct port_msg {
    int				msg_code;
    cbuf*			data_cbuf;
    size_t			data_len;
};

struct port_entry {
#if CONF_NEW_PORTS
    struct port_entry		*hash_next;
#endif // CONF_NEW_PORTS
    port_id 			id;
    proc_id 			owner;
    int32 			capacity;
    hal_spinlock_t              lock;
    char			*name;
    hal_sem_t			read_sem;
    hal_sem_t			write_sem;
    int				head;
    int				tail;
    int				total_count;
    bool			closed;
    struct port_msg*		msg_queue;
};



// MAX_PORTS must be power of 2
#define MAX_PORTS 4096
#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE 65536

static bool ports_active = false;

static hal_spinlock_t port_spinlock;
#define GRAB_PORT_LIST_LOCK() acquire_spinlock(&(port_spinlock))
#define RELEASE_PORT_LIST_LOCK() release_spinlock(&(port_spinlock))

#define GRAB_PORT_LOCK(s) hal_spin_lock_cli(&(s).lock)
#define RELEASE_PORT_LOCK(s) hal_spin_unlock_sti(&(s).lock)


// internal API
void dump_port_list(int argc, char **argv);
static void _dump_port_info(struct port_entry *port);
static void dump_port_info(int argc, char **argv);



#if CONF_NEW_PORTS
static void port_pool_el_destroy(void *_el)
{
    struct port_entry *el = _el;
    SHOW_FLOW0( 0, "port handle finalizer");

    el->closed = true; // too late, but...

    if( hash_remove( port_hash, el ) )
        SHOW_ERROR( 0, "Hash remove fail for %s", el->name );

    //struct port_msg*		msg_queue;

    hal_sem_destroy( &el->read_sem );
    hal_sem_destroy( &el->write_sem );

    struct port_msg *q = el->msg_queue;

    // delete the cbuf's that are left in the queue (if any)
    int i;
    for( i = 0; i < el->capacity; i++ )
    {
        if (q[i].data_cbuf != NULL)
            cbuf_free_chain(q[i].data_cbuf);
    }

    free( q );

    free( el->name ); // frees names of 2 semas too
}

static void *port_pool_el_create(void *init)
{
    // Name is allocated outside, reused as port name and name of 2 semas, and freed in port_pool_el_destroy
    char *name = init;

    struct port_entry *el = calloc(1, sizeof(struct port_entry));
    SHOW_FLOW0( 0, "port handle init");


    hal_spin_init(&(el->lock));
    hal_spin_lock_cli(&(el->lock));

    el->name = name;

    if( hal_sem_init( &el->read_sem, name) ) {
        // cleanup
        free(el);
        return 0;
    }

    if( hal_sem_init( &el->write_sem, name ) ) {
        // cleanup
        hal_sem_destroy( &el->read_sem );
        free(el);
        return 0;
    }

    if( hash_insert( port_hash, el ) )
        SHOW_ERROR( 0, "Hash insert fail for %s", name );

    return el;
}


static int port_compare_func(void *_hentry, const void *_name)
{
    //ipv4_fragment *frag = _frag;
    //const ipv4_fragment_key *key = _key;
    struct port_entry *he = _hentry;
    const char *name = _name;

    return strcmp( he->name, name );
}

// XXX lameo hash
static unsigned int port_hash_func(void *_hentry, const void *_name, unsigned int range)
{
    struct port_entry *he = _hentry;
    const char *name = _name;

    if(he)
        return calc_hash( he->name, 0 ) % range;
    else
        return calc_hash( name, 0 ) % range;
}


#endif // CONF_NEW_PORTS



int port_init(void)
{
    hal_spin_init(&port_spinlock);

    port_hash = hash_init( MAX_PORTS, offsetof(struct port_entry, hash_next), &port_compare_func, &port_hash_func);

#if CONF_NEW_PORTS
    port_pool = create_pool_ext( MAX_PORTS, 512 ); // hardcoded arena size
    if( !port_pool )
    {
        // TODO ERROR remove hash? Or just panic?
        return ENOMEM;
    }

    port_pool->destroy = port_pool_el_destroy;
    port_pool->init =    port_pool_el_create;
#else
    int sz;
    sz = sizeof(struct port_entry) * MAX_PORTS;

    ports = calloc(1, sz);

    int i;
    for(i=0; i<MAX_PORTS; i++)
        ports[i].id = -1;
#endif
    // add debugger commands
    dbg_add_command(&dump_port_list, "ports", "Dump a list of all active ports");
    dbg_add_command(&dump_port_info, "port", "Dump info about a particular port");

    ports_active = true;

    return 0;
}



#if CONF_NEW_PORTS

port_id
port_create(int32 queue_length, const char *name)
{
    port_id 	retval;
    char 	*temp_name;
    int 	name_len;
    void 	*q;
    //proc_id	owner;
    //int __newos_intstate;

    if(ports_active == false)
        return -ENXIO;


    // Prepare name ------------------------------------------------------

    if(name == NULL)
        name = "unnamed port";

    name_len = strlen(name) + 1;
    name_len = min(name_len, SYS_MAX_OS_NAME_LEN);

    temp_name = (char *)malloc(name_len);
    if(temp_name == NULL)
        return -ENOMEM;

    strlcpy(temp_name, name, name_len);

    // Make queue --------------------------------------------------------

    // check queue length
    if( queue_length < 1 || queue_length > MAX_QUEUE_LENGTH )
    {
        SHOW_ERROR( 1, "Port %s queue size wrong = %d, made %d", name, queue_length, MAX_QUEUE_LENGTH );
        queue_length = MAX_QUEUE_LENGTH;
    }

    // alloc a queue
    q = malloc( queue_length * sizeof(struct port_msg) );
    if(q == NULL)
    {
        free(temp_name); // dealloc name, too
        SHOW_ERROR( 1, "Port %s queue out of mem, wanted %d bytes", name, queue_length * sizeof(struct port_msg) );
        return -ENOMEM;
    }


    // Get pool el -------------------------------------------------------


    pool_handle_t ph = pool_create_el( port_pool, temp_name );
    if( ph < 0 )
    {
        SHOW_ERROR( 0, "Pool insert fail port %s", temp_name );
        retval = -ENOMEM;
        goto err;
    }

    struct port_entry *port = pool_get_el( port_pool, ph );
    if( !port )
    {
        SHOW_ERROR( 0, "Integrity fail: not in pool after create!? %s", temp_name );
        retval = -ENOENT;
        goto err;
    }

    // Make sure thread will kill ports on death
    assert( 0 == t_add_flags( get_current_tid(), THREAD_FLAG_HAS_PORT ) );
    // wrong - pool el is available to others here, must be locked in the el creation code
    // wrong - redo pool code with spinlocks!
    port->owner = get_current_tid();

    //GRAB_PORT_LIST_LOCK();

    port->id = ph; // my id is my pool handle

    // locked in pool el create func above
    //GRAB_PORT_LOCK(port);
    //RELEASE_PORT_LIST_LOCK();

    port->capacity	= queue_length;

    // simulate hal_sem_init_etc by releasing sema
    {
        int c = queue_length;
        while(c--)
            hal_sem_release( &port->write_sem );
    }

    port->msg_queue	= q;
    port->head 		= 0;
    port->tail 		= 0;
    port->total_count= 0;

    retval = port->id;
    hal_spin_unlock( &port->lock ); // locked in pool port create code


    return retval;

//ierr:
//    int_restore_interrupts();

err:


    if( temp_name ) free(temp_name);

    return retval;
}





int
port_close(port_id id)
{
    if(ports_active == false)   return -ENXIO;

    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }

    port->closed = true;

    pool_release_el( port_pool, id );
    return 0;
}






int
port_delete(port_id id)
{
    //int slot;
    //sem_id	r_sem, w_sem;
    //int capacity;
    //int i;
    //int __newos_intstate;

    //char *old_name;
    //struct port_msg *q;

    if(ports_active == false)   return -ENXIO;


    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }

    //#warning hash remove - check ret?
    //hash_remove(port_hash, port->name );


    // We need to grab port lock to make sure every one is finished with it
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(*port);

    port->closed = true;

    RELEASE_PORT_LOCK(*port);
    //int_restore_interrupts();

    int count;

    while( 1 )
    {
        if( sem_get_count( &port->read_sem, &count) )
            break;

        hal_sleep_msec(1); // FIXME races?

        if( count <= 0 )
            hal_sem_release( &port->read_sem );
        else
            break;

    }

    while( 1 )
    {
        if( sem_get_count( &port->write_sem, &count) )
            break;

        hal_sleep_msec(1); // FIXME races?

        if( count <= 0 )
            hal_sem_release( &port->write_sem );
        else
            break;
    }

//#warning check port->closed after any sem_acquire
    pool_release_el( port_pool, id ); // release my get_el ref count
    pool_release_el( port_pool, id ); // release initial ref_count

    hal_sleep_msec(1); // FIXME races? 

    if( pool_get_el( port_pool, id ) )
    {
        SHOW_ERROR( 0, "port 0x%x still exist after delete!", id );
        pool_release_el( port_pool, id );
    }


    return 0;
}







#warning must return errno_t

port_id
port_find(const char *port_name)
{
    if(ports_active == false)   return -ENXIO;
    if(port_name == NULL)	return -EINVAL;

    // if pe is just being created it can already be in hash but have no id field set
    struct port_entry* pe = hash_lookup(port_hash, port_name );
    if( (pe == 0) || (pe->id == 0) )
        return -ENOENT;

    return pe->id;
}







int
port_get_info(port_id id, struct port_info *info)
{
    //int __newos_intstate;

    if(ports_active == false)	return -ENXIO;
    if (info == NULL)        	return -EINVAL;


    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }

    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(*port);

    // fill a port_info struct with info
    info->id			= port->id;
    info->owner 		= port->owner;
    strncpy(info->name, port->name, min(strlen(port->name),SYS_MAX_OS_NAME_LEN-1));
    info->capacity		= port->capacity;
    sem_get_count( &(port->read_sem), &info->queue_count);
    info->total_count		= port->total_count;

    RELEASE_PORT_LOCK(*port);
    //int_restore_interrupts();

    pool_release_el( port_pool, id ); 

    return 0;
}












ssize_t
port_read(port_id port,
          int32 *msg_code,
          void *msg_buffer,
          size_t buffer_size)
{
    return port_read_etc(port, msg_code, msg_buffer, buffer_size, 0, 0);
}

#warning check err signs, return errno_t

ssize_t
port_read_etc(port_id id,
              int32	*msg_code,
              void	*msg_buffer,
              size_t	buffer_size,
              uint32	flags,
              bigtime_t	timeout)
{
    size_t 	siz;
    int		res;
    int		t;
    cbuf*	msg_store;
    int32	code;
    //int		ei;

    if(ports_active == false)        			return -ENXIO;
    if(msg_code == NULL)        			return -EINVAL;
    if((msg_buffer == NULL) && (buffer_size > 0))       return -EINVAL;
    //if (timeout < 0)        				return -EINVAL;

    flags &= (PORT_FLAG_USE_USER_MEMCPY | PORT_FLAG_INTERRUPTABLE | PORT_FLAG_TIMEOUT);


    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }


    // get 1 entry from the queue, block if needed
    res = sem_acquire_etc(port->read_sem, 1, flags, timeout, NULL);

    // port deletion code might wake us up here, check it
    if( port->closed )
        goto dead;

    if (res != 0) {
        if(res != ETIMEDOUT)
            SHOW_ERROR( 0, "write_port_etc: res unknown error %d", res);
        goto reterr;
    }

    //ei = hal_save_cli();
    GRAB_PORT_LOCK(*port);

    // port deletion code might wake us up here, check it
    if( port->closed )
    {
        RELEASE_PORT_LOCK(*port);
        //if(ei) hal_sti();
        goto dead;
    }

    t = port->tail;
    if (t < 0)        	    	panic("port %id: tail < 0", port->id );
    if (t > port->capacity)     panic("port %id: tail > cap %d", port->id, port->capacity );

    port->tail = (port->tail + 1) % port->capacity;

    msg_store	= port->msg_queue[t].data_cbuf;
    code 	= port->msg_queue[t].msg_code;

    // mark queue entry unused
    port->msg_queue[t].data_cbuf	= NULL;

    // check output buffer size
    siz	= min(buffer_size, port->msg_queue[t].data_len);

    RELEASE_PORT_LOCK(*port);
    //if(ei) hal_sti();

    // copy message
    *msg_code = code;
    if (siz > 0)
        cbuf_memcpy_from_chain(msg_buffer, msg_store, 0, siz);

    // free the cbuf
    cbuf_free_chain(msg_store);

    // make one spot in queue available again for write
    sem_release(port->write_sem);

    pool_release_el( port_pool, id ); 

    return siz;

reterr:
    pool_release_el( port_pool, id ); 
    return -res; // sem_acquire_etc returns positive errno, and we're negative

dead:
    pool_release_el( port_pool, id ); 
    return -ENOENT;
}









errno_t
port_write(port_id id,
           int32 msg_code,
           void *msg_buffer,
           size_t buffer_size)
{
    return port_write_etc(id, msg_code, msg_buffer, buffer_size, 0, 0);
}

errno_t
port_write_etc(port_id id,
               int32 msg_code,
               void *msg_buffer,
               size_t buffer_size,
               uint32 flags,
               bigtime_t timeout)
{
    int res;
    int h;
    cbuf* msg_store;
    //int c1, c2;
    int err; //, ei;

    if(ports_active == false)           return -ENXIO;
    if(id < 0)        			return -EINVAL;

    // mask irrelevant flags
    flags &= PORT_FLAG_USE_USER_MEMCPY | PORT_FLAG_INTERRUPTABLE | PORT_FLAG_TIMEOUT;

    // check buffer_size
    if (buffer_size > PORT_MAX_MESSAGE_SIZE)
        return -EINVAL;

    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }

    res = sem_acquire_etc( port->write_sem, 1, flags & (SEM_FLAG_TIMEOUT | SEM_FLAG_INTERRUPTABLE), timeout, NULL);

    // port deletion code might wake us up here, check it
    GRAB_PORT_LOCK(*port);
    if( port->closed )
    {
        RELEASE_PORT_LOCK(*port);
        goto dead;
    }
    RELEASE_PORT_LOCK(*port);


    if (res != 0) {
        if(res != ETIMEDOUT)
            dprintf("write_port_etc: res unknown error %d\n", res);
        return -res; // negative errno
    }

    if (buffer_size > 0) {
        msg_store = cbuf_get_chain(buffer_size);
        if (msg_store == NULL)
            return -ENOMEM;
        if ((err = cbuf_memcpy_to_chain(msg_store, 0, msg_buffer, buffer_size)) < 0)
            return err; // memory exception
    } else {
        msg_store = NULL;
    }

    // attach copied message to queue
    GRAB_PORT_LOCK(*port);

    h = port->head;
    if (h < 0)        		panic("port %id: head < 0", port->id);
    if (h >= port->capacity)    panic("port %id: head > cap %d", port->id, port->capacity);

    port->msg_queue[h].msg_code	= msg_code;
    port->msg_queue[h].data_cbuf	= msg_store;
    port->msg_queue[h].data_len	= buffer_size;
    port->head = (port->head + 1) % port->capacity;
    port->total_count++;


    RELEASE_PORT_LOCK(*port);


    //sem_get_count( &(port->read_sem), &c1);
    //sem_get_count( &(port->write_sem), &c2);

    // release sem, allowing read (might reschedule)
    sem_release( port->read_sem );

    pool_release_el( port_pool, id ); 

    return 0;

dead:
    pool_release_el( port_pool, id ); 
    return -ENOENT;
}






int32
port_count(port_id id)
{
    int count;

    if(ports_active == false)   return -ENXIO;

    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }


    GRAB_PORT_LOCK(*port);

    sem_get_count( &port->read_sem, &count );
    // do not return negative numbers
    if (count < 0)
        count = 0;

    RELEASE_PORT_LOCK(*port);
    pool_release_el( port_pool, id ); 

    // return count of messages (sem_count)
    return count;
}





ssize_t
port_buffer_size(port_id id)
{
    return port_buffer_size_etc(id, 0, 0);
}

ssize_t
port_buffer_size_etc(port_id id,
                     uint32 flags,
                     bigtime_t timeout)
{
    int res;
    int t;
    int len;

    if(ports_active == false)	return -ENXIO;

    struct port_entry *port = pool_get_el( port_pool, id );
    if( port == 0 )
    {
        SHOW_ERROR( 0, "port 0x%x doesn't exist!", id );
        return -EINVAL;
    }

    res = hal_sem_acquire_etc( &port->read_sem, 1, flags & (SEM_FLAG_TIMEOUT | SEM_FLAG_INTERRUPTABLE), timeout );

    GRAB_PORT_LOCK(*port);
    if (res) {
        // somebody deleted the port ?
        RELEASE_PORT_LOCK(*port);
        pool_release_el( port_pool, id ); 
        return -res; // sem_acquire_etc returns positive errno, and we're negative
    }

    // once message arrived, read data's length

    // determine tail
    // read data's head length
    t = port->head;
    if (t < 0)        			panic("port %id: tail < 0", port->id);
    if (t > port->capacity)       panic("port %id: tail > cap %d", port->id, port->capacity);
    len = port->msg_queue[t].data_len;

    // restore readsem
    hal_sem_release(&port->read_sem);

    RELEASE_PORT_LOCK(*port);
    pool_release_el( port_pool, id ); 

    // return length of item at end of queue
    return len;
}










static errno_t pool_list_port(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct port_entry *port = el;
    (void) pool;
    (void) handle;
    (void) arg;

    printf( "%p\tid: 0x%x\t\tname: '%s'\n", port, port->id, port->name );

    return 0;
}



void dump_port_list(int argc, char **argv)
{
    pool_foreach( port_pool, pool_list_port, 0 );
/*
    int i;

    for(i=0; i<MAX_PORTS; i++) {
        if(ports[i].id >= 0) {
            dprintf("%p\tid: 0x%x\t\tname: '%s'\n", &ports[i], ports[i].id, ports[i].name);
        }
    }
*/
}


struct port_enum_rm
{
    tid_t               owner;
    pool_handle_t 	h;
    bool                found;
};

static errno_t pool_rm_port(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct port_entry *port = el;
    struct port_enum_rm *rm = arg;
    (void) pool;

    if( port->owner == rm->owner )
    {
        rm->found = true;
        rm->h = handle;
        return EEXIST; // stop loop
    }

    return 0;
}


/* this function cycles through the ports, deleting all the ports that are owned by the passed thread */
errno_t port_delete_owned_ports(tid_t owner, int *o_count)
{
    int count = 0;
    errno_t rc;

    if(!ports_active)
        return ENXIO;

    // owns no ports? skip.
    u_int32_t flags;
    assert( 0 == t_get_flags( owner, &flags ) );
    if( !(flags & THREAD_FLAG_HAS_PORT) )
        return 0;

    while(1)
    {
        struct port_enum_rm rm;

        rm.owner = owner;
        rm.found = false;

        rc = pool_foreach( port_pool, pool_rm_port, &rm );

        if( rc != EEXIST )
            break; // not found

        if( !rm.found )
        {
            SHOW_ERROR( 0, "foreach error, !rm.found for %d", rc );
            if( rc == 0 ) rc = ESRCH;
            break;
        }

        count++;
        port_delete( rm.h );
    }

    if( rc && (rc != EEXIST) )
        SHOW_ERROR( 0, "foreach error %d", rc );

    SHOW_FLOW( 11, "foreach killed %d for tid %d", count, owner );
    return (rc != EEXIST) ? rc : 0;
}















#endif // CONF_NEW_PORTS
























































#if 1



static void _dump_port_info(struct port_entry *port)
{
    int cnt;
    dprintf("PORT:   %p\n", port);
    dprintf("name:  '%s'\n", port->name);
    dprintf("owner: 0x%x\n", port->owner);
    dprintf("cap:  %d\n", port->capacity);
    dprintf("head: %d\n", port->head);
    dprintf("tail: %d\n", port->tail);
    sem_get_count(&(port->read_sem), &cnt);
    dprintf("read_sem:  %d\n", cnt);
    sem_get_count(&(port->write_sem), &cnt);
    dprintf("write_sem: %d\n", cnt);
}

errno_t pool_dump_port(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct port_entry *port = el;
    (void) pool;
    (void) handle;
    (void) arg;

    _dump_port_info(port);
    return 0;
}

static void dump_port_info(int argc, char **argv)
{
    if(argc < 2) {
        dprintf("port: not enough arguments\n");
        return;
    }

    // if the argument looks like a hex number, treat it as such
    if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
        unsigned long num = (unsigned long)atol(argv[1]);

#if CONF_NEW_PORTS
        struct port_entry *port = pool_get_el( port_pool, num );
        if( port == 0 )
        {
            SHOW_ERROR( 0, "port 0x%lx doesn't exist!", num);
            return;
        }
        _dump_port_info(port);
        pool_release_el( port_pool, num );

#else
        unsigned slot = num % MAX_PORTS;
        if(ports[slot].id != (int)num) {
            dprintf("port 0x%lx doesn't exist!\n", num);
            return;
        }
        _dump_port_info(&ports[slot]);
#endif
        return;

    }

#if CONF_NEW_PORTS

    pool_foreach( port_pool, pool_dump_port, 0 );

#else
    int i;

    // walk through the ports list, trying to match name
    for(i=0; i<MAX_PORTS; i++) {
        if (ports[i].name != NULL)
            if(strcmp(argv[1], ports[i].name) == 0) {
                _dump_port_info(&ports[i]);
                return;
            }
    }
#endif

}
#endif


#if !CONF_NEW_PORTS
port_id
port_create(int32 queue_length, const char *name)
{
    int 	i;
    hal_sem_t 	sem_r, sem_w;
    port_id retval;
    char 	*temp_name;
    int 	name_len;
    void 	*q;
    proc_id	owner;
    int __newos_intstate; // wee need it as spinlocks overlap

    memset( &sem_r, 0, sizeof(sem_r) );
    memset( &sem_w, 0, sizeof(sem_w) );

    if(ports_active == false)
        return -ENXIO;

    if(name == NULL)
        name = "unnamed port";

    name_len = strlen(name) + 1;
    name_len = min(name_len, SYS_MAX_OS_NAME_LEN);

    temp_name = (char *)kmalloc(name_len);
    if(temp_name == NULL)
        return -ENOMEM;

    strlcpy(temp_name, name, name_len);

    // check queue length
    if (queue_length < 1 || queue_length > MAX_QUEUE_LENGTH) {
        kfree(temp_name);
        return -EINVAL;
    }

    // alloc a queue
    q = kmalloc( queue_length * sizeof(struct port_msg) );
    if (q == NULL) {
        kfree(temp_name); // dealloc name, too
        return -ENOMEM;
    }

    // create sem_r with owner set to -1
    //sem_r = sem_create_etc(0, temp_name, -1);
    if (hal_sem_init(&sem_r, temp_name)) {
        // cleanup
        kfree(temp_name);
        kfree(q);
        return -ENOMEM;
    }

    // create sem_w
    //sem_w = sem_create_etc(queue_length, temp_name, -1);
    //if (hal_sem_init_etc(&sem_w, temp_name, queue_length )) {
    if (hal_sem_init(&sem_w, temp_name )) {
        // cleanup
        hal_sem_destroy(&sem_r);
        kfree(temp_name);
        kfree(q);
        return -ENOMEM;
    }

    // simulate hal_sem_init_etc by releasing sema
    {
        int c = queue_length;
        while(c--)
            hal_sem_release( &sem_w );
    }

    owner = 0;
    //owner = proc_get_current_proc_id();

    //int_disable_interrupts();
    __newos_intstate = hal_save_cli();
    GRAB_PORT_LIST_LOCK();

    // find the first empty spot
    for(i=0; i<MAX_PORTS; i++) {
        if(ports[i].id == -1) {
            // make the port_id be a multiple of the slot it's in
            if(i >= next_port % MAX_PORTS) {
                next_port += i - next_port % MAX_PORTS;
            } else {
                next_port += MAX_PORTS - (next_port % MAX_PORTS - i);
            }
            ports[i].id		= next_port++;

            //ports[i].lock	= 0;
            hal_spin_init(&(ports[i].lock));

            GRAB_PORT_LOCK(ports[i]);
            RELEASE_PORT_LIST_LOCK();

            ports[i].capacity	= queue_length;
            ports[i].name 		= temp_name;

            // assign sem
            ports[i].read_sem	= sem_r;
            ports[i].write_sem	= sem_w;
            ports[i].msg_queue	= q;
            ports[i].head 		= 0;
            ports[i].tail 		= 0;
            ports[i].total_count= 0;
            ports[i].owner 		= owner;
            retval = ports[i].id;
            RELEASE_PORT_LOCK(ports[i]);
            goto out;
        }
    }
    // not enough ports...
    RELEASE_PORT_LIST_LOCK();
    //kfree(q);
    //kfree(temp_name);
    retval = -ENOMEM;
    dprintf("port_create(): ERR_PORT_OUT_OF_SLOTS\n");

    // cleanup
    sem_delete(sem_w);
    sem_delete(sem_r);
    kfree(temp_name);
    kfree(q);

out:
    int_restore_interrupts();

    return retval;
}

void dump_port_list(int argc, char **argv)
{
    int i;

    for(i=0; i<MAX_PORTS; i++) {
        if(ports[i].id >= 0) {
            dprintf("%p\tid: 0x%x\t\tname: '%s'\n", &ports[i], ports[i].id, ports[i].name);
        }
    }
}





int
port_close(port_id id)
{
    int		slot;
    //int     __newos_intstate;

    if(ports_active == false)   return -ENXIO;
    if(id < 0)                  return -EINVAL;
    slot = id % MAX_PORTS;

    // walk through the sem list, trying to match name
    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();

    GRAB_PORT_LOCK(ports[slot]);

    if (ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        int_restore_interrupts();
        return -EINVAL;
    }

    // mark port to disable writing
    ports[slot].closed = true;

    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    return 0;
}




#warning port delete kills OS
int
port_delete(port_id id)
{
    int slot;
    sem_id	r_sem, w_sem;
    int capacity;
    int i;
    //int __newos_intstate;

    char *old_name;
    struct port_msg *q;

    if(ports_active == false)   return -ENXIO;
    if(id < 0)                  return -EINVAL;

    slot = id % MAX_PORTS;

    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        //int_restore_interrupts();
        dprintf("port_delete: invalid port_id %d\n", id);
        return -EINVAL;
    }

    /* mark port as invalid */
    ports[slot].id	 = -1;
    old_name 		 = ports[slot].name;
    q				 = ports[slot].msg_queue;
    r_sem			 = ports[slot].read_sem;
    w_sem			 = ports[slot].write_sem;
    capacity		 = ports[slot].capacity;
    ports[slot].name = NULL;

    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    // delete the cbuf's that are left in the queue (if any)
    for (i=0; i<capacity; i++) {
        if (q[i].data_cbuf != NULL)
            cbuf_free_chain(q[i].data_cbuf);
    }

    kfree(q);
    kfree(old_name);

#warning implement what is written below
    // release the threads that were blocking on this port by deleting the sem
    // read_port() will see the ERR_SEM_DELETED acq_sem() return value, and act accordingly
    sem_delete(r_sem);
    sem_delete(w_sem);

    return 0;
}






port_id
port_find(const char *port_name)
{
    int i;
    int ret_val = -EINVAL;
    //int __newos_intstate;

    if(ports_active == false)   return -ENXIO;
    if(port_name == NULL)	return -EINVAL;

    // lock list of ports
    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LIST_LOCK();

    // loop over list
    for(i=0; i<MAX_PORTS; i++) {
        // lock every individual port before comparing
        GRAB_PORT_LOCK(ports[i]);
        if(ports[i].id >= 0 && strcmp(port_name, ports[i].name) == 0) {
            ret_val = ports[i].id;
            RELEASE_PORT_LOCK(ports[i]);
            break;
        }
        RELEASE_PORT_LOCK(ports[i]);
    }

    RELEASE_PORT_LIST_LOCK();
    //int_restore_interrupts();

    return ret_val;
}



int
port_get_info(port_id id, struct port_info *info)
{
    int slot;
    //int __newos_intstate;

    if(ports_active == false)	return -ENXIO;
    if (info == NULL)        	return -EINVAL;
    if(id < 0)        		return -EINVAL;

    slot = id % MAX_PORTS;

    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        //int_restore_interrupts();
        dprintf("port_get_info: invalid port_id %d\n", id);
        return -EINVAL;
    }

    // fill a port_info struct with info
    info->id			= ports[slot].id;
    info->owner 		= ports[slot].owner;
    strncpy(info->name, ports[slot].name, min(strlen(ports[slot].name),SYS_MAX_OS_NAME_LEN-1));
    info->capacity		= ports[slot].capacity;
    sem_get_count( &(ports[slot].read_sem), &info->queue_count);
    info->total_count	= ports[slot].total_count;

    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    // from our port_entry
    return 0;
}


int
port_get_next_port_info(proc_id proc,
                        uint32 *cookie,
                        struct port_info *info)
{
    int slot;
    //int __newos_intstate;

    if(ports_active == false)	return -ENXIO;
    if (cookie == NULL)        	return -EINVAL;

    if (*cookie == NULL) {
        // return first found
        slot = 0;
    } else {
        // start at index cookie, but check cookie against MAX_PORTS
        slot = *cookie;
        if (slot >= MAX_PORTS)	return -EINVAL;
    }

    // spinlock
    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LIST_LOCK();

    info->id = -1; // used as found flag
    while (slot < MAX_PORTS) {
        GRAB_PORT_LOCK(ports[slot]);
        if (ports[slot].id != -1)
            if (ports[slot].owner == proc) {
                // found one!
                // copy the info
                info->id			= ports[slot].id;
                info->owner 		= ports[slot].owner;
                strncpy(info->name, ports[slot].name, min(strlen(ports[slot].name),SYS_MAX_OS_NAME_LEN-1));
                info->capacity		= ports[slot].capacity;
                sem_get_count( &(ports[slot].read_sem), &info->queue_count);
                info->total_count	= ports[slot].total_count;
                RELEASE_PORT_LOCK(ports[slot]);
                slot++;
                break;
            }
        RELEASE_PORT_LOCK(ports[slot]);
        slot++;
    }
    RELEASE_PORT_LIST_LOCK();
    //int_restore_interrupts();

    if (info->id == -1)
        return -ENOENT;
    *cookie = slot;
    return 0;
}

ssize_t
port_buffer_size(port_id id)
{
    return port_buffer_size_etc(id, 0, 0);
}

ssize_t
port_buffer_size_etc(port_id id,
                     uint32 flags,
                     bigtime_t timeout)
{
    int slot;
    int res;
    int t;
    int len;
    //int __newos_intstate;

    if(ports_active == false)	return -ENXIO;
    if(id < 0)                  return -EINVAL;

    slot = id % MAX_PORTS;

    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        int_restore_interrupts();
        dprintf("port_get_info: invalid port_id %d\n", id);
        return -EINVAL;
    }
    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    // block if no message,
    // if TIMEOUT flag set, block with timeout

    // XXX - is it a race condition to acquire a sem just after we
    // unlocked the port ?
    // XXX: call an acquire_sem which does the release lock, restore int & block the right way
    res = sem_acquire_etc(ports[slot].read_sem, 1, flags & (SEM_FLAG_TIMEOUT | SEM_FLAG_INTERRUPTABLE), timeout, NULL);

    GRAB_PORT_LOCK(ports[slot]);
    if (res) {
        // somebody deleted the port ?
        RELEASE_PORT_LOCK(ports[slot]);
        return -res; // sem_acquire_etc returns positive errno, and we're negative
    }

    // once message arrived, read data's length

    // determine tail
    // read data's head length
    t = ports[slot].head;
    if (t < 0)
        panic("port %id: tail < 0", ports[slot].id);
    if (t > ports[slot].capacity)
        panic("port %id: tail > cap %d", ports[slot].id, ports[slot].capacity);
    len = ports[slot].msg_queue[t].data_len;

    // restore readsem
    hal_sem_release(&ports[slot].read_sem);

    RELEASE_PORT_LOCK(ports[slot]);

    // return length of item at end of queue
    return len;
}

int32
port_count(port_id id)
{
    int slot;
    int count;
    //int __newos_intstate;

    if(ports_active == false)   return -ENXIO;
    if(id < 0)                  return -EINVAL;

    slot = id % MAX_PORTS;

    //int_disable_interrupts();
    //__newos_intstate = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        //int_restore_interrupts();
        dprintf("port_count: invalid port_id %d\n", id);
        return -EINVAL;
    }

    sem_get_count( &(ports[slot].read_sem), &count);
    // do not return negative numbers
    if (count < 0)
        count = 0;

    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    // return count of messages (sem_count)
    return count;
}

ssize_t
port_read(port_id port,
          int32 *msg_code,
          void *msg_buffer,
          size_t buffer_size)
{
    return port_read_etc(port, msg_code, msg_buffer, buffer_size, 0, 0);
}

ssize_t
port_read_etc(port_id id,
              int32	*msg_code,
              void	*msg_buffer,
              size_t	buffer_size,
              uint32	flags,
              bigtime_t	timeout)
{
    int		slot;
    hal_sem_t	cached_semid;
    size_t 	siz;
    int		res;
    int		t;
    cbuf*	msg_store;
    int32	code;
    //int		ei;

    if(ports_active == false)        			return -ENXIO;
    if(id < 0)        					return -EINVAL;
    if(msg_code == NULL)        			return -EINVAL;
    if((msg_buffer == NULL) && (buffer_size > 0))       return -EINVAL;
    //if (timeout < 0)        				return -EINVAL;

    flags &= (PORT_FLAG_USE_USER_MEMCPY | PORT_FLAG_INTERRUPTABLE | PORT_FLAG_TIMEOUT);

    slot = id % MAX_PORTS;

    //ei = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        //if(ei) hal_sti();
        dprintf("read_port_etc: invalid port_id %d\n", id);
        return -EINVAL;
    }
    // store sem_id in local variable
    cached_semid = ports[slot].read_sem;

    // unlock port && enable ints/
    RELEASE_PORT_LOCK(ports[slot]);
    //if(ei) hal_sti();

    // XXX -> possible race condition if port gets deleted (->sem deleted too), therefore
    // sem_id is cached in local variable up here

    // get 1 entry from the queue, block if needed
    res = sem_acquire_etc(cached_semid, 1,
                          flags, timeout, NULL);
    // XXX: possible race condition if port read by two threads...
    //      both threads will read in 2 different slots allocated above, simultaneously
    // 		slot is a thread-local variable

    if (res != 0) {
        if(res != ETIMEDOUT)
            dprintf("write_port_etc: res unknown error %d\n", res);
        goto reterr;
    }

    //ei = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    t = ports[slot].tail;
    if (t < 0)
        panic("port %id: tail < 0", ports[slot].id);
    if (t > ports[slot].capacity)
        panic("port %id: tail > cap %d", ports[slot].id, ports[slot].capacity);

    ports[slot].tail = (ports[slot].tail + 1) % ports[slot].capacity;

    msg_store	= ports[slot].msg_queue[t].data_cbuf;
    code 	= ports[slot].msg_queue[t].msg_code;

    // mark queue entry unused
    ports[slot].msg_queue[t].data_cbuf	= NULL;

    // check output buffer size
    siz	= min(buffer_size, ports[slot].msg_queue[t].data_len);

    cached_semid = ports[slot].write_sem;

    RELEASE_PORT_LOCK(ports[slot]);
    //if(ei) hal_sti();

    // copy message
    *msg_code = code;
    if (siz > 0) {
        /*
         if (flags & PORT_FLAG_USE_USER_MEMCPY) {
         if ((err = cbuf_user_memcpy_from_chain(msg_buffer, msg_store, 0, siz) < 0))	{
         // leave the port intact, for other threads that might not crash
         cbuf_free_chain(msg_store);
         sem_release(cached_semid);
         return err;
         }
         } else */

        cbuf_memcpy_from_chain(msg_buffer, msg_store, 0, siz);
    }
    // free the cbuf
    cbuf_free_chain(msg_store);

    // make one spot in queue available again for write
    sem_release(cached_semid);

    return siz;

reterr:

    return -res; // sem_acquire_etc returns positive errno, and we're negative
}

#if 0
errno_t
port_set_owner(port_id id, proc_id proc)
{
    int slot;

    if(ports_active == false)        	return -ENXIO;
    if(id < 0)        			return -EINVAL;

    slot = id % MAX_PORTS;

    //int_disable_interrupts();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        int_restore_interrupts();
        dprintf("port_set_owner: invalid port_id %d\n", id);
        return -EINVAL;
    }

    // transfer ownership to other process
    ports[slot].owner = proc;

    // unlock port
    RELEASE_PORT_LOCK(ports[slot]);
    //int_restore_interrupts();

    return 0;
}
#endif


errno_t
port_write(port_id id,
           int32 msg_code,
           void *msg_buffer,
           size_t buffer_size)
{
    return port_write_etc(id, msg_code, msg_buffer, buffer_size, 0, 0);
}

errno_t
port_write_etc(port_id id,
               int32 msg_code,
               void *msg_buffer,
               size_t buffer_size,
               uint32 flags,
               bigtime_t timeout)
{
    int slot;
    int res;
    hal_sem_t cached_semid;
    int h;
    cbuf* msg_store;
    int c1, c2;
    int err, ei;

    if(ports_active == false)           return -ENXIO;
    if(id < 0)        			return -EINVAL;

    // mask irrelevant flags
    flags &= PORT_FLAG_USE_USER_MEMCPY | PORT_FLAG_INTERRUPTABLE | PORT_FLAG_TIMEOUT;

    slot = id % MAX_PORTS;

    // check buffer_size
    if (buffer_size > PORT_MAX_MESSAGE_SIZE)
        return -EINVAL;

    ei = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    if(ports[slot].id != id) {
        RELEASE_PORT_LOCK(ports[slot]);
        if(ei) hal_sti();
        dprintf("write_port_etc: invalid port_id %d\n", id);
        return -EINVAL;
    }

    if (ports[slot].closed) {
        RELEASE_PORT_LOCK(ports[slot]);
        if(ei) hal_sti();
        dprintf("write_port_etc: port %d closed\n", id);
        return -EPERM;
    }

    // store sem_id in local variable
    cached_semid = ports[slot].write_sem;

    RELEASE_PORT_LOCK(ports[slot]);
    if(ei) hal_sti();

    // XXX -> possible race condition if port gets deleted (->sem deleted too),
    // and queue is full therefore sem_id is cached in local variable up here

    // get 1 entry from the queue, block if needed
    // assumes flags
    res = sem_acquire_etc(cached_semid, 1,
                          flags & (SEM_FLAG_TIMEOUT | SEM_FLAG_INTERRUPTABLE), timeout, NULL);

    // XXX: possible race condition if port written by two threads...
    //      both threads will write in 2 different slots allocated above, simultaneously
    // 		slot is a thread-local variable

    if (res != 0) {
        if(res != ETIMEDOUT)
            dprintf("write_port_etc: res unknown error %d\n", res);
        return -res; // negative errno
    }

    if (buffer_size > 0) {
        msg_store = cbuf_get_chain(buffer_size);
        if (msg_store == NULL)
            return -ENOMEM;
        /*
         if (flags & PORT_FLAG_USE_USER_MEMCPY) {
         // copy from user memory
         if ((err = cbuf_user_memcpy_to_chain(msg_store, 0, msg_buffer, buffer_size)) < 0)
         return err; // memory exception
         } else
         */
        // copy from kernel memory
        if ((err = cbuf_memcpy_to_chain(msg_store, 0, msg_buffer, buffer_size)) < 0)
            return err; // memory exception
    } else {
        msg_store = NULL;
    }

    // attach copied message to queue
    ei = hal_save_cli();
    GRAB_PORT_LOCK(ports[slot]);

    h = ports[slot].head;
    if (h < 0)
        panic("port %id: head < 0", ports[slot].id);
    if (h >= ports[slot].capacity)
        panic("port %id: head > cap %d", ports[slot].id, ports[slot].capacity);
    ports[slot].msg_queue[h].msg_code	= msg_code;
    ports[slot].msg_queue[h].data_cbuf	= msg_store;
    ports[slot].msg_queue[h].data_len	= buffer_size;
    ports[slot].head = (ports[slot].head + 1) % ports[slot].capacity;
    ports[slot].total_count++;

    // store sem_id in local variable
    cached_semid = ports[slot].read_sem;

    RELEASE_PORT_LOCK(ports[slot]);
    if(ei) hal_sti();

    sem_get_count( &(ports[slot].read_sem), &c1);
    sem_get_count( &(ports[slot].write_sem), &c2);

    // release sem, allowing read (might reschedule)
    sem_release(cached_semid);

    return 0;
}


#if 0
/* this function cycles through the ports table, deleting all the ports that are owned by
 the passed proc_id */
int port_delete_owned_ports(proc_id owner)
{
    int i;
    int count = 0;

    if(ports_active == false)
        return -ENXIO;

    //int ei = hal_save_cli();
    GRAB_PORT_LIST_LOCK();

    for(i=0; i<MAX_PORTS; i++) {
        if(ports[i].id != -1 && ports[i].owner == owner) {
            port_id id = ports[i].id;

            RELEASE_PORT_LIST_LOCK();
            hal_sti();

            port_delete(id);
            count++;

            hal_cli();
            GRAB_PORT_LIST_LOCK();
        }
    }

    RELEASE_PORT_LIST_LOCK();
    //if(ei) hal_sti();

    return count;
}
#endif

#endif // !CONF_NEW_PORTS


