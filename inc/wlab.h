/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: wlab.h
 * --------------------------------------------------------------------------*/
#ifndef WLAB_H_
#define WLAB_H_

#include <stdint.h>

#include "config_wlab.h"

/**
 * @brief Initialize weatherlab service with provided sensor type
 *
 */
void wlab_init(void);

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