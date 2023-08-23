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

LOG_MODULE_REGISTER(NVSDATA, LOG_LEVEL_DBG);

static struct nvs_fs Fs = {0};

void nvs_data_init(void) {
    struct flash_pages_info info = {0};
    Fs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(Fs.flash_device)) {
        LOG_ERR("Flash device %s is not ready", Fs.flash_device->name);
        while (true)
            ;
    }
    Fs.offset = NVS_PARTITION_OFFSET;
    int32_t ret =
        flash_get_page_info_by_offs(Fs.flash_device, Fs.offset, &info);
    if (ret) {
        LOG_ERR("Unable to get page info err %d", ret);
        while (true)
            ;
    }
    LOG_INF("NVS sector size %u part size %u", info.size, NVS_PARTITION_SIZE);

    Fs.sector_size = info.size;
    Fs.sector_count = NVS_PARTITION_SIZE / info.size;

    ret = nvs_mount(&Fs);
    if (0 != ret) {
        LOG_ERR("Flash Init failed err %d", ret);
        while (true)
            ;
    }

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

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/