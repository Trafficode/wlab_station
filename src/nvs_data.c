/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: nvs_data.c
 * --------------------------------------------------------------------------*/
#include "nvs_data.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(NVSDATA, LOG_LEVEL_DBG);

static struct nvs_fs Fs = {0};

void nvs_data_init(void) {
    int ret = 0;
    struct flash_pages_info info = {0};

    Fs.flash_device = NVS_PARTITION_DEVICE;
    ret = device_is_ready(Fs.flash_device);
    __ASSERT((ret != 0), "Flash not ready");

    Fs.offset = NVS_PARTITION_OFFSET;
    ret = flash_get_page_info_by_offs(Fs.flash_device, Fs.offset, &info);
    __ASSERT((0 == ret), "Unable to get page info err %d", ret);

    LOG_INF("NVS sector size %u part size %u", info.size, NVS_PARTITION_SIZE);

    Fs.sector_size = info.size;
    Fs.sector_count = NVS_PARTITION_SIZE / info.size;

    ret = nvs_mount(&Fs);
    __ASSERT((0 == ret), "Flash mount failed");

    uint32_t boot_counter = UINT32_C(0);
    size_t area_len = sizeof(boot_counter);
    ret = nvs_read(&Fs, NVS_ID_BOOT_COUNT, &boot_counter, area_len);
    if (ret > 0) { /* item was found, show it */
        LOG_INF("boot counter: %d", boot_counter);
    } else { /* item was not found, add it */
        LOG_INF("No boot counter found, adding it at id %d", NVS_ID_BOOT_COUNT);
    }
    boot_counter++;

    if (area_len ==
        nvs_write(&Fs, NVS_ID_BOOT_COUNT, &boot_counter, area_len)) {
        LOG_INF("Save boot counter %d succ", boot_counter);
    } else {
        LOG_ERR("Save boot counter %d err", boot_counter);
    }
}

void nvs_data_net_settings_get(struct net_settings *dst) {
    __ASSERT((dst != NULL), "null pointer passed");
    size_t net_settings_len = sizeof(struct net_settings);

    int ret = nvs_read(&Fs, NVS_ID_NET_SETTINGS, dst, net_settings_len);
    if (ret > 0) { /* item was found, show it */
        LOG_INF("net_settings->wifi_ssid: %s", dst->wifi_ssid);
        LOG_INF("net_settings->wifi_ssid: %s", dst->wifi_pass);
        LOG_INF("net_settings->wifi_ssid: %s", dst->mqtt_broker);
        LOG_INF("net_settings->wifi_ssid: %u", dst->mqtt_port);
    } else { /* item was not found, add it */
        LOG_WRN("No net_settings found, restore default");
        memset(dst, 0x00, sizeof(struct net_settings));
        if (net_settings_len ==
            nvs_write(&Fs, NVS_ID_NET_SETTINGS, dst, net_settings_len)) {
            LOG_INF("Network settings cleared");
        } else {
            LOG_ERR("Network settings clear failed");
        }
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/