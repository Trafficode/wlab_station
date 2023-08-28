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

static int cmd_wlab_name(const struct shell *shell, size_t argc, char *argv[]) {
    return (0);
}

static int cmd_wlab_publish_period(const struct shell *shell, size_t argc,
                                   char *argv[]) {
    return (0);
}

static int cmd_pconfig(const struct shell *shell, size_t argc, char *argv[]) {
    struct wifi_config wificfg = {};
    struct mqtt_config mqttcfg = {};
    struct gps_position gpspos = {};
    uint64_t device_id = 0;
    uint32_t pub_period = 0;
    char wlab_name[CONFIG_BUFF_MAX_STRING_LEN];

    nvs_data_wifi_config_get(&wificfg);
    nvs_data_mqtt_config_get(&mqttcfg);
    nvs_data_wlab_device_id_get(&device_id);
    nvs_data_wlab_name_get(wlab_name);
    nvs_data_wlab_gps_position_get(&gpspos);
    nvs_data_wlab_pub_period_get(&pub_period);

    shell_fprintf(shell, SHELL_NORMAL, "wifi_ssid: <%s>\n", wificfg.wifi_ssid);
    shell_fprintf(shell, SHELL_NORMAL, "wifi_pass: <%s>\n", wificfg.wifi_pass);
    shell_fprintf(shell, SHELL_NORMAL, "mqtt_broker: <%s>\n",
                  mqttcfg.mqtt_broker);
    shell_fprintf(shell, SHELL_NORMAL, "mqtt_port: %u\n", mqttcfg.mqtt_port);
    shell_fprintf(shell, SHELL_NORMAL, "mqtt_ping_period: %u [secs]\n",
                  mqttcfg.mqtt_ping_period);
    shell_fprintf(shell, SHELL_NORMAL, "mqtt_max_ping_no_answer: %u [mins]\n",
                  mqttcfg.mqtt_max_ping_no_answer);
    shell_fprintf(shell, SHELL_NORMAL, "latitude: %.1f\n", gpspos.latitude);
    shell_fprintf(shell, SHELL_NORMAL, "longitude: %.1f\n", gpspos.longitude);
    shell_fprintf(shell, SHELL_NORMAL, "timezone: <%s>\n", gpspos.timezone);
    shell_fprintf(shell, SHELL_NORMAL, "device_id: %" PRIX64 "\n", device_id);
    shell_fprintf(shell, SHELL_NORMAL, "pub_period: %u [secs]\n", pub_period);
    shell_fprintf(shell, SHELL_NORMAL, "wlab_name: <%s>\n", wlab_name);
    return (0);
}

SHELL_CMD_REGISTER(pconf, NULL,
                   "Print all custom user config\n"
                   "usage:\n"
                   "$ pconfig\n",
                   cmd_pconfig);

SHELL_CMD_REGISTER(wificonf, NULL,
                   "Configure wifi credentials\n"
                   "usage:\n"
                   "(WPA/WPA2 enabled)  $ wificonf <ssid> <passwd>\n"
                   "(open network)      $ wificonf <ssid>",
                   cmd_wifi_config);

SHELL_CMD_REGISTER(
    mqttconf, NULL,
    "Configure mqtt worker settings\n"
    "usage:\n"
    "$ mqttconf <hostname/ip> <port> <ping_period_secs> <max_ping_no_ans_mins>",
    cmd_mqtt_config);

SHELL_CMD_REGISTER(wlabid, NULL,
                   "Set custom wlab device id\n"
                   "usage:\n"
                   "(set custom wlab id)             $ wlabid <str_hex_id>\n"
                   "(restore taking id as mac addr)  $ wlabid",
                   cmd_wlab_id);

SHELL_CMD_REGISTER(wlabname, NULL,
                   "Set wlab station name\n"
                   "usage:\n"
                   "$ wlabname <string_name>",
                   cmd_wlab_name);

SHELL_CMD_REGISTER(wlabpubp, NULL,
                   "Set wlab publish period in seconds\n"
                   "usage:\n"
                   "$ wlabpubp <publish_period>",
                   cmd_wlab_publish_period);
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/