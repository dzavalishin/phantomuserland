/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * TCP.
 *
 * Based on NeOS code:
 * Copyright 2001-2004, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
 *
**/

#define NET_CHATTY 0

#include <kernel/config.h>
#include <kernel/stats.h>
#include <kernel/atomic.h>

#if HAVE_NET


#include <kernel/debug.h>

#include <phantom_libc.h>
#include <time.h>

#include <limits.h>

#include "khash.h"

#include <endian.h>
#include <hal.h>

#include <newos/nqueue.h>

#include <kernel/net/tcp.h>
#include <kernel/net/tcp_private.h>
#include <kernel/net_timer.h>

#include "misc.h"


#define DEBUG_REF_COUNT 0


static tcp_socket *socket_table;
static hal_mutex_t socket_table_lock;
static int next_ephemeral_port = 1024;

/* the following are in bigtime_t units (microseconds) */
#define SYN_RETRANSMIT_TIMEOUT 1000000
#define MAX_RETRANSMIT_TIMEOUT 90000000 /* 90 secs */

#define FIN_RETRANSMIT_TIMEOUT 3000000 /* 3 sec */
//#define FIN_RETRANSMIT_TIMEOUT 5000
#define PERSIST_TIMEOUT 500
#define ACK_DELAY 200
#define DEFAULT_RX_WINDOW_SIZE (32*1024)
#define DEFAULT_TX_WRITE_BUF_SIZE (128*1024)
#define DEFAULT_MAX_SEGMENT_SIZE 536
#define MSL 30000 /* 30 seconds */

#define SEQUENCE_GTE(a, b) ((int)((a) - (b)) >= 0)
#define SEQUENCE_LTE(a, b) ((int)((a) - (b)) <= 0)
#define SEQUENCE_GT(a, b) ((int)((a) - (b)) > 0)
#define SEQUENCE_LT(a, b) ((int)((a) - (b)) < 0)

// forward decls
static void tcp_send(ipv4_addr dest_addr, uint16 dest_port, ipv4_addr src_addr, uint16 source_port, cbuf *buf, tcp_flags flags,
                     uint32 ack, const void *options, uint16 options_length, uint32 sequence, uint16 window_size);
static void tcp_socket_send(tcp_socket *s, cbuf *data, tcp_flags flags, const void *options, uint16 options_length, uint32 sequence);
static void handle_ack(tcp_socket *s, uint32 sequence, uint32 window_size, bool with_data);
static void handle_data(tcp_socket *s, cbuf *buf);
static void handle_ack_delay_timeout(void *_socket);
static void handle_persist_timeout(void *_socket);
static void handle_retransmit_timeout(void *_socket);
static void handle_fin_retransmit(void *_socket);
static void handle_time_wait_timeout(void *_socket);
static int destroy_tcp_socket(tcp_socket *s);
static tcp_socket *create_tcp_socket(void);
static void send_ack(tcp_socket *s);
static void tcp_remote_close(tcp_socket *s);
static int tcp_flush_pending_data(tcp_socket *s);
static void tcp_retransmit(tcp_socket *s);

static int tcp_socket_compare_func(void *_s, const void *_key)
{
    tcp_socket *s = _s;
    const tcp_socket_key *key = _key;

    if(s->local_addr == key->local_addr &&
       s->remote_addr == key->remote_addr &&
       s->local_port == key->local_port &&
       s->remote_port == key->remote_port)
        return 0;
    else
        return 1;
}

static unsigned int tcp_socket_hash_func(void *_s, const void *_key, unsigned int range)
{
    tcp_socket *s = _s;
    const tcp_socket_key *key = _key;
    unsigned int hash;

    if(s) {
        hash = *(uint32 *)&s->local_addr ^ *(uint32 *)&s->remote_addr ^ s->local_port ^ s->remote_port;
#if NET_CHATTY
        dprintf("hash 4 sck: local port %d, rem port %d, local addr %x, rem addr %x\n",
                s->local_port, s->remote_port, *(uint32 *)&s->local_addr, *(uint32 *)&s->remote_addr
               );
#endif
    } else {
        hash = *(uint32 *)&key->local_addr ^ *(uint32 *)&key->remote_addr ^ key->local_port ^ key->remote_port;
#if NET_CHATTY
        dprintf("hash 4 key: local port %d, rem port %d, local addr %x, rem addr %x\n",
                key->local_port, key->remote_port, *(uint32 *)&key->local_addr, *(uint32 *)&key->remote_addr
               );
#endif
    }


    return hash % range;
}

#if DEBUG_REF_COUNT
#define inc_socket_ref(s) _inc_socket_ref(s, __FUNCTION__, __LINE__);
static void _inc_socket_ref(tcp_socket *s, const char *where, int line)
{
    if((addr_t)s < KERNEL_BASE)
        panic("inc_socket_ref: %s:%d s %p bad pointer\n", where, line, s);
    dprintf("inc_socket_ref: %s:%d s %p, %d -> %d\n", where, line, s, s->ref_count, s->ref_count+1);
#else
static void inc_socket_ref(tcp_socket *s)
{
#endif
    if(atomic_add(&s->ref_count, 1) <= 0)
        panic("inc_socket_ref: socket %p has bad ref %d\n", s, s->ref_count);
}

#if DEBUG_REF_COUNT
#define dec_socket_ref(s) _dec_socket_ref(s, __FUNCTION__, __LINE__);
static void _dec_socket_ref(tcp_socket *s, const char *where, int line)
{
    if((addr_t)s < KERNEL_BASE)
        panic("dec_socket_ref: %s:%d s %p bad pointer\n", where, line, s);
    dprintf("dec_socket_ref: %s:%d s %p, %d -> %d\n", where, line, s, s->ref_count, s->ref_count-1);
#else
static void dec_socket_ref(tcp_socket *s)
{
#endif
    if(atomic_add(&s->ref_count, -1) == 1) {
        // pull the socket out of the hash table
        mutex_lock(&socket_table_lock);
        hash_remove(socket_table, s);
        mutex_unlock(&socket_table_lock);

        destroy_tcp_socket(s);
    }
}

static tcp_socket *lookup_socket(ipv4_addr src_addr, ipv4_addr dest_addr, uint16 src_port, uint16 dest_port)
{
    tcp_socket_key key;
    tcp_socket *s;

    key.local_addr = dest_addr;
    key.local_port = dest_port;

    // first search for a socket matching the remote address
    key.remote_addr = src_addr;
    key.remote_port = src_port;

    mutex_lock(&socket_table_lock);

    s = hash_lookup(socket_table, &key);
    if(s)
        goto found;

    // didn't see it, lets search for the null remote address (a socket in listen state)
    key.remote_addr = 0;
    key.remote_port = 0;

    s = hash_lookup(socket_table, &key);
    if(s)
        goto found;

    // one last search for a socket with 0.0.0.0 as the local addr (will accept to any local address)
    key.local_addr = 0;

    s = hash_lookup(socket_table, &key);
    if(!s)
        goto out;

found:
    inc_socket_ref(s);
out:
    mutex_unlock(&socket_table_lock);
    return s;
}

static tcp_socket *create_tcp_socket(void)
{
    tcp_socket *s;

    s = kmalloc(sizeof(tcp_socket));
    if(!s)
        return NULL;

    memset(s, 0, sizeof(tcp_socket));

    // set up the new socket structure
    s->next = NULL;
    s->state = STATE_CLOSED;

    if(hal_mutex_init(&s->lock, "socket lock") )
        goto err;

    if( hal_sem_init( &s->read_sem, "socket read sem" ) )
        goto err1;

    if( hal_sem_init( &s->write_sem, "socket write sem" ) )
        goto err2;

    if(hal_mutex_init(&s->write_lock, "socket write lock") )
        goto err3;

    if( hal_sem_init( &s->accept_sem, "socket accept sem" ) )
        goto err4;

    s->ref_count = 1;
    s->local_addr = 0;
    s->local_port = 0;
    s->remote_addr = 0;
    s->remote_port = 0;
    s->mss = DEFAULT_MAX_SEGMENT_SIZE;
    s->rx_win_size = DEFAULT_RX_WINDOW_SIZE;
    s->rx_win_low = 0;
    s->rx_win_high = 0;
    s->tx_win_low = random() & 0xFF; // FIXME - range?
    s->tx_win_high = s->tx_win_low;
    s->retransmit_tx_seq = s->tx_win_low;
    s->tx_write_buf_size = DEFAULT_TX_WRITE_BUF_SIZE;
    s->write_buffer = NULL;
    s->writers_waiting = false;

    s->smoothed_deviation = 0;
    s->smoothed_rtt = 500;
    s->rto = 500;

    s->cwnd = s->mss;
    s->ssthresh = 0x10000;

    hal_queue_init(&s->accept_queue);

    return s;

err4:
    mutex_destroy(&s->write_lock);
err3:
    sem_delete(s->write_sem);
err2:
    sem_delete(s->read_sem);
err1:
    mutex_destroy(&s->lock);
err:
    kfree(s);
    return NULL;
}

static int destroy_tcp_socket(tcp_socket *s)
{
    //ASSERT(s->state == STATE_CLOSED);
    if(s->state != STATE_CLOSED)
        printf("!!! destroy_tcp_socket: state (%d) != STATE_CLOSED !!!\n", s->state);

    sem_delete(s->accept_sem);
    mutex_destroy(&s->write_lock);
    sem_delete(s->write_sem);
    sem_delete(s->read_sem);
    mutex_destroy(&s->lock);
    kfree(s);
    return 0;
}

static void dump_socket(tcp_socket *s)
{
    dprintf("tcp dump_socket on socket @ %p\n", s);
    dprintf("\tstate %d ref_count %d\n", s->state, s->ref_count);
    dprintf("\tlocal_addr: "); dump_ipv4_addr(s->local_addr); dprintf(".%d\n", s->local_port);
    dprintf("\tremote_addr: "); dump_ipv4_addr(s->remote_addr); dprintf(".%d\n", s->remote_port);
    dprintf("\tmss: %u\n", s->mss);
    dprintf("\tread_sem 0x%x\n", s->read_sem);
    dprintf("\trx_win_size %u rx_win_low %u rx_win_high %u\n", s->rx_win_size, s->rx_win_low, s->rx_win_high);
    dprintf("\tread_buffer %p (%ld)\n", s->read_buffer, cbuf_get_len(s->read_buffer));
    dprintf("\treassembly_q %p\n", s->reassembly_q);
    dprintf("\twrite_sem 0x%x writers_waiting %d\n", s->write_sem, s->writers_waiting);
    dprintf("\ttx_win_low %u tx_win_high %u retransmit_tx_seq %u write_buf_size %d\n",
            s->tx_win_low, s->tx_win_high, s->retransmit_tx_seq, s->tx_write_buf_size);
    dprintf("\tunacked_data_len %d write_buffer %p (%ld)\n", s->unacked_data_len, s->write_buffer, cbuf_get_len(s->write_buffer));
}


static void dump_socket_info(int argc, char **argv)
{
    if(argc < 2) {
        dprintf("tcp_socket: not enough arguments\n");
        return;
    }

    // if the argument looks like a hex number, treat it as such
    if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
        long num = atol(argv[1]);

        // TODO implement is_kernel_address(addr)
        //if(is_kernel_address(num))
        {
            // XXX semi-hack
            dump_socket((tcp_socket *)num);
            return;
        }
    }
}


static void list_tcp_sockets(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    struct hash_iterator i;
    tcp_socket *s;

    dprintf("tcp sockets:\n");
    hash_open(socket_table, &i);
    while((s = hash_next(socket_table, &i)) != NULL) {
        dprintf("\t%p\tlocal ", s);
        dump_ipv4_addr(s->local_addr); dprintf(".%d\t", s->local_port);
        dprintf("remote ");
        dump_ipv4_addr(s->remote_addr); dprintf(".%d\t", s->remote_port);
        dprintf("\n");
    }
}

static int bind_local_address(tcp_socket *s, netaddr *remote_addr)
{
    int err = 0;

    // find the local ip address to bind this socket to
    if(s->local_addr == 0) {
        err = ipv4_lookup_srcaddr_for_dest(NETADDR_TO_IPV4(*remote_addr), &s->local_addr);
        if(err < 0)
            return err;
    }

    // find a local port to bind this socket to
    // XXX hack hack hack
    if(s->local_port == 0) {
        s->local_port = atomic_add(&next_ephemeral_port, 1);
#if USHRT_MAX > 0x10000
        if(s->local_port >= 0x10000)
            s->local_port = 0;
#endif
    }

    return err;
}

int tcp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address)
{
    (void) i;


    tcp_header *header;
    int err = 0;
    int length = cbuf_get_len(buf);
    tcp_socket *s = NULL;
    uint8 packet_flags;
    uint16 header_len;
    uint16 data_len;
    uint32 highest_sequence;

    STAT_INC_CNT(STAT_CNT_TCP_RX);

    header = cbuf_get_ptr(buf, 0);
    header_len = ((ntohs(header->length_flags) >> 12) & 0x0f) * 4;

#if NET_CHATTY
    //dprintf("tcp_input: src port %d, dest port %d, buf len %d, checksum 0x%x, flags 0x%b\n",
    //        ntohs(header->source_port), ntohs(header->dest_port), (int)cbuf_get_len(buf), ntohs(header->checksum),
    //        ntohs(header->length_flags) & 0x3f, "\020\1FIN\2SYN\3RST\4PSH\5ACK\6URG");

    dprintf("tcp_input: src port %d, dest port %d, buf len %d, flags 0x%b seq %d ack %d\n",
            ntohs(header->source_port), ntohs(header->dest_port), (int)cbuf_get_len(buf),
            ntohs(header->length_flags) & 0x3f, "\020\1FIN\2SYN\3RST\4PSH\5ACK\6URG",
            ntohl(header->seq_num), ntohl(header->ack_num)
           );
#endif

    // check to see if the length looks correct
    if(header_len > cbuf_get_len(buf)) {
        // bogus packet length
        dprintf("tcp_input: received packet with bad length: header len %d, len %ld\n", header_len, cbuf_get_len(buf));
        goto ditch_packet;
    }

    // deal with the checksum check
    {
        tcp_pseudo_header pheader;
        uint16 checksum;

        // set up the pseudo header for checksum purposes
        pheader.source_addr = htonl(source_address);
        pheader.dest_addr = htonl(target_address);
        pheader.zero = 0;
        pheader.protocol = IP_PROT_TCP;
        pheader.tcp_length = htons(length);

        checksum = cbuf_ones_cksum16_2(buf, 0, cbuf_get_len(buf), &pheader, sizeof(pheader));
        if(checksum != 0) {
#if NET_CHATTY
            dprintf("tcp_receive: packet failed checksum\n");
#endif
            err = ERR_NET_BAD_PACKET;
            goto ditch_packet;
        }
    }

    // convert the tcp header data to host format
    header->source_port = ntohs(header->source_port);
    header->dest_port = ntohs(header->dest_port);
    header->seq_num = ntohl(header->seq_num);
    header->ack_num = ntohl(header->ack_num);
    header->length_flags = ntohs(header->length_flags);
    header->win_size = ntohs(header->win_size);
    header->urg_pointer = ntohs(header->urg_pointer);

    // get some data from the packet
    packet_flags = header->length_flags & 0x3f;
    data_len = cbuf_get_len(buf) - header_len;
    //was: highest_sequence = header->seq_num + (data_len > 0) ? (data_len - 1) : 0;
    highest_sequence = header->seq_num + ((data_len > 0) ? (data_len - 1) : 0);

    // see if it matches a socket we have
    s = lookup_socket(source_address, target_address, header->source_port, header->dest_port);
    if(!s) {
#if NET_CHATTY
        dprintf("socket not found\n");
#endif
        // send a RST packet
        goto send_reset;
    }

    // lock the socket
    mutex_lock(&s->lock);

    // see if the other side wants to reset the connection
    if(packet_flags & PKT_RST) {
        if(s->state != STATE_CLOSED && s->state != STATE_LISTEN) {
            tcp_remote_close(s);
        }
        goto ditch_packet;
    }

    // check for out of window packets
    if(!(packet_flags & PKT_SYN)) {
        if(SEQUENCE_LT(header->seq_num, s->rx_win_low)
           || (SEQUENCE_GT(header->seq_num, s->rx_win_high)
               && (!((data_len == 0) && (header->seq_num != s->rx_win_high + 1))))) {
            /* out of window, ack it */
            dprintf("tcp_input: out of win ack, seq %d, win hi %d\n", header->seq_num, s->rx_win_high );
            send_ack(s);
            goto ditch_packet;
        }
    }

#if NET_CHATTY
    const char *state_name = "?";
    switch(s->state) {
    case STATE_CLOSED:                  state_name = "closed";          break;
    case STATE_SYN_SENT:                state_name = "syn_sent";        break;
    case STATE_ESTABLISHED:             state_name = "establ";          break;
    case STATE_CLOSE_WAIT:              state_name = "close_wait";      break;
    case STATE_LAST_ACK:                state_name = "last_ack";        break;
    case STATE_FIN_WAIT_1:              state_name = "fin_wait_1";      break;
    case STATE_FIN_WAIT_2:              state_name = "fin_wait_2";      break;
    case STATE_CLOSING:                 state_name = "closing";         break;
    case STATE_LISTEN:                  state_name = "listen";          break;
    case STATE_SYN_RCVD:                state_name = "syn_rcvd";        break;
    case STATE_TIME_WAIT:               state_name = "time_wait";       break;
    }

    dprintf("tcp_input: socket %p, state 0x%x (%s)\n", s, s->state, state_name);
#endif

    switch(s->state) {
    case STATE_CLOSED:
        // socket is closed, send RST packet
        goto send_reset;
    case STATE_SYN_SENT:
#if NET_CHATTY
        dprintf("tcp_input: in STATE_SYN_SENT s->tx_win_low=%d\n", s->tx_win_low );
#endif

        s->tx_win_low++;
        s->retransmit_tx_seq = s->tx_win_low;
        s->tx_win_high = s->tx_win_low + header->win_size;
        if(packet_flags & PKT_SYN) {
            s->rx_win_low = header->seq_num + 1;
            s->rx_win_high = s->rx_win_low + s->rx_win_size - 1;
            if(packet_flags & PKT_ACK) {
                // they're acking our SYN
                if(header->ack_num != s->tx_win_low)
                    goto send_reset;

                tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
                s->state = STATE_ESTABLISHED;
                //hal_sem_release(s->read_sem, 1);
                hal_sem_release(&s->read_sem);
            } else {
                // simultaneous open
                // XXX handle
                goto send_reset;
            }
        } else {
            s->state = STATE_CLOSED;
            goto ditch_packet;
        }
        break;
    case STATE_ESTABLISHED: {
        if(packet_flags & PKT_ACK)
            handle_ack(s, header->ack_num, header->win_size, data_len > 0);

        if(data_len > 0) {
            handle_data(s, buf);
            buf = NULL; // handle_data will deal with the buffer from now on
        }

        if((packet_flags & PKT_FIN) && SEQUENCE_GT(s->rx_win_low, highest_sequence)) {
            // someone wants to close with us, and there's no outstanding data

            // FIN consumed a sequence
            s->rx_win_low++;

            // send an ack and transition to a new state
            send_ack(s);
            s->state = STATE_CLOSE_WAIT;

            // wake up any readers
            //hal_sem_release(s->read_sem, 1);
            hal_sem_release(&s->read_sem);
        }
        break;
    }
    case STATE_CLOSE_WAIT:
        if(packet_flags & PKT_FIN) {
            // send another ACK, the original FIN must have been lost
            send_ack(s);
        }
        break;
    case STATE_LAST_ACK:
        if(packet_flags & PKT_ACK) {
            // XXX may be technically bad, since there could be pending data still unacked
            tcp_remote_close(s);
        }
        break;
    case STATE_FIN_WAIT_1:
        if((packet_flags & (PKT_ACK|PKT_FIN)) == (PKT_ACK|PKT_FIN)
           && header->ack_num == (s->tx_win_low + 1)) {
            // they've acked us and piggybacked a FIN on the same packet
            // send an ack and transition directly to TIME_WAIT

            // FIN consumes a sequence
            s->rx_win_low ++;
            // consume our FIN sequence
            s->tx_win_low++;

            s->state = STATE_TIME_WAIT;
            tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
            if(set_net_timer(&s->time_wait_timer, 2*MSL, &handle_time_wait_timeout, s, 0) >= 0)
                inc_socket_ref(s);
        } else if(packet_flags & PKT_ACK
                  && header->ack_num == (s->tx_win_low + 1)) {
            // they've acked us, we need to wait for their FIN

            // consume our FIN sequence
            s->tx_win_low++;
            s->state = STATE_FIN_WAIT_2;
        } else if(packet_flags & PKT_FIN) {
            // simultaneous close

            // FIN consumes a sequence
            s->rx_win_low ++;
            // consume our FIN sequence
            s->tx_win_low++;

            s->state = STATE_CLOSING;
            tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
        } else if(packet_flags & PKT_ACK)
            handle_ack(s, header->ack_num, header->win_size, data_len > 0);
        break;
    case STATE_FIN_WAIT_2:
        if(packet_flags & PKT_FIN) {
            // FIN consumes a sequence
            s->rx_win_low ++;

            s->state = STATE_TIME_WAIT;
            tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
            if(set_net_timer(&s->time_wait_timer, 2*MSL, &handle_time_wait_timeout, s, 0) >= 0)
                inc_socket_ref(s);
        }
        break;
    case STATE_CLOSING:
        if(packet_flags & PKT_ACK) {
            // XXX do we need to make sure it's a valid ack?
            s->state = STATE_TIME_WAIT;
            tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
            if(set_net_timer(&s->time_wait_timer, 2*MSL, &handle_time_wait_timeout, s, 0) >= 0)
                inc_socket_ref(s);
        }
        break;

        /* passive open states */
    case STATE_LISTEN: {
        tcp_socket *accept_socket;
        tcp_mss_option mss_option;

        if(!(packet_flags & PKT_SYN)) {
            // didn't have a SYN flag, send a reset
            goto send_reset;
        }

        // packet had an ack flag, create a new socket
        accept_socket = create_tcp_socket();
        if(!accept_socket)
            break;

        // set up the address
        ipv4_lookup_srcaddr_for_dest(source_address, &accept_socket->local_addr);
        accept_socket->local_port = s->local_port;
        accept_socket->remote_addr = source_address;
        accept_socket->remote_port = header->source_port;

        // put it in the right state
        accept_socket->state = STATE_SYN_RCVD;
#if NET_CHATTY
        dprintf("insert accepted socket\n");
#endif
        // add it to the hash table
        mutex_lock(&socket_table_lock);
        hash_insert(socket_table, accept_socket);
        mutex_unlock(&socket_table_lock);

        // add it to the accept queue
        queue_enqueue(&s->accept_queue, accept_socket);
        //hal_sem_release(s->accept_sem, 1);
        hal_sem_release(&s->accept_sem);

        // record their sequence
        accept_socket->rx_win_low = header->seq_num + 1;
        accept_socket->rx_win_high = accept_socket->rx_win_low + accept_socket->rx_win_size - 1;

        // [dz] fix accept bug
        //accept_socket->tx_win_low = header->seq_num + 1;
        accept_socket->tx_win_low = header->seq_num;
        accept_socket->tx_win_high = accept_socket->tx_win_low;

        // figure out what the mss will be
        err = ipv4_get_mss_for_dest(accept_socket->remote_addr, &s->mss);
        if(err < 0)
            accept_socket->mss = DEFAULT_MAX_SEGMENT_SIZE;

        accept_socket->mss -= sizeof(tcp_header);
        accept_socket->cwnd = accept_socket->mss;

        // set up the mss option
        mss_option.kind = 0x2;
        mss_option.len = 0x4;
        mss_option.mss = htons(s->mss);

        // grab a lock on the new socket and send an ack to the syn
        inc_socket_ref(accept_socket);
        mutex_lock(&accept_socket->lock);
        tcp_socket_send(accept_socket, NULL, PKT_ACK|PKT_SYN, &mss_option, sizeof(mss_option), accept_socket->tx_win_low);
        mutex_unlock(&accept_socket->lock);
        dec_socket_ref(accept_socket);

        break;
    }
    case STATE_SYN_RCVD: {
        if(packet_flags & PKT_SYN) {
            // they must have not received our ack to their syn, retransmit
            // XXX implement
            goto send_reset;
            break;
        }

        // see if they're acking our SYN
        if((packet_flags & PKT_ACK)) {
            if(header->ack_num != s->tx_win_low + 1)
                goto send_reset;
            s->tx_win_low++;
            s->retransmit_tx_seq = s->tx_win_low;
            s->tx_win_high = s->tx_win_low + header->win_size;

            s->state = STATE_ESTABLISHED;
            //hal_sem_release(s->read_sem, 1);
            hal_sem_release(&s->read_sem);
        } else {
            goto send_reset;
        }

        break;
    }
    case STATE_TIME_WAIT:
    default:
        dprintf("tcp_receive: incoming packet on socket with unhandled state %d\n", s->state);
        goto ditch_packet;
    }

    err = NO_ERROR;
    goto ditch_packet;

send_reset:
    if(!(packet_flags & PKT_RST))
        tcp_send(source_address, header->source_port, target_address, header->dest_port,
                 NULL, PKT_RST|PKT_ACK, header->seq_num + 1, NULL, 0, header->ack_num, 0);
ditch_packet:
    cbuf_free_chain(buf);
    if(s) {
        ASSERT_LOCKED_MUTEX(&s->lock);
        mutex_unlock(&s->lock);
        dec_socket_ref(s);
    }

    return err;
}

int tcp_open(void **prot_data)
{
    tcp_socket *s;

    s = create_tcp_socket();
    if(!s)
        return ERR_NO_MEMORY;

    *prot_data = s;

    return NO_ERROR;
}

int tcp_bind(void *prot_data, i4sockaddr *addr)
{
    tcp_socket *s = prot_data;
    int err = 0;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    if(s->local_port != 0 || s->local_addr != 0) {
        err = ERR_NET_SOCKET_ALREADY_BOUND;
        goto out;
    }

    mutex_lock(&socket_table_lock);
    hash_remove(socket_table, s);

    // XXX check to see if this address is used or makes sense
    s->local_port = addr->port;
    s->local_addr = NETADDR_TO_IPV4(addr->addr);

    hash_insert(socket_table, s);
    mutex_unlock(&socket_table_lock);

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);

    return err;
}

int tcp_getpeername(void *prot_data, i4sockaddr *addr)
{
    tcp_socket *s = prot_data;
    int err = 0;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    if(s->remote_port == 0
       || s->remote_addr == 0
       || s->state != STATE_ESTABLISHED)
    {
        err = -1;
        goto out;
    }

    addr->port = s->remote_port;
    NETADDR_TO_IPV4(addr->addr) = s->local_addr;

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);

    return err;
}


int tcp_connect(void *prot_data, i4sockaddr *addr)
{
    tcp_socket *s = prot_data;
    int err;
    int i;
    tcp_mss_option mss_option;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    // see if the socket can be connected
    if(s->state > STATE_CLOSED) {
        err = ERR_NET_ALREADY_CONNECTED;
        goto out;
    }

    // pull the socket out of the hash table
    mutex_lock(&socket_table_lock);
    hash_remove(socket_table, s);
    mutex_unlock(&socket_table_lock);

    // allocate a local address, if needed
    if(s->local_port == 0 || s->local_addr == 0) {
        err = bind_local_address(s, &addr->addr);
        if(err < 0)
            goto out;
    }

    s->remote_addr = NETADDR_TO_IPV4(addr->addr);
    s->remote_port = addr->port;

    mutex_lock(&socket_table_lock);
    hash_insert(socket_table, s);
    mutex_unlock(&socket_table_lock);

    // figure out what the mss will be
    err = ipv4_get_mss_for_dest(s->remote_addr, &s->mss);
    if(err < 0)
        s->mss = DEFAULT_MAX_SEGMENT_SIZE;

    s->mss -= sizeof(tcp_header);
    s->cwnd = s->mss;

    // set up the mss option
    mss_option.kind = 0x2;
    mss_option.len = 0x4;
    mss_option.mss = htons(s->mss);

    // welcome to the machine
    s->state = STATE_SYN_SENT;
    for(i=0; i < 3 && s->state != STATE_ESTABLISHED && s->state != STATE_CLOSED; i++) {
        if(s->state == STATE_SYN_SENT)
            tcp_socket_send(s, NULL, PKT_SYN, &mss_option, sizeof(mss_option), s->tx_win_low);
        mutex_unlock(&s->lock);
        //hal_sem_acquire_etc(s->read_sem, 1, SEM_FLAG_TIMEOUT, SYN_RETRANSMIT_TIMEOUT, NULL);
        hal_sem_acquire_etc(&s->read_sem, 1, SEM_FLAG_TIMEOUT, SYN_RETRANSMIT_TIMEOUT );
        mutex_lock(&s->lock);
    }

    if(s->state == STATE_CLOSED) {
        err = ERR_NET_CONNECTION_REFUSED;
        goto out;
    }

    err = NO_ERROR;

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);

    return err;
}

int tcp_listen(void *prot_data)
{
    tcp_socket *s = prot_data;
    int err;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    if(s->state != STATE_CLOSED) {
        err = ERR_NET_ALREADY_CONNECTED;
        goto out;
    }

    // trasition into listen state
    s->state = STATE_LISTEN;

    err = NO_ERROR;

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);

    return err;
}

int tcp_accept(void *prot_data, i4sockaddr *saddr, void **_new_socket)
{
    tcp_socket *s = prot_data;
    tcp_socket *new_socket;
    int err;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

retry:
    if(s->state != STATE_LISTEN) {
        err = ERR_NET_NOT_LISTENING;
        goto out;
    }

    // wait on the accept queue
    mutex_unlock(&s->lock);
    //hal_sem_acquire(s->accept_sem, 1);
    hal_sem_acquire(&s->accept_sem);
    mutex_lock(&s->lock);

    // see if this socket is still valid
    if(s->state != STATE_LISTEN) {
        err = s->last_error; // it was probably closed
        goto out;
    }

    // pull something from the head of the accept queue
    new_socket = queue_dequeue(&s->accept_queue);
    ASSERT(new_socket != NULL);
    ASSERT(new_socket->ref_count > 0);

    // we have the new socket, make sure it's ready to go
    mutex_unlock(&s->lock);
    //hal_sem_acquire(new_socket->read_sem, 1);
    hal_sem_acquire(&new_socket->read_sem);
    mutex_lock(&new_socket->lock);

    if(new_socket->state != STATE_ESTABLISHED) {
        // this socket didn't make it
        mutex_unlock(&new_socket->lock);
        mutex_lock(&s->lock);
        goto retry;
    }

    // copy the address out
    if(saddr) {
        saddr->addr.len = 4;
        saddr->addr.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(saddr->addr) = s->remote_addr;
        saddr->port = s->remote_port;
    }

    mutex_unlock(&new_socket->lock);
    mutex_lock(&s->lock);

    // this is the new socket
    *_new_socket = (void *)new_socket;

    err = NO_ERROR;

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);

    return err;
}

int tcp_close(void *prot_data)
{
    tcp_socket *s = prot_data;
    int err;

    mutex_lock(&s->lock);

    // handle some special cases
    switch(s->state) {
    case STATE_ESTABLISHED:
        tcp_socket_send(s, NULL, PKT_FIN|PKT_ACK, NULL, 0, s->tx_win_low);

        if(set_net_timer(&s->fin_retransmit_timer, FIN_RETRANSMIT_TIMEOUT, &handle_fin_retransmit, s, 0) >= 0)
            inc_socket_ref(s);
        s->state = STATE_FIN_WAIT_1;
        break;
    case STATE_CLOSE_WAIT:
        tcp_socket_send(s, NULL, PKT_FIN|PKT_ACK, NULL, 0, s->tx_win_low);

        if(set_net_timer(&s->fin_retransmit_timer, FIN_RETRANSMIT_TIMEOUT, &handle_fin_retransmit, s, 0) >= 0)
            inc_socket_ref(s);
        s->state = STATE_LAST_ACK;
        break;
    case STATE_SYN_SENT:
    case STATE_LISTEN:
        s->state = STATE_CLOSED;
        break;
    default:
        // close not supported in this state
        err = ERR_NET_NOT_CONNECTED;
        inc_socket_ref(s);
        goto out;
    }

    // wake up anyone that may be blocked on this socket
    hal_sem_release(&s->accept_sem);
    hal_sem_release(&s->read_sem);
    hal_sem_release(&s->write_sem);
    s->writers_waiting = false;

    err = NO_ERROR;

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
    return err;
}

ssize_t tcp_recvfrom(void *prot_data, void *buf, ssize_t len, i4sockaddr *saddr, int flags, bigtime_t timeout)
{
    (void) saddr;

    tcp_socket *s = prot_data;
    int err;
    ssize_t bytes_read = 0;
    int avail;
    ssize_t to_copy;
    uint32 new_rx_win_size;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    if(s->state != STATE_ESTABLISHED) {
        bytes_read = 0;
        goto out;
    }

    /* wait for the buffer to have something in it, or timeout */
    while(s->state == STATE_ESTABLISHED && s->read_buffer == NULL) {
        mutex_unlock(&s->lock);
        if(flags & SOCK_FLAG_TIMEOUT)
            err = hal_sem_acquire_etc(&s->read_sem, 1, SEM_FLAG_TIMEOUT|SEM_FLAG_INTERRUPTABLE, timeout );
        else
            err = hal_sem_acquire_etc(&s->read_sem, 1, SEM_FLAG_INTERRUPTABLE, 0 );
        mutex_lock(&s->lock);
        if(err) {
            if(s->last_error)
                bytes_read = s->last_error;
            else
                bytes_read = -err;
            goto out;
        }
    }

    if(s->state != STATE_ESTABLISHED) {
        bytes_read = s->last_error;
        goto out;
    }

    /* copy as much data as we can */
    avail = cbuf_get_len(s->read_buffer);
    to_copy = min(avail, len);
    err = cbuf_memcpy_from_chain(buf, s->read_buffer, 0, to_copy);
    if(err < 0) {
        bytes_read = err;
        goto out;
    }

    /* truncate the read buffer */
    s->read_buffer = cbuf_truncate_head(s->read_buffer, to_copy, true);
    if(cbuf_get_len(s->read_buffer) == 0) {
        cbuf_free_chain(s->read_buffer);
        s->read_buffer = NULL;
    }
    bytes_read = to_copy;

    /* update the receive window */
    new_rx_win_size = s->rx_win_size - cbuf_get_len(s->read_buffer);

    /* see if the window is opening, and needs an ack to be sent */
    if(new_rx_win_size >= s->mss && s->rx_win_high - s->rx_win_low < s->mss)
        send_ack(s);

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
    return bytes_read;
}

ssize_t tcp_sendto(void *prot_data, const void *_inbuf, ssize_t len, i4sockaddr *toaddr)
{
    (void) toaddr;

    tcp_socket *s = prot_data;
    const uint8 *inbuf = _inbuf;
    ssize_t sent = 0;
    int err;

    inc_socket_ref(s);
    mutex_lock(&s->lock);

    if(s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT) {
        sent = 0;
        goto out;
    }

    while(sent < len) {
        int buf_size;
        int chunk_size;

        if(s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT) {
            sent = s->last_error;
            goto out;
        }

        // figure out how much of this buffer we can add to the transmit queue
        buf_size = cbuf_get_len(s->write_buffer);
        chunk_size = min(len - sent, s->tx_write_buf_size - buf_size);
        if(chunk_size == 0) {
            // wait for some space to free
            ASSERT(s->write_buffer != NULL);
            s->writers_waiting = true;
            mutex_unlock(&s->lock);
            //hal_sem_acquire(&s->write_sem, 1);
            hal_sem_acquire(&s->write_sem);
            mutex_lock(&s->lock);
            continue;
        }

        if(s->write_buffer != NULL) {
            err = cbuf_extend_tail(s->write_buffer, chunk_size);
            if(err < 0) {
                sent = err;
                goto out;
            }

            err = cbuf_memcpy_to_chain(s->write_buffer, buf_size, inbuf, chunk_size);
            if(err < 0) {
                cbuf_truncate_tail(s->write_buffer, chunk_size, true);
                sent = err;
                goto out;
            }
        } else {
            // the write buffer is null, create a new one
            s->write_buffer = cbuf_get_chain(chunk_size);
            if(s->write_buffer == NULL) {
                sent = ERR_NO_MEMORY;
                goto out;
            }

            err = cbuf_memcpy_to_chain(s->write_buffer, 0, inbuf, chunk_size);
            if(err < 0) {
                cbuf_free_chain(s->write_buffer);
                s->write_buffer = NULL;
                sent = err;
                goto out;
            }
        }

        sent += chunk_size;
        inbuf += chunk_size;

        // XXX do nagle or something
        tcp_flush_pending_data(s);
    }

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
    return sent;
}

static void send_ack(tcp_socket *s)
{
    ASSERT_LOCKED_MUTEX(&s->lock);

    if(cancel_net_timer(&s->ack_delay_timer) >= 0)
        dec_socket_ref(s);

    if(s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT)
        return;

    // XXX handle sending any pending data here, have the ack piggyback that
    if(tcp_flush_pending_data(s) == 0)
        tcp_socket_send(s, NULL, PKT_ACK, NULL, 0, s->tx_win_low);
}


static void handle_ack(tcp_socket *s, uint32 sequence, uint32 window_size, bool with_data)
{
    bool wake_writers = false;
    uint32 ack_len = 0;

    ASSERT_LOCKED_MUTEX(&s->lock);

    //	dprintf("handle_ack: sequence %d window_size %d with_data %d\n", sequence, window_size, with_data);
    //	dprintf("\tretransmit_tx_seq %d tx_win_low %d tx_win_high %d tx_write_buf_size %d\n",
    //		s->retransmit_tx_seq, s->tx_win_low, s->tx_win_high, s->tx_write_buf_size);

    if(sequence == s->retransmit_tx_seq
       && sequence + window_size == s->tx_win_high
       && !with_data) {
        // the other side is telling us it got a packet out of date, do fast retransmit
        if(++s->duplicate_ack_count == 3) {
            s->retransmit_timeout = s->rto;
            if(set_net_timer(&s->retransmit_timer, s->retransmit_timeout, &handle_retransmit_timeout, s, 0) >= 0)
                inc_socket_ref(s);

            tcp_retransmit(s);
            s->duplicate_ack_count = 0;

            // save 1/2 of the congestion window into ssthresh
            s->ssthresh = min(s->mss * 2, s->cwnd / 2);

            return;
        }
    }
    s->duplicate_ack_count = 0;

    if(SEQUENCE_GTE(sequence, s->retransmit_tx_seq)) {
        if(s->tracking_rtt && SEQUENCE_GTE(sequence, s->rtt_seq)) {
            // this sequence acked the data we are tracking to recalc rtt
            // as per Jacobson
            long Err = ((long)(system_time() - s->rtt_seq_timestamp) / 1000) - s->smoothed_rtt;
            s->smoothed_rtt = s->smoothed_rtt + Err / 8;
            s->smoothed_deviation = s->smoothed_deviation + ((Err < 0 ? -Err : Err) - s->smoothed_deviation) / 4;
            s->rto = s->smoothed_rtt + 4 * s->smoothed_deviation;
            s->tracking_rtt = false;
        }


        if(SEQUENCE_GT(sequence, s->retransmit_tx_seq)) {
            if(!s->write_buffer) {
                dprintf("tcp: data was acked that we didn't send\n");
                goto out;
            }

            ASSERT(cbuf_get_len(s->write_buffer) >= s->unacked_data_len);

            // remove acked data from the transmit queue
            ack_len = sequence - s->retransmit_tx_seq;
            if(ack_len == 0)
                goto out;
            if(ack_len > s->unacked_data_len)
                ack_len = s->unacked_data_len;
            s->write_buffer = cbuf_truncate_head(s->write_buffer, ack_len, true);
            s->unacked_data_len -= ack_len;

            s->retransmit_tx_seq = sequence;

            // reset the retransmit timer
            if(cancel_net_timer(&s->retransmit_timer) >= 0)
                dec_socket_ref(s);
            if(s->unacked_data_len > 0) {
                s->retransmit_timeout = s->rto;
                set_net_timer(&s->retransmit_timer, s->retransmit_timeout, &handle_retransmit_timeout, s, 0);
                inc_socket_ref(s);
            }

            // see if we need to wake up any writers
            if(s->writers_waiting) {
                if(s->write_buffer == NULL || cbuf_get_len(s->write_buffer) < s->tx_write_buf_size - s->mss) {
                    s->writers_waiting = false;
                    wake_writers = true;
                }
            }

            // open the congestion window
            if(s->cwnd <= s->ssthresh) {
                // doing slow start, increment the window by a mss per ack received
                s->cwnd += s->mss;
            } else {
                // congestion avoidance, increment the congestion window by 1/cwnd
                s->cwnd += s->mss * s->mss / s->cwnd;
            }
            // XXX need to clamp to max observed window
            if(s->cwnd > 0xffff)
                s->cwnd = 0xffff;
        }
    }

out:
    s->tx_win_high += ack_len;
    tcp_flush_pending_data(s);
    if(wake_writers)
    {
        //hal_sem_release(s->write_sem, 1);
        hal_sem_release(&s->write_sem);
    }
}

static void handle_ack_delay_timeout(void *_socket)
{
    tcp_socket *s = (tcp_socket *)_socket;

    mutex_lock(&s->lock);
    send_ack(s);
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
}

static void handle_persist_timeout(void *_socket)
{
    tcp_socket *s = (tcp_socket *)_socket;

    //	dprintf("handle_persist_timeout: entry\n");

    mutex_lock(&s->lock);

    if(s->write_buffer != NULL
       && s->unacked_data_len < cbuf_get_len(s->write_buffer)
       && s->state == STATE_ESTABLISHED) {

        // reset this timer
        if(set_net_timer(&s->persist_timer, PERSIST_TIMEOUT, &handle_persist_timeout, s, 0) >= 0)
            inc_socket_ref(s);

        if(tcp_flush_pending_data(s) == 0) {
            // we've flushed everything, send one byte past the end of the window
            cbuf *data = cbuf_duplicate_chain(s->write_buffer, s->unacked_data_len, 1, 0);
            if(data == NULL)
                goto out;
            tcp_socket_send(s, data, PKT_PSH | PKT_ACK, NULL, 0, s->tx_win_low);
        }
    }

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
}

static void handle_retransmit_timeout(void *_socket)
{
    tcp_socket *s = (tcp_socket *)_socket;

    mutex_lock(&s->lock);

    // XXX check here to see if we've retransmitted too many times

    tcp_retransmit(s);

    // save 1/2 of the congestion window into ssthresh
    s->ssthresh = max(s->mss * 2, s->cwnd / 2);
    s->cwnd = s->mss; // slow start

    // exponentially backoff the retransmit timeout
    s->retransmit_timeout *= 2;
    if(s->retransmit_timeout > MAX_RETRANSMIT_TIMEOUT)
        s->retransmit_timeout = MAX_RETRANSMIT_TIMEOUT;

    if(set_net_timer(&s->retransmit_timer, s->retransmit_timeout, &handle_retransmit_timeout, s, NET_TIMER_PENDING_IGNORE) >= 0)
        inc_socket_ref(s);

    mutex_unlock(&s->lock);
    dec_socket_ref(s);
}

static void handle_fin_retransmit(void *_socket)
{
    tcp_socket *s = (tcp_socket *)_socket;

    mutex_lock(&s->lock);

    if(s->state != STATE_LAST_ACK && s->state != STATE_FIN_WAIT_1 && s->state != STATE_CLOSING)
        goto out;

    if(set_net_timer(&s->fin_retransmit_timer, FIN_RETRANSMIT_TIMEOUT, &handle_fin_retransmit, s, NET_TIMER_PENDING_IGNORE) >= 0)
        inc_socket_ref(s);

    tcp_socket_send(s, NULL, PKT_FIN | PKT_ACK, NULL, 0, s->tx_win_low);

out:
    mutex_unlock(&s->lock);
    dec_socket_ref(s);
}

static void handle_time_wait_timeout(void *_socket)
{
    tcp_socket *s = (tcp_socket *)_socket;

    mutex_lock(&s->lock);

    ASSERT(s->state == STATE_TIME_WAIT);

    // killing it
    s->state = STATE_CLOSED;

    mutex_unlock(&s->lock);
    dec_socket_ref(s);
}

static void handle_data(tcp_socket *s, cbuf *buf)
{
    tcp_header header;
    int header_length;
    uint32 seq_low, seq_high;

    ASSERT_LOCKED_MUTEX(&s->lock);

    // copy the header
    memcpy(&header, cbuf_get_ptr(buf, 0), sizeof(header));
    header_length = ((header.length_flags >> 12) % 0xf) * 4;
    seq_low = header.seq_num;
    seq_high = seq_low + cbuf_get_len(buf) - header_length - 1;

    if(SEQUENCE_LTE(seq_low, s->rx_win_low) && SEQUENCE_GTE(seq_high, s->rx_win_low)) {
        // it's in order, so truncate from the head and add to the receive buffer
        buf = cbuf_truncate_head(buf, header_length + (s->rx_win_low - seq_low), true);
        s->rx_win_low += cbuf_get_len(buf);
        s->read_buffer = cbuf_merge_chains(s->read_buffer, buf);

        //hal_sem_release(s->read_sem, 1);
        hal_sem_release(&s->read_sem);

        // see if any reassembly packets can now be dealt with
        while(s->reassembly_q) {
            tcp_header *q_header = (tcp_header *)cbuf_get_ptr(s->reassembly_q, 0);
            int packet_header_len = ((q_header->length_flags >> 12) % 0xf) * 4;
            uint32 packet_low = q_header->seq_num;
            uint32 packet_high = packet_low + cbuf_get_len(s->reassembly_q) - packet_header_len;

            if(SEQUENCE_LT(packet_high, s->rx_win_low)) {
                /* this packet is totally out of window */
                cbuf *tmp = s->reassembly_q;
                s->reassembly_q = tmp->packet_next;
                cbuf_free_chain(tmp);
            } else if(SEQUENCE_LTE(packet_low, s->rx_win_low)) {
                /* a portion of this packet may be useful now */
                cbuf *tmp = s->reassembly_q;
                s->reassembly_q = tmp->packet_next;

                tmp = cbuf_truncate_head(tmp, packet_header_len + (s->rx_win_low - packet_low), true);
                s->rx_win_low += cbuf_get_len(tmp);

                /* merge it with the read data */
                if(s->read_buffer)
                    s->read_buffer = cbuf_merge_chains(s->read_buffer, tmp);
                else
                    s->read_buffer = tmp;
            } else {
                break;
            }
        }

        // set up a delayed ack
        if((int)(s->rx_win_low + s->rx_win_size - s->rx_win_high) > (int)s->rx_win_size / 2) {
            send_ack(s);
        } else if(set_net_timer(&s->ack_delay_timer, ACK_DELAY, handle_ack_delay_timeout, s, NET_TIMER_PENDING_IGNORE) >= 0) {
            // a delayed ack timeout was set
            inc_socket_ref(s);
        }
    } else {
        // packet is out of order, stick it on the reassembly queue
        if(s->reassembly_q == NULL ||
           SEQUENCE_GT(((tcp_header *)cbuf_get_ptr(s->reassembly_q, 0))->seq_num, seq_low)) {
            // stick it on the head of the queue
            buf->packet_next = s->reassembly_q;
            s->reassembly_q = buf;
        } else {
            // find the spot in the queue where we need to stick it
            cbuf *last = s->reassembly_q;

            for(; last; last = last->packet_next) {
                cbuf *next = last->packet_next;
                if(next == NULL || SEQUENCE_GT(((tcp_header *)cbuf_get_ptr(next, 0))->seq_num, seq_low)) {
                    // we found a spot
                    buf->packet_next = next;
                    last->packet_next = buf;
                    break;
                }
            }
        }

        send_ack(s);
    }
}

static void tcp_retransmit(tcp_socket *s)
{
    cbuf *retransmit_data;
    int retransmit_len;
    tcp_flags flags = PKT_PSH | PKT_ACK;

    ASSERT_LOCKED_MUTEX(&s->lock);

    if((s->state != STATE_ESTABLISHED && s->state != STATE_FIN_WAIT_1)
       || s->unacked_data_len == 0)
        return;

    // slice off some data to retransmit
    retransmit_len = min(s->unacked_data_len, s->mss);
    retransmit_data = cbuf_duplicate_chain(s->write_buffer, 0, retransmit_len, 0);
    tcp_socket_send(s, retransmit_data, flags, NULL, 0, s->retransmit_tx_seq);

    if(s->tracking_rtt) {
        if(SEQUENCE_LTE(s->retransmit_tx_seq, s->rtt_seq)
           && SEQUENCE_GTE(s->retransmit_tx_seq + retransmit_len, s->rtt_seq)) {
            // Karn sez dont follow this sequence when calculating rtt
            s->tracking_rtt = false;
        }
    }
}

static int tcp_flush_pending_data(tcp_socket *s)
{
    int data_flushed = 0;

    ASSERT_LOCKED_MUTEX(&s->lock);

    //	dprintf("tcp_flush_pending_data: write_buffer len %d, unacked_data_len %d, tx_win_low %d\n",
    //		cbuf_get_len(s->write_buffer), s->unacked_data_len, s->tx_win_low);

    while(s->write_buffer != NULL
          && s->unacked_data_len < cbuf_get_len(s->write_buffer)
          && s->unacked_data_len < s->cwnd
          && s->state == STATE_ESTABLISHED) {
        size_t send_len;
        cbuf *packet;

        ASSERT(s->tx_win_high >= s->tx_win_low);
        ASSERT(s->cwnd >= s->unacked_data_len);
        send_len = min(min(s->mss, s->tx_win_high - s->tx_win_low), s->cwnd - s->unacked_data_len);

        // XXX take care of silly window

        // see if we have anything to send
        if(send_len == 0) {
            if(s->unacked_data_len == 0) {
                // the other side's rx window is closed, set the persist timer
                if(set_net_timer(&s->persist_timer, PERSIST_TIMEOUT, &handle_persist_timeout, s, NET_TIMER_PENDING_IGNORE) >= 0)
                    inc_socket_ref(s);
            }
            break;
        }

        // cancel the persist timer, since we're gonna send something
        if(cancel_net_timer(&s->persist_timer) >= 0)
            dec_socket_ref(s);

        send_len = min(send_len, cbuf_get_len(s->write_buffer) - s->unacked_data_len);

        packet = cbuf_duplicate_chain(s->write_buffer, s->unacked_data_len, send_len, 0);
        if(!packet)
            return data_flushed;

        s->unacked_data_len += send_len;
        ASSERT(s->unacked_data_len <= cbuf_get_len(s->write_buffer));
        s->tx_win_low += send_len;
        if(s->tx_win_low > s->tx_win_high)
            dump_socket(s);
        ASSERT(s->tx_win_low <= s->tx_win_high);
        data_flushed += send_len;
        if(!s->tracking_rtt) {
            // track this packet
            s->tracking_rtt = true;
            s->rtt_seq = s->tx_win_low - send_len;
            s->rtt_seq_timestamp = system_time();
        }
        tcp_socket_send(s, packet, PKT_ACK, NULL, 0, s->tx_win_low - send_len);
        s->retransmit_timeout = s->rto;
        if(cancel_net_timer(&s->retransmit_timer) >= 0)
            dec_socket_ref(s);
        if(set_net_timer(&s->retransmit_timer, s->retransmit_timeout, &handle_retransmit_timeout, s, 0) >= 0)
            inc_socket_ref(s);
    }

    return data_flushed;
}

static void tcp_send(ipv4_addr dest_addr, uint16 dest_port, ipv4_addr src_addr, uint16 source_port, cbuf *buf, tcp_flags flags,
                     uint32 ack, const void *options, uint16 options_length, uint32 sequence, uint16 window_size)
{
    tcp_pseudo_header pheader;
    tcp_header *header;
    cbuf *header_buf;

    STAT_INC_CNT(STAT_CNT_TCP_TX);

#if NET_CHATTY
    dprintf("tcp_send: src port %d, dest port %d, buf len %d, flags 0x%b seq %d ack %d\n",
            source_port, dest_port, (int)cbuf_get_len(buf),
            flags, "\020\1FIN\2SYN\3RST\4PSH\5ACK\6URG",
            sequence, ack
           );
#endif

    // grab a buf large enough to hold the header + options
    header_buf = cbuf_get_chain(sizeof(tcp_header) + options_length);
    if(!header_buf)
        goto error;

    header = (tcp_header *)cbuf_get_ptr(header_buf, 0);
    header->ack_num = htonl(ack);
    header->dest_port = htons(dest_port);
    header->length_flags = htons(((sizeof(tcp_header) + options_length) / 4) << 12 | flags);
    header->seq_num = htonl(sequence);
    header->source_port = htons(source_port);
    header->urg_pointer = 0;
    if(options)
        memcpy(header + 1, options, options_length);
    header->win_size = htons(window_size);

    header_buf = cbuf_merge_chains(header_buf, buf);

    // checksum
    pheader.source_addr = htonl(src_addr);
    pheader.dest_addr = htonl(dest_addr);
    pheader.zero = 0;
    pheader.protocol = IP_PROT_TCP;
    pheader.tcp_length = htons(cbuf_get_len(header_buf));

    header->checksum = 0;
    header->checksum = cbuf_ones_cksum16_2(header_buf, 0, cbuf_get_len(header_buf), &pheader, sizeof(pheader));

    ipv4_output(header_buf, dest_addr, IP_PROT_TCP);
    return;

error:
    cbuf_free_chain(buf);
}

static void tcp_socket_send(tcp_socket *s, cbuf *data, tcp_flags flags, const void *options, uint16 options_length, uint32 sequence)
{
    uint32 rx_win_high;
    uint16 win_size;

    ASSERT_LOCKED_MUTEX(&s->lock);

    // calculate the new right edge of the rx window
    rx_win_high = s->rx_win_low + s->rx_win_size - cbuf_get_len(s->read_buffer) - 1;

#if NET_CHATTY
    //dprintf("** s->rx_win_low %u s->rx_win_size %ud read_buf_len %d, new win high %ud\n", s->rx_win_low, s->rx_win_size, cbuf_get_len(s->read_buffer), rx_win_high);
#endif
    if(SEQUENCE_GTE(rx_win_high, s->rx_win_high)) {
        s->rx_win_high = rx_win_high;
        win_size = rx_win_high - s->rx_win_low;
    } else {
        // the window size has shrunk, but we can't move the
        // right edge of the window backwards
        win_size = s->rx_win_high - s->rx_win_low;
    }

    // we are piggybacking a pending ACK, so clear the delayed ACK timer
    if(flags & PKT_ACK) {
        if(cancel_net_timer(&s->ack_delay_timer) == 0)
            dec_socket_ref(s);
    }

    mutex_unlock(&s->lock);
    tcp_send(s->remote_addr, s->remote_port, s->local_addr, s->local_port, data, flags, s->rx_win_low,
             options, options_length, sequence, win_size);
    mutex_lock(&s->lock);
}

static void tcp_remote_close(tcp_socket *s)
{

    ASSERT_LOCKED_MUTEX(&s->lock);

    if(s->state == STATE_CLOSED)
        return;

    inc_socket_ref(s);
    mutex_unlock(&s->lock);

    // pull the socket out of the hash table
    mutex_lock(&socket_table_lock);
    hash_remove(socket_table, s);
    mutex_unlock(&socket_table_lock);

    mutex_lock(&s->lock);

    s->last_error = ERR_NET_REMOTE_CLOSE;
    s->state = STATE_CLOSED;

    dec_socket_ref(s);
}

int tcp_init(void)
{
    hal_mutex_init(&socket_table_lock, "tcp socket table lock");

    socket_table = hash_init(256, offsetof(tcp_socket, next), &tcp_socket_compare_func, &tcp_socket_hash_func);
    if(!socket_table)
        return ERR_NO_MEMORY;

    next_ephemeral_port = random() % 32000 + 1024;

    dbg_add_command(&dump_socket_info, "tcp_socket", "dump info about socket at address");
    dbg_add_command(&list_tcp_sockets, "tcp_sockets", "list all active tcp sockets");

    return 0;
}

#endif
