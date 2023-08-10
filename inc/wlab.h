/* ---------------------------------------------------------------------------
 *  wlab
 * ---------------------------------------------------------------------------
 *  Name: wlab.h
 * --------------------------------------------------------------------------*/
#ifndef WLAB_H_
#define WLAB_H_

#include <stdint.h>

#include "config_wlab.h"

#define WLAB_TEMP_SERIE     (1)
#define WLAB_HUMIDITY_SERIE (2)

/* Sometimes max return weird value not fitted to the other if this value
 * will be more than WLAB_EXT2AVG_MAX then skip */
#define WLAB_EXT2AVG_MAX       (32)
#define WLAB_MIN_SAMPLES_COUNT (16)

#define WLAB_SAMPLE_BUFFER_SIZE \
    (8 + ((60 * CONFIG_WLAB_PUB_PERIOD) / CONFIG_WLAB_MEASURE_PERIOD))

typedef enum wlab_sensor { WLAB_SENSOR_DHT22 } wlab_sensor_t;

typedef struct {
    int32_t _min;
    int32_t _max;
    uint32_t _max_ts;
    uint32_t _min_ts;
    int32_t buff;
    int32_t cnt;
    uint32_t sample_ts;
    int32_t sample_ts_val;

    int16_t sample_buff[WLAB_SAMPLE_BUFFER_SIZE];
    uint32_t sample_buff_ts[WLAB_SAMPLE_BUFFER_SIZE];
} buffer_t;

/**
 * @brief
 *
 * @param sensor_type
 */
void wlab_init(wlab_sensor_t sensor_type);

int32_t wlab_authorize(void);
void wlab_process(int64_t timestamp_secs);

#endif /* WLAB_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/