/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: mqtt_worker.c
 * --------------------------------------------------------------------------*/
#include "mqtt_worker.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socketutils.h>
#include <zephyr/sys/reboot.h>

#include "nvs_data.h"
#include "wdg.h"

LOG_MODULE_REGISTER(MQTT, LOG_LEVEL_DBG);

typedef struct subs_data {
    uint8_t topic[MQTT_WORKER_MAX_TOPIC_LEN];
    uint16_t topic_len;
    uint8_t payload[MQTT_WORKER_MAX_PAYLOAD_LEN];
    uint16_t payload_len;
} subs_data_t;

typedef enum worker_state {
    DNS_RESOLVE,
    CONNECT_TO_BROKER,
    SUBSCRIBE,
    CONNECTED,
    DISCONNECTED
} worker_state_t;

static void mqtt_evt_handler(struct mqtt_client *const client,
                             const struct mqtt_evt *evt);
static void mqtt_worker_disconnect(int32_t reason);

static int32_t wait_for_input(int32_t timeout);
static int32_t dns_resolve(void);
static int32_t connect_to_broker(void);
static int32_t input_handle(void);
static int32_t mqtt_worker_subscribe(void);

static void mqtt_proc(void *, void *, void *);
static void subscribe_proc(void *, void *, void *);

/* The mqtt client struct */
static struct mqtt_client ClientCtx;

/* MQTT Broker details. */
static struct sockaddr_storage Broker;

/* Buffers for MQTT client. */
static uint8_t RxBuffer[1024];
static uint8_t TxBuffer[1024];

static char BrokerHostnameStr[32];
static char BrokerAddrStr[16];
static char BrokerPortStr[32];
static int32_t BrokerPort = 0;
struct mqtt_subscription_list *SubsList = NULL;
static struct mqtt_publish_param PubData = {0};
static enum mqtt_evt_type LastEvt = 0;

static worker_state_t StateMachine = DNS_RESOLVE;
static char PublishBuffer[MQTT_WORKER_MAX_PUBLISH_LEN];

static bool Connected = false;
static bool DisconnectReqExternal = false;
static bool Subscribed = false;

static subs_cb_t SubsCb = NULL;

#define MQTT_NET_STACK_SIZE (2 * 1024)
#define MQTT_NET_PRIORITY   (5)
K_THREAD_DEFINE(MqttNetTid, MQTT_NET_STACK_SIZE, mqtt_proc, NULL, NULL, NULL,
                MQTT_NET_PRIORITY, 0, 0);

#define SUBSCRIBE_STACK_SIZE (2 * 1024)
#define SUBSCRIBE_PRIORITY   (5)
K_THREAD_DEFINE(SubsTid, SUBSCRIBE_STACK_SIZE, subscribe_proc, NULL, NULL, NULL,
                SUBSCRIBE_PRIORITY, 0, 0);

K_SEM_DEFINE(PublishAckSem, 0, 1);
K_SEM_DEFINE(ConectedAckSem, 0, 1);
K_SEM_DEFINE(WorkerProcStartSem, 0, 1);
K_MSGQ_DEFINE(SubsQueue, sizeof(subs_data_t *), 4, 4);
K_MEM_SLAB_DEFINE_STATIC(SubsQueueSlab, sizeof(subs_data_t), 4, 4);

static int64_t LastKeepaliveResp = 0;
static uint32_t PingPeriodSec = 0;
static uint32_t MaxPingNoAnsMins = 0;

/**
 * @brief This function has to be delivered by network layer(wifi, modem) to
 * notify mqtt worker about disconnection event
 *
 * @param disco_cb
 */
extern void net_on_disconnect_reqister(void (*disco_cb)(int reason));

void mqtt_worker_keepalive_test(void) {
    /* wait 2 hours for mqtt connection, samples has to be stored in nvs */
    int64_t mqtt_alive_timeout = MaxPingNoAnsMins * 60 * 1000;
    if (k_uptime_get() > LastKeepaliveResp + mqtt_alive_timeout) {
        sys_reboot(SYS_REBOOT_COLD);
    }
}

int mqtt_worker_publish_qos1(const char *topic, const char *fmt, ...) {
    int ret = 0;

    va_list args;
    va_start(args, fmt);

    if (!Connected || DisconnectReqExternal) {
        LOG_WRN("Cannot publish, client not connected");
        ret = -ENETUNREACH;
        goto failed_done;
    }

    uint32_t len =
        vsnprintf(PublishBuffer, MQTT_WORKER_MAX_PUBLISH_LEN, fmt, args);

    PubData.message.payload.len = len;
    PubData.message.topic.topic.utf8 = (uint8_t *)topic;
    PubData.message.topic.topic.size = strlen((const char *)topic);
    PubData.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
    PubData.message_id += 1U;

    struct mqtt_client *client = &ClientCtx;

    k_sem_take(&PublishAckSem, K_NO_WAIT);
    ret = mqtt_publish(client, &PubData);
    if (ret != 0) {
        LOG_ERR("could not publish, err %d", ret);
        goto failed_done;
    }

    ret =
        k_sem_take(&PublishAckSem, K_SECONDS(MQTT_WORKER_PUBLISH_ACK_TIMEOUT));
    if (ret != 0) {
        LOG_ERR("publish ack timeout");
    }

failed_done:
    va_end(args);
    return (ret);
}

void mqtt_worker_init(const char *hostname, int32_t port, uint32_t ping_period,
                      uint32_t max_ping_no_answer,
                      struct mqtt_subscription_list *subs, subs_cb_t subs_cb) {
    struct mqtt_client *client = &ClientCtx;

    MaxPingNoAnsMins = max_ping_no_answer;
    PingPeriodSec = ping_period;
    SubsCb = subs_cb;
    SubsList = subs;
    strncpy(BrokerHostnameStr, hostname, sizeof(BrokerHostnameStr));
    BrokerPort = port;
    snprintf(BrokerPortStr, sizeof(BrokerPortStr), "%d", port);

    mqtt_client_init(client);

    /* MQTT client configuration */
    client->broker = &Broker;
    client->evt_cb = mqtt_evt_handler;
    client->protocol_version = MQTT_VERSION_3_1_1;
    client->client_id.utf8 = (uint8_t *)MQTT_WORKER_CLIENT_ID;
    client->client_id.size = strlen(MQTT_WORKER_CLIENT_ID);
    client->password = NULL;
    client->user_name = NULL;

    /* MQTT buffers configuration */
    client->rx_buf = RxBuffer;
    client->rx_buf_size = sizeof(RxBuffer);
    client->tx_buf = TxBuffer;
    client->tx_buf_size = sizeof(TxBuffer);

    PubData.message.payload.data = (uint8_t *)PublishBuffer;
    PubData.message_id = 1U;
    PubData.dup_flag = 0U;
    PubData.retain_flag = 1U;

    net_on_disconnect_reqister(mqtt_worker_disconnect);
    k_sem_give(&WorkerProcStartSem);

    int32_t sec_cnt = 0;
    for (sec_cnt = 0; sec_cnt < CONFIG_MQTT_FIRST_CONN_TIMEOUT_SEC; sec_cnt++) {
        wdg_feed();
        if (0 == k_sem_take(&ConectedAckSem, K_SECONDS(1))) {
            break;
        }
    }
    __ASSERT((CONFIG_MQTT_FIRST_CONN_TIMEOUT_SEC != sec_cnt),
             "Mqtt connection timeout");
}

static void subscribe_proc(void *arg1, void *arg2, void *arg3) {
    subs_data_t *subs_data = NULL;
    for (;;) {
        if (0 == k_msgq_get(&SubsQueue, &subs_data, K_SECONDS(1))) {
            /* handle incomming message here*/
            if (NULL != SubsCb) {
                SubsCb((char *)subs_data->topic, subs_data->topic_len,
                       (char *)subs_data->payload, subs_data->payload_len);
            }
            k_mem_slab_free(&SubsQueueSlab, (void **)&subs_data);
        }
    }
}

static void mqtt_proc(void *arg1, void *arg2, void *arg3) {
    StateMachine = DNS_RESOLVE;
    Connected = false;

    k_sem_take(&WorkerProcStartSem, K_FOREVER);
    for (;;) {
        switch (StateMachine) {
            case DNS_RESOLVE: {
                LOG_INF("DNS_RESOLVE");
                int ret = dns_resolve();
                if (0 == ret) {
                    StateMachine = CONNECT_TO_BROKER;
                } else {
                    k_sleep(K_SECONDS(2));
                }
                break;
            }
            case CONNECT_TO_BROKER: {
                LOG_INF("CONNECT_TO_BROKER");
                static int32_t err_trials = 0;
                int res = connect_to_broker();
                if (0 == res) {
                    LOG_INF("MQTT client connected!");
                    StateMachine = SUBSCRIBE;
                    err_trials = 0;
                } else {
                    err_trials++;
                    if (4 == err_trials) {
                        StateMachine = DNS_RESOLVE;
                        err_trials = 0;
                    }
                    k_sleep(K_SECONDS(2));
                }
                break;
            }
            case SUBSCRIBE: {
                LOG_INF("SUBSCRIBE");
                static int32_t err_trials = 0;
                int ret = mqtt_worker_subscribe();
                if (0 == ret) {
                    LOG_INF("Subscribe done");
                    StateMachine = CONNECTED;
                    err_trials = 0;
                } else {
                    err_trials++;
                    if (4 == err_trials) {
                        StateMachine = DNS_RESOLVE;
                        err_trials = 0;
                    }
                }
            }
            case CONNECTED: {
                LOG_DBG("CONNECTED");
                int ret = input_handle();
                if (0 != ret) {
                    StateMachine = DNS_RESOLVE;
                }
                break;
            }
            case DISCONNECTED: {
                k_sleep(K_MSEC(10));
            }
            default: {
                break;
            }
        }
    }
}

static void mqtt_worker_disconnect(int32_t reason) {
    DisconnectReqExternal = true;
}

static int mqtt_worker_subscribe(void) {
    struct mqtt_client *client = &ClientCtx;
    int ret = 0;

    if (NULL == SubsList) {
        LOG_WRN("Subscription list empty");
        goto failed_done;
    }

    Subscribed = false;
    ret = mqtt_subscribe(client, SubsList);
    if (0 != ret) {
        LOG_ERR("Failed to subscribe topics, err %d", ret);
        goto failed_done;
    }

    while (true) {
        LastEvt = 0xFF;
        ret = wait_for_input(4000);
        if (0 < ret) {
            mqtt_input(client);
            if (LastEvt != MQTT_EVT_SUBACK && LastEvt != 0xFF) {
                LOG_WRN("Unexpected event got, try again");
                continue;
            }
        }
        break;
    }

    if (!Subscribed) {
        LOG_ERR("Subscribe timeout");
    } else {
        ret = 0;
    }

failed_done:
    return (ret);
}

static int wait_for_input(int32_t timeout) {
#if defined(CONFIG_MQTT_LIB_TLS)
    struct zsock_pollfd fds[1] = {
        [0] =
            {
                .fd = ClientCtx.transport.tls.sock,
                .events = ZSOCK_POLLIN,
                .revents = 0,
            },
    };
#else
    struct zsock_pollfd fds[1] = {
        [0] =
            {
                .fd = ClientCtx.transport.tcp.sock,
                .events = ZSOCK_POLLIN,
                .revents = 0,
            },
    };
#endif

    int ret = zsock_poll(fds, 1, timeout);
    if (0 > ret) {
        LOG_ERR("zsock_poll event err %d", ret);
    }

    return (ret);
}

static int connect_to_broker(void) {
    struct mqtt_client *client = &ClientCtx;
    int ret = 0;

    DisconnectReqExternal = false;
    Connected = false;
    ret = mqtt_connect(client);
    if (ret != 0) {
        LOG_ERR("mqtt_connect, err %d", ret);
        mqtt_disconnect(client);
        goto failed_done;
    }

    ret = wait_for_input(2000);
    if (0 < ret) {
        mqtt_input(client);
    }

    if (!Connected) {
        LOG_ERR("Connection timeout, abort...");
        mqtt_abort(client);
        ret = -1;
    } else {
        ret = 0;
    }

failed_done:
    return (ret);
}

static int input_handle(void) {
    int ret = 0;
    struct mqtt_client *client = &ClientCtx;
    static int64_t next_alive = INT64_MIN;

    /* idle and process messages */
    int64_t uptime_ms = k_uptime_get();
    if (uptime_ms < next_alive) {
        ret = wait_for_input(1 * MSEC_PER_SEC);
        if (0 < ret) {
            mqtt_input(client);
        }

        if (!Connected) {
            ret = -1;
            goto failed_done;
        }

        if (DisconnectReqExternal) {
            mqtt_disconnect(client);
            k_sem_take(&ConectedAckSem, K_NO_WAIT);
            Connected = false;
            ret = -1;
            goto failed_done;
        }
    } else {
        LOG_INF("Keepalive...");
        mqtt_live(client);
        next_alive = uptime_ms + (PingPeriodSec * MSEC_PER_SEC);
    }

    ret = 0; /* success done */

failed_done:
    return (ret);
}

static int dns_resolve(void) {
    static struct zsock_addrinfo hints;
    struct zsock_addrinfo *haddr;
    int ret = 0;
    uint8_t *in_addr = NULL;

    memset(&Broker, 0, sizeof(struct sockaddr_storage));
    struct sockaddr_in *ipv4_broker = (struct sockaddr_in *)&Broker;

    ipv4_broker->sin_family = AF_INET;
    ipv4_broker->sin_port = htons(BrokerPort);
    ret = zsock_inet_pton(AF_INET, BrokerHostnameStr, &ipv4_broker->sin_addr);
    if (ret != 0) {
        in_addr = ipv4_broker->sin_addr.s4_addr;
        LOG_INF("Broker addr %d.%d.%d.%d", in_addr[0], in_addr[1], in_addr[2],
                in_addr[3]);
        ret = 0; /* 0 - success, string ip address delivered, dns not needed */
        goto resolve_done;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    ret = net_getaddrinfo_addr_str(BrokerHostnameStr, BrokerPortStr, &hints,
                                   &haddr);
    if (ret != 0) {
        LOG_ERR("Unable to get address of broker, err %d", ret);
        goto resolve_done;
    }

    ipv4_broker->sin_family = AF_INET;
    ipv4_broker->sin_port = htons(BrokerPort);
    net_ipaddr_copy(&ipv4_broker->sin_addr, &net_sin(haddr->ai_addr)->sin_addr);
    in_addr = ipv4_broker->sin_addr.s4_addr;
    LOG_INF("Broker addr %d.%d.%d.%d", in_addr[0], in_addr[1], in_addr[2],
            in_addr[3]);

resolve_done:
    return (ret);
}

static void mqtt_evt_handler(struct mqtt_client *const client,
                             const struct mqtt_evt *evt) {
    LOG_INF("mqtt_evt_handler");
    LastEvt = evt->type;

    switch (evt->type) {
        case MQTT_EVT_SUBACK: {
            LOG_INF("MQTT_EVT_SUBACK");
            Subscribed = true;
            break;
        }
        case MQTT_EVT_UNSUBACK: {
            LOG_INF("MQTT_EVT_UNSUBACK");
            break;
        }
        case MQTT_EVT_CONNACK: {
            if (evt->result != 0) {
                LOG_ERR("MQTT connect failed %d", evt->result);
            } else {
                Connected = true;
                LastKeepaliveResp = k_uptime_get();
                k_sem_give(&ConectedAckSem);
            }
            break;
        }
        case MQTT_EVT_DISCONNECT: {
            LOG_ERR("MQTT client disconnected %d", evt->result);
            Connected = false;
            k_sem_take(&ConectedAckSem, K_NO_WAIT);
            break;
        }
        case MQTT_EVT_PUBLISH: {
            LOG_INF("MQTT_EVT_PUBLISH");
            subs_data_t *subs_data = NULL;
            const struct mqtt_publish_param *pub = &evt->param.publish;
            int32_t len = pub->message.payload.len;
            int32_t bytes_read = INT32_MIN;
            bool cleanup_and_break = false;

            LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
            LOG_INF("   id: %d, qos: %d", pub->message_id,
                    pub->message.topic.qos);
            LOG_INF("   topic: %s", pub->message.topic.topic.utf8);

            if (0 != k_mem_slab_alloc(&SubsQueueSlab, (void **)&subs_data,
                                      K_MSEC(1000))) {
                LOG_ERR("Get free subs slab failed");
                cleanup_and_break = true;
            }

            if (MQTT_WORKER_MAX_PAYLOAD_LEN < (pub->message.payload.len + 1)) {
                LOG_ERR("Payload to long %d", len);
                cleanup_and_break = true;
            }

            if (MQTT_WORKER_MAX_TOPIC_LEN < pub->message.topic.topic.size) {
                LOG_ERR("Topic to long %d", pub->message.topic.topic.size);
                cleanup_and_break = true;
            }

            if (!Connected) {
                LOG_WRN("Not connected yet");
                cleanup_and_break = true;
            }

            if (cleanup_and_break) {
                uint8_t tmp[32];
                if (NULL != subs_data) {
                    k_mem_slab_free(&SubsQueueSlab, (void **)&subs_data);
                }

                while (len) { /* cleanup mqtt socket buffer */
                    bytes_read = mqtt_read_publish_payload_blocking(
                        client, tmp, len >= 32 ? 32 : len);
                    if (0 > bytes_read) {
                        break;
                    }
                    len -= bytes_read;
                }
                break;
            }

            /* assuming the config message is textual */
            int32_t read_idx = 0;
            while (len) {
                bytes_read = mqtt_read_publish_payload_blocking(
                    client, subs_data->payload + read_idx,
                    len >= 32 ? 32 : len);

                if (0 > bytes_read) {
                    LOG_ERR("Failure to read payload");
                    break;
                }

                len -= bytes_read;
                read_idx += bytes_read;
            }

            subs_data->payload[pub->message.payload.len] = '\0';
            subs_data->payload_len = pub->message.payload.len;
            subs_data->topic_len = pub->message.topic.topic.size;
            if (0 <= bytes_read) {
                LOG_INF("   payload: %s", subs_data->payload);
                memcpy(subs_data->topic, pub->message.topic.topic.utf8,
                       subs_data->topic_len);
                subs_data->topic[subs_data->topic_len] = '\0';
                int32_t ret = k_msgq_put(&SubsQueue, &subs_data, K_MSEC(1000));
                if (0 != ret) {
                    LOG_ERR("Timeout to put subs msg into queue");
                }
            }

            break;
        }
        case MQTT_EVT_PUBACK: {
            if (evt->result != 0) {
                LOG_ERR("PUBACK error %d", evt->result);
            } else {
                LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
                k_sem_give(&PublishAckSem);
            }
            break;
        }
        case MQTT_EVT_PUBREC: {
            LOG_INF("MQTT_EVT_PUBREC");
            break;
        }
        case MQTT_EVT_PUBREL: {
            LOG_INF("MQTT_EVT_PUBREL");
            break;
        }
        case MQTT_EVT_PUBCOMP: {
            LOG_INF("MQTT_EVT_PUBCOMP");
            break;
        }
        case MQTT_EVT_PINGRESP: {
            LOG_INF("MQTT_EVT_PINGRESP");
            LastKeepaliveResp = k_uptime_get();
            break;
        }
        default: {
            LOG_WRN("MQTT event unknown %d", evt->type);
            break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/