#ifndef _YD_TRADER_H
#define _YD_TRADER_H
#include <stdint.h>
#include <atomic>
#include <rte_common.h>
#include "ydApi.h"

struct rte_port* port;

#define YD_OVERRIDE override
#define PHOSP_OVERRIDE override

namespace polaris {

#define YDORDER_CACHE 10
struct ydtrader: public phospherus::traderapi, public YDListener {
    ydtrader(const char* cfgfilename, struct pss_ports* port);
    bool init()PHOSP_OVERRIDE;
    bool release()PHOSP_OVERRIDE;
    __rte_always_inline bool order_insert(struct order_req** req, unsigned int cnt)PHOSP_OVERRIDE;

    void notifyReadyForLogin(bool hasLoginFailed)YD_OVERRIDE;
    void notifyLogin(int errorNo,int maxOrderRef,bool isMonitor)YD_OVERRIDE;
    void notifyFinishInit(void)YD_OVERRIDE;
    void notifyCaughtUp(void)YD_OVERRIDE;
    void notifyBeforeApiDestroy(void)YD_OVERRIDE;
    void notifyAfterApiDestroy(void)YD_OVERRIDE; 

    struct yd_order_raw ydorders_[YDORDER_CACHE];
    YDApi*  api_;
    std::atomic<int8_t> status_;
};

} // polaris

#include "./ydtrader.inl"

#endif // _YD_TRADER_H