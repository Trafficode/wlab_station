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

int main(void) {
    wdg_init(CONFIG_WDG_TIMEOUT_SEC);

    uint32_t ver = sys_kernel_version_get();
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("Firmware: %s Zephyr: %u.%u.%u", FIRMWARE_VERSION,
            SYS_KERNEL_VER_MAJOR(ver), SYS_KERNEL_VER_MINOR(ver),
            SYS_KERNEL_VER_PATCHLEVEL(ver));

    gpio_pin_configure_dt(&InfoLed, GPIO_OUTPUT_ACTIVE);
    nvs_data_init();
    wifi_net_init(WIFI_SSID, WIFI_PASS);
    mqtt_worker_init(CONFIG_WLAB_MQTT_BROKER, CONFIG_WLAB_MQTT_BROKER_PORT,
                     NULL, NULL);
    timestamp_init();
    wlab_init();

    int64_t ts_now = 0;
    for (;;) {
        k_sleep(K_MSEC(100));
        gpio_pin_toggle_dt(&InfoLed);

        ts_now = timestamp_get();
        wlab_process(ts_now);
        timestamp_update();
        mqtt_worker_keepalive_test();
        wdg_feed();
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/