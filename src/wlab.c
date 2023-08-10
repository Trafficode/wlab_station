/* ---------------------------------------------------------------------------
 *  wlab
 * ---------------------------------------------------------------------------
 *  Name: wlab.c
 * --------------------------------------------------------------------------*/
#include "wlab.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/logging/log.h>

#include "config_wlab.h"
#include "mqtt_worker.h"
#include "wifi_net.h"

LOG_MODULE_REGISTER(WLAB, LOG_LEVEL_DBG);

static bool wlab_buffer_commit(buffer_t *buffer, int32_t val, uint32_t ts,
                               uint32_t threshold);
static void wlab_buffer_init(buffer_t *buffer);
static void wlab_itostrf(char *dest, int32_t signed_int);

const char *AuthTemplate =
    "{\"timezone\":\"%s\",\"longitude\":%s,\"latitude\":%s,\"serie\":"
    "{\"Temperature\":%d,\"Humidity\":%d},"
    "\"name\":\"%s\",\"description\":\"%s\", \"uid\":\"%s\"}";

const char *DHTJsonDataTemplate =
    "{\"UID\":\"%s\",\"TS\":%d,\"SERIE\":{\"Temperature\":"
    "{\"f_avg\":%s,\"f_act\":%s,\"f_min\":%s,\"f_max\":%s,"
    "\"i_min_ts\":%u,\"i_max_ts\":%u},"
    "\"Humidity\":{\"f_avg\":%s,\"f_act\":%s,\"f_min\":%s,"
    "\"f_max\":%s,\"i_min_ts\":%u,\"i_max_ts\":%u}}}";

static int wlab_dht_publish_sample(buffer_t *temp, buffer_t *rh);

static buffer_t TempBuffer, RhBuffer;

void wlab_init(const wlab_sensor_t sensor_type) {
    switch (sensor_type) {
        case WLAB_SENSOR_DHT22: {
            wlab_buffer_init(&TempBuffer);
            wlab_buffer_init(&RhBuffer);
            break;
        }
        default: {
            break;
        }
    }
}

void wlab_process(int64_t timestamp_secs) {
    int32_t rc = 0;
    time_t now = 0;
    struct tm timeinfo;
    static uint32_t last_minutes = 0;
    static int64_t last_secs = 0;
    int16_t temp = 0, rh = 0;
    int32_t temp_avg = 0, rh_avg = 0;

    if (timestamp_secs - last_secs < CONFIG_WLAB_MEASURE_PERIOD) {
        goto process_done;
    }

    now = timestamp_secs;
    gmtime_r(&now, &timeinfo);

    // if (!dht_read(&dht21, &temp, &rh)) {
    //     LOG_INF("%s, temp:%d rh:%d", __FUNCTION__, temp, rh);
    //     if (500 < temp || 1000 < rh) {
    //         LOG_ERR("%s, Failed to validate data", __FUNCTION__);
    //         continue;
    //     }
    // } else {
    //     LOG_ERR("%s, DHT21 failed", __FUNCTION__);
    //     continue;
    // }

    if ((0x00 == timeinfo.tm_min % CONFIG_WLAB_PUB_PERIOD) &&
        (timeinfo.tm_min != last_minutes)) {
        temp_avg = TempBuffer.buff / TempBuffer.cnt;
        LOG_INF("temp - min: %d max: %d avg: %d", TempBuffer._min,
                TempBuffer._max, temp_avg);

        rh_avg = RhBuffer.buff / RhBuffer.cnt;
        LOG_INF("rh - min: %d max: %d avg: %d", RhBuffer._min, RhBuffer._max,
                rh_avg);

        LOG_INF("Sample ready to send...");
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

static int32_t wlab_dht_publish_sample(buffer_t *temp, buffer_t *rh) {
    int32_t rc = 0;
    int32_t temp_avg = 0, rh_avg = 0;
    char tavg_str[8], tact_str[8], tmin_str[8], tmax_str[8];
    char rhavg_str[8], rhact_str[8], rhmin_str[8], rhmax_str[8];
    char mac_addr[13];

    temp_avg = temp->buff / temp->cnt;
    wlab_itostrf(tavg_str, temp_avg);
    LOG_INF("%s, %d [%s]", __FUNCTION__, temp_avg, tavg_str);
    wlab_itostrf(tact_str, temp->sample_ts_val);
    wlab_itostrf(tmin_str, temp->_min);
    wlab_itostrf(tmax_str, temp->_max);

    rh_avg = rh->buff / rh->cnt;
    wlab_itostrf(rhavg_str, rh_avg);
    wlab_itostrf(rhact_str, rh->sample_ts_val);
    wlab_itostrf(rhmin_str, rh->_min);
    wlab_itostrf(rhmax_str, rh->_max);

    wifi_net_mac_string(mac_addr);
    rc = mqtt_worker_publish_qos1(
        CONFIG_WLAB_PUB_TOPIC, DHTJsonDataTemplate, mac_addr, temp->sample_ts,
        tavg_str, tact_str, tmin_str, tmax_str, temp->_min_ts, temp->_max_ts,
        rhavg_str, rhact_str, rhmin_str, rhmax_str, rh->_min_ts, rh->_max_ts);
    return (rc);
}

int32_t wlab_authorize(void) {
    int32_t rc = 0;
    char mac_addr[13];

    wifi_net_mac_string(mac_addr);
    rc = mqtt_worker_publish_qos1(
        CONFIG_WLAB_AUTH_TOPIC, AuthTemplate, CONFIG_WLAB_TIMEZONE,
        CONFIG_WLAB_LATITIUDE, CONFIG_WLAB_LONGITUDE, WLAB_TEMP_SERIE,
        WLAB_HUMIDITY_SERIE, CONFIG_WLAB_NAME, CONFIG_WLAB_DHT_DESC, mac_addr);
    return (rc);
}

static void wlab_itostrf(char *dest, int32_t signed_int) {
    if (0 > signed_int) {
        signed_int = -signed_int;
        sprintf(dest, "-%d.%d", signed_int / 10, abs(signed_int % 10));
    } else {
        sprintf(dest, "%d.%d", signed_int / 10, abs(signed_int % 10));
    }
}

static void wlab_buffer_init(buffer_t *buffer) {
    buffer->buff = 0;
    buffer->cnt = 0;
    buffer->_max = INT32_MIN;
    buffer->_min = INT32_MAX;
    buffer->_max_ts = 0;
    buffer->_min_ts = 0;
    buffer->sample_ts_val = INT32_MAX;
    buffer->sample_ts = 0;
}

/* wlab_buffer_commit
 * :param threshold
 * when that value exceed average then skip sample
 * when first sample will bad, then no next sample will be added so finally
 * sample count wont be enought
 */
static bool wlab_buffer_commit(buffer_t *buffer, int32_t val, uint32_t ts,
                               uint32_t threshold) {
    if ((buffer->_max != INT32_MIN) && ((val - threshold) > buffer->_max)) {
        LOG_ERR("%s, Value %d max exceed threshold\n", __FUNCTION__, val);
        return (false);
    }

    if ((buffer->_min != INT32_MAX) && ((val + threshold) < buffer->_min)) {
        LOG_ERR("%s, Value %d min exceed threshold\n", __FUNCTION__, val);
        return (false);
    }

    if (WLAB_SAMPLE_BUFFER_SIZE > buffer->cnt) {
        buffer->sample_buff[buffer->cnt] = (int16_t)val;
        buffer->sample_buff_ts[buffer->cnt] = ts;
    } else {
        LOG_ERR("%s, buffer length exceed\n", __FUNCTION__);
        return (false);
    }

    if (INT32_MAX == buffer->sample_ts_val) {
        /* Mark buffer timestamp as first sample time */
        buffer->sample_ts = ts - (ts % (60 * CONFIG_WLAB_PUB_PERIOD));
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
    return (true);
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/