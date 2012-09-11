/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * TCP impl private structs.
 *
**/



typedef struct tcp_header {
    uint16 source_port;
    uint16 dest_port;
    uint32 seq_num;
    uint32 ack_num;
    uint16 length_flags;
    uint16 win_size;
    uint16 checksum;
    uint16 urg_pointer;
} _PACKED tcp_header;

typedef struct tcp_pseudo_header {
    ipv4_addr source_addr;
    ipv4_addr dest_addr;
    uint8 zero;
    uint8 protocol;
    uint16 tcp_length;
} _PACKED tcp_pseudo_header;

typedef struct tcp_mss_option {
    uint8 kind; /* 0x2 */
    uint8 len;  /* 0x4 */
    uint16 mss;
} _PACKED tcp_mss_option;

typedef struct tcp_window_scale_option {
    uint8 kind; /* 0x3 */
    uint8 len;  /* 0x3 */
    uint8 shift_count;
} _PACKED tcp_window_scale_option;

typedef enum tcp_state {
    STATE_CLOSED,
    STATE_LISTEN,
    STATE_SYN_SENT,
    STATE_SYN_RCVD,
    STATE_ESTABLISHED,
    STATE_CLOSE_WAIT,
    STATE_LAST_ACK,
    STATE_CLOSING,
    STATE_FIN_WAIT_1,
    STATE_FIN_WAIT_2,
    STATE_TIME_WAIT
} tcp_state;

typedef enum tcp_flags {
    PKT_FIN = 1,
    PKT_SYN = 2,
    PKT_RST = 4,
    PKT_PSH = 8,
    PKT_ACK = 16,
    PKT_URG = 32
} tcp_flags;

typedef struct tcp_socket {
    queue_element accept_next; // must be first
    struct tcp_socket *next;
    tcp_state state;
    hal_mutex_t lock;
    int ref_count;
    int last_error;

    ipv4_addr local_addr;
    ipv4_addr remote_addr;
    uint16 local_port;
    uint16 remote_port;

    uint32 mss;

    /* rx */
    hal_sem_t read_sem;
    uint32 rx_win_size;
    uint32 rx_win_low;
    uint32 rx_win_high;
    cbuf *reassembly_q;
    cbuf *read_buffer;
    // FIXME
    net_timer_event ack_delay_timer;

    /* tx */
    hal_mutex_t write_lock;
    hal_sem_t write_sem;
    bool writers_waiting;
    uint32 tx_win_low;
    uint32 tx_win_high;
    uint32 retransmit_tx_seq;
    int retransmit_timeout;
    int tx_write_buf_size;
    uint32 unacked_data_len;
    int duplicate_ack_count;
    cbuf *write_buffer;


    net_timer_event retransmit_timer;
    net_timer_event persist_timer;
    net_timer_event fin_retransmit_timer;
    net_timer_event time_wait_timer;


    /* as per example in tcp/ip illustrated */
    uint32 cwnd;
    uint32 ssthresh;

    /* rtt */
    bool tracking_rtt;
    uint32 rtt_seq;
    bigtime_t rtt_seq_timestamp;
    long smoothed_rtt;
    long smoothed_deviation;
    long rto;

    /* accept queue */
    queue accept_queue;
    hal_sem_t accept_sem;
} tcp_socket;

typedef struct tcp_socket_key {
    ipv4_addr local_addr;
    ipv4_addr remote_addr;
    uint16 local_port;
    uint16 remote_port;
} tcp_socket_key;
