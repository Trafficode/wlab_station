/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: mqtt_worker.h
 * --------------------------------------------------------------------------*/
#ifndef MQTT_WORKER_H_
#define MQTT_WORKER_H_

#include <stdint.h>
#include <zephyr/net/mqtt.h>

#define MQTT_WORKER_CLIENT_ID           ("zephyrux")
#define MQTT_WORKER_MAX_TOPIC_LEN       (128)
#define MQTT_WORKER_MAX_PAYLOAD_LEN     (256)
#define MQTT_WORKER_MAX_PUBLISH_LEN     (512)
#define MQTT_WORKER_PUBLISH_ACK_TIMEOUT (4)  /* seconds */
#define MQTT_WORKER_PING_TIMEOUT        (60) /* secs */

typedef void (*subs_cb_t)(char *topic, uint16_t topic_len, char *payload,
                          uint16_t payload_len);

/**
 * @brief Initialize worker. All next action will be executed in separated
 * thread. Function is not blocked, to verify if driver is connected with broker
 * use api below.
 * @param hostname It can be name of host or string representation of ip addr.
 * @param port Broker port
 * @param subs Pointer to list of topics to be subscribed, NULL if no topics
 * should be subcribed
 * @param subs_cb Function to handle data incomming to subscribed topics, NULL
 * if not needed.
 */
void mqtt_worker_init(const char *hostname, int32_t port,
                      struct mqtt_subscription_list *subs, subs_cb_t subs_cb);

/**
 * @brief Publish data to given topic. Use in the same way as typical printf().
 * @param topic Topic where msg will be published
 */
int32_t mqtt_worker_publish_qos1(const char *topic, const char *fmt, ...);

/**
 * @brief Test if last keepalive response is longer than
 * CONFIG_MQTT_KEEPALIVE_TIMEOUT_MINS, if yes, then reboot device.
 *
 */
void mqtt_worker_keepalive_test(void);

#endif /* MQTT_WORKER_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/