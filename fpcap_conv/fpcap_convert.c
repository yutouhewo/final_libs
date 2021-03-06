/*
 * =====================================================================================
 *
 *       Filename:  fpcap_convert.c
 *
 *    Description:  The framework of convertion
 *
 *        Version:  1.0
 *        Created:  03/17/2013 14:55:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  finaldie
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include <pcap.h>

#include "lhash.h"
#include "fpcap_convert.h"

#ifdef DEBUG
#define debug_printf(fmt, ...) \
        printf("%s:%d - " fmt, __FILE__, __LINE__, __VA_ARGS__);
#else
#define debug_printf(fmt, ...)
#endif

#define SESSION_HASH_SIZE 65535

typedef struct fopt_action_t {
    convert_action_t* iaction;
    f_hash* phash;
} fopt_action_t;

static inline
uint64_t fsession_key(uint32_t ip, uint16_t port)
{
    uint64_t id = ((uint64_t)ip) << 16;
    id += port;

    return id;
}

static inline
session_t* fsession_find(f_hash* hash_tbl, uint64_t id)
{
    return hash_get_uint64(hash_tbl, id);
}

static
session_t* fsession_create(uint64_t session_id)
{
    session_t* session = malloc(sizeof(session_t));
    session->id = session_id;
    session->ud = NULL;

    return session;
}

static
void fsession_del(f_hash* hash_tbl, uint64_t session_id)
{
    void* session = hash_del_uint64(hash_tbl, session_id);
    free(session);
}

static
int fsession_add(f_hash* hash_tbl, session_t* session)
{
    if( fsession_find(hash_tbl, session->id) ) {
        return 1;
    }

    hash_set_uint64(hash_tbl, session->id, session);
    return 0;
}

static
void fill_app_data(fapp_data_t* app_data, struct timeval* ts,
                   char* data, int data_len)
{
    app_data->ts = *ts;
    app_data->data = data;
    app_data->len = data_len;
}

int filter_tcpip_pkg(fopt_action_t* action, const struct pcap_pkthdr* pkg_header, const char* pkg_data)
{
    struct ether_header* eptr;
    struct iphdr* ipptr;
    struct tcphdr* tcpptr;
    char* data = NULL;
    int tcphdr_len = 0;
    int data_len = 0;
    uint32_t caplen = (uint32_t)pkg_header->caplen;
    fconv_handler handler = action->iaction->handler;
    uint32_t serving_type = action->iaction->type;
    void* ud = action->iaction->ud;
    f_hash* phash = action->phash;

    if( caplen < sizeof(struct ether_header) ) {
        return 1;
    }

    // validate package type
    eptr = (struct ether_header *)pkg_data;
    if( ntohs(eptr->ether_type) != ETHERTYPE_IP ) {
        printf("!!!pkg dropped, pkg type=%d,%d\n", ntohs(eptr->ether_type), eptr->ether_type);
        return 2;
    }

    caplen -= sizeof(struct ether_header);
    if( caplen < sizeof(struct iphdr) ) {
        return 3;
    }

    ipptr = (struct iphdr *)(pkg_data + sizeof(struct ether_header));
    if( caplen != ntohs(ipptr->tot_len) ) {
        return 4;
    }

    if( ipptr->protocol != 0x06 ) {
        return 5;
    }

    int iphdr_len = ipptr->ihl << 2;
    tcpptr = (struct tcphdr*)((char*)ipptr + iphdr_len);
    tcphdr_len = tcpptr->doff << 2;
    data_len   = ntohs(ipptr->tot_len) - iphdr_len - tcphdr_len;
    data       = (char*)tcpptr + tcphdr_len;

    uint64_t session_id = 0;
    if( serving_type == FPCAP_CONV_CLIENT )
        session_id = fsession_key(ipptr->saddr, tcpptr->source);
    else
        session_id = fsession_key(ipptr->daddr, tcpptr->dest);

    // create session
    if( tcpptr->syn ) {
        session_t* session = fsession_create(session_id);
        if( fsession_add(phash, session) ) {
            debug_printf("must be something wrong, the session(%" PRIu64 ") already exist\n", session_id);
            return 6;
        }

        fapp_data_t app_data;
        fill_app_data(&app_data, (struct timeval*)&pkg_header->ts, NULL, 0);
        handler(FSESSION_CREATE, session, &app_data, ud);
    } else if( tcpptr->fin || tcpptr->rst ) {
        // collect one session complete
        session_t* session = fsession_find(phash, session_id);
        if( !session) {
            return 7;
        }

        // push the session data to output list
        fapp_data_t app_data;
        fill_app_data(&app_data, (struct timeval*)&pkg_header->ts, NULL, 0);
        handler(FSESSION_DELETE, session, &app_data, ud);
        fsession_del(phash, session_id);
    } else {
        session_t* session = fsession_find(phash, session_id);
        if( !session ) {
            debug_printf("we dont know this pkg where come from\n");
            return 8;
        }

        if( data_len <= 0 ) {
            debug_printf("maybe this is only a ack pkg, ignore it\n");
            return 9;
        }

        fapp_data_t app_data;
        fill_app_data(&app_data, (struct timeval*)&pkg_header->ts, data, data_len);
        handler(FSESSION_PROCESS, session, &app_data, ud);
    }

    return 0;
}

// currently we only support ip and tcp protocol
void dump_cb(u_char* ud, const struct pcap_pkthdr* pkg_header, const u_char* pkg_data)
{
    fopt_action_t* opt_action = (fopt_action_t*)ud;
    filter_tcpip_pkg(opt_action, pkg_header, (char*)pkg_data);
}

static
void fpcap_session_foreach(fopt_action_t* action)
{
    int loop_handler(void* data)
    {
        session_t* session = (session_t*)data;
        int ret = action->iaction->cleanup(session, action->iaction->ud);
        fsession_del(action->phash, session->id);
        return ret;
    }

    int final_clean(void* data)
    {
        session_t* session = (session_t*)data;
        fsession_del(action->phash, session->id);
        return 0;
    }

    f_hash* phash = action->phash;
    hash_foreach(phash, loop_handler);
    hash_foreach(phash, final_clean);
}

int fpcap_convert(convert_action_t action)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* p = pcap_open_offline(action.pcap_filename, errbuf);
    if( !p ) {
        printf("cannot open offline mode:%s\n", errbuf);
        return 1;
    }

    struct bpf_program filter;
    if( pcap_compile(p, &filter, action.filter_rules, 1, 0) ) {
        printf("pcap_compile error:%s\n", pcap_geterr(p));
        return 2;
    }

    if( pcap_setfilter(p, &filter) ) {
        printf("pcap_setfilter error:%s\n", pcap_geterr(p));
        return 3;
    }

    fopt_action_t opt_action;
    opt_action.iaction = &action;
    opt_action.phash = hash_create(SESSION_HASH_SIZE);
    int st = pcap_loop(p, 0, dump_cb, (u_char*)&opt_action);
    if( st != 0 ) {
        printf("pcap_loop error\n");
        return 4;
    }

    // cleanup
    if( action.cleanup ) {
        fpcap_session_foreach(&opt_action);
    }

    hash_delete(opt_action.phash);
    pcap_close(p);
    return 0;
}
