#ifndef _I_DIRECTORY_H
#define _I_DIRECTORY_H

#define DIR_MUTEX_O 1 // TODO kill me


/// Classic hash map
struct data_area_4_directory
{
    u_int32_t                           capacity;       // size of 1nd level arrays
    u_int32_t                           nEntries;       // number of actual entries stored

    pvm_object_t                        keys;           // Where we actually hold keys
    pvm_object_t                        values;         // Where we actually hold values
    u_int8_t                           *flags;          // Is this keys/values slot pointing to 2nd level array

    pvm_spinlock_t                      pvm_lock;

};

typedef struct data_area_4_directory hashdir_t;

// directory.c
errno_t hdir_add( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t add );
errno_t hdir_find( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t *out, int delete_found );
//! Get all keys as array
errno_t hdir_keys( hashdir_t *dir, pvm_object_t *out );




#endif // _I_DIRECTORY_H
