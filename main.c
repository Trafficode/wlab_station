/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: main.c
 * --------------------------------------------------------------------------*/
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_version.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include "config_wifi.h"
#include "config_wlab.h"
#include "dht2x.h"
#include "mqtt_worker.h"
#include "nvs_data.h"
#include "timestamp.h"
#include "version.h"
#include "wdg.h"
#include "wifi_net.h"
#include "wlab.h"

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

static const struct gpio_dt_spec InfoLed =
    GPIO_DT_SPEC_GET(DT_NODELABEL(info_led), gpios);

static const struct gpio_dt_spec ConfigButton =
    GPIO_DT_SPEC_GET(DT_NODELABEL(config_btn), gpios);

static bool ConfigureMode = false;

int main(void) {
    int ret = 0;
    struct net_settings net_sett = {};
    wdg_init(CONFIG_WDG_TIMEOUT_SEC);

    uint32_t ver = sys_kernel_version_get();
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("Firmware: %s Zephyr: %u.%u.%u", FIRMWARE_VERSION,
            SYS_KERNEL_VER_MAJOR(ver), SYS_KERNEL_VER_MINOR(ver),
            SYS_KERNEL_VER_PATCHLEVEL(ver));

    ret = gpio_pin_configure_dt(&InfoLed, GPIO_OUTPUT_ACTIVE);
    __ASSERT((0 == ret), "Info led init failed");
    ret = gpio_pin_configure_dt(&ConfigButton, GPIO_INPUT);
    __ASSERT((0 == ret), "Config button init failed");

    nvs_data_init();
    nvs_data_net_settings_get(&net_sett);
    if (gpio_pin_get_dt(&ConfigButton) || (0 == net_sett.wifi_ssid[0])) {
        LOG_WRN("CONFIG MODE ENABLED");
        // ConfigureMode = true;
    }

    if (!ConfigureMode) {
        wifi_net_init(WIFI_SSID, WIFI_PASS);
        mqtt_worker_init(CONFIG_WLAB_MQTT_BROKER, CONFIG_WLAB_MQTT_BROKER_PORT,
                         NULL, NULL);
        timestamp_init();
        wlab_init();
    }

    for (;;) {
        k_sleep(K_MSEC(100));
        wdg_feed();
        gpio_pin_toggle_dt(&InfoLed);

        if (ConfigureMode) {
            continue;
        }

        if (gpio_pin_get_dt(&ConfigButton)) {
            sys_reboot(SYS_REBOOT_COLD);
        }

        int64_t ts_now = timestamp_get();
        wlab_process(ts_now);
        timestamp_update();
        mqtt_worker_keepalive_test();
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/