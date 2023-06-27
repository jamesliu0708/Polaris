#ifndef _SUZIRAWDATASTRUCT_H
#define _SUZIRAWDATASTRUCT_H

#include <stdint.h>

namespace polaris {
#pragma pack(push, 1)

//api和svr之间通讯的头结构
struct ftd_head
{
	char	        tag;		//#
	unsigned char	cmdtype;	//操作符
};

//期货、期权报单录入结构
struct NwOrderInsertField_Opt
{
	uint32_t    RequestNo: 32;
	uint32_t    SessionNo: 32;
	uint32_t    ClientNo : 32;
	uint32_t    InstrumentNo : 32;
	int32_t     LocalOrderNo : 32;
	char        ExchangeId;		//交易所代码
	char        FrontNo;			//本次报单的席位序号,当所填的序号在系统中不存在时系统会优选最佳席位进行报单,否则,根据用户所选席位进行报单
	char        PriceType;	//报单价格条件限价单事价单等
	char        Side;	//买卖方向
	char        OffsetFlag;	//开平标记
	char        HedgeFlag;	//投机套保标记
	char        TimeInForce;	//有效期类型
	char        CoveredOrUncovered;		//备兑标签
	char        VolumeCondition;//成交量类型
	char        TrigCondition;//触发条件
	uint32_t    Volume:32;//数量
	uint32_t    MinVolume : 32;//最小成交量
	double      Price;//价格
	double      StopPrice;//止损价
	char        OwnerType;	//订单所有类型
};

struct NwOrderInsertField_Opt_Req
{
	ftd_head head;
	NwOrderInsertField_Opt reqdata;
};
const int NwOrderInsertField_Opt_Req_s = sizeof(NwOrderInsertField_Opt_Req);

union NwOrderInsertField_Opt_Req_u
{
	char c[NwOrderInsertField_Opt_Req_s];
	NwOrderInsertField_Opt_Req s;
};
const int NwOrderInsertField_Opt_Req_u_s = sizeof(NwOrderInsertField_Opt_Req_u);

struct NwOrderInsertField_Opt_Rsp
{
	ftd_head head;
	int errcord : 32;
	char ordersysid[17];
	NwOrderInsertField_Opt rspdata;
};
const int NwOrderInsertField_Opt_Rsp_s = sizeof(NwOrderInsertField_Opt_Rsp);

union NwOrderInsertField_Opt_Rsp_u
{
	char c[NwOrderInsertField_Opt_Rsp_s];
	NwOrderInsertField_Opt_Rsp s;
};
const int NwOrderInsertField_Opt_Rsp_u_s = sizeof(NwOrderInsertField_Opt_Rsp_u);



//撤单结构
struct NwOrderActionField
{
	uint32_t    RequestNo : 32;
	uint32_t    SessionNo : 32;
	int32_t     OriLocalOrderNo : 32;
	uint32_t    OriSessionNo : 32;		//原始报单的sessionno如果是通过LocalOrderNo撤单，需要填写，不填写则默认撤销本次登录后的LocalOrderNo
	char        OrderSysId[17];			//原始报单的交易所报单编号
};

struct NwOrderActionField_Req
{
	ftd_head head;
	NwOrderActionField reqdata;
};
const int NwOrderActionField_Req_s = sizeof(NwOrderActionField_Req);

union NwOrderActionField_Req_u
{
	char c[NwOrderActionField_Req_s];
	NwOrderActionField_Req s;
};
const int NwOrderActionField_Req_u_s = sizeof(NwOrderActionField_Req_u);

struct NwOrderActionField_Rsp
{
	ftd_head head;
	int errcord : 32;
	NwOrderActionField rspdata;
};
const int NwOrderActionField_Rsp_s = sizeof(NwOrderActionField_Rsp);

union NwOrderActionField_Rsp_u
{
	char c[NwOrderActionField_Rsp_s];
	NwOrderActionField_Rsp s;
};
const int NwOrderActionField_Rsp_u_s = sizeof(NwOrderActionField_Rsp_u);

#pragma pack(pop)

} // polaris

#endif // _SUZIRAWDATASTRUCT_H