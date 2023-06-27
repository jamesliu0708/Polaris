#include "faktrader.h"
#include <rte_cfgfile.h>
#include <app/trading/datastruct.h>

static constexpr unsigned int kOrderFrequency = 0;
static constexpr unsigned int kThrottle = 1;

faktrader::faktrader(const char* cfg, struct risk* risk, unsigned int cnt, struct traderapi* trader_api, struct rtn_trader* rtn):
    trader(risk, cnt, trader_api, rtn)
{
    risk_setup(cfg);
}

faktrader::~faktrader()
{}

unsigned int faktrader::order_insert_slow_path(struct order_req** req, 
                            unsigned int cnt) {
    unsigned int i;
    bool ret = true;
    std::error_code error;
    for (i = 0; i < cnt; ++i) {
        if ((ret = risk[i].on_freq_risk(req[i], 1, &error))) {
            struct orderstat orderstat = {
                .instr_id = req[i]->instr_id,
                .status = orderstatus::kError,
                .orderref = -1,
                .traded_volume = 0,
                .volume = req[i].volume,
                .error = error
            };

            rtntrader->on_rtn_order(&stat, error);
            return i;
        }
    }
    
    return cnt;
}

void risk_setup(const char* cfgpath)
{
    struct rte_cfgfile *cfgfile = NULL;
    file = rte_cfgfile_load(cfgpath, 0);
    if (file == NULL)
        return;

    if ()
}