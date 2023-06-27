#ifndef _YDLISTENER_H
#define _YDLISTENER_H

#include <linux/ip.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <app/trading/rtn_trader.h>
#include "ydproto.h"

struct rte_port;

namespace polaris {

struct ydrtn_configuration {
    struct in_addr srcaddr;
    struct in_addr dstaddr;
    uint16_t srcport;
    uint16_t dstport;
};

enum rtn_cache_type {
    RTN_UNKOWN_CACHE,
    RTN_HEADER_CACHE,
};

struct rtn_stream_info {
    union {
        struct yd_rtn_order ordercache;
        struct yd_rtn_trade tradecache;
        struct yd_rtn_header header;
    } ;
    uint8_t type;
    uint32_t cache_size;
    bool f_cache;
};

struct ydrtntrader: public phospherus::rtn_trader {
    
    ydrtntrader(struct rte_port* port, const char* cfgfilename);
    
    void on_recv_yd(const void** pkt, unsigned int num);

    void pkt_process(const void* pkt, unsigned int len);

    unsigned int rtn_header_cache(const void* pkt, rtn_stream_info* dst, unsigned int len);

    void on_yd_order(const yd_rtn_order *__restrict__ yd_order);

    void on_yd_trade(const yd_rtn_trade *__restrict__ yd_trade);

    struct rte_port* port_;
    struct ydrtn_configuration cfg_;
    struct rtn_stream_info rtn_stream_info_ = { 
        .cache_size = 0,
        .f_cache = false
    };
};

} // polaris

#endif // _YDLISTENER_H