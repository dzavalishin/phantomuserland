void pvm_add_object_to_restart_list( pvm_object_t o )
{
#if COMPILE_WEAKREF
    pvm_object_t wr = pvm_create_weakref_object(o);
    // TODO sync?
    pvm_append_array( pvm_root.restart_list.data, wr );
#else
    pvm_append_array( pvm_root.restart_list.data, o );
#endif
}


void process_restart_list()
{
#if COMPILE_WEAKREF
    int items = get_array_size(pvm_root.restart_list.data);

    pvm_object_t wrc = pvm_get_weakref_class();

    if( !pvm_is_null( pvm_root.restart_list ) )
    {
        printf("Processing restart list: %d items.\n", items);
        for( i = 0; i < items; i++ )
        {
            pvm_object_t wr = pvm_get_array_ofield(pvm_root.restart_list.data, i);
            pvm_object_t o;
            if(!pvm_object_class_is( wr, wrc ))
            {
                //SHOW_ERROR("Not weakref in restart list!");
                printf("Not weakref in restart list!\n");
                //o = wr;
                handle_object_at_restart(wr);
            }
            else
            {
                o = pvm_weakref_get_object( wr );

                if(!pvm_is_null(o) )
                {
                    handle_object_at_restart(o);
                    ref_dec_o(o);
                }
            }
        }

        printf("Done processing restart list.\n");
    }
#endif

}
