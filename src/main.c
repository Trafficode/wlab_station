/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: main.c
 * --------------------------------------------------------------------------*/
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/sntp.h>

#include "config_wifi.h"
#include "config_wlab.h"
#include "dht2x.h"
#include "mqtt_worker.h"
#include "wifi_net.h"
#include "wlab.h"

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

static const struct gpio_dt_spec InfoLed =
    GPIO_DT_SPEC_GET(DT_NODELABEL(info_led), gpios);

static int64_t UptimeSyncMs = 0;
static int64_t SntpSyncSec = 0;

static int64_t rtc_time_get(void) {
    int64_t sec_elapsed = (k_uptime_get() - UptimeSyncMs) / 1000;
    return (SntpSyncSec + sec_elapsed);
}

static int32_t rtc_time_sync(void) {
    int32_t rc = 0;
    struct sntp_time sntp_time = {0};

    rc = sntp_simple("0.pl.pool.ntp.org", 2000, &sntp_time);
    if (0 == rc) {
        SntpSyncSec = (int64_t)sntp_time.seconds;
        UptimeSyncMs = k_uptime_get();
    } else {
        LOG_ERR("Failed to acquire SNTP, code %d", rc);
    }

    return (rc);
}

int main(void) {
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("sys_clock_hw_cycles_per_sec = %u", sys_clock_hw_cycles_per_sec());

    int32_t ret = gpio_pin_configure_dt(&InfoLed, GPIO_OUTPUT_ACTIVE);
    if (0 != ret) {
        LOG_ERR("gpio configuration failed");
        return (0);
    }

    if (!device_is_ready(InfoLed.port)) {
        LOG_ERR("GPIO0 not ready");
        return (0);
    }

    mqtt_worker_init(CONFIG_WLAB_MQTT_BROKER, CONFIG_WLAB_MQTT_BROKER_PORT,
                     NULL, NULL);

    wifi_net_init(WIFI_SSID, WIFI_PASS);

    if (0 != mqtt_worker_connection_wait(24 * 1000)) {
        /* system reboot */
    }

    wlab_init(WLAB_SENSOR_DHT22);

    int32_t auth_attempts = 0;
    while (0 != wlab_authorize()) {
        auth_attempts++;
        if (8 == auth_attempts) {
            /* system reboot */
        }
    }
    LOG_INF("wlab authorize success");

    int32_t sntp_sync_attempts = 0;
    while (0 != rtc_time_sync()) {
        sntp_sync_attempts++;
        if (8 == sntp_sync_attempts) {
            /* system reboot */
        }
    }
    LOG_INF("rtc sync success");

    int64_t last_rtc_sync_ts = rtc_time_get();
    int64_t ts_now = 0;
    for (;;) {
        k_sleep(K_MSEC(200));

        ts_now = rtc_time_get();

        wlab_process(ts_now);

        if (ts_now - last_rtc_sync_ts > 1 * 60) {
            rtc_time_sync();
            last_rtc_sync_ts = ts_now;
        }

        gpio_pin_toggle_dt(&InfoLed);
    }
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/