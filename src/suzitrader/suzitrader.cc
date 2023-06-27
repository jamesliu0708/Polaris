#include "suzitrader.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <time.h>
#include <cdefs.h>
#include <atomic>
#include <fstream>
#include <string>
#include "json/json.h"

namespace polaris {

namespace {

enum : int {
    COUNTER_STATUS_NOT_INIT,
    COUNTER_STATUS_CONNECTED,
    COUNTER_STATUS_DISCONNECTED,
    COUNTER_STATUS_LOGOUT,
    COUNTER_STATUS_LOGINED
};

int counter_status = COUNTER_STATUS_NOT_INIT;

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

constexpr unsigned int default_timeout_tvsec = 3;

inline int futex(int* uaddr, int futex_op, int val, const struct timespec *timeout,
            int *uaddr2, int val3) 
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int status_wait(int expected) 
{
    struct timespec timeout;
    int ret = clock_gettime(CLOCK_REALTIME, &timeout);
    if (__glibc_unlikely(ret != 0)) 
        return ret;
    timeout.tv_sec += default_timeout_tvsec;
    return futex(&counter_status, FUTEX_WAIT, expected, &timeout, NULL, 0);
}

int status_wake(int expected) {
    return futex(&counter_status, FUTEX_WAKE, expected, NULL, NULL, 0);
}

int read_counter_status() {
    return ACCESS_ONCE(counter_status);
}

}

suzitrader::suzitrader():
    handle_(-1), qi_(-1), suziapi_(nullptr), max_order_id_((typeof(max_order_id_))-1), orderhelper_.request_no(0)
{}

suzitrader::suzitrader(pss_port_handle_t handle, uint16_t qi):
    handle_(handle), qi_(qi), suziapi_(nullptr), max_order_id_((typeof(max_order_id_))-1), orderhelper_.request_no(0)
{}

suzitrader::~suzitrader()
{
    if (responsor_.joinable())
        responsor_.join();
    handle_ = -1;
    qi_ = -1;
    suziapi_ = nullptr;
    max_order_id_ = -1;
    orderhelper_.request_no = 0;
}

bool suzitrader::init() 
{
    const char* cfgfile = "./suzitrader_cfg.json";
    bool success = false;
    std::ifstream ifile;
    ifile.open(cfgfile);

    Json::CharReaderBuilder jsoncfgbuilder;
    Json::Value root;
    std::string cfgerr;
    success = Json::parseFromStream(jsoncfgbuilder, ifile, &root, &cfgerr);
    if (!success) {
        fprintf(stderr, "suzitrader init failed: Cannot parse json [%s]\n", cfgerr.c_str());
        return false;
    }

    std::string userid = root.get("userid", "null").asString();
    if (userid == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get user id\n").asString();
        return false;
    }

    std::string password = root.get("password", "null").asString();
    if (password == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get user password\n");
        return false;
    }

    std::string protocolinfo = root.get("protocolinfo", "null").asString();
    if (protocolinfo == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get protocolinfo\n");
        return false;
    }

    std::string appid = root.get("appid", "null").asString();
    if (appid == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get appid\n");
        return false;
    }

    std::string client_id = root.get("client_id", "null").asString();
    if (client_id == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get clientid\n");
        return false;
    }

    std::string remote = root.get("remote", "null").asString();
    if (remote == "null") {
        fprintf(stderr, "suzitrader init failed: Cannot get remote\n");
        return false;
    }

    int port = root.get("port", -1).asInt();
    if (port == -1) {
        fprintf(stderr, "suzitrader init failed: failed to get remote port\n");
        return false;
    }

    suziapi_ = CTiTdApi::CreateTiTdApi();
    if (suziapi_ == nullptr) {
        fprintf(stderr, "Failed to create titd api\n");
        return false;
    }

    titd::CTdReqUserLoginField req = {0};
    int ret = 0;
    strcpy(req.UserId, suzi_cfg_.userid.c_str());
    strcpy(req.Password, suzi_cfg_.password.c_str());
    strcpy(req.ProtocolInfo, suzi_cfg_.protocolinfo.c_str());
    strcpy(req.AppId, suzi_cfg_.appid.c_str());
    strcpy(req.AppIdIdentify, suzi_cfg_.appididentify.c_str());
    ret = suziapi_->ReqUserLogin(&req, orderhelper_.request_no);
    if (ret != 0) {
        fprintf(stderr, "suzitrader init failed: Failed to login\n");
        return false;
    }

    ret = status_wait(COUNTER_STATUS_LOGINED);
    if (ret != 0) {
        fprintf(stderr, "suzitrader init failed: Failed to login, status %d\n", read_counter_status());
        release();
        return false;
    }

    ret = suziapi_->GetClientNo((char*)client_id.c_str(), orderhelper_.client_no);
    if (ret != 0) {
        fprintf(stderr, "suzitrader init failed: Failed to get client no, ret %d\n", ret);
        release();
        return false;
    }

    n_rxq_ = get_nb_rxq(handle_);
    pss_rxtx_pkt_alloc(handle_, pkt_, MAX_ORDER_CNT, NULL);

    on_rx_burst_preparation();

    responsor_ = std::thread([&] {
        on_rx_burst();
    });

    return true;
}

void suzitrader::release()
{
    counter_status = COUNTER_STATUS_LOGOUT;
}

void suzitrader::OnConnected()
{
    status_wake(COUNTER_STATUS_CONNECTED);
    fprintf(stdout, "%s\n", __func__);
}

void suzitrader::OnDisconnected()
{
    status_wake(COUNTER_STATUS_DISCONNECTED);
    fprintf(stdout, "%s\n", __func__);
}

void suzitrader::OnRspUserLogin(CTdRspUserLoginField& pRspUserLogin, CTdRspInfoField& pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo.ErrorId != 0) {
        fprintf(stdout, "login failed: errid %d\n");
        status_wake(COUNTER_STATUS_LOGOUT);
        return;
    }
    max_order_id_ = pRspUserLogin.MaxLocalOrderNo;
    orderhelper_.session_no = pRspUserLogin.SessionNo;
    status_wake(COUNTER_STATUS_LOGINED);
}

void suzitrader::on_rx_burst() {
    while (ACCESS_ONCE(counter_status) == COUNTER_STATUS_LOGINED) {

    }
}

void refill(void* pkt)
{
    unsigned int i = 0;
    for (i = 0; i < MAX_ORDER_CNT; ++i) {
        struct ethhdr* ethheader = (struct ethhdr*)(pkt_[i]->payload);
        memcpy(ethheader, pkt, sizeof(struct ethhdr));
        memcpy(ethheader->h_dest, pkt->h_source, ETH_ALEN);
        memcpy(ethheader->h_source, pkt->h_dest, ETH_ALEN);
        
    }
}

bool suzitrader::on_recv_preparation_pkt(struct pss_pktbuf *buf, uint32_t dstip, uint16_t port)
{
    void *netpkt = buf->payload;
    struct ethhdr* ethheader = (struct ethhdr*)netpkt;
    if (ethheader->h_proto == ETH_P_IP) {
        struct iphdr* ipheader = (struct iphdr*)(ethheader + 1);
        if (ipheader->saddr == dstip && ipheader->protocol == 16) {
            struct udphdr* udpheader = (struct udphdr*)(ipheader + 1);
            if (udpheader->source == port) {
                refill(netpkt);
                return true;
            }
        }
    }
    return false;
}

void suzitrader::on_rx_burst_preparation(std::string remote, int port) {
    unsigned int i = 0;
    unsigned int recv = 0;
    bool done = false;

    struct pss_pktbuf pssbuf;
    struct pss_pktbuf* p_buf = {&pssbuf};
    uint32_t dstaddr = inet_addr(remote.c_str());
    do {
        uint16_t qi = 0;

        for (qi = 0; qi < n_rxq_; ++qi) {
            recv = pss_pkt_rx_burst_at(handle_, p_buf, qi, 1, NULL);
            done = on_recv_preparation_pkt(p_buf[0], dstaddr, port);
            if (done)
                break;
        }
    } while (true);
}



} // polaris