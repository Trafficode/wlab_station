/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: wdg.c
 * --------------------------------------------------------------------------*/
#include "wdg.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(WDG, LOG_LEVEL_DBG);

static const struct device *const Wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

int32_t WdtChannelId = 0;

void wdg_init(uint32_t timeout_sec) {
    if (!device_is_ready(Wdt)) {
        LOG_ERR("%s: device not ready", Wdt->name);
        while (true)
            ;
    }

    struct wdt_timeout_cfg wdt_config = {
        /* Reset SoC when watchdog timer expires. */
        .flags = WDT_FLAG_RESET_SOC,

        /* Expire watchdog after max window */
        .window.min = 0,                  /* for esp32 it has to be 0 */
        .window.max = 1000 * timeout_sec, /* millis */
        .callback = NULL,
    };

    WdtChannelId = wdt_install_timeout(Wdt, &wdt_config);
    if (WdtChannelId < 0) {
        LOG_ERR("Watchdog install error");
        while (true)
            ;
    }

    int32_t ret = wdt_setup(Wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (ret < 0) {
        LOG_ERR("Watchdog setup error");
        while (true)
            ;
    }
}

void wdg_feed(void) {
    wdt_feed(Wdt, WdtChannelId);
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/