#ifndef _YDPROTO_H
#define _YDPROTO_H
#include <stdint.h>

#pragma pack(push, 1)
#define CLIENTHEADERLEN     16
struct yd_order_raw {
    uint8_t                 client_header[16];
    int32_t                 instrument_ref;
    int8_t                  direction;
    int8_t                  offset_flag;
    int8_t                  hedge_flag;
    int8_t                  connect_selection;
    double                  price;
    int                     order_num;
    int                     order_ref;
    int8_t                  order_type;
    int8_t                  pad1;
    int8_t                  connect_id;
    int8_t                  pad2[5];
    uint8_t                 group_id;
    int8_t                  order_control;
    int8_t                  trigger_type;
    int8_t                  pad3[13];
    double                  trigger_price;
};

#define RTN_HEARTBEAT 0x1
#define RTN_ORDER 0x22
#define RTN_TRADE 0x23
#define RTN_QUERY_PRICE 0x25
#define RTN_QUOTE 0X28
#define RTN_CANCELFAILED_OR_QUOTE_FAILED 0x2b

struct yd_rtn_header {
    uint16_t                len;
    uint16_t                pad1;
    uint32_t                proto;
    uint32_t                pad2;
    uint32_t                account_id;
};

struct yd_rtn_order {
    struct yd_rtn_header    header; /**< Packet header */
    uint32_t                instrumentref; /**< Instrument reference */
    uint8_t                 direction; /**< Direction 0: buy 1: Sell */
    uint8_t                 offset_flag; /**< Offset flag (0: Open, 1: Close, 3: CloseToday, 4: CloseYesterday) */
    uint8_t                 hedge_flag; /**< Hedge flag (0: SpecSpec, 2: SpecHedge, 3: HedgeHedge, 4: HedgeSpec) */
    uint8_t                 seat; /**< Seat selection (0：YD_CS_Any, 1：YD_CS_Fixed, 2：YD_CS_Prefered)*/
    double                  price; /**< Order price */
    uint32_t                volume; /**< Order volume */
    uint32_t                orderref; /**< User defined reference of the order */
    uint8_t                 ordertype; /**< Order Type (0: Limit, 1: FAK, 2: Market, 3:FOK)*/
    uint8_t                 connection_id; /**< Target connection ID if ConnectionSelectionType is not YD_CS_Any */
    uint8_t                 seat_id; /**<  Real connection ID for this order, <0 if not from connections of other systems */
    uint32_t                error; /**< Set by ydAPI, refer to "ydError.h" */
    uint32_t                ex_id; /**< Exchange id */
    uint32_t                sys_id; /**< sys id */
    uint32_t                orderstatus; /**< Refer to "Order Status" section of ydDataType.h */
    uint32_t                traded_volume; /**< Traded volume */
    uint32_t                insert_time; /**< insert time */
    uint32_t                localno; /**< local id */
    uint8_t                 groupid; /**< Indicates OrderRef management group, 0 for normal, 1-63 for strict management */
    uint8_t                 control; /**< OrderRef control method for this order if OrderGroupID in in [1,63], refer to "Order group ref control" section of ydDataType.h */
    uint8_t                 trigger; /**< Refer to "Order trigger type" section of ydDataType.h */
    uint8_t                 pad[13];
    double                  trigger_price; /**< Trigger price for trigger order */
    uint32_t                trigger_status; /**< Refer to "Order trigger status" section of ydDataType.h */
    uint32_t                milisec;
    uint64_t                exchange_ref;
};

struct yd_rtn_trade {
    struct yd_rtn_header    header; /**< Packet header */
    uint16_t                instrument_ref;
    uint8_t                 direction;
    uint8_t                 offset_flag;
    uint8_t                 hedge_flag;
    uint8_t                 pad1;
    uint32_t                trade_id;
    uint32_t                trade_sys_id;
    double                  trade_price;
    uint32_t                trade_volume;
    uint32_t                trade_timestamp;
    double                  fee;
    uint32_t                local_id;
    uint32_t                account_id;
    uint8_t                 group_id;
    uint8_t                 seat;
    uint16_t                pad2;
    uint32_t                msec;
    uint64_t                traded_id1;
    uint64_t                traded_id2;                             
};

#pragma pack(pop)

#endif // _YDPROTO_H