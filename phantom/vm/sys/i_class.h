#ifndef _I_CLASS_H
#define _I_CLASS_H

// NB! See JIT assembly hardcode for object structure offsets
struct data_area_4_class
{
    unsigned int                        object_flags;                   // object of this class will have such flags
    unsigned int                        object_data_area_size;  // object of this class will have data area of this size
    pvm_object_t                        object_default_interface; // default one

    unsigned int                        sys_table_id; // See above - index into the kernel's syscall tables table

    pvm_object_t                        class_name;
    pvm_object_t                        class_parent;

    pvm_object_t                        static_vars; // array of static variables

    pvm_object_t                        ip2line_maps; // array of maps: ip->line number
    pvm_object_t                        method_names; // array of method names
    pvm_object_t                        field_names; // array of field names

    pvm_object_t                        const_pool; // array of object constants
};

#endif // _I_CLASS_H
