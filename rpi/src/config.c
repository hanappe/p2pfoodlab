/* 

    P2P Food Lab Sensorbox

    Copyright (C) 2013  Sony Computer Science Laboratory Paris
    Author: Peter Hanappe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <stdlib.h>
#include "config.h"
#include "log_message.h"
#include "arduino.h"

typedef struct _sensor_t {
        char* name;
        int flag;
} sensor_t;

sensor_t sensorlist[] = {
        { "rht03_1", SENSOR_TRH },
        { "rht03_2", SENSOR_TRHX },
        { "trh", SENSOR_TRH },
        { "trhx", SENSOR_TRHX },
        { "light", SENSOR_LUM },
        { "soil", SENSOR_SOIL },
        { NULL, 0 } };

json_object_t config_load(const char* file)
{
        char buffer[512];

        json_object_t config = json_load(file, buffer, 512);
        if (json_isnull(config)) {
                log_err("%s", buffer); 
                return json_null();
        } 
        return config;
}

int config_get_sensors(json_object_t config, 
                       unsigned char* enabled, 
                       unsigned char* period)
{
        unsigned char _enabled = 0; 
        unsigned char _period = 0;

        *enabled = 0;
        *period = 0;

        json_object_t sensors_config = json_object_get(config, "sensors");
        if (json_isnull(sensors_config) || !json_isobject(sensors_config)) {
                log_err("Config: Failed to get the sensors configuration"); 
                return -1;
        } 

        for (int i = 0; sensorlist[i].name != NULL; i++) {
                json_object_t s = json_object_get(sensors_config, sensorlist[i].name);
                if (json_isnull(s)) {
                        log_info("Config: sensor %s not configured", sensorlist[i].name); 
                        continue;
                }
                if (!json_isstring(s)) {
                        log_warn("Config: Sensor setting is not a JSON string (%s)", 
                                sensorlist[i].name); 
                        continue;
                }
                if (json_string_equals(s, "yes")) {
                        log_info("Config: sensor %s enabled", sensorlist[i].name); 
                        _enabled |= sensorlist[i].flag;
                } else {
                        log_info("Config: sensor %s disabled", sensorlist[i].name); 
                }
        }

        json_object_t p = json_object_get(sensors_config, "update");
        if (!json_isstring(p)) {
                log_err("Config: Sensor update setting is not a JSON string, as expected"); 
                return -1;
        }
        int v = atoi(json_string_value(p));
        if ((v < 0) || (v > 255)) {
                log_err("Config: Invalid sensor update setting: %d", v); 
                return -1;
        }
        _period = (v & 0xff);

        *enabled = _enabled;
        *period = _period;

        return 0;
}

const char* config_get_network_interface(json_object_t config)
{
        json_object_t wifi_config = json_object_get(config, "wifi");
        if (json_isnull(wifi_config) || !json_isobject(wifi_config)) {
                log_err("Config: Failed to get the WiFi configuration"); 
        } else {
                json_object_t enabled = json_object_get(wifi_config, "enable");
                if (!json_isstring(enabled)) {
                        log_err("Config: WiFi enabled setting is not a JSON string, as expected"); 
                } else if (!json_string_equals(enabled, "yes")) {
                        log_debug("Sensorbox: Camera not enabled");
                        return "wlan0";
                }
        }

        json_object_t gsm_config = json_object_get(config, "gsm");
        if (json_isnull(gsm_config) || !json_isobject(gsm_config)) {
                log_err("Config: Failed to get the GSM configuration"); 
        } else {
                json_object_t enabled = json_object_get(gsm_config, "enable");
                if (!json_isstring(enabled)) {
                        log_err("Config: GSM enabled setting is not a JSON string, as expected"); 
                } else if (!json_string_equals(enabled, "yes")) {
                        log_debug("Sensorbox: Camera not enabled");
                        return "ppp0";
                }
        }
        
        return "eth0";
}

