/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: wifi_net.c
 * --------------------------------------------------------------------------*/
#include "wifi_net.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/reboot.h>

#include "wdg.h"

LOG_MODULE_REGISTER(WIFI, LOG_LEVEL_DBG);

static K_SEM_DEFINE(wifi_connected, 0, 1);
static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);
static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);
static void handle_ipv4_result(struct net_if *iface);
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint32_t mgmt_event, struct net_if *iface);
static void wifi_status(void);
static void reconnect_work_handler(struct k_work *work);
static void reconnect_timer_handler(struct k_timer *dummy);

K_WORK_DEFINE(ReconnectWork, reconnect_work_handler);
K_TIMER_DEFINE(ReconnectTimer, reconnect_timer_handler, NULL);

static struct wifi_connect_req_params WifiInit = {0};
static char MacHexStr[13];

K_SEM_DEFINE(FirstConnSem, 0, 1);

static void (*DiscoCb)(int32_t) = NULL;

void net_on_disconnect_reqister(void (*disco_cb)(int32_t reason)) {
    DiscoCb = disco_cb;
}

void net_mac_string(char mac_buffer[13]) {
    memcpy(mac_buffer, MacHexStr, sizeof(MacHexStr));
}

void wifi_net_init(char *ssid, char *passwd) {
    net_mgmt_init_event_callback(
        &wifi_cb, wifi_mgmt_event_handler,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);

    net_mgmt_init_event_callback(&ipv4_cb, wifi_mgmt_event_handler,
                                 NET_EVENT_IPV4_ADDR_ADD);

    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_add_event_callback(&ipv4_cb);

    struct net_if *iface = net_if_get_default();
    WifiInit.ssid = (const uint8_t *)ssid;
    WifiInit.ssid_length = strlen(ssid);

    if (NULL != passwd) {
        WifiInit.security = WIFI_SECURITY_TYPE_PSK;
        WifiInit.psk = (uint8_t *)passwd;
        WifiInit.psk_length = strlen(passwd);
    }

    WifiInit.channel = WIFI_CHANNEL_ANY;
    WifiInit.band = WIFI_FREQ_BAND_2_4_GHZ;
    WifiInit.mfp = WIFI_MFP_OPTIONAL;

    LOG_INF("Connecting to SSID: %s", WifiInit.ssid);

    uint8_t mac_address[6];
    struct net_linkaddr *addr = net_if_get_link_addr(iface);
    memcpy(mac_address, addr->addr, sizeof(mac_address));
    sprintf(MacHexStr, "%02X%02X%02X%02X%02X%02X", mac_address[0],
            mac_address[1], mac_address[2], mac_address[3], mac_address[4],
            mac_address[5]);
    LOG_INF("WIFI_MAC: %s", MacHexStr);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &WifiInit,
                 sizeof(struct wifi_connect_req_params))) {
        LOG_ERR("WiFi Connection Request Failed");
    }

    int32_t sec_cnt = 0;
    for (sec_cnt = 0; sec_cnt < CONFIG_WIFI_FIRST_CONN_TIMEOUT_SEC; sec_cnt++) {
        wdg_feed();
        if (0 == k_sem_take(&FirstConnSem, K_SECONDS(1))) {
            break;
        }
    }

    if (CONFIG_WIFI_FIRST_CONN_TIMEOUT_SEC == sec_cnt) {
        LOG_ERR("Wifi connection timeout");
        sys_reboot(SYS_REBOOT_COLD);
    }

    LOG_INF("Wifi connected");
    return;
}

static void reconnect_timer_handler(struct k_timer *dummy) {
    k_work_submit(&ReconnectWork);
}

static void reconnect_work_handler(struct k_work *work) {
    struct net_if *iface = net_if_get_default();

    LOG_INF("Make wifi connection attempt...");
    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &WifiInit,
                 sizeof(struct wifi_connect_req_params))) {
        LOG_ERR("WiFi Connection Request Failed");
    }
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb) {
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status) {
        LOG_INF("Connection request failed (%d)", status->status);
        k_timer_start(&ReconnectTimer, K_SECONDS(4), K_NO_WAIT);
    } else {
        LOG_INF("Connected");
        wifi_status();
    }
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb) {
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status) {
        LOG_INF("Disconnection request (%d)", status->status);
    } else {
        LOG_INF("Disconnected");
    }

    if (NULL != DiscoCb) {
        DiscoCb(status->status);
    }

    /* one shot timer */
    k_timer_start(&ReconnectTimer, K_SECONDS(4), K_NO_WAIT);
}

static void handle_ipv4_result(struct net_if *iface) {
    int32_t i = 0;

    LOG_INF("handle_ipv4_result");
    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        char buf[NET_IPV4_ADDR_LEN];

        if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP) {
            continue;
        }

        LOG_INF("IPv4 address: %s",
                net_addr_ntop(
                    AF_INET, &iface->config.ip.ipv4->unicast[i].address.in_addr,
                    buf, sizeof(buf)));
        LOG_INF("Subnet: %s",
                net_addr_ntop(AF_INET, &iface->config.ip.ipv4->netmask, buf,
                              sizeof(buf)));
        LOG_INF("Router: %s", net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
                                            buf, sizeof(buf)));
        k_sem_give(&FirstConnSem);
    }
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint32_t mgmt_event, struct net_if *iface) {
    switch (mgmt_event) {
        case NET_EVENT_WIFI_CONNECT_RESULT: {
            handle_wifi_connect_result(cb);
            break;
        }
        case NET_EVENT_WIFI_DISCONNECT_RESULT: {
            handle_wifi_disconnect_result(cb);
            break;
        }
        case NET_EVENT_IPV4_ADDR_ADD: {
            handle_ipv4_result(iface);
            break;
        }
        default: {
            LOG_ERR("Unknown mgmt_event event: %d", mgmt_event);
            break;
        }
    }
}

static void wifi_status(void) {
    struct net_if *iface = net_if_get_default();

    struct wifi_iface_status status = {0};

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                 sizeof(struct wifi_iface_status))) {
        LOG_INF("WiFi Status Request Failed\n");
    }

    if (status.state >= WIFI_STATE_ASSOCIATED) {
        LOG_INF("SSID: %-32s", status.ssid);
        LOG_INF("Band: %s", wifi_band_txt(status.band));
        LOG_INF("Channel: %d", status.channel);
        LOG_INF("Security: %s", wifi_security_txt(status.security));
        LOG_INF("RSSI: %d", status.rssi);
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/