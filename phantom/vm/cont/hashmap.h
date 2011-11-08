#ifndef HASHMAP_H
#define HASHMAP_H

/*
  Written from scratch 2001-08-02 sandro@w3.org.
  W3C license
*/

#include "iter.h"

unsigned int calc_hash(const char *datum, const char *datum_end);


struct hashmap_entry {
    struct hashmap_entry *next_in_bucket;
    void *value;
};

struct hashmap {
    struct hashmap_entry **table;
    unsigned used_slots;
    short table_size_index;  /* 0 is the smallest table, 1 is step larger... */
};

typedef struct hashmap Hashmap;

void hashmap_open(Hashmap*, unsigned int initial_size);

void hashmap_close(Hashmap*);

void* hashmap_get(Hashmap*, void *key, void *key_end);

void hashmap_put(Hashmap*, void *key, void *key_end, 
          void *data, void *data_end);

void* hashmap_get_or_put(Hashmap*, 
                void *key, void *key_end, 
                void *data, void *data_end);

int hashmap_remove(Hashmap*, void *key, void *key_end);

void* hashmap_iterate(Hashmap*, iter* i, void** value);

#endif

