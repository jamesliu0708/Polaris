
#include <rte_branch_prediction.h>
#include <rte_prefetch.h>

static __rte_always_inline uint8_t direction_td_2_yd(enum direction direction)
{
    return (uint8_t)direction;
}

static __rte_always_inline uint8_t offset_td_2_yd(enum offsetflag offsetflag)
{
    if (likely(offsetflag <= (uint8_t)offsetflag::kClose))
        return (uint8_t)offsetflag::kClose;
    return (uint8_t)offsetflag::kClose + 1;
}

static __rte_always_inline uint8_t hedgeflag_td_2_yd(enum hedgeflag hedgeflag)
{
    return (uint8_t)hedgeflag;
}

static __rte_always_inline uint8_t order_type_td_2_yd(enum ordertype ordertype, enum timecondition timecondition, enum volumecondition volumecondition)
{
    if (ordertype == enum ordertype::kLimit && time_condition == enum timecondition::kGFD && volumecondition == enum volumecondition::kAny)
        return YD_ODT_Limit;
    if (ordertype == enum ordertype::kMarket && time_condition == enum timecondition::kGFD && volumecondition == enum volumecondition::kAny)
        return YD_ODT_Limit;
    if (ordertype == enum ordertype::kLimit && time_condition == enum timecondition::kIOC && volumecondition == enum volumecondition::kAny)
        return YD_ODT_Limit;
    if (ordertype == enum ordertype::kLimit && time_condition == enum timecondition::kIOC && volumecondition == enum volumecondition::kComplete)
        return YD_ODT_Limit;
    __builtin_unreachable();
}

static __always_inline order_td_2_yd(struct order_req* req, struct yd_order_raw* ydorder)
{
    ydorder->direction = direction_td_2_yd(req->order.direction);
    ydorder->offset = offset_td_2_yd(req->order.offsetflag);
    ydorder->hedgeflag = hedgeflag_td_2_yd(req->order.hedgeflag);
    //todo
    ydorder->seat_id = -1;
    ydorder->price = req->order.price;
    ydorder->order_num = req->order.volume;
    ydorder->order_ref = req->orderref;
    ydorder->order_type = order_type_td_2_yd(req->order.order_type, req->order.time_condition, req->order.volumecondition);
    ydorder->group_id = 0;
}

bool ydtrader::order_insert(struct order_req** req, unsigned int cnt)
{
    struct yd_order_raw ydorder = nullptr;
    unsigned int i;
    int remained;
    unsigned int ms_cnt = cnt > YDORDER_CACHE? YDORDER_CACHE: cnt;

    for (i = 0; i < ms_cnt;  ++i) {
        ydorder = &ydorders[i];
        rte_prefetch((void*)req[i + 1]);
        order_td_2_yd(req[i], ydorder);
    }

    port->ops->tx_burst((void*)ydorders, ms_cnt * sizeof(struct yd_order_raw));

    return true;
}