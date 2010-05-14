#include "errno.h"
#include "amap.h"
#include "test.h"

int main( int ac, char ** av )
{
    amap_t _map;
    amap_t *map = &_map;

    amap_init( map, 0, 1025, 0 );
    testPrint(0,"init");
}

