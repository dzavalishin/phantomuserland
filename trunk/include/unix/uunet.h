
#define MAX_UU_HOSTNAME 255

// This is pointed by uufile.impl

struct uusocket
{
    sockaddr 	addr;
    void *	prot_data;  	// kern udp_open(&prot_data)/udp_close(prot_data)
    //char *	dest; 		// host:port or addr:port

};

