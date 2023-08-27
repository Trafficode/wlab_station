/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: nvs_data.h
 * --------------------------------------------------------------------------*/
#ifndef NVS_DATA_H_
#define NVS_DATA_H_

#include <stdint.h>

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_PARTITION_SIZE   FIXED_PARTITION_SIZE(NVS_PARTITION)

#define NVS_ID_BOOT_COUNT     (1)
#define NVS_ID_WIFI_CONFIG    (2)
#define NVS_ID_MQTT_CONFIG    (3)
#define NVS_ID_WLAB_DEVICE_ID (4)

struct wifi_config {
    char wifi_ssid[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
    char wifi_pass[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
};

struct mqtt_config {
    char mqtt_broker[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
    uint32_t mqtt_port;
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

#endif /* NVS_DATA_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/