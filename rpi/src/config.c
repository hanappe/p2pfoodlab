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
#define _BSD_SOURCE
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "log_message.h"
#include "arduino.h"

static char* _file = NULL;

json_object_t config_load(const char* file)
{
        int err;
        char buffer[512];

        // For backward compatibility.
        {
                if (_file != NULL)
                        free(_file);
                _file = strdup(file);
        }

        json_object_t config = json_load(file, &err, buffer, 512);
        if (err != 0) {
                log_err("%s", buffer); 
                return json_null();
        } 
        return config;
}

// For backward compatibility.
static void config_save(json_object_t config)
{
        json_tofile(config, k_json_pretty, _file);
}

int config_get_sensors(json_object_t config, 
                       sensor_t* sensors, int len,                       
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

        // For backward compatibility.
        {
                int save = 0;
                json_object_t s = json_object_get(sensors_config, "rht03_1");
                if (!json_isnull(s)) {
                        save = 1;
                        json_object_set(sensors_config, "trh", s);
                        json_object_unset(sensors_config, "rht03_1");
                }
                s = json_object_get(sensors_config, "rht03_2");
                if (!json_isnull(s)) {
                        save = 1;
                        json_object_set(sensors_config, "trhx", s);
                        json_object_unset(sensors_config, "rht03_2");
                }
                if (save)
                        config_save(config);
        }
        //


        for (int i = 0; i < len; i++) {
                json_object_t s = json_object_get(sensors_config, sensors[i].name);
                if (json_isnull(s)) {
                        log_info("Config: sensor %s not configured", sensors[i].name); 
                        continue;
                }
                if (!json_isstring(s)) {
                        log_warn("Config: Sensor setting is not a JSON string (%s)", 
                                sensors[i].name); 
                        continue;
                }
                if (json_string_equals(s, "yes")) {
                        log_info("Config: sensor %s enabled", sensors[i].name); 
                        _enabled |= sensors[i].flag;
                } else {
                        log_info("Config: sensor %s disabled", sensors[i].name); 
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
                } else if (json_string_equals(enabled, "yes")) {
                        log_debug("Config: Using WiFi interface");
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
                } else if (json_string_equals(enabled, "yes")) {
                        log_debug("Config: Using GSM interface");
                        return "ppp0";
                }
        }
        
        return "eth0";
}

int config_powersaving_enabled(json_object_t config)
{
        json_object_t power = json_object_get(config, "power");
        if (!json_isobject(power)) {
                log_err("Config: Power settings are not a JSON object, as expected"); 
                return 0;
        }
        json_object_t poweroff = json_object_get(power, "poweroff");
        if (!json_isstring(poweroff)) {
                log_err("Config: Poweroff setting is not a JSON string, as expected"); 
                return 0;
        }
        if (json_string_equals(poweroff, "yes")) {
                return 1;
        }
        return 0;
}

int config_camera_enabled(json_object_t config)
{
        json_object_t camera_obj = json_object_get(config, "camera");
        if (json_isnull(camera_obj)) {
                log_err("Config: Could not find the camera configuration"); 
                return -1;
        }
        json_object_t enabled = json_object_get(camera_obj, "enable");
        if (!json_isstring(enabled)) {
                log_err("Config: Camera enabled setting is not a JSON string, as expected"); 
                return -1;
        }
        return json_string_equals(enabled, "yes")? 1 : 0;
}

static double config_get_general_value(json_object_t config, const char* name)
{
        json_object_t general_obj = json_object_get(config, "general");
        if (json_isnull(general_obj)) {
                log_err("Config: Could not find the general configuration"); 
                return 0.0;
        }

        json_object_t v = json_object_get(general_obj, name);
        double x;
        if (json_isstring(v)) {
                x = atof(json_string_value(v));
        } else if (json_isnumber(v)) {
                x = json_number_value(v);
        } else {
                x = NAN;
        }
        return x;
}

double config_get_timezone(json_object_t config)
{
        return config_get_general_value(config, "timezone");
}

double config_get_latitude(json_object_t config)
{
        return config_get_general_value(config, "latitude");
}

double config_get_longitude(json_object_t config)
{
        return config_get_general_value(config, "longitude");
}

const char* config_get_sensorbox_name(json_object_t config)
{
        json_object_t general_obj = json_object_get(config, "general");
        if (json_isnull(general_obj)) {
                log_err("Config: Could not find the general configuration"); 
                return "sensorbox";
        }
        const char* name = json_object_getstr(general_obj, "name");
        return (name == NULL)? "sensorbox" : name;
}


