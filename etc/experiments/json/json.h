
typedef int (*json_out)(char c);

struct json_output
{
    json_out    putc;
    int         depth;
};

typedef struct json_output json_output;

void json_start( json_output *jo );
void json_stop( json_output *jo );


void json_out_int( json_output *jo, const char *name, int value );
void json_out_long( json_output *jo, const char *name, long value );
void json_out_string( json_output *jo, const char *name, const char * value );


void json_out_int_array( json_output *jo, const char *name, int *value );
void json_out_string_array( json_output *jo, const char *name, const char * value );

void json_out_open_struct( json_output *jo, const char *name );
void json_out_close_struct( json_output *jo  );

void json_out_open_array( json_output *jo, const char *name );
void json_out_close_array( json_output *jo );

void json_out_delimiter( json_output *jo );

