/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix subsystem socket machinery.
 *
 *
**/


/**
 * \ingroup Unix
 * @{
**/

#define MAX_UU_HOSTNAME 255

//! This is pointed by uufile.impl

struct uusocket
{
    int         domain;         // Can be ignored as it's allways tcpip now
    int         type;           // need?
    int         protocol;

    //! Bits, see setsockopt
    unsigned 	options;        

    i4sockaddr 	addr;
    void *	prot_data;  	// kern udp_open(&prot_data)/udp_close(prot_data)
    //char *	dest; 		// host:port or addr:port

};

