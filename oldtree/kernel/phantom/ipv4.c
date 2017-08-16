#include <kernel/config.h>

#if HAVE_NET

/*
 ** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <time.h>

#include <newos/nqueue.h>

#include <kernel/net_timer.h>

#include <kernel/net/arp.h>
#include <kernel/net/udp.h>
#include <kernel/net/tcp.h>

#include <kernel/net.h>

#include <arpa/inet.h>

#include <kernel/khash.h>
#include <kernel/net/ethernet.h>

#include <kernel/atomic.h>
#include <endian.h>
#include <hal.h>

#include "misc.h"


//#include <arpa/inet.h>


typedef struct ipv4_header {
    uint8 version_length;
    uint8 tos;
    uint16 total_length;
    uint16 identification;
    uint16 flags_frag_offset;
    uint8 ttl;
    uint8 protocol;
    uint16 header_checksum;
    ipv4_addr src;
    ipv4_addr dest;
} _PACKED ipv4_header;

#define IPV4_FLAG_MORE_FRAGS   0x2000
#define IPV4_FLAG_MAY_NOT_FRAG 0x4000
#define IPV4_FRAG_OFFSET_MASK  0x1fff

typedef struct ipv4_routing_entry {
    struct ipv4_routing_entry *next;
    ipv4_addr network_addr;
    ipv4_addr netmask;
    ipv4_addr gw_addr;
    ipv4_addr if_addr;
    if_id interface_id;
    int flags;
} ipv4_routing_entry;

#define ROUTE_FLAGS_GW 1

// routing table
static ipv4_routing_entry *route_table;
static hal_mutex_t route_table_mutex;

typedef struct ipv4_fragment {
    struct ipv4_fragment *hash_next;
    struct ipv4_fragment *frag_next;
    cbuf *buf;
    uint16 offset;
    uint16 len;
    bigtime_t entry_time;
    uint16 total_len;
    // copied from the header, enough data to uniquely identify the frag
    ipv4_addr src;
    ipv4_addr dest;
    uint16 identification;
    uint8 protocol;
} ipv4_fragment;

typedef struct ipv4_fragment_key {
    ipv4_addr src;
    ipv4_addr dest;
    uint16 identification;
    uint8 protocol;
} ipv4_fragment_key;

// current ip identification number
static uint32 curr_identification;

// fragment hash table
static void *frag_table;
static hal_mutex_t frag_table_mutex;

static net_timer_event frag_killer_event;


#define FRAG_KILLER_QUANTUM 5000 /* 5 secs */
#define MAX_FRAG_AGE 60000000 /* 1 min */

static int frag_compare_func(void *_frag, const void *_key)
{
    ipv4_fragment *frag = _frag;
    const ipv4_fragment_key *key = _key;

    if(frag->src == key->src && frag->dest == key->dest &&
       frag->identification == key->identification && frag->protocol == key->protocol) {
        return 0;
    } else {
        return -1;
    }
}

// XXX lameo hash
static unsigned int frag_hash_func(void *_frag, const void *_key, unsigned int range)
{
    ipv4_fragment *frag = _frag;
    const ipv4_fragment_key *key = _key;

    if(frag)
        return (frag->src ^ frag->dest ^ frag->protocol ^ frag->identification) % range;
    else
        return (key->src ^ key->dest ^ key->protocol ^ key->identification) % range;
}

// expects hosts order
void dump_ipv4_addr(ipv4_addr addr)
{
    uint8 *nuaddr = (uint8 *)&addr;

    dprintf("%d.%d.%d.%d", nuaddr[3], nuaddr[2], nuaddr[1], nuaddr[0]);
}

static void dump_ipv4_header(ipv4_header *head)
{
#if NET_CHATTY
    dprintf("ipv4 header: src ");
    dump_ipv4_addr(ntohl(head->src));
    dprintf(" dest ");
    dump_ipv4_addr(ntohl(head->dest));
    dprintf(" prot %d, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
            head->protocol, ntohs(head->header_checksum), ntohs(head->total_length), ntohs(head->identification), ntohs(head->flags_frag_offset) & 0x1fff);
#else
    (void) head;
#endif
}

static void ipv4_frag_killer(void *unused)
{
    (void) unused;

    struct hash_iterator i;
    ipv4_fragment *frag, *last;
    ipv4_fragment *free_list = NULL;
    bigtime_t now = system_time();

    set_net_timer(&frag_killer_event, FRAG_KILLER_QUANTUM, &ipv4_frag_killer, NULL, 0);

    mutex_lock(&frag_table_mutex);

    // cycle through the list, searching for a chain that's older than the max age
    hash_open(frag_table, &i);
    frag = hash_next(frag_table, &i);
    while(frag != NULL) {
        last = frag;
        frag = hash_next(frag_table, &i);

        // see if last is eligable for death
        if(now - last->entry_time > MAX_FRAG_AGE) {
            hash_remove(frag_table, last);
            last->hash_next = free_list;
            free_list = last;
        }
    }

    mutex_unlock(&frag_table_mutex);

    // erase the frags we scheduled to be killed
    while(free_list) {
        frag = free_list;
        free_list = frag->hash_next;

        // walk this frag chain
        while(frag) {
            last = frag;
            frag = frag->frag_next;

            // kill last
            cbuf_free_chain(last->buf);
            kfree(last);
        }
    }
}

static int ipv4_route_add_etc(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num, int flags, ipv4_addr gw_addr)
{
    ipv4_routing_entry *e;
    ipv4_routing_entry *temp;
    ipv4_routing_entry *last;

    // make sure the netmask makes sense
    if((netmask | (netmask - 1)) != 0xffffffff) {
        return ERR_INVALID_ARGS;
    }

    e = kmalloc(sizeof(ipv4_routing_entry));
    if(!e)
        return ERR_NO_MEMORY;

    e->network_addr = network_addr;
    e->netmask = netmask;
    e->gw_addr = gw_addr;
    e->if_addr = if_addr;
    e->interface_id = interface_num;
    e->flags = flags;

    mutex_lock(&route_table_mutex);

    // add it to the list, sorted by netmask 'completeness'
    last = NULL;
    for(temp = route_table; temp; temp = temp->next) {
        if((netmask | e->netmask) == e->netmask) {
            // insert our route entry here
            break;
        }
        last = temp;
    }
    if(last)
        last->next = e;
    else
        route_table = e;
    e->next = temp;

    mutex_unlock(&route_table_mutex);

    return NO_ERROR;
}

int ipv4_route_add(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num)
{
    return ipv4_route_add_etc(network_addr, netmask, if_addr, interface_num, 0, 0);
}

int ipv4_route_add_gateway(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num, ipv4_addr gw_addr)
{
    return ipv4_route_add_etc(network_addr, netmask, if_addr, interface_num, ROUTE_FLAGS_GW, gw_addr);
}

int ipv4_route_add_default(ipv4_addr if_addr, if_id interface_num, ipv4_addr gw_addr)
{
    ipv4_addr network_addr = 0;
    ipv4_addr netmask = 0;
    return ipv4_route_add_gateway(network_addr, netmask, if_addr, interface_num, gw_addr);
}

//! Remove all routes for given interface
errno_t ipv4_route_remove_iface(if_id interface_num)
{
    //ipv4_routing_entry *e;
    ipv4_routing_entry *temp;
    ipv4_routing_entry *last;

    mutex_lock(&route_table_mutex);

    // Process list head
    while( route_table && (route_table->interface_id == interface_num) )
    {
        temp = route_table->next;
        free( route_table );
        route_table = temp;
    }

    // Now in the middle?
    last = NULL;
    for(temp = route_table; temp; temp = temp->next) {

        if( temp->next && (temp->next->interface_id == interface_num) )
        {
            last = temp->next;
            temp->next = last->next;
            free(last);
        }
    }

    mutex_unlock(&route_table_mutex);

    return 0;
}

//! Dump all routes
void ipv4_route_dump(void)
{
    ipv4_routing_entry *temp;

    mutex_lock(&route_table_mutex);

    for(temp = route_table; temp; temp = temp->next)
    {
        printf("Route if %d flags %x\n", temp->interface_id, temp->flags );
        printf("\tif_addr %s, gw_addr %s\n",  __inet_itoa(htonl(temp->if_addr)), __inet_itoa(htonl(temp->gw_addr)) );
        printf("\tnet_addr %s, netmask %08X\n\n",  __inet_itoa(htonl(temp->network_addr)), temp->netmask );
    }

    mutex_unlock(&route_table_mutex);
}


static int ipv4_route_match(ipv4_addr ip_addr, if_id *interface_num, ipv4_addr *target_addr, ipv4_addr *if_addr)
{
    ipv4_routing_entry *e;
    ipv4_routing_entry *last_e = NULL;
    int err;

    // walk through the routing table, finding the last entry to match
    mutex_lock(&route_table_mutex);
    for(e = route_table; e; e = e->next) {
        ipv4_addr masked_addr = ip_addr & e->netmask;
        if(masked_addr == e->network_addr)
            last_e = e;
    }

    if(last_e) {
        *interface_num = last_e->interface_id;
        *if_addr = last_e->if_addr;
        if(last_e->flags & ROUTE_FLAGS_GW) {
            *target_addr = last_e->gw_addr;
        } else {
            *target_addr = ip_addr;
        }
        err = NO_ERROR;
    } else {
        *interface_num = -1;
        *target_addr = 0;
        *if_addr = 0;
        err = ERR_NET_NO_ROUTE;
    }
    mutex_unlock(&route_table_mutex);

    return err;
}

int ipv4_lookup_srcaddr_for_dest(ipv4_addr dest_addr, ipv4_addr *src_addr)
{
    if_id id;
    ipv4_addr target_addr;

    return ipv4_route_match(dest_addr, &id, &target_addr, src_addr);
}

int ipv4_get_mss_for_dest(ipv4_addr dest_addr, uint32 *mss)
{
    if_id id;
    ifnet *i;
    ipv4_addr target_addr;
    ipv4_addr src_addr;
    int err;

    err = ipv4_route_match(dest_addr, &id, &target_addr, &src_addr);
    if(err < 0)
        return err;

    i = if_id_to_ifnet(id);
    if(i == NULL)
        return ERR_NET_NO_ROUTE;

    *mss = i->mtu - sizeof(ipv4_header);

    return NO_ERROR;
}

static void ipv4_arp_callback(int arp_code, void *args, ifnet *i, netaddr *link_addr)
{
    cbuf *buf = args;

    if(arp_code == ARP_CALLBACK_CODE_OK) {
        // arp found us an address and called us back with it
        i->link_output(buf, i, link_addr, PROT_TYPE_IPV4);
    } else if(arp_code == ARP_CALLBACK_CODE_FAILED) {
        // arp retransmitted and failed, so we're screwed
        cbuf_free_chain(buf);
    } else {
        // uh
        ;
    }
}

int ipv4_output(cbuf *buf, ipv4_addr target_addr, int protocol)
{
    cbuf *header_buf;
    ipv4_header *header;
    netaddr link_addr;
    if_id iid;
    ifnet *i;
    ipv4_addr transmit_addr;
    ipv4_addr if_addr;
    int err;
    uint16 len;
    uint16 curr_offset;
    uint16 identification;
    bool must_frag = false;

#if NET_CHATTY
    dprintf("ipv4_output: buf %p, target_addr ", buf);
    dump_ipv4_addr(target_addr);
    dprintf(", protocol %d, len %d\n", protocol, cbuf_get_len(buf));
#endif

    // figure out what interface we will send this over
    err = ipv4_route_match(target_addr, &iid, &transmit_addr, &if_addr);
    if(err < 0) {
        cbuf_free_chain(buf);
        printf("ipv4 oom");
        return ERR_NO_MEMORY;
    }
    i = if_id_to_ifnet(iid);
    if(!i) {
        cbuf_free_chain(buf);
        printf("ipv4 oom");
        return ERR_NO_MEMORY;
    }

    // figure out the total len
    len = cbuf_get_len(buf);
    if(len + sizeof(ipv4_header) > i->mtu)
        must_frag = true;

    //	dprintf("did route match, result iid %d, i 0x%x, transmit_addr 0x%x, if_addr 0x%x\n", iid, i, transmit_addr, if_addr);

    identification = atomic_add((int32_t *)&curr_identification, 1);
    identification = htons(identification);

    curr_offset = 0;
    while(len > 0) {
        uint16 packet_len;
        uint16 header_len;
        cbuf *send_buf;

        header_len = sizeof(ipv4_header);
        header_buf = cbuf_get_chain(header_len);
        if(!header_buf) {
            cbuf_free_chain(buf);
            printf("ipv4 oom");
            return ERR_NO_MEMORY;
        }
        header = cbuf_get_ptr(header_buf, 0);

        packet_len = min(i->mtu, (unsigned)(len + header_len));
        if(packet_len == i->mtu)
            packet_len = ROUNDOWN(packet_len - header_len, 8) + header_len;

        header->version_length = 0x4 << 4 | 5;
        header->tos = 0;
        header->total_length = htons(packet_len);
        header->identification = identification;
        header->flags_frag_offset = htons((curr_offset / 8) & IPV4_FRAG_OFFSET_MASK);
        if(packet_len != len + header_len)
            header->flags_frag_offset |= htons(IPV4_FLAG_MORE_FRAGS);
        header->ttl = 255;
        header->protocol = protocol;
        header->header_checksum = 0;
        header->src = htonl(if_addr);
        header->dest = htonl(target_addr);

#if NET_CHATTY
        printf("ipv4_output src addr ");
        dump_ipv4_addr(if_addr);
        printf("\n");
#endif
        // calculate the checksum
        header->header_checksum = cksum16(header, (header->version_length & 0xf) * 4);

        if(must_frag) {
            send_buf = cbuf_duplicate_chain(buf, curr_offset, packet_len - header_len, 0);
            if(!send_buf) {
                cbuf_free_chain(header_buf);
                cbuf_free_chain(buf);
                printf("ipv4 oom");
                return ERR_NO_MEMORY;
            }
        } else {
            send_buf = buf;
        }
        send_buf = cbuf_merge_chains(header_buf, send_buf);

        // do the arp thang
        err = arp_lookup(i, if_addr, transmit_addr, &link_addr, &ipv4_arp_callback, send_buf);
        if(err == ERR_NET_ARP_QUEUED) {
            // the arp request is queued up so we can just exit here
            // and the rest of the work will be done via the arp callback
        } else if(err < 0) {
#if NET_CHATTY
            dprintf("ipv4_output: failed arp lookup\n");
#endif
            cbuf_free_chain(send_buf);
        } else {
            // we got the link layer address, send the packet
            i->link_output(send_buf, i, &link_addr, PROT_TYPE_IPV4);
        }

        // update the offset
        curr_offset += packet_len - header_len;
        len -= packet_len - header_len;
    }

    if(must_frag)
        cbuf_free_chain(buf);

    return err;
}

static ipv4_fragment *ipv4_create_frag_struct(ipv4_fragment_key *key, cbuf *buf, uint16 offset, uint16 len, bool last_frag)
{
    ipv4_fragment *frag;

    // create a new frag
    frag = kmalloc(sizeof(ipv4_fragment));
    if(!frag)
        return NULL;

    frag->hash_next = NULL;
    frag->frag_next = NULL;
    frag->buf = buf;
    frag->offset = offset;
    frag->len = len;
    frag->entry_time = system_time();
    frag->src = key->src;
    frag->dest = key->dest;
    frag->identification = key->identification;
    frag->protocol = key->protocol;

    // if this was the last frag, we now know the total len
    if(last_frag)
        frag->total_len = frag->offset + frag->len;
    else
        frag->total_len = 0;

    return frag;
}

static int ipv4_process_frag(cbuf *inbuf, ifnet *i, cbuf **outbuf)
{
    (void) i;

    int err;
    ipv4_header *header;
    ipv4_fragment_key key;
    ipv4_fragment *frag;
    ipv4_fragment *temp;
    ipv4_fragment *last;
    uint16 offset;
    uint16 len;
    bool last_frag;

    *outbuf = NULL;

#if NET_CHATTY
    dprintf("ipv4_process_frag: inbuf %p, i %p, outbuf %p\n", inbuf, i, outbuf);
#endif
    header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
    offset = (ntohs(header->flags_frag_offset) & IPV4_FRAG_OFFSET_MASK) * 8;
    len = ntohs(header->total_length) - ((header->version_length & 0xf) * 4);
    last_frag = (ntohs(header->flags_frag_offset) & IPV4_FLAG_MORE_FRAGS) ? false : true;

    // look in the hash table to see if there are any frags this will help complete
    key.src = ntohl(header->src);
    key.dest = ntohl(header->dest);
    key.identification = ntohs(header->identification);
    key.protocol = header->protocol;

#if NET_CHATTY
    dprintf("ipv4_process_frag frag: src 0x%x dest 0x%x ident %d prot %d offset %d len %d last_frag %d\n",
            key.src, key.dest, key.identification, key.protocol, offset, len, last_frag);
#endif

    mutex_lock(&frag_table_mutex);

    frag = hash_lookup(frag_table, &key);
    if(frag) {
        // this is part of an older frag
        bool bad_frag = false;
        bool found_spot = false;

        // find the spot where this frag would be in the frag list
        for(last = NULL, temp = frag; temp; last = temp, temp = temp->frag_next) {

#if NET_CHATTY
            dprintf("last %p, temp %p\n", last, temp);
            dprintf("bad_frag %d, found_spot %d\n", bad_frag, found_spot);
#endif

            // if we haven't already found a spot, look for it, and make sure
            // the new frag would insert properly (no cross-overs, etc)
            if(!found_spot) {
                // first, make sure it would insert into the list cleanly
                // see if this frag is the same as one we've received already
                if(last) {
                    if(offset < last->offset + last->len) {
                        bad_frag = true;
                        goto done_frag_spot_search;
                    }
                }
                if(temp->offset > offset && (offset + len > temp->offset)) {
                    bad_frag = true;
                    goto done_frag_spot_search;
                }

                // now, see if we can stop here
                if(last && offset > last->offset && offset < temp->offset) {
                    found_spot = true;
                } else if(offset < temp->offset) {
                    found_spot = true;
                }
            }
        }
        // if we still hadn't found a spot, do a last check to see if it'll tack on
        // to the end of the frag list properly
#if NET_CHATTY
        dprintf("out of loop: last %p, temp %p, found_spot %d, bad_frag %d\n", last, temp, found_spot, bad_frag);
#endif
        if(!found_spot) {
            if(offset < last->offset + last->len) {
                // crosses last in list
                bad_frag = true;
                goto done_frag_spot_search;
            }
            // see if it's valid to tack on to the end of the list
            if(frag->total_len > 0) {
                // this frag chain had already received the end of frag packet
                if(offset + len > frag->total_len) {
                    bad_frag = true;
                    goto done_frag_spot_search;
                }
            } else {
                if(last_frag) {
                    // we now know the full size of this fragment chain
                    frag->total_len = offset + len;
                }
            }
        }

    done_frag_spot_search: // dz We come here with (bad_frag==true) || (temp==null) only! All cases below which require (temp!=0) fail!
        if(bad_frag) {
            dprintf("ipv4_process_frag: received fragment is bad\n");
            cbuf_free_chain(inbuf);
            err = ERR_NET_BAD_PACKET;
            goto out;
        }

        // now we know we have the spot
        // see if we can merge it with one of the others

        // will it merge with the last one?
        if(last) {
            if(last->offset + last->len == offset) {
                // merge it
                inbuf = cbuf_truncate_head(inbuf, ((header->version_length & 0xf) * 4), 0);
                last->buf = cbuf_merge_chains(last->buf, inbuf);
                inbuf = last->buf;
                last->len += len;
                header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
                header->total_length = htons(ntohs(header->total_length) + len);

                // the free floating frag has been 'eaten'
                len = 0;
            }
        }
        // will it merge with the next one?
        if(len > 0 && temp) {
            if(offset + len == temp->offset) {
                // merge it
                ipv4_header *next_header = cbuf_get_ptr(temp->buf, 0);
                temp->buf = cbuf_truncate_head(temp->buf, ((next_header->version_length & 0xf) * 4), 0);
                temp->buf = cbuf_merge_chains(inbuf, temp->buf);
                inbuf = temp->buf;
                header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
                header->total_length = htons(ntohs(header->total_length) + temp->len);
                temp->len += len;
                temp->offset = offset;

                // the free floating frag has been 'eaten'
                len = 0;
            }
        }

        // see if last and next frag will merge
        if(last && temp) {
            if(last->offset + len == temp->offset) {
                // merge them
                ipv4_header *next_header = cbuf_get_ptr(temp->buf, 0);
                temp->buf = cbuf_truncate_head(temp->buf, ((next_header->version_length & 0xf) * 4), true);
                last->buf = cbuf_merge_chains(last->buf, temp->buf);
                inbuf = last->buf;
                header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
                header->total_length = htons(ntohs(header->total_length) + temp->len);
                last->len += temp->len;

                // delete the next frag structure
                last->frag_next = temp->frag_next;
                kfree(temp);
            }
        }

        // if we still have a free floating frag, create a new frag.
        // otherwise, see if we've completed a frag
        if(len > 0) {
            // create a new frag and put it in the list
            ipv4_fragment *newfrag;

            newfrag = ipv4_create_frag_struct(&key, inbuf, offset, len, last_frag);
            if(!frag) {
                cbuf_free_chain(inbuf);
                err = ERR_NO_MEMORY;
                goto out;
            }

            // are we at the beginning of the frag chain
            if(!last) {
                // we are, so this will be the new head of the list
                hash_remove(frag_table, frag);
                newfrag->frag_next = frag;
                newfrag->total_len = frag->total_len;
                hash_insert(frag_table, newfrag);
            } else {
                newfrag->frag_next = temp;
                last->frag_next = newfrag;
            }
        } else {
            // we ate the frag in some way, so see if we completed the list
            if(frag->offset == 0 && frag->total_len == frag->len) {
                // we have completed the frag
                hash_remove(frag_table, frag);
                *outbuf = frag->buf;
                if(frag->frag_next)
                    panic("ipv4_process_frag: found completed frag but still has a chain! frag %p\n", frag);
                kfree(frag);
            }
        }
    } else {
        // create a new frag
        frag = ipv4_create_frag_struct(&key, inbuf, offset, len, last_frag);
        if(!frag) {
            cbuf_free_chain(inbuf);
            err = ERR_NO_MEMORY;
            goto out;
        }

        // add it to the list
        hash_insert(frag_table, frag);
    }

    err= NO_ERROR;

out:
    mutex_unlock(&frag_table_mutex);

    return err;

}

int ipv4_input(cbuf *buf, ifnet *i)
{
    int err;
    ipv4_header *header;
    ipv4_addr src, dest;
    uint8 protocol;

    header = (ipv4_header *)cbuf_get_ptr(buf, 0);

    if(cbuf_get_len(buf) < 4) {
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }

    dump_ipv4_header(header);

    if(((header->version_length >> 4) & 0xf) != 4) {
        dprintf("ipv4 packet has bad version\n");
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }

    if(cbuf_get_len(buf) < sizeof(ipv4_header)) {
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }

    if(cksum16(header, (header->version_length & 0xf) * 4) != 0) {
        dprintf("ipv4 packet failed cksum\n");
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }

    // verify that this packet is for us
    if(ntohl(header->dest) != 0xffffffff) {
        ifaddr *iaddr;
        ipv4_addr dest = ntohl(header->dest);

        for(iaddr = i->addr_list; iaddr; iaddr = iaddr->next) {
            if(iaddr->addr.type == ADDR_TYPE_IP) {
                ipv4_addr idest = NETADDR_TO_IPV4(iaddr->addr);
                ipv4_addr bdest = NETADDR_TO_IPV4(iaddr->broadcast);

                    // see if it matches one of this interface's ip addresses
                if(dest == idest)
                    break;
                // see if it matches the broadcast address
                if(dest == bdest)
                    break;
            }
        }
        if(!iaddr) {
            dprintf("ipv4 packet for someone else: ");
            dump_ipv4_addr(dest);
            dprintf("\n");
            err = NO_ERROR;
            goto ditch_packet;
        }
    }

    // do some sanity checks and buffer trimming
    {
        size_t buf_len = cbuf_get_len(buf);
        uint16 packet_len = ntohs(header->total_length);

        // see if the packet is too short
        if(buf_len < packet_len) {
            dprintf("ipv4 packet too short (buf_len %ld, packet len %d)\n", buf_len, packet_len);
            err = ERR_NET_BAD_PACKET;
            goto ditch_packet;
        }

        // see if we need to trim off any padding
        if(buf_len > packet_len) {
            cbuf_truncate_tail(buf, buf_len - packet_len, true);
        }
    }

    // see if it's a fragment
    if(ntohs(header->flags_frag_offset) & IPV4_FLAG_MORE_FRAGS ||
       (ntohs(header->flags_frag_offset) & IPV4_FRAG_OFFSET_MASK) != 0) {
        cbuf *new_buf;

        err = ipv4_process_frag(buf, i, &new_buf);
        if(err < 0)
            goto ditch_packet;
        if(new_buf) {
            // it processed the frag, and built us a complete packet
            buf = new_buf;
            header = cbuf_get_ptr(buf, 0);
        } else {
            // it ate the frag, so we're done here
            err = NO_ERROR;
            goto out;
        }
    }

    // save some data
    protocol = header->protocol;
    src = ntohl(header->src);
    dest = ntohl(header->dest);

    // strip off the ip header
    buf = cbuf_truncate_head(buf, (header->version_length & 0xf) * 4, true);

    // demultiplex and hand to the proper module
    switch(protocol) {
    case IP_PROT_ICMP:  return icmp_input(buf, i, src);

    case IP_PROT_TCP:   return tcp_input(buf, i, src, dest);

    case IP_PROT_UDP:   return udp_input(buf, i, src, dest);
    default:
        dprintf("ipv4_receive: packet with unknown protocol (%d)\n", protocol);
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }
    /* unreachable */
    err = NO_ERROR;

ditch_packet:
    cbuf_free_chain(buf);
out:
    return err;
}

int ipv4_init(void)
{
    hal_mutex_init(&route_table_mutex, "ipv4 routing");
    hal_mutex_init(&frag_table_mutex, "ipv4 fragment");

    route_table = NULL;
    curr_identification = system_time();

    frag_table = hash_init(256, offsetof(ipv4_fragment, hash_next),
                           &frag_compare_func, &frag_hash_func);

    set_net_timer(&frag_killer_event, FRAG_KILLER_QUANTUM, &ipv4_frag_killer, NULL, 0);

    return 0;
}

/**
 *
 * Parse IP address string. If string is parsable, assign IP address to out.
 * Returns 0 if OK and EINVAL else.
 *
 **/

errno_t parse_ipv4_addr( ipv4_addr *out, const char *str )
{
    unsigned int a,b,c,d;

    if( str == 0 )
        return EINVAL;

    int n = sscanf( str, "%d.%d.%d.%d", &a, &b, &c, &d );

    if( n != 4 || a > 255 || b > 255 || c > 255 || d > 255 )
        return EINVAL;

    *out = IPV4_DOTADDR_TO_ADDR(a, b, c, d);
    return 0;
}



#endif // HAVE_NET

