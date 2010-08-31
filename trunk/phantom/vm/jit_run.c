


//! Maps bytecode IP values to native IP values
struct jit_address_map
{
    int 	map_size;
    int         *bc2n; // or []?
    int         *n2bc; // or []?
};


typedef struct jit_address_map jit_address_map_t;

// bin search
//! Returns -1 if can't find
int get_native_IP_from_bc_IP( int bytecode_IP, jit_address_map_t *map );

// bin search
//! Returns -1 if can't find
int get_bc_IP_from_native_IP( int bytecode_IP, jit_address_map_t *map );


