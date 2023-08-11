/* ---------------------------------------------------------------------------
 *  dht2x
 * ---------------------------------------------------------------------------
 *  Name: dht2x.h
 * --------------------------------------------------------------------------*/
#ifndef DHT2X_H_
#define DHT2X_H_

#include <stdint.h>

/**
 * @brief Initialize custom implementation of dht sensors
 *
 */
void dht2x_init(void);

/**
 * @brief Read DHTxy sensor value using interrupt service runtime, not blocking
 * for rtos
 *
 * @param temp Pointer to temperature value
 * @param rh Pointer to humidity value
 * @return int32_t 0 - success, errno code otherwise
 */
int32_t dht2x_read(int16_t *temp, int16_t *rh);

#endif /* DHT2X_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/