#ifndef TRFS_H
#define TRFS_H

#include <phantom_types.h>
#include <queue.h>
#include <sys/cdefs.h>

#include "pager_io_req.h"

#define TRFS_PORT 2052

#define TRFS_MAX_PKT 10240

#define TRFS_SECTOR_SIZE 512

typedef struct {
    u_int32_t           fileId;
    u_int32_t           ioId;
    u_int32_t           nSectors;
    u_int64_t           startSector;
} __packed trfs_fio_t;


typedef struct {
    trfs_fio_t          fio;
    u_int32_t           type;
    u_int32_t           recombine_map;
    pager_io_request *  orig_request;

    queue_chain_t       chain;

    int                 resend_count;
    //bigtime_t         sent_time; // When did we sent it?
} trfs_queue_t;


#define TRFS_QEL_TYPE_READ      (1 << 0)
#define TRFS_QEL_TYPE_WRITE     (1 << 1)


// TODO add timing
#define TRFS_NEED_RESEND(elt)  1

struct trfs_pkt {
    u_int32_t           type;
    u_int64_t           sessionId;

    union {

        struct {
            u_int32_t           errorId;
            u_int32_t           textLen;
            char                text[];
        } error;

        struct {
            u_int32_t           fileNameLen;
            char                fileName[];
        } findRq;

        struct {
            u_int32_t           fileId;
        } findReply;

        struct {
            u_int32_t           nRequests;
            trfs_fio_t		request[1];
        } readRq;

        struct {
            trfs_fio_t		info;
            u_int8_t            data[];
        } readReply;

        struct {
            trfs_fio_t		info;
            u_int8_t            data[];
        } writeRq;

        struct {
            trfs_fio_t		info;
        } writeAck;

    };

} __packed;

typedef struct trfs_pkt trfs_pkt_t;


#define PKT_T_NOP  		0
#define PKT_T_Error 		1
	
#define PKT_T_FindRQ 		11
#define PKT_T_FindReply 	12
#define PKT_T_ReadRQ 		13
#define PKT_T_ReadReply 	14
#define PKT_T_WriteRQ 		15
#define PKT_T_WriteReply 	16



#endif // TRFS_H

