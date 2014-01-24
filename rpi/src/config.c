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
        { "rht03_1", RHT03_1_FLAG },
        { "rht03_2", RHT03_2_FLAG },
        { "sht15_1", SHT15_1_FLAG },
        { "sht15_2", SHT15_2_FLAG },
        { "light", LUMINOSITY_FLAG },
        { "soil", SOIL_FLAG },
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
        unsigned char _enabled; 
        unsigned char _period;

        *enabled = 0;
        *period = 0;

        json_object_t sensors_config = json_object_get(config, "sensors");
        if (json_isnull(sensors_config) || !json_isobject(sensors_config)) {
                log_err("Failed to get the sensors configuration"); 
                return -1;
        } 

        for (int i = 0; sensorlist[i].name != NULL; i++) {
                json_object_t s = json_object_get(sensors_config, sensorlist[i].name);
                if (!json_isstring(s)) {
                        log_err("Sensorbox: Sensor setting is not a JSON string, as expected (%s)", 
                                sensorlist[i].name); 
                        return -1;
                }
                if (json_string_equals(s, "yes")) {
                        _enabled |= sensorlist[i].flag;
                }
        }

        json_object_t p = json_object_get(sensors_config, "update");
        if (!json_isstring(p)) {
                log_err("Sensorbox: Sensor update setting is not a JSON string, as expected"); 
                return -1;
        }
        int v = atoi(json_string_value(p));
        if ((v < 0) || (v > 255)) {
                log_err("Sensorbox: Invalid sensor update setting: %d", v); 
                return -1;
        }
        _period = (v & 0xff);

        *enabled = _enabled;
        *period = _period;

        return 0;
}



