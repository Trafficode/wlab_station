/* ---------------------------------------------------------------------------
 *  wlab
 * ---------------------------------------------------------------------------
 *  Name: config_wlab.h
 * --------------------------------------------------------------------------*/
#ifndef WLAB_CONFIG_H_
#define WLAB_CONFIG_H_

#define CONFIG_WLAB_DHT_DESC ("DHT22")

#define CONFIG_WLAB_PUB_PERIOD     (10) /* minutes */
#define CONFIG_WLAB_MEASURE_PERIOD (4)  /* secs */

#define CONFIG_WLAB_PUB_TOPIC  ("/testsamples")
#define CONFIG_WLAB_AUTH_TOPIC ("/testauth")

#define CONFIG_WLAB_NAME      ("Station Name")
#define CONFIG_WLAB_LATITIUDE ("40")
#define CONFIG_WLAB_LONGITUDE ("30")
#define CONFIG_WLAB_TIMEZONE  ("Europe/Warsaw")

#define CONFIG_WLAB_MQTT_BROKER      ("mqtt://hostname")
#define CONFIG_WLAB_MQTT_BROKER_PORT (1883)

#endif /* WLAB_CONFIG_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/