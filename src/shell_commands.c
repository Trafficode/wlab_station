/* ---------------------------------------------------------------------------
 *  wlab_station
 * ---------------------------------------------------------------------------
 *  Name: shell_commands.c
 * --------------------------------------------------------------------------*/
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include "nvs_data.h"

// (WPA/WPA2 enabled)  $ wificonf <ssid> <passwd>
// (open network)      $ wificonf <ssid>
static int cmd_wifi_config(const struct shell *shell, size_t argc,
                           char *argv[]) {
    if (!IN_RANGE(argc, 2, 3)) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }
    struct wifi_config wificfg = {0};
    strncpy(wificfg.wifi_ssid, argv[1], CONFIG_BUFF_MAX_STRING_LEN);
    if (3 == argc) {
        strncpy(wificfg.wifi_pass, argv[2], CONFIG_BUFF_MAX_STRING_LEN);
    }
    if (0 == nvs_data_wifi_config_set(&wificfg)) {
        shell_fprintf(shell, SHELL_NORMAL, "wifi_ssid: <%s>\n",
                      wificfg.wifi_ssid);
        shell_fprintf(shell, SHELL_NORMAL, "wifi_pass: <%s>\n",
                      wificfg.wifi_pass);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }

    return (0);
}

// $ mqttconf <hostname/ip> <port> <ping_period_secs> <max_ping_no_ans_mins>
static int cmd_mqtt_config(const struct shell *shell, size_t argc,
                           char *argv[]) {
    if (argc != 5) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }

    struct mqtt_config mqttcfg = {0};
    strncpy(mqttcfg.mqtt_broker, argv[1], CONFIG_BUFF_MAX_STRING_LEN);
    mqttcfg.mqtt_port = strtoul(argv[2], NULL, 10);
    mqttcfg.mqtt_ping_period = strtoul(argv[3], NULL, 10);
    mqttcfg.mqtt_max_ping_no_answer = strtoul(argv[4], NULL, 10);
    if (0 == nvs_data_mqtt_config_set(&mqttcfg)) {
        shell_fprintf(shell, SHELL_NORMAL, "\tmqtt_broker: <%s>\n",
                      mqttcfg.mqtt_broker);
        shell_fprintf(shell, SHELL_NORMAL, "\tmqtt_port: %u\n",
                      mqttcfg.mqtt_port);
        shell_fprintf(shell, SHELL_NORMAL, "\tmqtt_ping_period: %u [secs]\n",
                      mqttcfg.mqtt_ping_period);
        shell_fprintf(shell, SHELL_NORMAL,
                      "\tmqtt_max_ping_no_answer: %u [mins]\n",
                      mqttcfg.mqtt_max_ping_no_answer);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }
    return (0);
}

// (set custom wlab id)             $ wlabid <str_hex_id>
// (restore taking id as mac addr)  $ wlabid
static int cmd_wlab_id(const struct shell *shell, size_t argc, char *argv[]) {
    if (!IN_RANGE(argc, 1, 2)) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }

    uint64_t device_id = 0;
    if (2 == argc) {
        device_id = strtoull(argv[1], NULL, 16);
    }

    if (0 == nvs_data_wlab_device_id_set(&device_id)) {
        shell_fprintf(shell, SHELL_NORMAL, "device_id: %" PRIX64 "\n",
                      device_id);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }
    return (0);
}

// $ wlabname <string_name>
static int cmd_wlab_name(const struct shell *shell, size_t argc, char *argv[]) {
    if (argc != 2) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }

    char wlab_name[CONFIG_BUFF_MAX_STRING_LEN];
    memset(wlab_name, 0x00, CONFIG_BUFF_MAX_STRING_LEN);
    strncpy(wlab_name, argv[1], CONFIG_BUFF_MAX_STRING_LEN);
    if (0 == nvs_data_wlab_name_set(wlab_name)) {
        shell_fprintf(shell, SHELL_NORMAL, "wlab_name: <%s>\n", wlab_name);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }

    return (0);
}

// $ wlabpubp <publish_period_mins>
static int cmd_wlab_publish_period(const struct shell *shell, size_t argc,
                                   char *argv[]) {
    if (argc != 2) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }

    uint32_t pub_period = strtoul(argv[1], NULL, 10);
    if (0 == nvs_data_wlab_pub_period_set(&pub_period)) {
        shell_fprintf(shell, SHELL_NORMAL, "pub_period: %u [secs]\n",
                      pub_period);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }
    return (0);
}

// $ wlabgpsp <timezone> <latitude> <longitude>
static int cmd_wlab_gps_position(const struct shell *shell, size_t argc,
                                 char *argv[]) {
    if (argc != 4) {
        shell_fprintf(shell, SHELL_NORMAL, "\tBad command usage!");
        return (0);
    }
    struct gps_position gpspos = {0};
    strncpy(gpspos.timezone, argv[1], CONFIG_BUFF_MAX_STRING_LEN);

    if (0 == nvs_data_wlab_gps_position_set(&gpspos)) {
        shell_fprintf(shell, SHELL_NORMAL, "latitude: %.2f\n", gpspos.latitude);
        shell_fprintf(shell, SHELL_NORMAL, "longitude: %.2f\n",
                      gpspos.longitude);
        shell_fprintf(shell, SHELL_NORMAL, "timezone: <%s>\n", gpspos.timezone);
        shell_fprintf(shell, SHELL_NORMAL, "\tOK!\n");
    } else {
        shell_fprintf(shell, SHELL_NORMAL, "\tFailed!\n");
    }
    return (0);
}

// $ pconfig
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
    shell_fprintf(shell, SHELL_NORMAL, "latitude: %.2f\n", gpspos.latitude);
    shell_fprintf(shell, SHELL_NORMAL, "longitude: %.2f\n", gpspos.longitude);
    shell_fprintf(shell, SHELL_NORMAL, "timezone: <%s>\n", gpspos.timezone);
    shell_fprintf(shell, SHELL_NORMAL, "device_id: %" PRIX64 "\n", device_id);
    shell_fprintf(shell, SHELL_NORMAL, "pub_period: %u [mins]\n", pub_period);
    shell_fprintf(shell, SHELL_NORMAL, "wlab_name: <%s>\n", wlab_name);
    return (0);
}

SHELL_CMD_REGISTER(pconfig, NULL,
                   "Print all custom user config\n"
                   "Usage:                      \n"
                   "$ pconfig                     ",
                   cmd_pconfig);

SHELL_CMD_REGISTER(wificonf, NULL,
                   "Configure wifi credentials\n"
                   "Usage:\n"
                   "(WPA/WPA2 enabled)  $ wificonf <ssid> <passwd>\n"
                   "(WPA/WPA2 enabled)  $ wificonf wifi_1234 qwerty1234\n"
                   "(open network)      $ wificonf <ssid>\n"
                   "(open network)      $ wificonf wifi_1234",
                   cmd_wifi_config);

SHELL_CMD_REGISTER(mqttconf, NULL,
                   "Configure mqtt worker settings\n"
                   "Usage:\n"
                   "$ mqttconf <hostname/ip> <port> <ping_period_secs> "
                   "<max_ping_no_ans_mins>\n"
                   "$ mqttconf 192.168.1.10 1883 60 10",
                   cmd_mqtt_config);

SHELL_CMD_REGISTER(wlabid, NULL,
                   "Set custom wlab device id\n"
                   "Usage:\n"
                   "(set custom wlab id)             $ wlabid <str_hex_id>\n"
                   "(set custom wlab id)             $ wlabid 1122EEFF22AA\n"
                   "(restore taking id as mac addr)  $ wlabid",
                   cmd_wlab_id);

SHELL_CMD_REGISTER(wlabname, NULL,
                   "Set wlab station name\n"
                   "Usage:\n"
                   "$ wlabname <string_name>        \n"
                   "$ wlabname WLAB_STATION           ",
                   cmd_wlab_name);

SHELL_CMD_REGISTER(wlabpubp, NULL,
                   "Set wlab publish period in seconds\n"
                   "Usage:\n"
                   "$ wlabpubp <publish_period_mins>\n"
                   "$ wlabpubp 10                     ",
                   cmd_wlab_publish_period);

SHELL_CMD_REGISTER(wlabgpsp, NULL,
                   "Set wlab gps position\n"
                   "Usage:\n"
                   "$ wlabgpsp <timezone> <latitude> <longitude>\n"
                   "$ wlabgpsp Europe/Warsaw 20.4 40.2",
                   cmd_wlab_gps_position);
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/