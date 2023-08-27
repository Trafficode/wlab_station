/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: wifi_net.h
 * --------------------------------------------------------------------------*/
#ifndef WIFI_NET_H_
#define WIFI_NET_H_

/**
 * @brief Initialize wifi network, everything is proceeded in background, do
 * reconnection if needed
 *
 * @param ssid Network ssid string
 * @param passwd Network passwd string, provide NULL if network is open
 */
void wifi_net_init(char *ssid, char *passwd);

/**
 * @brief Get wifi interface MAC address
 *
 * @param mac_buffer buffer for mac address, will be ended with \0 sign
 */
void net_mac_string(char mac_buffer[13]);

/**
 * @brief Register callback to notify network disconnected event.
 *
 * @param disco_cb Cb to register.
 */
void net_on_disconnect_reqister(void (*disco_cb)(int reason));

#endif /* WIFI_NET_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/