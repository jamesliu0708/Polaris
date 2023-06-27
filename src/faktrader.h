#ifndef _FAK_TRADER_H
#define _FAK_TRADER_H

#include <app/trading/trader.h>

struct faktrader: public trader {
    faktrader(struct risk* risk, unsigned int cnt, struct traderapi* trader_api, struct rtn_trader* rtn);
    ~faktrader();

    __rte_noinline __rte_cold 
    unsigned int order_insert_slow_path(struct order_req** req, 
                            unsigned int cnt)override;

    void risk_setup(const char* cfgpath);

};

#endif // _FAK_TRADER_H