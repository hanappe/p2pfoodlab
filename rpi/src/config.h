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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "json.h"
#include "sensorbox.h"

        json_object_t config_load(const char* file);

        int config_get_sensors(json_object_t config,
                               sensor_t* sensors, int len,
                               unsigned char* enabled, 
                               unsigned char* period);

        const char* config_get_network_interface(json_object_t config);

        int config_powersaving_enabled(json_object_t config);

        int config_camera_enabled(json_object_t config);
 
        const char* config_get_sensorbox_name(json_object_t config);
        double config_get_timezone(json_object_t config);
        double config_get_latitude(json_object_t config);
        double config_get_longitude(json_object_t config);

        const char* config_getstr(json_object_t config, const char* expr);
        double config_getnum(json_object_t config, const char* expr);

#ifdef __cplusplus
}
#endif

#endif


