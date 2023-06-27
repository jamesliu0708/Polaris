#ifndef _SUZITRADER_H
#define _SUZITRADER_H

#include <rte_common.h>
#include <app/trading/trader_api.h>
#include <driver/pss_port.h>

#include "TitdApi.h"

#define PHOSPHERUS_OVERRIDE override
#define SUZI_OVERRIDE override

namespace polaris {
#define MAX_ORDER_CNT 256
struct suzitrader: public phospherus::traderapi, public titd::CTiTdSpi {
    struct orderhelper {
        uint32_t request_no;
        uint32_t session_no;
        uint32_t local_order_no;
    };
public:
    suzitrader();
    suzitrader(pss_port_handle_t handle);
    ~suzitrader();
    bool init() PHOSPHERUS_OVERRIDE;
    bool release() PHOSPHERUS_OVERRIDE;
    __rte_always_inline bool order_insert(struct order_req** req, unsigned int reqcnt) PHOSPHERUS_OVERRIDE;

    void OnConnected() SUZI_OVERRIDE;
    void OnDisconnected(int reason) SUZI_OVERRIDE;
    void OnRspUserLogin(CTdRspUserLoginField& pRspUserLogin, CTdRspInfoField& pRspInfo, int nRequestID, bool bIsLast) SUZI_OVERRIDE;

private:
    void on_rx_burst();
    void on_rx_burst_preparation(std::string remote, int port);
    bool on_recv_preparation_pkt(struct pss_pktbuf* buf, uint32_t dstip, uint16_t port);
    void refill(void* pkt);
private:
    uint16_t n_rxq_;
    uint16_t qi_;
    pss_port_handle_t handle_;
    titd::CTiTdApi* suziapi_;
    uint64_t max_order_id_;
    uint32_t client_no_;
    struct orderhelper orderhelper_;
    static pss_pktbuf* pkt_[MAX_ORDER_CNT];
    static struct NwOrderInsertField_Opt *raworder[MAX_ORDER_CNT];
    std::thread responsor_;
};

} // polaris

#endif // _SUZITRADER_H

#include "suzitrader.inl"