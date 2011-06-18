#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
//#include <sys/resolv.h>

#include <sys/unistd.h>
#include <user/sys_getset.h>
#include <user/sys_fio.h>

#include <user/sys_misc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../mpeg/tabos_compat.h"


#define IP_GET_HDR_LEN( ip )		( ( ( uint32 )( ip ) -> nHdrSize ) << 2 )

struct sHostEnt *GetHostByName( const char * host );

uint32 nSeqNo = 0;


struct sHostEnt {
	sint8 *pnName;
	sint8 **ppnAliases;
	sint32 nAddrType;
	sint32 nLength;
	sint8 **ppnAddrList;
};

struct sIcmpHeader {
    uint8 nType;
    uint8 nCode;
    uint16 nChkSum;
    uint16 nId;
    uint16 nSeq;
    uint32 nTimeStamp;
} __attribute__ ((packed));

struct sPingStat {
    sint32 nTmin;
    sint32 nTmax;
    sint32 nTsum;
    sint32 nTransmitted;
    sint32 nRecieved;
};

/** IP Header */
struct sIpHeader {
	uint8 nHdrSize:4;
	uint8 nVersion:4;

	uint8 nTypeOfService;
	uint16 nPacketSize;
	uint16 nPacketId;
	uint16 nFragOffset;
	uint8 nTimeToLive;
	uint8 nProtocol;
	uint16 nCheckSum;
#ifdef _KERNEL_
	ipaddr anSrcAddr;
	ipaddr anDstAddr;
#else
	struct in_addr anSrcAddr;
	struct in_addr anDstAddr;
#endif
};


struct sTimeVal {
	sint32 nSec;
	sint32 nUsec;
};


#define DEFAULT_PACKET_SIZE	32
#define MAX_PACKET			1024
#define PACKET_SIZE			( sizeof( struct sIcmpHeader ) + MAX_PACKET )
#define ICMP_ECHO			8
#define ICMP_ECHOREPLY		0
#define ICMP_MIN			8

void tvsub( struct sTimeVal *psOut, struct sTimeVal *psIn ) {
    if ( ( psOut -> nUsec -= psIn -> nUsec ) < 0 ) {
        psOut -> nSec--;
        psOut -> nUsec += 1000000;
    }
    psOut -> nSec -= psIn -> nSec;
}

void fillIcmpData( sint8 *pnIcmpData, sint32 nDataSize ) {
    struct sIcmpHeader *psHeader;
    sint8 *pnDataPart;

    psHeader = ( struct sIcmpHeader * ) pnIcmpData;

    psHeader -> nType = ICMP_ECHO;
    psHeader -> nCode = 0;
    psHeader -> nId = ( uint16 ) 42;
    psHeader -> nChkSum = 0;
    psHeader -> nSeq = 0;

    pnDataPart = pnIcmpData + sizeof( struct sIcmpHeader );
    memset( pnDataPart, 'E', nDataSize - sizeof( struct sIcmpHeader ) );
}

void decodeResp( sint8 *pnBuf, sint32 nBytes, struct sockaddr_in *psFrom, struct sPingStat *psStat ) {
    struct sIpHeader *psIpHdr;
    struct sIcmpHeader *psIcmpHeader;
    uint16 nIpHdrLen;
    sint32 nTripTime;
    struct sTimeVal sTv;
    struct sTimeVal *psTp;

    psIpHdr = ( struct sIpHeader * ) pnBuf;
    nIpHdrLen = IP_GET_HDR_LEN( psIpHdr );

    if ( nBytes < nIpHdrLen + ICMP_MIN ) {
        printf( "Too few bytes from %s (%d/%d)\n",
                inet_ntoa( psFrom -> sin_addr ),
                nBytes, nIpHdrLen + ICMP_MIN );
        return;
    }

    psIcmpHeader = ( struct sIcmpHeader * ) ( pnBuf + nIpHdrLen );

    if ( psIcmpHeader -> nType != ICMP_ECHOREPLY && psIcmpHeader -> nType != ICMP_ECHO ) {
        printf( "non-echo type %d recvd\n", psIcmpHeader -> nType );
        return;
    }

    if ( psIcmpHeader -> nId != ( uint16 ) 42 ) {
        printf( "someone else packet!\n" );
        return;
    }

    GetTimeOfDay( &sTv, NULL );

    psTp = ( struct sTimeVal * ) &psIcmpHeader -> nTimeStamp;
    //Print( "%ld %ld\n", psTp -> nSec, psTp -> nUsec );
    tvsub( &sTv, psTp );
    nTripTime = sTv.nSec * 1000 + ( sTv.nUsec / 1000 );
    psStat -> nTsum += nTripTime;
    if ( nTripTime < psStat -> nTmin ) {
        psStat -> nTmin = nTripTime;
    }
    if ( nTripTime > psStat -> nTmax ) {
        psStat -> nTmax = nTripTime;
    }
    psStat -> nRecieved++;

    nBytes -= nIpHdrLen + sizeof( struct sIcmpHeader );
    printf( "%d bytes from %s:", nBytes, inet_ntoa( psFrom -> sin_addr ) );
    printf( " icmp_seq=%d", psIcmpHeader -> nSeq );
    printf( " time=%d ms", nTripTime );
    printf( " TTL=%d", psIpHdr -> nTimeToLive );
    printf( "\n" );
}

uint16 checksum( uint16 *pnBuffer, sint32 nSize ) {
    uint32 nChkSum = 0;

    while ( nSize > 1 ) {
        nChkSum += *pnBuffer++;
        nSize -= sizeof( uint16 );
    }

    if ( nSize ) {
        nChkSum += *( uint8 * ) pnBuffer;
    }

    nChkSum = ( nChkSum >> 16 ) + ( nChkSum & 0xFFFF );
    nChkSum += ( nChkSum >> 16 );

    return ( uint16 ) ~nChkSum;
}

int main( int argc, char **argv ) {

    struct sockaddr_in sDest;
    struct sockaddr_in sFrom;

    uint8 *pnHostName;
    sint8 *pnDestIp;
    uint32 nAddr = INADDR_NONE;
    sint32 nSocketRaw;
    sint32 nDataSize;
    sint32 nNumPackets;
    sint32 nRc;
    sint32 nTimeOut = 1000;
    sint32 nFromLen = 1000;
    sint8 anIcmpData[ PACKET_SIZE ];
    sint8 anRecvBuf[ PACKET_SIZE ];
    struct sPingStat sStat;
    sint32 nWrote, nRead;
    struct sHostEnt *psHp = NULL;

    if ( argc < 2 ) {
        printf( "usage: %s <host> [data_size]\n", argv[ 0 ] );
        return 0;
    }

    memset( &sDest, 0, sizeof( sDest ) );

    pnHostName = argv[ 1 ];
    psHp = GetHostByName( (void *)pnHostName );
    if ( !psHp ) {
        nAddr = inet_addr( (void *)pnHostName );
    }

    if ( !psHp && nAddr == INADDR_NONE ) {
        printf( "%s: unknown host %s\n", argv[ 0 ], pnHostName );
        return 1;
    }

    if ( psHp != NULL ) {
        memcpy( &( sDest.sin_addr ), psHp -> ppnAddrList[ 0 ], psHp -> nLength );
        sDest.sin_family = psHp -> nAddrType;
    } else {
        sDest.sin_addr.s_addr = nAddr;
        sDest.sin_family = AF_INET;
    }

    sDest.sin_port = 1;
    pnDestIp = (void *)inet_ntoa( sDest.sin_addr );

    nDataSize = DEFAULT_PACKET_SIZE;

    if ( nDataSize + sizeof( struct sIcmpHeader ) > PACKET_SIZE ) {
        printf( "%s: packet size too large\n", argv[ 0 ] );
        return 1;
    }

    nSocketRaw = Socket( AF_INET, SOCK_RAW, IPPROTO_ICMP );
    if ( nSocketRaw < 0 ) {
        printf( "could not open raw socket\n" );
        return 1;
    }

    /*nRc = SetSocketOption( nSocketRaw, SOL_SOCKET, SO_RCVTIMEO, ( sint8 * ) &nTimeOut, sizeof( nTimeOut ) );
     if ( nRc < 0 ) {
     fprintf( stderr, "recv timeout" );
     return 1;
     }

     nTimeOut = 1000;
     nRc = SetSocketOption( nSocketRaw, SOL_SOCKET, SO_SNDTIMEO, ( sint8 * ) &nTimeOut, sizeof( nTimeOut ) );
     if ( nRc < 0 ) {
     fprintf( stderr, "send timeout" );
     return 1;
     }*/

    sStat.nTmin = 999999999;
    sStat.nTmax = 0;
    sStat.nTsum = 0;
    sStat.nTransmitted = 0;
    sStat.nRecieved = 0;

    memset( anIcmpData, 0, MAX_PACKET );
    fillIcmpData( anIcmpData, nDataSize + sizeof( struct sIcmpHeader ) );

    if ( sDest.sin_family == AF_INET ) {
        printf( "PING %s (%s): %d data bytes\n", pnHostName, inet_ntoa( sDest.sin_addr ), nDataSize );
    } else {
        printf( "PING %s: %d data bytes\n", pnHostName, nDataSize );
    }
    printf( "\n" );

    for ( nNumPackets = 0; nNumPackets < 5; nNumPackets++ ) {
        struct sIcmpHeader *psHeader = ( struct sIcmpHeader * ) anIcmpData;
        struct sTimeVal *psTp = ( struct sTimeVal * ) &anIcmpData[ 8 ];

        //ThreadSnooze( 100000 );
        sleepmsec(1000);


        psHeader -> nChkSum = 0;
        GetTimeOfDay( psTp, NULL );
        //Print( "%ld %ld\n", psTp -> nSec, psTp -> nUsec );
        psHeader -> nSeq = nSeqNo++;
        //printf( "nSeq: %d\n", psHeader -> nSeq );
        psHeader -> nChkSum = checksum( ( uint16 * ) anIcmpData, nDataSize + sizeof( struct sIcmpHeader ) );

        nWrote = SendTo( nSocketRaw, anIcmpData, nDataSize + sizeof( struct sIcmpHeader ), 0, ( struct sockaddr * ) &sDest, sizeof( sDest ) );
        if ( nWrote < 0 ) {
            printf( "time out?\n" );
            CloseSocket( nSocketRaw );
            return 1;
        }

        if ( nWrote < nDataSize ) {
            printf( "Wrote %d bytes\n", nWrote );
        }

        //fflush( stdout );
        sStat.nTransmitted++;

        memset( &sFrom, 0, sizeof( sFrom ) );
        nRead = RecvFrom( nSocketRaw, anRecvBuf, MAX_PACKET, 0, ( struct sockaddr * ) &sFrom, &nFromLen );
        if ( nRead < 0 ) {
            printf( "time out?\n" );
            CloseSocket( nSocketRaw );
            return 1;
        }

        decodeResp( anRecvBuf, nRead, &sFrom, &sStat );
        //ThreadSnooze( 100 );
        sleepmsec(100);
    }

    printf( "----%s PING Statistics----\n", pnHostName );
    printf( "%d packets transmitted, ", sStat.nTransmitted );
    printf( "%d packets recieved, ", sStat.nRecieved );
    if ( sStat.nTransmitted ) {
        if ( sStat.nRecieved > sStat.nTransmitted ) {
            printf( "-- error!" );
        } else {
            printf( "%d%% packet loss", ( sint32 ) ( ( ( sStat.nTransmitted - sStat.nRecieved ) * 100 ) / sStat.nTransmitted ) );
        }
        printf( "\n" );
    }

    if ( sStat.nRecieved ) {
        printf( "round-trip (ms)  min/avg/max = %d/%d/%d\n",
                sStat.nTmin, sStat.nTsum / sStat.nRecieved, sStat.nTmax );
    }

    Close( nSocketRaw );

    return 0;
}
