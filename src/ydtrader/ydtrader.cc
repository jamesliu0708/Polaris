#include "ydtrader.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <rte_cfgfile.h>
#include "ydDataType.h"
#include "ydApi.h"

namespace polaris {

#define YD_EXIT_WITH_WARNING(args...) do {    \
    fprintf(stderr, args);    \
    exit(-1);   \
} while (0)

__attribute__((visibility("hidden"))) struct ydapi_configuration {
    uint8_t conn_t;
    uint8_t conn_id;
    std::string user;
    std::string passwrd;
    std::string ydcfg;
};

namespace {

enum trader_status_enum {
    TRADER_STATUS_RELEASE = -1,
    TRADER_STATUS_NOT_READY = 0,
    TRADER_STATUS_CONNECTED,
    TRADER_STATUS_LOGINED,
    TRADER_STATUS_READY
};

uint64_t now() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

using condition_t = std::function<bool(void* args)>;
inline bool poll_(condition_t condition, void * args, unsigned int timeout) {
    auto begin = now();
    while (!condition(args)) {
        auto end = now();
        if (end - begin > timeout) 
            return false;
        sleep(0);
    }
    return true;
}
}

ydtrader::ydtrader(const char* cfgfilename, struct pss_ports* port):
    port_(port)
{
    int ret;
    bool success;
    const char* sec_name;
    const char* entry;
    unsigned int i;
    struct ydapi_configuration ydapi_cfg;
    struct rte_cfgfile *file = rte_cfgfile_load(cfgfilename, 0);
    if (file == nullptr) 
        YD_EXIT_WITH_WARNING("failed to load cfgfile %s\n", cfgfilename)

    sec_name = "yd";
    ret = rte_cfgfile_has_section(file, sec_name);
    if (ret) 
        YD_EXIT_WITH_WARNING("failed to get yd section\n");
    
    entry = rte_cfgfile_get_entry(file, sec_name, "conn_type");
    if (entry == NULL) {
        ydapi_cfg.conn_t = (uint8_t)YD_CS_Any;
        ydapi_cfg.conn_id = -1;
    } else {
        ydapi_cfg.conn_t = (uint8_t)(entry[0] - '0');
        entry = rte_cfgfile_get_entry(file, sec_name, "conn_id");
        if (entry == NULL) 
            YD_EXIT_WITH_WARNING("failed to get conn_id\n");
        ydapi_cfg.conn_id = (uint8_t)(entry[0] - '0');
    }

    entry = rte_cfgfile_get_entry(file, sec_name, "cfg");
    if (entry == nullptr) 
        YD_EXIT_WITH_WARNING("failed to get yd cfg\n");
    ydapi_cfg.ydcfg = entry;

    api_ = makeYDApi(ydapi_cfg.ydcfg.c_str());
    if (api_ == nullptr) 
        YD_EXIT_WITH_WARNING("Failed to create yd api\n");
    
    if (api_->start(this) == false)
        YD_EXIT_WITH_WARNING("Failed to init yd api\n");

    memset(ydorders_, 0, sizeof(struct yd_order_raw) * YDORDER_CACHE);

    entry = rte_cfgfile_get_entry(file, sec_name, "username");
    if (entry == nullptr) 
        YD_EXIT_WITH_WARNING("Failed to get username cfg\n");
    ydapi_cfg.user = entry;

    entry = rte_cfgfile_get_entry(file, sec_name, "passwrd");
    if (entry == nullptr) 
        YD_EXIT_WITH_WARNING("Failed to get passwrd cfg\n");
    ydapi_cfg.passwrd = entry;

    api_->login(ydapi_cfg.user.c_str(), ydapi_cfg.passwrd.c_str(), NULL, NULL);
    if (!(success = poll_([&](void* args) {
        (void)args;
        if (this->status_.load(std::memory_acquire) != TRADER_STATUS_READY)
            return false;
        return true;
    }, NULL, 3 * 1000000000))) 
        YD_EXIT_WITH_WARNING("failed to login\n");

    char client_header[16];
    if (api_->getClientPacketHeader(YDApi::YD_CLIENT_PACKET_INSERT_ORDER, 
                                    reinterpret_cast<unsigned char*>(client_header), 
                                    CLIENTHEADERLEN) == 0) 
        YD_EXIT_WITH_WARNING("Failed to get client packet header");
    
    for (i = 0; i < YDORDER_CACHE; ++i)
        memcpy(ydorders_.client_header, client_header, 16);
    
}

bool ydtrader::init()
{
    return true;
}

bool ydtrader::release()
{
    bool success;
    api_->startDestroy();
    if (!(success = poll_([&](void* args) {
        (void)args;
        if (this->status_.load(std::memory_acquire) != TRADER_STATUS_RELEASE)
            return false;
        return true;
    }, NULL, 3 * 1000000000))) 
        YD_EXIT_WITH_WARNING("failed to release\n");
}

void ydtrader::notifyReadyForLogin()
{
    fprintf(stdout, "%s", __func__);
    status_.store(TRADER_STATUS_CONNECTED);
}

void ydtrader::notifyLogin()
{
    fprintf(stdout, "%s", __func__);
    status_.store(TRADER_STATUS_LOGINED);
}

void ydtrader::notifyFinishInit()
{
    fprintf(stdout, "%s", __func__);
}

void ydtrader::notifyCaughtUp()
{
    fprintf(stdout, "%s", __func__);
    status_.store(TRADER_STATUS_READY);
}

void ydtrader::notifyBeforeApiDestroy()
{
    fprintf(stdout, "%s", __func__);
}

void ydtrader::notifyAfterApiDestroy()
{
    status_.store(TRADER_STATUS_RELEASE, std::memory_order_release);
}

} // polaris