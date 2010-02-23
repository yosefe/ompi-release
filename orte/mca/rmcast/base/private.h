/*
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/** @file:
 */

#ifndef ORTE_MCA_RMCAST_PRIVATE_H
#define ORTE_MCA_RMCAST_PRIVATE_H

/*
 * includes
 */
#include "orte_config.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "opal/event/event.h"
#include "opal/class/opal_list.h"
#include "opal/class/opal_ring_buffer.h"

#include "orte/mca/rmcast/rmcast.h"

BEGIN_C_DECLS

#if !ORTE_DISABLE_FULL_SUPPORT

#define CLOSE_THE_SOCKET(socket)    \
    do {                            \
        shutdown(socket, 2);        \
        close(socket);              \
    } while(0)



/****    CLASS DEFINITIONS    ****/
/*
 * Data structure for tracking assigned channels
 */
typedef struct {
    opal_list_item_t item;
    char *name;
    orte_rmcast_channel_t channel;
    uint32_t network;
    uint16_t port;
    uint32_t interface;
    int xmit;
    orte_rmcast_seq_t seq_num;
    int recv;
    struct sockaddr_in addr;
    opal_event_t send_ev;
    opal_mutex_t send_lock;
    bool sends_in_progress;
    opal_list_t pending_sends;
    uint8_t *send_data;
    opal_event_t recv_ev;
    /* ring buffer to cache our messages */
    opal_ring_buffer_t cache;
} rmcast_base_channel_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(rmcast_base_channel_t);


/*
 * Data structure for tracking registered non-blocking recvs
 */
typedef struct {
    opal_list_item_t item;
    orte_process_name_t name;
    orte_rmcast_channel_t channel;
    bool recvd;
    bool iovecs_requested;
    orte_rmcast_tag_t tag;
    orte_rmcast_flag_t flags;
    struct iovec *iovec_array;
    int iovec_count;
    opal_buffer_t *buf;
    orte_rmcast_callback_fn_t cbfunc_iovec;
    orte_rmcast_callback_buffer_fn_t cbfunc_buffer;
    void *cbdata;
} rmcast_base_recv_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(rmcast_base_recv_t);


/*
 * Data structure for tracking pending sends
 */
typedef struct {
    opal_list_item_t item;
    bool retransmit;
    struct iovec *iovec_array;
    int32_t iovec_count;
    opal_buffer_t *buf;
    orte_rmcast_tag_t tag;
    orte_rmcast_callback_fn_t cbfunc_iovec;
    orte_rmcast_callback_buffer_fn_t cbfunc_buffer;
    void *cbdata;
} rmcast_base_send_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(rmcast_base_send_t);


/* Setup an event to process a multicast message
 *
 * Multicast messages can come at any time and rate. To minimize
 * the probability of loss, and to avoid conflict when we send
 * data when responding to an input message, we use a timer
 * event to break out of the recv and process the message later
 */
typedef struct {
    opal_object_t super;
    opal_event_t *ev;
    uint8_t *data;
    ssize_t sz;
    rmcast_base_channel_t *channel;
} orte_mcast_msg_event_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(orte_mcast_msg_event_t);

/* Data structure for tracking recvd sequence numbers */
typedef struct {
    opal_object_t super;
    orte_process_name_t name;
    orte_rmcast_channel_t channel;
    orte_rmcast_seq_t seq_num;
} rmcast_recv_log_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(rmcast_recv_log_t);


/* Data structure for holding messages in case
 * of retransmit
 */
typedef struct {
    opal_object_t super;
    orte_rmcast_seq_t seq_num;
    orte_rmcast_channel_t channel;
    opal_buffer_t *buf;
} rmcast_send_log_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(rmcast_send_log_t);


#define ORTE_MULTICAST_MESSAGE_EVENT(dat, n, chan, cbfunc)      \
    do {                                                        \
        orte_mcast_msg_event_t *mev;                            \
        struct timeval now;                                     \
        OPAL_OUTPUT_VERBOSE((1, orte_debug_output,              \
                            "defining mcast msg event: %s %d",  \
                            __FILE__, __LINE__));               \
        mev = OBJ_NEW(orte_mcast_msg_event_t);                  \
        mev->data = (dat);                                      \
        mev->sz = (n);                                          \
        mev->channel = (chan);                                  \
        opal_evtimer_set(mev->ev, (cbfunc), mev);               \
        now.tv_sec = 0;                                         \
        now.tv_usec = 0;                                        \
        opal_evtimer_add(mev->ev, &now);                        \
    } while(0);


#define ORTE_MULTICAST_MESSAGE_HDR_HTON(bfr, tg, seq)   \
    do {                                                \
        uint32_t nm;                                    \
        uint16_t tmp;                                   \
        nm = htonl(ORTE_PROC_MY_NAME->jobid);           \
        memcpy((bfr), &nm, 4);                          \
        nm = htonl(ORTE_PROC_MY_NAME->vpid);            \
        memcpy((bfr)+4, &nm, 4);                        \
        /* add the tag data, also converted */          \
        tmp = htons((tg));                              \
        memcpy((bfr)+8, &tmp, 2);                       \
        /* add the sequence number, also converted */   \
        nm = htonl((seq));                              \
        memcpy((bfr)+10, &nm, 4);                       \
    } while(0);

#define ORTE_MULTICAST_MESSAGE_HDR_NTOH(bfr, nm, tg, seq)   \
    do {                                                    \
        uint32_t tmp;                                       \
        uint16_t tmp16;                                     \
        /* extract the name and convert it to host order */ \
        memcpy(&tmp, (bfr), 4);                             \
        (nm)->jobid = ntohl(tmp);                           \
        memcpy(&tmp, (bfr)+4, 4);                           \
        (nm)->vpid = ntohl(tmp);                            \
        /* extract the target tag */                        \
        memcpy(&tmp16, (bfr)+8, 2);                         \
        (tg) = ntohs(tmp16);                                \
        /* extract the sequence number */                   \
        memcpy(&tmp, (bfr)+10, 4);                          \
        (seq) = ntohl(tmp);                                 \
    } while(0);

#define ORTE_MULTICAST_LOAD_MESSAGE(bfr, dat, sz, maxsz, endsz) \
    do {                                                        \
        if ((maxsz) <= (sz) + 14) {                             \
            *(endsz) = -1 * ((sz) + 14);                        \
        } else {                                                \
            memcpy((bfr)+14, (dat), (sz));                      \
            *(endsz) = (sz) + 14;                               \
        }                                                       \
    } while(0);

#define ORTE_MULTICAST_UNLOAD_MESSAGE(bfr, dat, sz) \
        opal_dss.load((bfr), (dat)+14, (sz)-14);

#define ORTE_MULTICAST_NEXT_SEQUENCE_NUM(seq)   \
    do {                                        \
        if ((seq) < ORTE_RMCAST_SEQ_MAX) {      \
            (seq) += 1;                         \
        } else {                                \
            (seq) = 0;                          \
        }                                       \
    } while(0);

#endif /* ORTE_DISABLE_FULL_SUPPORT */

END_C_DECLS

#endif