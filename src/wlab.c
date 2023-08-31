/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: wlab.c
 * --------------------------------------------------------------------------*/
#include "wlab.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include "dht2x.h"
#include "mqtt_worker.h"
#include "nvs_data.h"
#include "wdg.h"
#include "wifi_net.h"

LOG_MODULE_REGISTER(WLAB, LOG_LEVEL_DBG);

#define CONFIG_WLAB_DHT_DESC           ("DHT2X")
#define CONFIG_WLAB_PUB_TOPIC          ("/wlabdb")
#define CONFIG_WLAB_AUTH_TOPIC         ("/wlabauth")
#define CONFIG_WLAB_DEVICE_ID_BUFF_LEN (13)
#define CONFIG_WLAB_MEASURE_PERIOD     (4) /* secs */

#define WLAB_TEMP_SERIE     (1)
#define WLAB_HUMIDITY_SERIE (2)

/* Sometimes max return weird value not fitted to the other if this value
 * will be more than WLAB_EXT2AVG_MAX then skip */
#define WLAB_EXT2AVG_MAX       (32)
#define WLAB_MIN_SAMPLES_COUNT (8)

struct wlab_buffer {
    int32_t _min;
    int32_t _max;
    uint32_t _max_ts;
    uint32_t _min_ts;
    int32_t buff;
    int32_t cnt;
    uint32_t sample_ts;
    int32_t sample_ts_val;
};

static int wlab_authorize(void);
static bool wlab_buffer_commit(struct wlab_buffer *buffer, int32_t val,
                               uint32_t ts, uint32_t threshold);
static void wlab_buffer_init(struct wlab_buffer *buffer);
static void wlab_itostrf(char *dst, int32_t signed_int);
static void wlab_str_device_id_get(char dst[CONFIG_WLAB_DEVICE_ID_BUFF_LEN]);

const char *AuthTemplate =
    "{\"timezone\":\"%s\",\"longitude\":%.1f,\"latitude\":%.1f,\"serie\":"
    "{\"Temperature\":%d,\"Humidity\":%d},"
    "\"name\":\"%s\",\"description\":\"%s\", \"uid\":\"%s\"}";

const char *DHTJsonDataTemplate =
    "{\"UID\":\"%s\",\"TS\":%d,\"SERIE\":{\"Temperature\":"
    "{\"f_avg\":%s,\"f_act\":%s,\"f_min\":%s,\"f_max\":%s,"
    "\"i_min_ts\":%u,\"i_max_ts\":%u},"
    "\"Humidity\":{\"f_avg\":%s,\"f_act\":%s,\"f_min\":%s,"
    "\"f_max\":%s,\"i_min_ts\":%u,\"i_max_ts\":%u}}}";

static int wlab_dht_publish_sample(struct wlab_buffer *temp,
                                   struct wlab_buffer *rh);

static const struct gpio_dt_spec DHTx =
    GPIO_DT_SPEC_GET(DT_NODELABEL(dht_pin), gpios);

static struct wlab_buffer TempBuffer = {0}, RhBuffer = {0};
static char DeviceId[13];
static uint32_t PublishPeriodMins = 0;

void wlab_init(void) {
    int ret = dht2x_init(&DHTx);
    __ASSERT((0 == ret), "Unable to init dhtx");

    wlab_buffer_init(&TempBuffer);
    wlab_buffer_init(&RhBuffer);
    nvs_data_wlab_pub_period_get(&PublishPeriodMins);

    uint8_t auth_attempts = 0;
    for (auth_attempts = 0; auth_attempts < 8; auth_attempts++) {
        wdg_feed();
        if (0 == wlab_authorize()) {
            LOG_INF("wlab authorize success");
            break;
        }
    }
    __ASSERT((auth_attempts < 8), "Unable to init dhtx");
}

void wlab_process(int64_t timestamp_secs) {
    static uint32_t last_minutes = 0;
    static int64_t last_secs = 0;

    if (timestamp_secs - last_secs < CONFIG_WLAB_MEASURE_PERIOD) {
        goto process_done;
    }

    last_secs = timestamp_secs;

    int16_t temp = 0, rh = 0;
    int32_t temp_avg = 0, rh_avg = 0;
    int32_t rc = 0;
    time_t now = 0;
    struct tm timeinfo = {0};

    now = timestamp_secs;
    gmtime_r(&now, &timeinfo);

    if (0 != dht2x_read(&DHTx, &temp, &rh)) {
        LOG_ERR("Sensor fetch failed");
        goto process_done;
    }
    LOG_INF("Temp %d, RH %d", temp, rh);

    if ((0x00 == timeinfo.tm_min % PublishPeriodMins) &&
        (timeinfo.tm_min != last_minutes)) {
        temp_avg = TempBuffer.buff / TempBuffer.cnt;
        LOG_INF("temp - min: %d max: %d avg: %d", TempBuffer._min,
                TempBuffer._max, temp_avg);

        rh_avg = RhBuffer.buff / RhBuffer.cnt;
        LOG_INF("rh - min: %d max: %d avg: %d", RhBuffer._min, RhBuffer._max,
                rh_avg);

        LOG_DBG("Sample ready to send...");
        rc = wlab_dht_publish_sample(&TempBuffer, &RhBuffer);
        if (0 != rc) {
            LOG_ERR("%s, publish sample failed rc:%d", __FUNCTION__, rc);
        } else {
            LOG_INF("%s, publish sample success", __FUNCTION__);
        }

        wlab_buffer_init(&TempBuffer);
        wlab_buffer_init(&RhBuffer);
        last_minutes = timeinfo.tm_min;
    } else {
        wlab_buffer_commit(&TempBuffer, temp, now, 8);
        wlab_buffer_commit(&RhBuffer, rh, now, 40);
    }

process_done:
    return;
}

static void wlab_str_device_id_get(char dst[CONFIG_WLAB_DEVICE_ID_BUFF_LEN]) {
    uint64_t device_id = 0;

    nvs_data_wlab_device_id_get(&device_id);
    if (0 == device_id) {
        net_mac_string(DeviceId);
    } else {
        snprintf(dst, CONFIG_WLAB_DEVICE_ID_BUFF_LEN, "%012" PRIX64,
                 device_id & 0x0000FFFFFFFFFFFF);
    }
    LOG_INF("Wlab device id: %s", dst);
}

static int wlab_dht_publish_sample(struct wlab_buffer *temp,
                                   struct wlab_buffer *rh) {
    int rc = 0;

    int32_t temp_avg = 0;
    char tavg_str[8], tact_str[8], tmin_str[8], tmax_str[8];
    temp_avg = temp->buff / temp->cnt;
    wlab_itostrf(tavg_str, temp_avg);
    LOG_DBG("%s, %d [%s]", __FUNCTION__, temp_avg, tavg_str);
    wlab_itostrf(tact_str, temp->sample_ts_val);
    wlab_itostrf(tmin_str, temp->_min);
    wlab_itostrf(tmax_str, temp->_max);

    int32_t rh_avg = 0;
    char rhavg_str[8], rhact_str[8], rhmin_str[8], rhmax_str[8];
    rh_avg = rh->buff / rh->cnt;
    wlab_itostrf(rhavg_str, rh_avg);
    wlab_itostrf(rhact_str, rh->sample_ts_val);
    wlab_itostrf(rhmin_str, rh->_min);
    wlab_itostrf(rhmax_str, rh->_max);

    rc = mqtt_worker_publish_qos1(
        CONFIG_WLAB_PUB_TOPIC, DHTJsonDataTemplate, DeviceId, temp->sample_ts,
        tavg_str, tact_str, tmin_str, tmax_str, temp->_min_ts, temp->_max_ts,
        rhavg_str, rhact_str, rhmin_str, rhmax_str, rh->_min_ts, rh->_max_ts);
    return (rc);
}

int wlab_authorize(void) {
    int ret = 0;
    char station_name[CONFIG_BUFF_MAX_STRING_LEN];
    struct gps_position position = {0};

    nvs_data_wlab_name_get(station_name);
    wlab_str_device_id_get(DeviceId);
    nvs_data_wlab_gps_position_get(&position);

    ret = mqtt_worker_publish_qos1(
        CONFIG_WLAB_AUTH_TOPIC, AuthTemplate, position.timezone,
        position.latitude, position.longitude, WLAB_TEMP_SERIE,
        WLAB_HUMIDITY_SERIE, station_name, CONFIG_WLAB_DHT_DESC, DeviceId);
    return (ret);
}

static void wlab_itostrf(char *dst, int32_t signed_int) {
    if (0 > signed_int) {
        signed_int = -signed_int;
        sprintf(dst, "-%d.%d", signed_int / 10, abs(signed_int % 10));
    } else {
        sprintf(dst, "%d.%d", signed_int / 10, abs(signed_int % 10));
    }
}

static void wlab_buffer_init(struct wlab_buffer *buffer) {
    buffer->buff = 0;
    buffer->cnt = 0;
    buffer->_max = INT32_MIN;
    buffer->_min = INT32_MAX;
    buffer->_max_ts = 0;
    buffer->_min_ts = 0;
    buffer->sample_ts_val = INT32_MAX;
    buffer->sample_ts = 0;
}

/**
 * @brief When val exceeds MIN or MAX value more than threshold then skip this
 * measuremnt. It was neccessary to add when pt100 and maxXXXX is a sensor.
 */
static bool wlab_buffer_commit(struct wlab_buffer *buffer, int32_t val,
                               uint32_t ts, uint32_t threshold) {
    bool rc = false;
    if (buffer->cnt > 4) {
        if ((buffer->_max != INT32_MIN) && ((val - threshold) > buffer->_max)) {
            LOG_ERR("%s, Value %d max exceed threshold", __FUNCTION__, val);
            goto failed_done;
        }

        if ((buffer->_min != INT32_MAX) && ((val + threshold) < buffer->_min)) {
            LOG_ERR("%s, Value %d min exceed threshold", __FUNCTION__, val);
            goto failed_done;
        }
    }

    if (INT32_MAX == buffer->sample_ts_val) {
        /* Mark buffer timestamp as first sample time */
        buffer->sample_ts = ts - (ts % (60 * PublishPeriodMins));
        buffer->sample_ts_val = val;
    }

    if (val > buffer->_max) {
        buffer->_max = val;
        buffer->_max_ts = ts - (ts % 60);
    }

    if (val < buffer->_min) {
        buffer->_min = val;
        buffer->_min_ts = ts - (ts % 60);
    }

    buffer->buff += val;
    buffer->cnt++;

failed_done:
    return (rc);
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/