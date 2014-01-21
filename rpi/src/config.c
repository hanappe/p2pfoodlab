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
#include "arduino-api.h"

json_object_t config_load(const char* file)
{
        char buffer[512];

        json_object_t config = json_load(file, buffer, 512);
        if (json_isnull(config)) {
                log_err("%s\n", buffer); 
                return json_null();
        } 
        return config;
}

int config_get_enabled_sensors(json_object_t config)
{
        json_object_t enable;
        int b = 0;

        json_object_t sensors = json_object_get(config, "sensors");
        if (json_isnull(sensors) || !json_isobject(sensors)) {
                log_err("Failed to get the sensors configuration\n"); 
                return -1;
        } 

        enable = json_object_get(sensors, "rht03_1");
        if (json_isnull(enable)) 
                log_warn("Failed to read rht03_1 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("internal RHT03 sensor enabled\n"); 
                b |= RHT03_1_FLAG;
        } else {
                log_info("internal RHT03 sensor disabled\n"); 
        }

        enable = json_object_get(sensors, "rht03_2");
        if (json_isnull(enable)) 
                log_warn("Failed to read rht03_2 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("external RHT03 sensor enabled\n"); 
                b |= RHT03_2_FLAG;
        } else {
                log_info("external RHT03 sensor disabled\n"); 
        }

        enable = json_object_get(sensors, "sht15_1");
        if (json_isnull(enable))
                log_warn("Failed to read sht15_1 settings\n");
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("internal SHT15 sensor enabled\n");
                b |= SHT15_1_FLAG;
        } else {
                log_info("internal SHT15 sensor disabled\n");
        }

        enable = json_object_get(sensors, "sht15_2");
        if (json_isnull(enable))
                log_warn("Failed to read sht15_2 settings\n");
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("external SHT15 sensor enable\n");
                b |= SHT15_2_FLAG;
        } else {
                log_info("external SHT15 sensor disabled\n");
        }

        enable = json_object_get(sensors, "light");
        if (json_isnull(enable)) 
                log_warn("Failed to read light sensor settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("luminosity sensor enabled\n"); 
                b |= LUMINOSITY_FLAG;
        } else {
                log_info("luminosity sensors disabled\n"); 
        }

        enable = json_object_get(sensors, "soil");
        if (json_isnull(enable)) 
                log_warn("Failed to read soil humidity settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                log_info("soil humidity sensor enabled\n"); 
                b |= SOIL_FLAG;
        } else {
                log_info("soil humidity sensor disabled\n"); 
        }

        return b;
}

int config_get_sensors_period(json_object_t config)
{
        int period;
        json_object_t s;

        json_object_t sensors = json_object_get(config, "sensors");
        if (json_isnull(sensors) || !json_isobject(sensors)) {
                log_err("Failed to get the sensors configuration\n"); 
                return -1;
        } 

        s = json_object_get(sensors, "update");
        if (json_isnull(s) || !json_isstring(s)) {
                log_err("Failed to read sensors update period\n"); 
                return 0;
        } else if (json_string_equals(s, "debug")) {
                period = 1;
        } else {
                period = atoi(json_string_value(s));
        }

        if (period > 255) {
                log_warn("Invalid update period %d > 255\n", period);
                return -1;
        } else if (period < 1) {
                log_warn("Invalid update period %d < 255\n", period);
                return -1;
        }

        log_info("Sensors update period is %d\n", period);

        return period;
}



