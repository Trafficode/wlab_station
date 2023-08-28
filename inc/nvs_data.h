/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: nvs_data.h
 * --------------------------------------------------------------------------*/
#ifndef NVS_DATA_H_
#define NVS_DATA_H_

#include <stdint.h>
#include <zephyr/device.h>

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_PARTITION_SIZE   FIXED_PARTITION_SIZE(NVS_PARTITION)

#define NVS_ID_BOOT_COUNT        (1)
#define NVS_ID_WIFI_CONFIG       (2)
#define NVS_ID_MQTT_CONFIG       (3)
#define NVS_ID_WLAB_DEVICE_ID    (4)
#define NVS_ID_WLAB_NAME         (5)
#define NVS_ID_WLAB_GPS_POSITION (6)
#define NVS_ID_WLAB_PUB_PERIOD   (7)

struct wifi_config {
    char wifi_ssid[CONFIG_BUFF_MAX_STRING_LEN];
    char wifi_pass[CONFIG_BUFF_MAX_STRING_LEN];
};

struct mqtt_config {
    char mqtt_broker[CONFIG_BUFF_MAX_STRING_LEN];
    uint32_t mqtt_port;
    uint32_t mqtt_ping_period;        /* secs */
    uint32_t mqtt_max_ping_no_answer; /* mins */
};

struct gps_position {
    float latitude;
    float longitude;
    char timezone[CONFIG_BUFF_MAX_STRING_LEN];
};

/**
 * @brief Initialise none volatile storage space.
 *
 */
void nvs_data_init(void);

/**
 * @brief Read wifi settings data, save in dst pointer
 *
 * @param dst Pointer to save settings.
 */
void nvs_data_wifi_config_get(struct wifi_config *dst);

/**
 * @brief Read mqtt settings data, save in dst pointer
 *
 * @param dst Pointer to save settings.
 */
void nvs_data_mqtt_config_get(struct mqtt_config *dst);

/**
 * @brief Read wlab device id, if not exists, save UINT64_MAX
 *
 * @param device_id Pointer to save device_id.
 */
void nvs_data_wlab_device_id_get(uint64_t *device_id);

/**
 * @brief Read wlab name, if not exists, copy WLAB_STATION to dst as default
 * value.
 *
 * @param dst Destination of wlab_name with min size CONFIG_BUFF_MAX_STRING_LEN
 */
void nvs_data_wlab_name_get(char *dst);

/**
 * @brief Read wlab gps position, if not exists, read and save default datata
 *
 * @param gps_pos Destination of wlab gps position
 */
void nvs_data_wlab_gps_position_get(struct gps_position *gps_pos);

/**
 * @brief Read wlab publish period.
 *
 * @param pub_period Destination of wlab gps position
 */
void nvs_data_wlab_pub_period_get(uint32_t *pub_period);

#endif /* NVS_DATA_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/