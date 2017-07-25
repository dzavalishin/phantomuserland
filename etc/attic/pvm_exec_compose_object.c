/**
 *
 * Compose object - opcode disabled for security reasons
 *
**/

//pvm_object
//pvm_exec_compose_object( pvm_object in_class, pow_ostack in_stack, int to_pop )
#if 0
static struct pvm_object pvm_exec_compose_object(
                                                 struct pvm_object in_class,
                                                 struct data_area_4_object_stack *in_stack,
                                                 int to_pop
                                                )
{
    //struct pvm_object out = pvm_object_create_fixed( in_class ); // This one does not initialize internal ones
    struct pvm_object out = pvm_create_object( in_class ); // This one does initialize internal ones

    //struct pvm_object_storage	* _data = out.data;

    //struct data_area_4_class *cda = (struct data_area_4_class *)in_class.data->da;

    int das = out.data->_da_size;

    struct pvm_object * data_area = (struct pvm_object *)out.data->da;

    int max = das/sizeof(struct pvm_object);

    if( to_pop > max ) pvm_exec_panic("compose: cant compose so many fields");

    int i;
    for( i = 0; i < to_pop; i++ )
    {
        data_area[i] = pvm_ostack_pop( in_stack );
    }

    for( ; i < max; i++ )
        data_area[i] = pvm_get_null_object(); // null

    /*
    struct pvm_object out;
    out.data = _data;


    out.interface = cda->object_default_interface;
    */
    return out;
}
#endif
