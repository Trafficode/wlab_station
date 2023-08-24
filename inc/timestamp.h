/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: timestamp.h
 * --------------------------------------------------------------------------*/
#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <stdint.h>

/**
 * @brief Initialize timestamp module, use sntp to get real epoch time. When no
 * possibilities to sync time - reboot cpu.
 *
 */
void timestamp_init(void);

/**
 * @brief Actual epoch timestamp.
 *
 * @return int64_t Actual epoch timestamp millis.
 */
int64_t timestamp_get(void);

/**
 * @brief Update internal timer to be up-to-date with epoch time.
 *
 */
void timestamp_update(void);

#endif /* TIMESTAMP_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/