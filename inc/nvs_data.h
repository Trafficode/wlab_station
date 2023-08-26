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

#define NVS_ID_BOOT_COUNT   (1)
#define NVS_ID_NET_SETTINGS (2)

struct net_settings {
    char wifi_ssid[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
    char wifi_pass[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
    char mqtt_broker[CONFIG_NET_SETTINGS_MAX_STRING_LEN];
    uint32_t mqtt_port;
};

/**
 * @brief Initialise none volatile storage space.
 *
 */
void nvs_data_init(void);

/**
 * @brief Read network settings data, save in dst pointer
 *
 * @param dst Pointer to save settings.
 */
void nvs_data_net_settings_get(struct net_settings *dst);

#endif /* NVS_DATA_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/