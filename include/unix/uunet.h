
#define MAX_UU_HOSTNAME 255

// This is pointed by uufile.impl

struct uusocket
{
    int         domain;         // Can be ignored as it's allways tcpip now
    int         type;           // need?
    int         protocol;

    unsigned 	options;        // Bits, see setsockopt

    sockaddr 	addr;
    void *	prot_data;  	// kern udp_open(&prot_data)/udp_close(prot_data)
    //char *	dest; 		// host:port or addr:port

};

