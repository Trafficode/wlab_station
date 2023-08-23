/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: timestamp.c
 * --------------------------------------------------------------------------*/
#include "timestamp.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/reboot.h>

#include "wdg.h"

LOG_MODULE_REGISTER(TS, LOG_LEVEL_DBG);

/**
 * @brief Call with some period to be sure that internal timer is up-to-data.
 *
 * @return int Negative errno.
 */
static int timestamp_sync(void);

static int64_t UptimeSyncMs = 0;
static int64_t SntpSyncSec = 0;

void timestamp_init(void) {
    int32_t sntp_sync_attempts = 0;
    while (0 != timestamp_sync()) {
        sntp_sync_attempts++;
        wdg_feed();
        if (8 == sntp_sync_attempts) {
            /* system reboot */
            sys_reboot(SYS_REBOOT_COLD);
        }
    }
}

int64_t timestamp_get(void) {
    int64_t sec_elapsed = (k_uptime_get() - UptimeSyncMs) / 1000;
    return (SntpSyncSec + sec_elapsed);
}

void timestamp_update(int64_t period_secs) {
    int64_t actual_uptime = k_uptime_get();
    if (UptimeSyncMs + period_secs < actual_uptime) {
        timestamp_sync();
    }
}

static int timestamp_sync(void) {
    int ret = 0;
    struct sntp_time sntp_time = {0};

    ret = sntp_simple("0.pl.pool.ntp.org", 4000, &sntp_time);
    if (0 == ret) {
        SntpSyncSec = (int64_t)sntp_time.seconds;
        UptimeSyncMs = k_uptime_get();
    } else {
        LOG_ERR("Failed to acquire SNTP, code %d", ret);
    }

    return (ret);
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/