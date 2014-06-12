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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"
#include "log_message.h"
#include "network.h"

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

int config_iface_enabled(json_object_t config, const char* iface)
{
        if (strcmp(iface, "eth0") == 0)
                return json_streq(config, "wired.enable", "yes");
        if (strcmp(iface, "wlan0") == 0)
                return json_streq(config, "wifi.enable", "yes");
        if (strcmp(iface, "ppp0") == 0)
                return json_streq(config, "gsm.enable", "yes");
        return 0;
}

const char* config_get_network_interface(json_object_t config)
{
        if (json_streq(config, "wired.enable", "yes"))
                return "eth0";
        if (json_streq(config, "wifi.enable", "yes"))
                return "wlan0";
        if (json_streq(config, "gsm.enable", "yes"))
                return "ppp0";
        log_warn("No network interface enabled.");
        return NULL;
}

int config_powersaving_enabled(json_object_t config)
{
        return json_streq(config, "power.poweroff", "yes");
}

int config_camera_enabled(json_object_t config)
{
        return json_streq(config, "camera.enable", "yes");
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

typedef struct _config_copy_stack_t {
        int top;
        json_object_t stack[32];
} config_copy_stack_t;

static void config_merge_object(json_object_t src, json_object_t dest);
static void config_merge_array(json_object_t src, json_object_t dest);

void config_merge_array(json_object_t src, json_object_t dest)
{
        int length = json_array_length(src);

        for (int i = 0; i < length; i++) {
                json_object_t val_src = json_array_get(src, i);
                json_object_t val_dst = json_array_get(dest, i);
                
                if (json_isstring(val_src)) {
                        json_array_set(dest, val_src, i);
                        
                } else if (json_isnumber(val_src)) {
                        json_array_set(dest, val_src, i);
                        
                } else if (json_isobject(val_src)) {
                        if (!json_isobject(val_dst))
                                json_array_set(dest, val_dst, i);
                        else
                                config_merge_object(val_src, val_dst);
                        
                }  else if (json_isarray(val_src)) {
                        if (!json_isarray(val_dst))
                                json_array_set(dest, val_dst, i);
                        else
                                config_merge_array(val_src, val_dst);
                }
        }
}

static int32 config_copy_object_field(json_object_t dest, const char* key, json_object_t value)
{
        if (json_isobject(value)) {
                json_object_t v = json_object_get(dest, key);
                if (!json_isobject(v))
                        json_object_set(dest, key, v);
                else
                        config_merge_object(value, v);

        } else if (json_isarray(value)) {
                json_object_t v = json_object_get(dest, key);
                if (!json_isarray(v))
                        json_object_set(dest, key, value);
                else
                        config_merge_array(value, v);

        } else if (json_isstring(value)) { 
                json_object_set(dest, key, value);

        } else if (json_isnumber(value)) { 
                json_object_set(dest, key, value);
        }

        return 0;
}
 
static int32 config_copy_iterator(const char* key, json_object_t value, json_object_t dest)
{
        return config_copy_object_field(dest, key, value);
}

static void config_merge_object(json_object_t src, json_object_t dest)
{
        json_object_foreach(src, (json_iterator_t) config_copy_iterator, dest);
}

int config_merge(json_object_t config, const char* filename)
{
        json_object_t newconfig;
        int err;
        char buffer[512];

        log_debug("Config: Loading '%s'", filename); 
        
        newconfig = json_load(filename, &err, buffer, sizeof(buffer));
        if (err != 0) {
                log_err("%s", buffer); 
                return -1;
        } 

        log_debug("Config: Merging '%s'", filename); 

        config_merge_object(newconfig, config);

        config_save(config);

        return 0;
}

void config_check_boot_file(json_object_t config, const char* bootfile)
{
        struct stat attrib_bootfile;
        struct stat attrib_curfile;

        if (stat(bootfile, &attrib_bootfile) != 0)
                return;

        if (stat(_file, &attrib_curfile) != 0)
                return;

        if (attrib_bootfile.st_mtime < attrib_curfile.st_mtime)
                return;

        config_merge(config, bootfile);
}

void config_check_online_file(json_object_t config)
{
        const char* account = json_getstr(config, "general.account");
        if (account == NULL) {
                log_warn("Config: no value for 'general.account'");
                return;
        }

        const char* name = json_getstr(config, "general.name");

        char url[512];
        snprintf(url, 512, "https://p2pfoodlab.net/sensorbox/%s/%s/config.json",
                 account, name);
        url[511] = 0;

        char filename[512];
        snprintf(filename, 512, "/tmp/config-%d.json", getpid());
        filename[511] = 0;

        int r = network_http_get(url, filename);
        if (r != 0) {
                log_warn("Config: failed to download '%s'", url);
                return;
        }

        config_merge(config, filename);
}
