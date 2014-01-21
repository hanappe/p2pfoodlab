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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "config.h"
#include "arduino-api.h"
#include "opensensordata.h"
#include "log_message.h"

int opensensordata_get_datastream(const char* cache, const char* name)
{
        char buffer[512];
        char filename[512];

        snprintf(filename, 511, "%s/%s.json", cache, name);
        filename[511] = 0;

        json_object_t def = json_load(filename, buffer, 512);
        if (json_isnull(def)) {
                log_err("%s", buffer); 
                return -1;
        } else if (!json_isobject(def)) {
                log_err("Invalid datastream definition: %s", filename); 
                return -1;
        }

        int id = -1;
        json_object_t v = json_object_get(def, "id");
        if (json_isnull(v)) {
                log_err("Invalid datastream id: %s", filename); 
                return -1;
        }
        if (json_isstring(v)) {
                id = atoi(json_string_value(v));
        } else if (json_isnumber(v)) {
                id = json_number_value(v);
        } else {
                log_err("Invalid datastream id: %s", filename); 
                return -1;
        }
        
        return id;
}

int opensensordata_get_datastreams(const char* osd_dir, unsigned char enabled, int* ids)
{
        int count = 0;
        if (enabled & RHT03_1_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"t");
                ids[count++] = opensensordata_get_datastream(osd_dir,"rh");
        }
        if (enabled & RHT03_2_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"tx");
                ids[count++] = opensensordata_get_datastream(osd_dir,"rhx");
        }
        if (enabled & SHT15_1_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"t2");
                ids[count++] = opensensordata_get_datastream(osd_dir,"rh2");
        }
        if (enabled & SHT15_2_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"tx2");
                ids[count++] = opensensordata_get_datastream(osd_dir,"rhx2");
        }
        if (enabled & LUMINOSITY_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"lum");
        }
        if (enabled & SOIL_FLAG) {
                ids[count++] = opensensordata_get_datastream(osd_dir,"soil");
        }
        return count;
}
