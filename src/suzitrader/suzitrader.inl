#ifndef _SUZITRADER_INL
#define _SUZITRADER_INL

#include "suzitrader.h"
#
#include <rte_common.h>

namespace polaris {

namespace {
using namespace phospherus;
__rte_always_inline char convert_price_type_local_to_suzi(ordertype price_type)
{
    switch (t) {
    case OrderType::kMarket :   return TD_PriceType_Market;
    case OrderType::kLimit : return TD_PriceType_Limit;
    default:    __builtin_unreachable();
    }
    return -1;
}

__rte_always_inline char convert_side_local_to_suzi(direction size)
{
    switch (t) {
    case direction::kLong: return TD_SIDE_Buy;
    case direction::kShort: return TD_SIDE_Sell;
    default:    __builtin_unreachable();
    }
    return -1;
}

__rte_always_inline char convert_offsetflag_local_to_suzi(offsetflag offsetflag)
{
    switch (t) {
    case offsetflag::kOpen: return TD_OC_Open;
    case offsetflag::kClose:    return TD_OC_Close;
    case offsetflag::kCloseToday:   return TD_OC_CloseToday;
    case offsetflag::kCloseYesterday:   return TD_OC_Close;
    default:    __builtin_unreachable();
    }
    return -1;
}

__rte_always_inline char convert_hedgeflag_local_to_suzi(hedgeflag t)
{
    switch (t) {
    case hedgeflag::kSpeculation: return TD_HF_Speculation;
    case hedgeflag::kArbitrage: return TD_HF_Arbitrage;
    case hedgeflag::kHedge: return TD_HF_Hedge;
    default:    __builtin_unreachable();
    }
    return -1;
}

__rte_always_inline char convert_timecondition_local_to_suzi(timecondition t, volumecondition v) {
    if (t == timecondition::kIOC && v == volumecondition::kAny) 
        return TD_TimeInForce_IOC;
    else if (timecondition::kIOC && v == volumecondition::kComplete)
        return TD_TimeInForce_FOK;
    else if (t == timecondition::kGFD)
        return TD_TimeInForce_GFD;
    __builtin_unreachable();
    return -1;
}

__rte_always_inline char convert_volumecondition_local_to_suzi(volumecondition t) {
    switch (t) {
    case volumecondition::kAny: return TD_VC_AV;
    case volumecondition::kComplete:    return TD_VC_CV;
    default:    __builtin_unreachable();
    }
    return -1;
}
}

bool suzitrader::order_insert(struct order_req** req, unsigned int reqcnt)
{
    unsigned int i = 0;
    assert(reqcnt < MAX_ORDER_CNT);
    struct NwOrderInsertField_Opt raworder[reqcnt];
    for (i = 0; i < reqcnt; ++i) {
        raworder[i]->RequestNo = orderhelper_.request_no + i;
        raworder[i]->LocalOrderNo = orderhelper_.local_order_no + i;
        raworder[i]->InstrumentNo = req[i]->instr_id;
        raworder[i]->PriceType = convert_price_type_local_to_suzi(req[i]->ordertype);
        raworder[i]->Side = convert_side_local_to_suzi(req[i]->direction);
        raworder[i]->OffsetFlag = convert_offsetflag_local_to_suzi(req[i]->offsetflag);
        raworder[i]->HedgeFlag = convert_hedgeflag_local_to_suzi(req[i]->hedgeflag);
        raworder[i]->TimeInForce = convert_timecondition_local_to_suzi(req[i]->timecondition, req[i]->volumecondition);
        raworder[i]->VolumeCondition = convert_volumecondition_local_to_suzi(req[i]->volumecondition);
        raworder[i]->Volume = req[i]->volume;
        raworder[i]->Price = req[i]->price;
    }
    uint16_t send = pss_pkt_tx_burst_at(handle_, pkt_, qi_, MAX_ORDER_CNT, NULL);
    if (__glibc_unlikely(send != reqcnt)) {
        
    }
}

} // polaris

#endif // _SUZITRADER_INL