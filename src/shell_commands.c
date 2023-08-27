/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: shell_commands.c
 * --------------------------------------------------------------------------*/
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "nvs_data.h"

static int cmd_wifi_config(const struct shell *shell, size_t argc,
                           char *argv[]) {
    shell_fprintf(shell, SHELL_NORMAL, "Hello Shell\n");
    return (0);
}

static int cmd_mqtt_config(const struct shell *shell, size_t argc,
                           char *argv[]) {
    return (0);
}

static int cmd_wlab_id(const struct shell *shell, size_t argc, char *argv[]) {
    return (0);
}

SHELL_CMD_REGISTER(wificonf, NULL,
                   "Configure wifi credentials\n"
                   "usage:\n"
                   "(WPA/WPA2 enabled)  $ wificonf <ssid> <passwd>\n"
                   "(open network)      $ wificonf <ssid>",
                   cmd_wifi_config);

SHELL_CMD_REGISTER(mqttconf, NULL,
                   "Configure mqtt broker configuration\n"
                   "usage:\n"
                   "$ mqttconf <hostname/ip> <port>",
                   cmd_mqtt_config);

SHELL_CMD_REGISTER(wlabid, NULL,
                   "Set custom wlab device id\n"
                   "usage:\n"
                   "$ wlabid <str_hex_id>",
                   cmd_wlab_id);

/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/