#include "ydlistener.h"
#include <stdlib.h>
#include <string>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <rte_cfgfile.h>
#include <rte_branch_prediction.h>
#include "ydproto.h"
#include "ydDataType.h"

#define RTN_EXIT_WITH_WARNING(args...) do {    \
    fprintf(stderr, args);    \
    exit(-1);   \
} while (0)

namespace polaris {

ydrtntrader::ydrtntrader(struct rte_port* port, const char* cfgfilename):
    port_(port)
{
    int ret;
    const char* sec_name;
    const char* entry;
    struct rte_cfgfile *file = rte_cfgfile_load(cfgfilename, 0);
    std::string localip, remoteip;
    if (file == NULL) 
        RTN_EXIT_WITH_WARNING("Failed to open configuation file\n");
    
    sec_name = "yd";
    ret = rte_cfgfile_has_section(file, sec_name);
    if (ret) 
        RTN_EXIT_WITH_WARNING("failed to get yd section\n");
    
    entry = rte_cfgfile_get_entry(file, sec_name, "local ip");
    if (entry == NULL)
        RTN_EXIT_WITH_WARNING("Failed to get local ip\n");
    localip = entry;
    inet_pton(AF_INET, localip.c_str(), &cfg_.srcaddr);
    
    entry = rte_cfgfile_get_entry(file, sec_name, "remote ip");
    if (entry == NULL)
        RTN_EXIT_WITH_WARNING("Failed to get remote ip\n");
    inet_pton(AF_INET, localip.c_str(), &cfg_.srcaddr);

    entry = rte_cfgfile_get_entry(file, sec_name, "local port");
    if (entry == NULL)
        RTN_EXIT_WITH_WARNING("Failed to get local port\n");
    cfg_.srcport = htons((uint16_t)atoi(entry));
    
    entry = rte_cfgfile_get_entry(file, sec_name, "remote port");
    if (entry == NULL)
        RTN_EXIT_WITH_WARNING("Failed to get remote port\n");
    cfg_.dstport = htons((uint16_t)atoi(entry));
}

static inline bool ip_filter(struct iphdr* ipheader, struct ydrtn_configuration* cfg)
{
    if (likely(ipheader->saddr == (uint32_t)cfg->dstaddr.s_addr && ipheader->daddr == (uint32_t)cfg->srcaddr.s_addr 
        && ipheader->protocol == IPPROTO_TCP)) 
        return true;
    return false;
}

static inline unsigned int tcp_filter(struct tcphdr* tcpheader, struct ydrtn_configuration* cfg)
{
    if (likely(tcpheader->source == cfg->dstport && tcpheader->dest == cfg->srcport))
        return true;
    return false;
}

static inline unsigned int rtn_unkown_cache(const void* pkt, rtn_stream_info* dst, unsigned int len) {
    unsigned int cp_size = sizeof(yd_rtn_header) - dst->cache_size;
    if (cp_size > 0) {
        memcpy(&dst->header + dst->cache_size, pkt, len);
        dst->cache_size += len;
        return len;
    }
    memcpy(&dst, pkt, cp_size);
    dst->cache_size += cp_size;
    dst->type = RTN_HEADER_CACHE;
    return cp_size;
}

static inline phospherus::orderstatus convert_yd_status(uint32_t orderstatus, const struct yd_rtn_order* __restrict__ yd_order)
{
    phospherus::orderstatus status;
    switch (orderstatus) {
    case YD_OS_Accepted:
        return phospherus::orderstatus::kWaiting;
        break;
    case YD_OS_Queuing:
        {
            status = phospherus::orderstatus::kInBook;
            if (yd_order->traded_volume > 0) {
                if (yd_order->traded_volume == yd_order->volume) {
                    return phospherus::orderstatus::kAllTrade;
                } else {
                    return phospherus::orderstatus::kPartTraded;
                }
            }
            break;
        }
    case YD_OS_Canceled:
        return phospherus::orderstatus::kCancel;
    case YD_OS_AllTraded:
        return phospherus::orderstatus::kAllTrade;
    case YD_OS_Rejected:
        return phospherus::orderstatus::kError;
    default:
        break;
    }
    return status;
}

static inline void fill_orderstat(struct phospherus::orderstat* __restrict__ stat, const struct yd_rtn_order* __restrict__ yd_order, std::error_code& err) {
    stat->instr_id = yd_order->instrumentref;
    stat->status = convert_yd_status(yd_order->orderstatus, yd_order);
    stat->orderref = yd_order->orderref;
    stat->traded_volume = yd_order->traded_volume;
    stat->volume = yd_order->volume;
}

static inline void fill_tradeinfo(struct phospherus::tradeinfo* __restrict__ trade, const struct yd_rtn_trade* __restrict__ yd_trade) {
    trade->instr_id = yd_trade->instrument_ref;
    trade->traded_id = yd_trade->trade_id;
    trade->price = yd_trade->trade_price;
    trade->traded_volume = yd_trade->trade_volume;
}

void ydrtntrader::on_yd_order(const yd_rtn_order *__restrict__ yd_order)
{
    struct phospherus::orderstat orderstat;
    std::error_code err;
    fill_orderstat(&orderstat, yd_order, err);
    this->on_rtn_order(&orderstat, err);
}

void ydrtntrader::on_yd_trade(const yd_rtn_trade *__restrict__ yd_trade)
{
    struct phospherus::tradeinfo tradeinfo;
    fill_tradeinfo(&tradeinfo, yd_trade);
    this->on_rtn_trade(&tradeinfo);
}

unsigned int ydrtntrader::rtn_header_cache(const void* pkt, rtn_stream_info* dst, unsigned int len)
{
    unsigned int pkt_len = (unsigned int)dst->header.len;
    unsigned int cp_size = pkt_len - dst->cache_size;
    if (len > cp_size) {
        if (dst->header.proto == RTN_ORDER) {
            memcpy((char*)&dst->ordercache + dst->cache_size, pkt, cp_size);
            on_yd_order(&dst->ordercache);
        } else if (dst->header.proto == RTN_TRADE) {
            memcpy((char*)&dst->tradecache + dst->cache_size, pkt, cp_size);
            on_yd_trade(&dst->tradecache);
        }
        dst->f_cache = false;
        dst->cache_size = 0;
        return cp_size;
    } else {
        if (dst->header.proto == RTN_ORDER || dst->header.proto == RTN_TRADE)
            memcpy((char*)&dst->header + dst->cache_size, pkt, len);
        dst->f_cache = true;
        dst->cache_size += len;
    }
    return len;   
}

void ydrtntrader::pkt_process(const void* pkt, unsigned int len) {
    struct yd_rtn_order* p_order = nullptr;
    char* p_raw = (char*)pkt;
    unsigned int remained = len;
    if (rtn_stream_info_.f_cache) {
        switch (rtn_stream_info_.type) {
        case RTN_UNKOWN_CACHE:
        {
            unsigned int len = rtn_unkown_cache(p_raw, &rtn_stream_info_, remained);
            remained -= len;
            p_raw += len;
            if (remained)
                return pkt_process(p_raw, remained);
            break;
        }
        case RTN_HEADER_CACHE:
        {
            unsigned int len = rtn_header_cache(p_raw, &rtn_stream_info_, remained);
            remained -= len;
            p_raw += len;
            break;
        }
        default:
            __builtin_unreachable();
        }
    }
    while (remained > 0) {
        if (remained > sizeof(struct yd_rtn_header)) {
            struct yd_rtn_header* header = (struct yd_rtn_header*)p_raw;
            if (header->len < remained) {
                switch (header->proto) {
                case RTN_ORDER:
                {
                    struct yd_rtn_order* p_order = (struct yd_rtn_order*)p_raw;
                    on_yd_order(p_order);
                    break;
                }
                case RTN_TRADE:
                {
                    struct yd_rtn_trade* p_trade = (struct yd_rtn_trade*)p_raw;
                    on_yd_trade(p_trade);
                    break;
                }
                default:
                    break;
                }
                p_raw += header->len;
                remained -= header->len;
            } else {
                memcpy(&rtn_stream_info_.header, p_raw, remained);
                rtn_stream_info_.cache_size += remained;
                rtn_stream_info_.type = RTN_HEADER_CACHE;
                rtn_stream_info_.f_cache = true;
            }
        } else {
            memcpy(&rtn_stream_info_.header, p_raw, remained);
            rtn_stream_info_.cache_size += remained;
            rtn_stream_info_.type = RTN_UNKOWN_CACHE;
            rtn_stream_info_.f_cache = true;
        }
    }
}

void ydrtntrader::on_recv_yd(const void** pkt, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; ++i) {
        struct ethhdr* etherheader = (struct ethhdr*)pkt;
        if (likely(etherheader->h_proto == ETH_P_IP)) {
            struct iphdr* ipheader = (struct iphdr*)(etherheader + 1);
            if (likely(ip_filter(ipheader, &cfg_))) {
                unsigned int ihl = ipheader->ihl;
                unsigned int tot_len = ipheader->tot_len;
                struct tcphdr* tcpheader = (struct tcphdr*)((char*)ipheader + ihl);
                if (likely(tcp_filter(tcpheader, &cfg_))) {
                    unsigned int payload_len = tot_len - ihl - tcpheader->doff * 4;
                    void* payload = (void*)((char*)tcpheader + tcpheader->doff * 4);
                    pkt_process(payload, payload_len);
                }
            }
        }
    }
}

} // polaris