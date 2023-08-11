/* ---------------------------------------------------------------------------
 *  dht2x
 * ---------------------------------------------------------------------------
 *  Name: dht2x.h
 * --------------------------------------------------------------------------*/
#ifndef DHT2X_H_
#define DHT2X_H_

#include <stdint.h>

void dht2x_init(void);
int32_t dht2x_read(int16_t *temp, int16_t *rh);

#endif /* DHT2X_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/