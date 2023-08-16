/* ---------------------------------------------------------------------------
 *  dht2x
 * ---------------------------------------------------------------------------
 *  Name: dht2x.h
 * --------------------------------------------------------------------------*/
#include "dht2x.h"

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(DHTX, LOG_LEVEL_DBG);

static const struct gpio_dt_spec DHT2X =
    GPIO_DT_SPEC_GET(DT_NODELABEL(dht_pin), gpios);

static uint32_t dht2x_us_now(void);

struct gpio_callback IrqCb;
K_SEM_DEFINE(DhtReadDone, 0, 1);

static volatile uint32_t PulseStartUs = 0;
static volatile uint32_t PulseCnt = 0;
static volatile uint64_t ReadData = 0;
static volatile uint32_t LastElapsed = 0;

void gpio_callback(const struct device *dev, struct gpio_callback *cb,
                   uint32_t pin) {
    if (42 == PulseCnt) {
        return; /* DHT21 = AM2301 sends 64 bit, rest of them are 0 */
    }

    uint32_t us_now = dht2x_us_now();
    LastElapsed = us_now - PulseStartUs;
    if (0 == PulseCnt) {
        if (LastElapsed > 100) {
            PulseCnt += 2;
            /* Sometime we are not able to catch first falling edge, if this
             * situation occure then second elapsed is ~180us. If first edge
             * catch it schould be < 30us */
        }
    } else if (1 == PulseCnt) {
        PulseCnt++;
    } else {
        if (LastElapsed < 110) {
            /* bit 0 */
            ReadData = ReadData << 1;
        } else if (LastElapsed >= 110) {
            /* bit 1 */
            ReadData = (ReadData << 1) | 1;
        } else {
            /* bit timing failed */
            k_sem_give(&DhtReadDone);
        }
        PulseCnt++;
    }

    if (42 == PulseCnt) {
        /* read success */
        k_sem_give(&DhtReadDone);
    }

    PulseStartUs = us_now;
}

void dht2x_init(void) {
    if (!device_is_ready(DHT2X.port)) {
        LOG_ERR("Dht2x gpio not ready");
        return;
    }

    gpio_pin_configure_dt(&DHT2X, GPIO_OUTPUT_INACTIVE);
}

int32_t dht2x_read(int16_t *temp, int16_t *rh) {
    int32_t rc = 0;
    LOG_INF("%s", __FUNCTION__);

    k_sem_take(&DhtReadDone, K_NO_WAIT);
    PulseCnt = 0;
    ReadData = 0;

    gpio_pin_configure_dt(&DHT2X, GPIO_OUTPUT_INACTIVE);

    /* assert to send start signal */
    gpio_pin_set_dt(&DHT2X, true);

    k_sleep(K_MSEC(18));

    k_sched_lock();
    gpio_pin_set_dt(&DHT2X, false);
    PulseStartUs = dht2x_us_now();

    gpio_pin_configure_dt(&DHT2X, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&DHT2X, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&IrqCb, gpio_callback, BIT(DHT2X.pin));
    gpio_add_callback_dt(&DHT2X, &IrqCb);
    k_sched_unlock();

    int32_t res = k_sem_take(&DhtReadDone, K_MSEC(50));
    if (0 != res) {
        LOG_ERR("Read failed, PulseCnt %d", PulseCnt);
        rc = -EIO;
        goto read_done;
    } else {
        LOG_INF("Read done, PulseCnt %d, LastElapsed %u, ReadData %010llX",
                PulseCnt, LastElapsed, ReadData);
    }

    uint8_t *buf = (uint8_t *)&ReadData;

    /* verify checksum */
    if (((buf[4] + buf[3] + buf[2] + buf[1]) & 0xFF) != buf[0] || 0 == buf[0]) {
        LOG_ERR("Invalid checksum in fetched sample");
        rc = -ENOTSUP;
        goto read_done;
    } else {
        LOG_INF("Checksum valid");
    }

    *rh = (ReadData >> 24) & 0xFFFF;
    *temp = (ReadData >> 8) & 0xFFFF;

    if (*temp & (1 << 15)) {
        *temp &= ~(1 << 15);
        *temp = -*temp;
    }

read_done:
    gpio_pin_configure_dt(&DHT2X, GPIO_OUTPUT_INACTIVE);
    return (rc);
}

static uint32_t dht2x_us_now(void) {
    uint64_t cyc = k_cycle_get_32();
    uint64_t cyc_per_us =
        ((uint32_t)sys_clock_hw_cycles_per_sec()) / (uint32_t)USEC_PER_SEC;
    return (cyc / cyc_per_us);
}

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/