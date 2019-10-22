#ifndef _I_UDP
#define _I_UDP

#include <kernel/net/udp.h>


struct data_area_4_udp
{
    void *                              udp_endpoint;

    int32_t                             port; // if bound
    //int32_t                             ipaddr; // if bound
    //int32_t                             connected; // Bind executed
};



#endif // _I_UDP
