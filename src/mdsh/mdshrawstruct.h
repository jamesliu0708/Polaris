#ifndef _MDSHRAWSTRUCT_H
#define _MDSHRAWSTRUCT_H

#include <stdint.h>

namespace polaris {

struct mdsh_proto {
    uint8_t ver_and_exchange;                 //高四位为版本号，低四位为交易所 上交所：0   深交所：1
    uint8_t length;                           //总体长度
    uint8_t tradeMode_and_securitytype;       //高四位为交易模式 Reserved:0  系统测试:1   模拟交易:2   正常交易:3
                                              //低四位为证券类型 1: 股票 2：衍生品 3：综合业务 4：债券
    uint8_t mdstreamid[5];                    //行情类别
    uint8_t trading_phase_code[8];            //实时阶段及标志
    uint32_t timestamp;                       //时间HHMMSSss 精度为10ms
    char     securityid[8];                   //产品代码
    uint64_t total_value;                     //成交金额
    uint64_t total_volume;                    //成交数量
    uint64_t last_price;                      //最新价
    uint64_t open_interest;                   //持仓量
    uint64_t bid_price[5];                    //五档买价
    uint32_t bid_volume[5];                   //五档买量
    uint64_t ask_price[5];                    //五档卖价
    uint32_t ask_volume[5];                   //五档卖量
};

} // polaris

#endif // _MDSHRAWSTRUCT_H