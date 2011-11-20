#warning unused
#if 0
#if HAVE_NET

/* Copyright 1999, Brian J. Swetland.  All Rights Reserved.
** This file is provided under the terms of the OpenBLT License
*/

#include "string.h"

#include <blt/namer.h>
#include <blt/syscall.h>
#include <blt/conio.h>
#include <blt/qsem.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "net.h"

#define NULL ((void *) 0)

#include "ne2k.h"

int find_pci(uint16 vendor, uint16 device, int *iobase, int *irq);

/* hard code these for now */
#define NIC_IRQ		5
#define NIC_ADDR	0x300

static snic TheSNIC;
int snic_irq = NIC_IRQ;
int snic_addr = NIC_ADDR;


static unsigned char prom[32];
static unsigned char IP[4] = { 10, 113, 216, 6 };	/* we get our ip from the booter */

/* messaging */
static int port_isr = 0;
static int port_xmit = 0;

static qsem_t *sem_xmit_done = NULL;
static int port_net = 0;

qsem_t *mutex = NULL;
qsem_t *sem_ring = NULL;

#define LOG_LEVEL 9999

/* to keep site's ne2000 code happy */
void kprintf(char *fmt,...)
{
}

#define trace(a, b) if (a >= LOG_LEVEL) printf("ne2000: %s\n", b);

#define RINGSIZE 16
#define PACKETSIZE 1536

typedef struct _pbuf {
    struct _pbuf *next;  /* 4 */
    packet_buffer pb;    /* 16 */
    packet_data pd;      /* 8 */
    int n;
} pbuf;

#define pb_to_ring(x) ((pbuf *) (((char *) (x)) - 4))
#define pd_to_ring(x) ((pbuf *) (((char *) (x)) - 20))
                       
pbuf *p_next = NULL;

typedef struct pmap pmap;

struct pmap {
    struct pmap *next;
    int udp_port;
    int blt_port;
};

pmap *pmaps = NULL;

pbuf *p_discard;

void init_ring(void)
{
    int i;
    pbuf *p;
    p_next = NULL;
    
    for(i=0;i<RINGSIZE;i++){
        p = (pbuf *) malloc(sizeof(pbuf));        
        p->next = p_next;
        p_next = p;
        p->pb.count = 1;
        p->pb.buf = &(p->pd);
        p->pd.ptr = (char *) malloc(PACKETSIZE);
        p->n = i;
    }

    p_discard = p_next;
    p_next = p_next->next;
}


/* called by the ne2k core to get a receive buffer */
packet_data *alloc_buffer_data(uint size)
{
    pbuf *P;

    qsem_acquire(sem_ring);
    if(!p_next){
        qsem_release(sem_ring);
        printf("ne2000: !!! out of packet ringbuffers (recv)\n");
        return NULL;
    }
    P = p_next;
    p_next = P->next;
    qsem_release(sem_ring);
    
    P->pd.len = size;
    return &(P->pd);
}


/* called by the us to sem_release a received buffer */
void free_buffer_data(packet_data *pd)
{
    pbuf *P = pd_to_ring(pd);
    qsem_acquire(sem_ring);
    P->next = p_next;
    p_next = P;
    qsem_release(sem_ring);
}


/* called by our send data routines to get a send buffer */
pbuf *get_pbuf(void)
{
    pbuf *P;
    qsem_acquire(sem_ring);
    if(!p_next){
        qsem_release(sem_ring);
        return p_discard;        
    }
    P = p_next;
    p_next = P->next;
    qsem_release(sem_ring);
    return P;
}

/* called after a pbuf is xmit'd */
void free_buffer(packet_buffer *ptr)
{
    pbuf *P = pb_to_ring(ptr);

	trace(1, "free_buffer");

    if(P == p_discard) return;

    qsem_acquire(sem_ring);
    P->next = p_next;
    p_next = P;
    qsem_release(sem_ring);

    qsem_release(sem_xmit_done);    
}

int ticks = 0;
void idle(void)
{
    int i;
    for(i=0;i<1000;i++);
    ticks++;
}

typedef struct 
{
    net_ether ether;
    net_arp arp;    
} arp_packet;

void print_arp(unsigned char *p);


void
xmit(pbuf *P)
{
    int i;
    msg_hdr_t mh;
        /* printf("xmit: P->n = %d\n",P->n);*/

    if(P == p_discard) return;
    
    mh.flags = 0;
    mh.src = port_isr;
    mh.dst = port_xmit;
    mh.size = 4;
    mh.data = &P;
    if((i = old_port_send(&mh)) != 4) printf("xmit: blargh! %d\n",i);
    
}

void handle_arp(arp_packet *req, pbuf *packet)
{
    if(htons(req->arp.arp_op) == ARP_OP_REQUEST){
        if(!memcmp(&(req->arp.arp_ip_target),IP,4)){
            pbuf *P;
            arp_packet *resp;
            P = get_pbuf();
            resp = (arp_packet *) P->pd.ptr;

/*            printf("handle_arp: IS the one!\n");*/
            memcpy(&(resp->ether.src),prom,6);
            memcpy(&(resp->ether.dst),&(req->ether.src),6);
            resp->ether.type = htons(0x0806);
            resp->arp.arp_hard_type = htons(1);
            resp->arp.arp_prot_type = htons(0x0800);
            resp->arp.arp_hard_size = 6;
            resp->arp.arp_prot_size = 4;            
            resp->arp.arp_op = htons(ARP_OP_REPLY);
            memcpy(&(resp->arp.arp_enet_sender),prom,6);
            memcpy(&(resp->arp.arp_ip_sender),IP,4);
            memcpy(&(resp->arp.arp_enet_target),&(req->arp.arp_enet_sender),6);
            memcpy(&(resp->arp.arp_ip_target),&(req->arp.arp_ip_sender),4);
            print_arp((unsigned char *) &(resp->arp));

            P->pd.len = sizeof(arp_packet);
            P->pb.len = sizeof(arp_packet);
            
            xmit(P);
        } else {
                /*printf("handle_arp: NOT the one.\n");            */
        }
    }
}

void print_arp(unsigned char *p)
{
    net_arp *arp = (net_arp *) p;
    unsigned char *b;    
    unsigned short t;
    
    printf("  ARP: ");
    t = htons(arp->arp_op);
    if(t == ARP_OP_REQUEST) printf("req ");
    else if(t == ARP_OP_REPLY) printf("rep ");
    else printf("??? ");

    b = (unsigned char *) &(arp->arp_enet_sender);
    printf("source:  %X:%X:%X:%X:%X:%X ",b[0],b[1],b[2],b[3],b[4],b[5]);
    b = (unsigned char *) &(arp->arp_ip_sender);
    printf("(%d.%d.%d.%d)\n",b[0],b[1],b[2],b[3]);

    printf("  ARP:     target:  ");    
    
    b = (unsigned char *) &(arp->arp_enet_target);
    printf("%X:%X:%X:%X:%X:%X ",b[0],b[1],b[2],b[3],b[4],b[5]);
    b = (unsigned char *) &(arp->arp_ip_target);
    printf("(%d.%d.%d.%d)\n",b[0],b[1],b[2],b[3]);    

}

void print_ip(unsigned char *p)
{
}

int ipchksum(void *_ip, int len)
{
    register unsigned short *ip = (unsigned short *) _ip;
    register unsigned long sum = 0;
    len >>= 1;
    while (len--) {
        sum += *(ip++);
        if (sum > 0xFFFF)
            sum -= 0xFFFF;
    }
    return((~sum) & 0x0000FFFF);
}


void
handle_icmp(icmp_packet *icmp)
{
    pbuf *P;
    icmp_packet *resp;

    if(icmp->icmp.icmp_type == ICMP_PING_REQ){
        P = get_pbuf();
        resp = (icmp_packet *) P->pd.ptr;
        
        memcpy(&(resp->ether.src),prom,6);
        memcpy(&(resp->ether.dst),&(icmp->ether.src),6);
        memcpy(&(resp->ether.type),&(icmp->ether.type),2);

        resp->ip.ip_hdr_len = 5;
        resp->ip.ip_version = 4;
        resp->ip.ip_tos = 0;
        resp->ip.ip_len = icmp->ip.ip_len;
        resp->ip.ip_id = 0;
        resp->ip.ip_off = 0;
        resp->ip.ip_ttl = 64;
        resp->ip.ip_proto = 0x01;
        resp->ip.ip_chk = 0;
	(int) resp->ip.ip_src = *(int *) IP;
        resp->ip.ip_dst = icmp->ip.ip_src;
        resp->ip.ip_chk = ipchksum(&(resp->ip),sizeof(net_ip));
        
        resp->icmp.icmp_type = ICMP_PING_REP;
        resp->icmp.icmp_code = 0;
        resp->icmp.icmp_chk = 0;
        resp->icmp.data.ping.id = icmp->icmp.data.ping.id;
        resp->icmp.data.ping.seq = icmp->icmp.data.ping.seq;
        memcpy(resp->icmp.data.ping.data,icmp->icmp.data.ping.data,
               ntohs(icmp->ip.ip_len) - 28);

        resp->icmp.icmp_chk = ipchksum(&(resp->ip),ntohs(resp->ip.ip_len));
/*        printf("ICMP resp l = %d\n",ntohs(resp->ip.ip_len));*/

        P->pb.len = P->pd.len = ntohs(resp->ip.ip_len)+14;

        xmit(P);
    }        
}


void
handle_udp(udp_packet *udp, pbuf *packet)
{
	pmap *m;
    msg_hdr_t msg;
    int i;
    
	for(m = pmaps; m; m = m->next){
		if(m->udp_port == ntohs(udp->udp.udp_dst)){
			msg.src = port_isr;
			msg.dst = m->blt_port;
			msg.flags = 0;
			msg.size = ntohs(udp->udp.udp_len) - 8;
			msg.data = udp->data;

			if((i = old_port_send(&msg)) <0) {	
				/* XXX: if resource is gone, tear down this mapping */
				/* printf("aieee %d / %d / %x\n",i,msg.size,msg.data);  */
			}
		}
	}
	
    
/*    printf("UDP %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d\n",
           src[0],src[1],src[2],src[3],ntohs(udp->udp.udp_src),
           dst[0],dst[1],dst[2],dst[3],ntohs(udp->udp.udp_dst)
           );*/
}


void
handle_tcp(tcp_packet *tcp, pbuf *packet)
{
		trace(2, "handle_tcp");
			/* full of worms */
}


void
handle_ip(ip_packet *ip, pbuf *packet)
{
    unsigned char *dst = (unsigned char *) &(ip->ip.ip_dst);
    
	trace(2, "handle_ip");

    if(!memcmp(dst,IP,4) || (dst[3] == 0xFF)){ /*!memcmp(dst,bcip,4)){*/
#ifdef DEBUG
      printf("IP %d.%d.%d.%d -> %d.%d.%d.%d\n",
      src[0],src[1],src[2],src[3],
           dst[0],dst[1],dst[2],dst[3]
           );
#endif
    
        switch(ip->ip.ip_proto){
        case 0x01:
            handle_icmp((icmp_packet *) ip);
            break;
        case 0x11:
            handle_udp((udp_packet *) ip, packet);
            break;
        case 0x6:
            handle_tcp((tcp_packet *) ip, packet);
            break;
        }
    }
}


void
receive(void *cb, packet_data *_packet)
{
    pbuf *packet = pd_to_ring(_packet);
    unsigned char *b = (unsigned char *) packet->pd.ptr;
            
trace(1, "receive");

    if(b[12] == 0x08){
        if(b[13] == 0x00) {
            handle_ip((ip_packet *) b, packet);
        } else if (b[13] == 0x06) {
            handle_arp((arp_packet *) b, packet);
        }
    }
    free_buffer_data(&(packet->pd));
}

#define NET_CONNECT  1
#define NET_SEND     2
#define NET_IP       3
typedef struct 
{
    int cmd; 
    int port;
    char data[0];
} net_cmd;


unsigned char bcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void send_udp(int src_port, int dst_port, unsigned char *ip, void *data, int size)
{
    pbuf *P;
    udp_packet *udp;

    P = get_pbuf();
    udp = (udp_packet *) P->pd.ptr;
    
    memcpy(&(udp->ether.src),prom,6);
    memcpy(&(udp->ether.dst),bcast,6);
    udp->ether.type = ntohs(0x0800);
    
    udp->ip.ip_hdr_len = 5;
    udp->ip.ip_version = 4;
    udp->ip.ip_tos = 0;
    udp->ip.ip_len = htons(20 + 8 + size);
    udp->ip.ip_id = 0;
    udp->ip.ip_off = 0;
    udp->ip.ip_ttl = 64;
    udp->ip.ip_proto = 17;
    udp->ip.ip_chk = 0;
    memcpy(&(udp->ip.ip_src),IP,4);
    memcpy(&(udp->ip.ip_dst),bcast,4);
    udp->ip.ip_chk = ipchksum(&(udp->ip),sizeof(net_ip));
    
    udp->udp.udp_src = htons(src_port);
    udp->udp.udp_dst = htons(dst_port);
    udp->udp.udp_len = htons(size + 8);
    udp->udp.udp_chk = 0;
    
    memcpy(udp->data, data, size);
    
/*    printf("sending UDP to %d / %d\n",port,size);*/
    P->pb.len = P->pd.len = 14 + 20 + 8 + size;
    
    xmit(P);
    
}

void
control(void)
{
    msg_hdr_t msg;
    char cbuf[1500];
    net_cmd *cmd = (net_cmd *) cbuf;
    int size;

    msg.flags=0;
    
    for(;;){
        msg.dst = port_net;
        msg.src = 0;
        msg.data = cbuf;
        msg.size = 1500;
        size = old_port_recv(&msg);

        if(size >= 8){
            switch(cmd->cmd){
            case NET_IP:
                memcpy(cbuf,IP,4);
                msg.size = 4;
                msg.data = cbuf;
                msg.dst = msg.src;
                msg.src = port_isr;
                old_port_send(&msg);
                break;
                
            case NET_CONNECT: {
				pmap *m = (pmap *) malloc(sizeof(pmap));
				if(m) {
					m->udp_port = cmd->port;
					m->blt_port = msg.src;
					m->next = pmaps;
					pmaps = m;
					printf("ne2000: routing udp:%d -> blt:%d\n",
						   cmd->port, msg.src);
				}                
                break;
			}	
            case NET_SEND: {
				pmap *m;
				for(m = pmaps; m; m = m->next){
					if(msg.src == m->blt_port) {
						send_udp(m->udp_port, cmd->port, NULL, cmd->data, size-8);
					}
				}
                break;
			}
			}
			
		}
    }

}

void
sender(void)
{
    msg_hdr_t mh;
    pbuf *packet;
    
    mh.flags = 0;
    mh.src = port_isr;
    mh.size = 4;
    mh.data = &packet;
    
    for(;;){
            /* wait for a send request */
        mh.dst = port_xmit;
        if(old_port_recv(&mh) == 4){
			trace(2, "sender: sending packet");
                /* send the packet */
            qsem_acquire(mutex);
            nic_send_packet(&TheSNIC, &(packet->pb));
            qsem_release(mutex);
            
            qsem_acquire(sem_xmit_done);
        }    
    }
}

void ISR(void)
{
    os_handle_irq(snic_irq);

    for(;;){
        os_sleep_irq();
        qsem_acquire(mutex);
        nic_isr(&TheSNIC);
        qsem_release(mutex);
    }
}

int main(int argc, char **argv)
{
    int i;
    
	if(argc == 5){
		for(i=0;i<4;i++){
			IP[i] = atoi(argv[i+1]);
		}
	}
		
	if(find_pci(0x10ec, 0x8029, &snic_addr, &snic_irq)){
		printf("ne2000: found PCI device\n");
	}
	
    os_brk(RINGSIZE*PACKETSIZE*2);

    sem_ring = qsem_create(1);
    mutex = qsem_create(1);

        /* create our send port */
    port_isr = port_create(0,"net_send_port");
    port_set_restrict(port_isr, port_isr);
	port_option(port_isr, PORT_OPT_NOWAIT, 1);

    namer_register(port_isr,"net_xmit");

    init_ring();
    TheSNIC.iobase = 0;
    nic_init(&TheSNIC, snic_addr, prom, NULL);

    printf("ne2000: irq %d @ 0x%S mac = %X:%X:%X:%X:%X:%X\n",
           snic_irq,snic_addr,prom[0],prom[1],prom[2],prom[3],prom[4],prom[5]);    

    nic_register_notify(&TheSNIC,receive,NULL);
	printf("ne2000: IP %d.%d.%d.%d\n",IP[0],IP[1],IP[2],IP[3]);
    
    printf("ne2000: starting sender, dispatcher, and control\n");

	namer_register(port_net = port_create(0,"net_listen_port"),"net");

    port_xmit = port_create(port_isr,"net_isr_port");
    sem_xmit_done = qsem_create(0);    
	
    os_thread(sender);
    os_thread(control);

    printf("ne2000: starting NIC\n");
    nic_start(&TheSNIC,0);
    printf("ne2000: \033[32mready.\033[37m\n");
    
	os_thread(ISR);
	
    return 0;
}


#endif // HAVE_NET

#endif
