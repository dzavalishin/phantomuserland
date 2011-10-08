
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"
#include "doomstat.h"
#include "i_net.h"

#ifdef DLHEAP

void* _cdecl dlmalloc(size_t);
void  _cdecl dlfree(void*);
void _cdecl mf_init();

#define malloc dlmalloc
#define free dlfree
#define realloc dlrealloc

#endif

#ifndef B_HOST_IS_LENDIAN
#define B_HOST_IS_LENDIAN 1
#endif
#if !defined(sparc) && B_HOST_IS_LENDIAN
#ifndef ntohl
#define ntohl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))
#endif

#ifndef ntohs
#define ntohs(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8)))
#endif
          
#ifndef htonl
#define htonl(x) ntohl(x)
#endif
#ifndef htons
#define htons(x) ntohs(x)
#endif
#endif

void    NetSend (void);
int NetListen (void);


//
// NETWORKING
//

#ifndef IPPORT_USERRESERVED
#define IPPORT_USERRESERVED     5000
#endif

int     DOOMPORT = (IPPORT_USERRESERVED+0x1d);
int     sendsocket[MAXNETNODES];
int     insocket;

void    (*netget) (void);
void    (*netsend) (void);

static int first_user_port=IPPORT_USERRESERVED+0x1D+0x10;

/**********
int GetAvailPort(void)
{
 int i,d0;
 //for(i=0;i<1024;i++)
 //{
 // __asm__ __volatile__(
 //     "int $0x40"
 //     :"=a"(d0)
 //     :"0"(53),"b"(9),"c"(first_user_port+i));
 // if(d0==1) return i+first_user_port;
 //}
 I_Error("Unable to get new port\n");
 return -1;
}
**********/

int CreateInputUDPsocket(void)
{
 int d0=0;
 //__asm__ __volatile__(
 //    "int $0x40"
 //    :"=a"(d0)
 //    :"0"(53),"b"(1),"c"(DOOMPORT),"d"(0),"S"(0));
 //if(d0==0xffffffff)
 //{
  I_Error("Unable to create socket\n");
 //}
 return d0;
}

int CreateOutputUDPSocket(int remote_ip)
{
 int d0;
 int p;
// p=GetAvailPort();
// __asm__ __volatile__(
//     "int $0x40"
//     :"=a"(d0)
//     :"0"(53),"b"(0),"c"(p),"d"(DOOMPORT),"S"(remote_ip));
// if(d0==0xffffffff)
// {
  I_Error("Unable to create output socket\n");
// }
 return d0;
}

//
// PacketSend
//
void PacketSend (void)
{
 int c;
 doomdata_t sw; 
 
 
 //printf("ERROR Packet Send\n\r");

                
 // byte swap
 sw.checksum = htonl(netbuffer->checksum);
 sw.player = netbuffer->player;
 sw.retransmitfrom = netbuffer->retransmitfrom;
 sw.starttic = netbuffer->starttic;
 sw.numtics = netbuffer->numtics;
 for (c=0 ; c< netbuffer->numtics ; c++)
 {
  sw.cmds[c].forwardmove = netbuffer->cmds[c].forwardmove;
  sw.cmds[c].sidemove = netbuffer->cmds[c].sidemove;
  sw.cmds[c].angleturn = htons(netbuffer->cmds[c].angleturn);
  sw.cmds[c].consistancy = htons(netbuffer->cmds[c].consistancy);
  sw.cmds[c].chatchar = netbuffer->cmds[c].chatchar;
  sw.cmds[c].buttons = netbuffer->cmds[c].buttons;
 }
// __libclog_printf("Sending packet %u to node %u\n",gametic,
//     doomcom->remotenode);
// __asm__ __volatile__(
//     "int $0x40"
//     :"=a"(c)
//     :"0"(53),"b"(4),"c"(sendsocket[doomcom->remotenode]),
//      "d"(doomcom->datalength),"S"((long)&sw));
// if(c==-1)
// {
//  __libclog_printf("Unable to send packet to remote node\n");
// }
}

//
// PacketGet
//
void PacketGet (void)
{
}

int GetLocalAddress (void)
{
 int d0;
// __asm__ __volatile__(
//     "int $0x40"
//     :"=a"(d0)
//     :"0"(52),"b"(1));
 return d0;
}


//
// I_InitNetwork
//

void I_InitNetwork (void)
{
    boolean             trueval = true;
    int                 i;
    int                 p;
        
    doomcom = malloc (sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );
    
    // set up for network
    i = M_CheckParm ("-dup");
    if (i && i< myargc-1)
    {
        doomcom->ticdup = myargv[i+1][0]-'0';
        if (doomcom->ticdup < 1)
            doomcom->ticdup = 1;
        if (doomcom->ticdup > 9)
            doomcom->ticdup = 9;
    }
    else
        doomcom-> ticdup = 1;
        
    if (M_CheckParm ("-extratic"))
        doomcom-> extratics = 1;
    else
        doomcom-> extratics = 0;
                
    p = M_CheckParm ("-port");
    if (p && p<myargc-1)
    {
        DOOMPORT = atoi (myargv[p+1]);
//      __libclog_printf ("using alternate port %i\n",DOOMPORT);
    }

    // parse network game options,
    //  -net <consoleplayer> <host> <host> ...
    i = M_CheckParm ("-net");

    if (!i)
    {
        // single player game
        netgame = false;
        doomcom->id = DOOMCOM_ID;
        doomcom->numplayers = doomcom->numnodes = 1;
        doomcom->deathmatch = false;
        doomcom->consoleplayer = 0;
        return;
    }

    netsend = PacketSend;
    netget = PacketGet;
    netgame = true;

    // parse player number and host list
    doomcom->consoleplayer = myargv[i+1][0]-'1';

    doomcom->numnodes = 1;      // this node for sure
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes;
    sendsocket[0]=0;
    for(i=1;i<MAXNETNODES;i++)
    {
     sendsocket[i]=-1;
    }
    insocket=CreateInputUDPsocket();
 //   __libclog_printf("DOOM: Input UDP socket is %d\n",insocket);
}

void I_NetCmd (void)
{


    //printf("ERROR NetCmd");


    if (doomcom->command == CMD_SEND)
    {
//      netsend ();
    }
    else if (doomcom->command == CMD_GET)
    {
//      netget ();
    }
    else
        I_Error ("Bad net cmd: %i\n",doomcom->command);
}
