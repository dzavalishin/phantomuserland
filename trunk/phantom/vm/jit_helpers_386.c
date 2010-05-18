#include <vm/object.h>
#include <vm/internal_da.h>

// Not really used, asm code is copied to JIT template
struct pvm_object_storage  * 
    jit_helper_get_iface( struct pvm_object_storage	* data )
{
    return pvm_object_da( data->_class, class )->object_default_interface.data;
}
