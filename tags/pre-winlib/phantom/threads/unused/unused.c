
// thread fork support - unused

#if USE_FORK_LUKE
static int t_prepare_fork();




phantom_thread_t *
phantom_create_thread( void (*func)(void *), void *arg, int flags )
{
    assert( ! (flags & ~CREATION_POSSIBLE_FLAGS) );


#if USE_FORK_LUKE
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 
    t->thread_flags = 0;

    t->tid = find_tid(t);
    t->name = "?";

    //t->priority = THREAD_PRIO_NORM;

    int ssize = 64*1024;

    t->stack_size = ssize;
    t->stack = calloc( 1, ssize );

    assert(t->stack != 0);

    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);

    // Let it be elegible to run
    //t->sleep_flags &= ~THREAD_SLEEP_LOCKED;
    //t_enqueue_runq(t);

    GET_CURRENT_THREAD()->child_tid = t->tid;


    GET_CURRENT_THREAD()->thread_flags |= THREAD_FLAG_PARENT;
    t->thread_flags |= THREAD_FLAG_CHILD;


    if(t_prepare_fork())
    {
        // child - NEW STACK, no local vars are accessible
        phantom_thread_t *me = GET_CURRENT_THREAD();

        void (*sstart)(void *) = me->start_func;

        sstart(me->start_func_arg);
        panic("thread returned");
    }

    // parent

    return t;
#else
    //phantom_thread_t *t = find_thread();
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    t->tid = find_tid(t);

    common_thread_init(t, 64*1024 );
        //t->priority = THREAD_PRIO_NORM;



    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);

    t->thread_flags |= flags;
    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    t_enqueue_runq(t);

    return t;
#endif
}







static hal_spinlock_t 	forkLock;
static int              retcount;


static int t_prepare_fork()
{
    hal_spin_lock(&forkLock);
    retcount = 2;



    GET_CURRENT_THREAD()->thread_flags |= THREAD_FLAG_PREFORK;

    while(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_PREFORK)
        ; // TODO need softint here

    if(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_CHILD)
    {
        GET_CURRENT_THREAD()->thread_flags &= ~THREAD_FLAG_CHILD;
        retcount--;
        if(retcount == 0)
            hal_spin_unlock(&forkLock);
        return 1;
    }

    if(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_PARENT)
    {
        GET_CURRENT_THREAD()->thread_flags &=  ~THREAD_FLAG_PARENT;
        retcount--;
        if(retcount == 0)
            hal_spin_unlock(&forkLock);
        return 0;
    }

    panic("t_prepare_fork no fork flag at all!");
}


static int old_sp;
static phantom_thread_t *parent;
static phantom_thread_t *child;

void phantom_thread_in_interrupt_fork()
{
    SHOW_INFO0( 10, "ifork in...");
    assert( forkLock.lock != 0 );
    hal_spin_lock(&schedlock);

    child = get_thread(GET_CURRENT_THREAD()->child_tid);
    parent = GET_CURRENT_THREAD();


//#warning cli
    // Save to me, switch to me
    SHOW_INFO0( 10, "ifork save...");
    phantom_switch_context(parent, parent, &schedlock );
    SHOW_INFO0( 10, "ifork saved...");
    // (OLD) phantom_switch_context() below returns here in old thread!!

    if(!(parent->thread_flags & THREAD_FLAG_PREFORK))
    {
        set_esp(old_sp);
        SHOW_INFO0( 10, "ifork in old...");
        // Second return. We're in old tread and done with fork;
        //GET_CURRENT_THREAD() = parent;
        SET_CURRENT_THREAD(parent);

        // Let child be elegible to run
        child->sleep_flags &= ~THREAD_SLEEP_LOCKED;
        t_enqueue_runq(child);

        return;
    }
    SHOW_INFO0( 10, "ifork cont...");

    parent->thread_flags &= ~THREAD_FLAG_PREFORK;
    //GET_CURRENT_THREAD() = child;
    SET_CURRENT_THREAD(child);

    // Now switch stack and copy some 512 bytes there to make sure
    // new one will have some place to return to

    int cp_size = 512;
    void *from = (void*) (old_sp = get_esp());
    void *to = (child->stack) - cp_size - 1;

    SHOW_INFO0( 10, "ifork memmove...");
    memmove(to, from, cp_size);

    //printf("set ESP...\n");
    SHOW_INFO0( 10, "set ESP...");
    set_esp((int)to);

//#warning sti
    // Save to new, switch to me -- WILL RETURN TO (X)
//printf("ifork GO...\n");
    phantom_switch_context(child, parent, &schedlock );
    // (NEW) phantom_switch_context() below returns here in new thread!!

//printf("ifork in NEW...\n");


}




#endif

