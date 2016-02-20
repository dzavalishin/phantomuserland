/*
 * Generic map implementation.
 */
#include "vm_hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_SIZE (256)
#define MAX_CHAIN_LENGTH (8)

/* We need to keep keys and values */
typedef struct _vm_hashmap_element
{
    char* key;
    int in_use;
    any_t data;
} vm_hashmap_element;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _vm_hashmap_map
{
    int table_size;
    int size;
    vm_hashmap_element *data;
} vm_hashmap_map;

/*
 * Return an empty hashmap, or NULL on failure.
 */
map_t vm_hashmap_new()
{
    vm_hashmap_map* m = (vm_hashmap_map*) malloc(sizeof(vm_hashmap_map));
    if(!m) goto err;

    m->data = (vm_hashmap_element*) calloc(INITIAL_SIZE, sizeof(vm_hashmap_element));
    if(!m->data) goto err;

    m->table_size = INITIAL_SIZE;
    m->size = 0;

    return m;
err:
    if (m)
        vm_hashmap_free(m);
    return NULL;
}



/*
 * Hashing function for a string
 */
unsigned int vm_hashmap_hash_int(vm_hashmap_map * m, char* keystring)
{

    unsigned long key = crc32((unsigned char*)(keystring), strlen(keystring));

    /* Robert Jenkins' 32 bit Mix Function */
    key += (key << 12);
    key ^= (key >> 22);
    key += (key << 4);
    key ^= (key >> 9);
    key += (key << 10);
    key ^= (key >> 2);
    key += (key << 7);
    key ^= (key >> 12);

    /* Knuth's Multiplicative Method */
    key = (key >> 3) * 2654435761;

    return key % m->table_size;
}

/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */
int vm_hashmap_hash(map_t in, char* key)
{
    int curr;
    int i;

    /* Cast the hashmap */
    vm_hashmap_map* m = (vm_hashmap_map *) in;

    /* If full, return immediately */
    if(m->size >= (m->table_size/2)) return MAP_FULL;

    /* Find the best index */
    curr = vm_hashmap_hash_int(m, key);

    /* Linear probing */
    for(i = 0; i< MAX_CHAIN_LENGTH; i++)
    {
        if(m->data[curr].in_use == 0)
            return curr;

        if(m->data[curr].in_use == 1 && (strcmp(m->data[curr].key,key)==0))
            return curr;

        curr = (curr + 1) % m->table_size;
    }

    return MAP_FULL;
}

/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
int vm_hashmap_rehash(map_t in){
    int i;
    int old_size;
    vm_hashmap_element* curr;

    /* Setup the new elements */
    vm_hashmap_map *m = (vm_hashmap_map *) in;
    vm_hashmap_element* temp = (vm_hashmap_element *)
        calloc(2 * m->table_size, sizeof(vm_hashmap_element));
    if(!temp) return MAP_OMEM;

    /* Update the array */
    curr = m->data;
    m->data = temp;

    /* Update the size */
    old_size = m->table_size;
    m->table_size = 2 * m->table_size;
    m->size = 0;

    /* Rehash the elements */
    for(i = 0; i < old_size; i++){
        int status;

        if (curr[i].in_use == 0)
            continue;

        status = vm_hashmap_put(m, curr[i].key, curr[i].data);
        if (status != MAP_OK)
            return status;
    }

    free(curr);

    return MAP_OK;
}

/*
 * Add a pointer to the hashmap with some key
 */
int vm_hashmap_put(map_t in, char* key, any_t value){
    int index;
    vm_hashmap_map* m;

    /* Cast the hashmap */
    m = (vm_hashmap_map *) in;

    /* Find a place to put our value */
    index = vm_hashmap_hash(in, key);
    while(index == MAP_FULL)
    {
        if (vm_hashmap_rehash(in) == MAP_OMEM)
        {
            return MAP_OMEM;
        }
        index = vm_hashmap_hash(in, key);
    }

    /* Set the data */
    m->data[index].data = value;
    m->data[index].key = key;
    m->data[index].in_use = 1;
    m->size++;

    return MAP_OK;
}

/*
 * Get your pointer out of the hashmap with a key
 */
int vm_hashmap_get(map_t in, char* key, any_t *arg)
{
    int curr;
    int i;
    vm_hashmap_map* m;

    /* Cast the hashmap */
    m = (vm_hashmap_map *) in;

    /* Find data location */
    curr = vm_hashmap_hash_int(m, key);

    /* Linear probing, if necessary */
    for(i = 0; i<MAX_CHAIN_LENGTH; i++)
    {

        int in_use = m->data[curr].in_use;
        if(in_use == 1)
        {
            if (strcmp(m->data[curr].key,key)==0)
            {
                *arg = (m->data[curr].data);
                return MAP_OK;
            }
        }

        curr = (curr + 1) % m->table_size;
    }

    *arg = NULL;

    /* Not found */
    return MAP_MISSING;
}

/*
 * Iterate the function parameter over each element in the hashmap.  The
 * additional any_t argument is passed to the function as its first
 * argument and the hashmap element is the second.
 */
int vm_hashmap_iterate(map_t in, PFany f, any_t item) {
    int i;

    /* Cast the hashmap */
    vm_hashmap_map* m = (vm_hashmap_map*) in;

    /* On empty hashmap, return immediately */
    if (vm_hashmap_length(m) <= 0)
        return MAP_MISSING;

    /* Linear probing */
    for(i = 0; i< m->table_size; i++)
    {
        if(m->data[i].in_use != 0)
        {
            any_t data = (any_t) (m->data[i].data);
            int status = f(item, data);
            if (status != MAP_OK)
            {
                return status;
            }
        }
    }
    return MAP_OK;
}

/*
 * Remove an element with that key from the map
 */
int vm_hashmap_remove(map_t in, char* key)
{
    int i;
    int curr;
    vm_hashmap_map* m;

    /* Cast the hashmap */
    m = (vm_hashmap_map *) in;

    /* Find key */
    curr = vm_hashmap_hash_int(m, key);

    /* Linear probing, if necessary */
    for(i = 0; i<MAX_CHAIN_LENGTH; i++)
    {

        int in_use = m->data[curr].in_use;
        if (in_use == 1)
        {
            if (strcmp(m->data[curr].key,key)==0)
            {
                /* Blank out the fields */
                m->data[curr].in_use = 0;
                m->data[curr].data = NULL;
                m->data[curr].key = NULL;

                /* Reduce the size */
                m->size--;
                return MAP_OK;
            }
        }
        curr = (curr + 1) % m->table_size;
    }

    /* Data not found */
    return MAP_MISSING;
}

/* Deallocate the hashmap */
void vm_hashmap_free(map_t in)
{
    vm_hashmap_map* m = (vm_hashmap_map*) in;
    free(m->data);
    free(m);
}

/* Return the length of the hashmap */
int vm_hashmap_length(map_t in)
{
    vm_hashmap_map* m = (vm_hashmap_map *) in;
    if(m != NULL) return m->size;
    else return 0;
}

