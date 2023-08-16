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
#define WLAB_MIN_SAMPLES_COUNT (8)

typedef enum wlab_sensor {
    WLAB_SENSOR_NONE = 0,
    WLAB_SENSOR_DHT22,
} wlab_sensor_t;

typedef struct {
    int32_t _min;
    int32_t _max;
    uint32_t _max_ts;
    uint32_t _min_ts;
    int32_t buff;
    int32_t cnt;
    uint32_t sample_ts;
    int32_t sample_ts_val;
} buffer_t;

/**
 * @brief Initialize weatherlab service with provided sensor type
 *
 * @param sensor_type
 */
void wlab_init(wlab_sensor_t sensor_type);

/**
 * @brief Send authorization to wheatherlab service
 *
 * @return int32_t 0 - success, errno code otherwise
 */
int32_t wlab_authorize(void);

/**
 * @brief Do sensor measurement and publish to weatherlab if needed
 *
 * @param timestamp_secs Actual epoch time in seconds
 */
void wlab_process(int64_t timestamp_secs);

#endif /* WLAB_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/