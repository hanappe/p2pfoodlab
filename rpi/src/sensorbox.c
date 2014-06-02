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
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h> 
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "json.h"
#include "log_message.h"
#include "config.h"
#include "event.h"
#include "arduino.h"
#include "camera.h"
#include "network.h"
#include "opensensordata.h"
#include "system.h"
#include "sensorbox.h"

struct _sensorbox_t {
        char* home_dir;
        json_object_t config;
        arduino_t* arduino;
        camera_t* camera;
        opensensordata_t* osd;
        event_t* events;
        char filenamebuf[2048];
        int test;
        FILE* datafp;
        int lock;
        unsigned char sensors_enabled;
        unsigned char sensors_period;
        sensor_t sensors[SENSOR_COUNT];
        datastream_t datastreams[DATASTREAM_COUNT];
        photostream_t photostream;
};

static int sensorbox_load_config(sensorbox_t* box);
static int sensorbox_load_sensors(sensorbox_t* box);
static char* sensorbox_path(sensorbox_t* box, const char* filename);
static int sensorbox_init_osd(sensorbox_t* box);
static int sensorbox_add_periodic_events(sensorbox_t* box, int period, int type);
static int sensorbox_add_fixed_events(sensorbox_t* box, json_object_t fixed, int type);
static int sensorbox_init_arduino(sensorbox_t* box);
static int sensorbox_init_camera(sensorbox_t* box);
static void sensorbox_handle_event(sensorbox_t* box, event_t* e, time_t t);
static void sensorbox_poweroff(sensorbox_t* box, int minutes);
static int sensorbox_upload_status(sensorbox_t* box);

sensorbox_t* new_sensorbox(const char* dir)
{
        sensorbox_t* box = (sensorbox_t*) malloc(sizeof(sensorbox_t));
        if (box == NULL) { 
                log_err("Sensorbox: out of memory");
                return NULL;
        }
        memset(box, 0, sizeof(sensorbox_t));

        box->home_dir = strdup(dir);
        box->lock = -1;

        if (sensorbox_load_config(box) != 0) {
                delete_sensorbox(box);
                return NULL;
        }

        return box;
}

int sensorbox_init(sensorbox_t* box)
{
        sensorbox_init_arduino(box);
        /* if (sensorbox_init_arduino(box) != 0) { */
        /*         delete_sensorbox(box); */
        /*         return NULL; */
        /* } */

        if (sensorbox_init_camera(box) != 0) {
                delete_sensorbox(box);
                return -1;
        }

        if (sensorbox_init_osd(box) != 0) {
                delete_sensorbox(box);
                return -1;
        }

        if (sensorbox_load_sensors(box) != 0) {
                delete_sensorbox(box);
                return -1;
        }

        return 0;
}

int delete_sensorbox(sensorbox_t* box)
{
        json_unref(box->config);
        if (box->osd)
                delete_opensensordata(box->osd);
        if (box->camera)
                delete_camera(box->camera);
        if (box->arduino)
                delete_arduino(box->arduino);
        eventlist_delete_all(box->events);
        free(box);
        return 0;
}

static char* sensorbox_path(sensorbox_t* box, const char* filename)
{
        int len = sizeof(box->filenamebuf);
        snprintf(box->filenamebuf, len, "%s/%s", box->home_dir, filename);
        box->filenamebuf[len-1] = 0;
        return box->filenamebuf;
}

static int sensorbox_load_config(sensorbox_t* box)
{
        box->config = config_load(sensorbox_path(box, "etc/config.json"));
        if (json_isnull(box->config)) {
                log_err("Sensorbox: Failed to load the config file"); 
                return -1;
        } 
        return 0;
}

static int sensorbox_init_osd(sensorbox_t* box)
{
        json_object_t osd_config = json_object_get(box->config, "opensensordata");
        if (!json_isobject(osd_config)) {
                log_err("Sensorbox: OpenSensorData settings are not a JSON object, as expected"); 
                return -1;
        }
        json_object_t server = json_object_get(osd_config, "server");
        if (!json_isstring(server)) {
                log_err("Sensorbox: OpenSensorData server setting is not a JSON string, as expected"); 
                return -1;
        }
        json_object_t key = json_object_get(osd_config, "key");
        if (!json_isstring(key)) {
                log_err("Sensorbox: OpenSensorData key is not a JSON string, as expected"); 
                return -1;
        }

        box->osd = new_opensensordata(json_string_value(server));
        if (box->osd == NULL) {
                log_err("Sensorbox: Out of memory"); 
                return -1;
        }

        opensensordata_set_key(box->osd, json_string_value(key));
        opensensordata_set_cache_dir(box->osd, sensorbox_path(box, "etc/opensensordata"));

        return 0;
}

static int sensorbox_load_sensors(sensorbox_t* box)
{
        /* sensors */
        box->sensors[SENSOR_TRH_ID].index = SENSOR_TRH_ID;
        box->sensors[SENSOR_TRH_ID].flag = SENSOR_TRH;
        box->sensors[SENSOR_TRH_ID].name = "trh";
        
        box->sensors[SENSOR_TRHX_ID].index = SENSOR_TRHX_ID;
        box->sensors[SENSOR_TRHX_ID].flag = SENSOR_TRHX;
        box->sensors[SENSOR_TRHX_ID].name = "trhx";
        
        box->sensors[SENSOR_LUM_ID].index = SENSOR_LUM_ID;
        box->sensors[SENSOR_LUM_ID].flag = SENSOR_LUM;
        box->sensors[SENSOR_LUM_ID].name = "light";

        box->sensors[SENSOR_USBBAT_ID].flag = SENSOR_USBBAT;
        box->sensors[SENSOR_USBBAT_ID].enabled = 1;
        box->sensors[SENSOR_USBBAT_ID].name = "usbbat";

        box->sensors[SENSOR_SOIL_ID].index = SENSOR_SOIL_ID;
        box->sensors[SENSOR_SOIL_ID].flag = SENSOR_SOIL;
        box->sensors[SENSOR_SOIL_ID].name = "soil";

        /* datastreams */
        box->datastreams[DATASTREAM_T].index = DATASTREAM_T;
        box->datastreams[DATASTREAM_T].sensor = SENSOR_TRH_ID;
        box->datastreams[DATASTREAM_T].name = "t";
        box->datastreams[DATASTREAM_T].unit = "Celsius";
        
        box->datastreams[DATASTREAM_RH].index = DATASTREAM_RH;
        box->datastreams[DATASTREAM_RH].sensor = SENSOR_TRH_ID;
        box->datastreams[DATASTREAM_RH].name = "rh";
        box->datastreams[DATASTREAM_RH].unit = "RH%";
        
        box->datastreams[DATASTREAM_TX].index = DATASTREAM_TX;
        box->datastreams[DATASTREAM_TX].sensor = SENSOR_TRHX_ID;
        box->datastreams[DATASTREAM_TX].name = "tx";
        box->datastreams[DATASTREAM_TX].unit = "Celsius";
        
        box->datastreams[DATASTREAM_RHX].index = DATASTREAM_RHX;
        box->datastreams[DATASTREAM_RHX].sensor = SENSOR_TRHX_ID;
        box->datastreams[DATASTREAM_RHX].name = "rhx";
        box->datastreams[DATASTREAM_RHX].unit = "RH%";

        box->datastreams[DATASTREAM_LUM].index = DATASTREAM_LUM;
        box->datastreams[DATASTREAM_LUM].sensor = SENSOR_LUM_ID;
        box->datastreams[DATASTREAM_LUM].name = "light";
        box->datastreams[DATASTREAM_LUM].unit = "none";

        box->datastreams[DATASTREAM_USBBAT].index = DATASTREAM_USBBAT;
        box->datastreams[DATASTREAM_USBBAT].sensor = SENSOR_USBBAT_ID;
        box->datastreams[DATASTREAM_USBBAT].name = "usbbat";
        box->datastreams[DATASTREAM_USBBAT].unit = "%";

        box->datastreams[DATASTREAM_SOIL].index = DATASTREAM_SOIL;
        box->datastreams[DATASTREAM_SOIL].sensor = SENSOR_SOIL_ID;
        box->datastreams[DATASTREAM_SOIL].name = "soil";
        box->datastreams[DATASTREAM_SOIL].unit = "RH%";

        int err = config_get_sensors(box->config, 
                                     box->sensors, SENSOR_COUNT,
                                     &box->sensors_enabled, 
                                     &box->sensors_period);
        if (err != 0) 
                return err;

        if (box->sensors_enabled & SENSOR_TRH) {
                box->sensors[SENSOR_TRH_ID].enabled = 1;
                box->datastreams[DATASTREAM_T].enabled = 1;
                box->datastreams[DATASTREAM_RH].enabled = 1;
                box->datastreams[DATASTREAM_T].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_T].name);

                box->datastreams[DATASTREAM_RH].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_RH].name);
        }

        if (box->sensors_enabled & SENSOR_TRHX) {
                box->sensors[SENSOR_TRHX_ID].enabled = 1;
                box->datastreams[DATASTREAM_TX].enabled = 1;
                box->datastreams[DATASTREAM_RHX].enabled = 1;
                box->datastreams[DATASTREAM_TX].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_TX].name);
                box->datastreams[DATASTREAM_RHX].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_RHX].name);
        }

        if (box->sensors_enabled & SENSOR_LUM) {
                box->sensors[SENSOR_LUM_ID].enabled = 1;
                box->datastreams[DATASTREAM_LUM].enabled = 1;
                box->datastreams[DATASTREAM_LUM].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_LUM].name);
        }

        if (box->sensors_enabled & SENSOR_SOIL) {
                box->sensors[SENSOR_SOIL_ID].enabled = 1;
                box->datastreams[DATASTREAM_SOIL].enabled = 1;
                box->datastreams[DATASTREAM_SOIL].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_SOIL].name);
        }

        if (box->sensors_enabled & SENSOR_USBBAT) {
                box->sensors[SENSOR_USBBAT_ID].enabled = 1;
                box->datastreams[DATASTREAM_USBBAT].enabled = 1;
                box->datastreams[DATASTREAM_USBBAT].osd_id = 
                        opensensordata_get_datastream_id(box->osd, 
                                                         box->datastreams[DATASTREAM_USBBAT].name);
        }

        /* photostream */
        
        box->photostream.enabled = config_camera_enabled(box->config);
        box->photostream.name = "webcam";
        if (box->photostream.enabled) 
                box->photostream.osd_id = 
                        opensensordata_get_photostream_id(box->osd, 
                                                          "webcam");


        /* for (int i = 0; i < DATASTREAM_COUNT; i++)  */
        /*         if (ids[i] == -1) */
        /*                 return -1; */

        return 0;        
}

static int get_image_size(const char* symbol, unsigned int* width, unsigned int* height)
{
        typedef struct _image_size_t {
                const char* symbol;
                unsigned int width;
                unsigned int height;
        } image_size_t;
        
        static image_size_t image_sizes[] = {
                { "320x240", 320, 240 },
                { "640x480", 640, 480 },
                { "960x720", 960, 720 },
                { "1024x768", 1024, 768 },
                { "1280x720", 1280, 720 },
                { "1280x960", 1280, 960 },
                { "1920x1080", 1920, 1080 },
                { NULL, 0, 0 }};

        for (int i = 0; image_sizes[i].symbol != 0; i++) {
                if (strcmp(image_sizes[i].symbol, symbol) == 0) {
                        *width = image_sizes[i].width;
                        *height = image_sizes[i].height;
                        return 0;
                }
        }

        return -1;
}

static int sensorbox_add_periodic_events(sensorbox_t* box, int period, int type)
{
        int minutes_day = 24 * 60;
        int minute = 0;
        while (minute < minutes_day) {
                event_t* e = new_event(minute, type);
                if (e == NULL)
                        return -1;
                box->events = eventlist_insert(box->events, e);
                minute += period;
        }
        return 0;
}

static int sensorbox_add_fixed_events(sensorbox_t* box, json_object_t fixed, int type)
{
        int num = json_array_length(fixed);
        for (int i = 0; i < num; i++) {
                json_object_t time = json_array_get(fixed, i);
                if (!json_isobject(time)) {
                        log_err("Sensorbox: Camera fixed update time setting is invalid"); 
                        continue;
                }
                json_object_t hs = json_object_get(time, "h");
                if (!json_isstring(hs)) {
                        log_err("Sensorbox: Camera fixed update time setting is invalid"); 
                        continue;
                }
                if (json_string_equals(hs, "")) {
                        continue;
                }
                int h = atoi(json_string_value(hs));
                if ((h < 0) || (h > 23)) {
                        log_err("Sensorbox: Invalid camera update hour: %d", h); 
                        continue;
                }
                json_object_t ms = json_object_get(time, "m");
                if (!json_isstring(ms)) {
                        log_err("Sensorbox: Camera fixed update time setting is invalid"); 
                        continue;
                }
                if (json_string_equals(ms, "")) {
                        continue;
                }
                int m = atoi(json_string_value(ms));
                if ((m < 0) || (m > 59)) {
                        log_err("Sensorbox: Invalid camera update minute: %d", m); 
                        continue;
                }
                event_t* e = new_event(h * 60 + m, type);
                if (e == NULL)
                        return -1;
                box->events = eventlist_insert(box->events, e);
        }
        return 0;
}

static int sensorbox_init_camera(sensorbox_t* box)
{
        if (!config_camera_enabled(box->config)) {
                log_debug("Sensorbox: Camera not enabled");
                return 0;
        }
        
        json_object_t camera_obj = json_object_get(box->config, "camera");
        if (json_isnull(camera_obj)) {
                log_err("Sensorbox: Could not find the camera configuration"); 
                return -1;
        }

        json_object_t update = json_object_get(camera_obj, "update");
        if (!json_isstring(update)) {
                log_err("Sensorbox: Camera update setting is not a JSON string, as expected"); 
                return -1;
        }

        if (json_string_equals(update, "fixed")) {
                json_object_t fixed = json_object_get(camera_obj, "fixed");
                if (!json_isarray(fixed)) {
                        log_err("Sensorbox: Camera fixed settings are not a JSON array, as expected"); 
                        return -1;
                }
                if (sensorbox_add_fixed_events(box, fixed, UPDATE_CAMERA) != 0)
                        return -1;

        } else if (json_string_equals(update, "periodical")) {
                json_object_t period = json_object_get(camera_obj, "period");
                if (!json_isstring(period)) {
                        log_err("Sensorbox: Camera period setting is not a JSON string, as expected"); 
                        return -1;
                }
                int value = atoi(json_string_value(period));
                if (value <= 0) {
                        log_err("Sensorbox: Invalid camera period setting: %d", value); 
                        return -1;
                }
                if (sensorbox_add_periodic_events(box, value, UPDATE_CAMERA) != 0)
                        return -1;
                        

        } else {
                log_err("Sensorbox: Invalid camera update setting: '%s'", 
                           json_string_value(update)); 
                return -1;
        }        

        json_object_t device_str = json_object_get(camera_obj, "device");
        if (!json_isstring(device_str)) {
                log_err("Sensorbox: Invalid device configuration"); 
                return -1;
        }        
        const char* device = json_string_value(device_str);

        json_object_t size_str = json_object_get(camera_obj, "size");
        if (!json_isstring(size_str)) {
                log_err("Sensorbox: Image size is not a string"); 
                return -1;
        }        

        unsigned int width, height;

        if (get_image_size(json_string_value(size_str), &width, &height) != 0) {
                log_err("Sensorbox: Invalid image size: %s", 
                        json_string_value(size_str)); 
                return -1;
        }

        box->camera = new_camera(device, IO_METHOD_MMAP,
                                 width, height, 90);

        if (box->camera == NULL)
                return -1;

        return 0;
}

static int sensorbox_init_arduino(sensorbox_t* box)
{
        static int bus = 1;
        static int address = 0x04;

        box->arduino = new_arduino(bus, address);
        if (box->arduino == NULL)
                return -1;

        json_object_t sensors = json_object_get(box->config, "sensors");
        if (!json_isobject(sensors)) {
                log_err("Sensorbox: Sensors settings are not a JSON object, as expected"); 
                goto error_recovery;
        }

        json_object_t upload = json_object_get(sensors, "upload");
        if (!json_isstring(upload)) {
                log_err("Sensorbox: Sensors upload setting is not a JSON string, as expected"); 
                goto error_recovery;
        }

        if (json_string_equals(upload, "fixed")) {
                json_object_t fixed = json_object_get(sensors, "fixed");
                if (!json_isarray(fixed)) {
                        log_err("Sensorbox: Sensors fixed settings are not a JSON array, as expected"); 
                        goto error_recovery;
                }
                if (sensorbox_add_fixed_events(box, fixed, UPDATE_SENSORS) != 0)
                        goto error_recovery;

        } else if (json_string_equals(upload, "periodical")) {
                json_object_t period = json_object_get(sensors, "period");
                if (!json_isstring(period)) {
                        log_err("Sensorbox: Sensors period setting is not a JSON string, as expected"); 
                        goto error_recovery;
                }
                int value = atoi(json_string_value(period));
                if (value <= 0) {
                        log_err("Sensorbox: Invalid sensors period setting: %d", value); 
                        goto error_recovery;
                }
                if (sensorbox_add_periodic_events(box, value, UPDATE_SENSORS) != 0)
                        goto error_recovery;

        } else {
                log_err("Sensorbox: Invalid sensors update setting: '%s'", 
                        json_string_value(upload)); 
                goto error_recovery;
        }        

        return 0;

 error_recovery:
        delete_arduino(box->arduino);
        box->arduino = NULL;
        return -1;
}

static void sensorbox_handle_event(sensorbox_t* box, event_t* e, time_t t)
{
        if (e->type == UPDATE_SENSORS) {
                sensorbox_store_sensor_data(box, NULL);
        } else if (e->type == UPDATE_CAMERA) {
                sensorbox_update_camera(box, t);
        }
}

void sensorbox_grab_image(sensorbox_t* box, const char* filename)
{
        if (box->camera == NULL)
                return;

        int error = camera_capture(box->camera);
        if (error) {
                log_err("Sensorbox: Failed to grab the image"); 
                return;
        }

        log_info("Sensorbox: Storing photo in %s", filename);

        int size = camera_getimagesize(box->camera);
        unsigned char* buffer = camera_getimagebuffer(box->camera);

        FILE* fp = NULL;

        if (strcmp(filename, "-") == 0) {
                fp = stdout;
        } else {
                fp = fopen(filename, "w");
        }


        if (fp == NULL) {
                log_info("Sensorbox: Failed to open file '%s'", filename);
                return;
        }

        size_t n = 0;
        while (n < size) {
                size_t m = fwrite(buffer + n, 1, size - n, fp);
                if ((m == 0) && (ferror(fp) != 0)) { 
                        fclose(fp);
                        log_info("Sensorbox: Failed to write to file '%s'", 
                                 filename);
                        return;
                        
                }
                n += m;
        }

        fclose(fp);

        log_info("Sensorbox: Photo capture finished");
}

void sensorbox_update_camera(sensorbox_t* box, time_t t)
{
        char id[512];
        struct tm tm;

        localtime_r(&t, &tm);
        snprintf(id, 512, "%04d%02d%02d-%02d%02d%02d.jpg",
                 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, 
                 tm.tm_hour, tm.tm_min, tm.tm_sec);

        char filename[512];
        snprintf(filename, 512, "photostream/%s", id);

        char* path = sensorbox_path(box, filename);

        sensorbox_grab_image(box, path);
}

int sensorbox_check_sensors(sensorbox_t* box)
{
        unsigned char sensors_a;
        unsigned char period_a;
        int err;

        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return -1;
        }
        
        err = arduino_get_sensors(box->arduino, &sensors_a);
        if (err != 0) 
                return err;
        
        err = arduino_get_period(box->arduino, &period_a);
        if (err != 0) 
                return err;

        log_info("Sensorbox: Arduino: sensors: 0x%02x, period %d", sensors_a, period_a); 
        log_info("Sensorbox: Config:  sensors: 0x%02x, period %d", box->sensors_enabled, box->sensors_period); 

        if (box->sensors_enabled != sensors_a) {
                log_info("Sensorbox: Sensor settings differ between Arduino and config file"); 
                err = arduino_set_sensors(box->arduino, box->sensors_enabled);
        }
        if (box->sensors_period != period_a) {
                log_info("Sensorbox: Period settings differ between Arduino and config file"); 
                err = arduino_set_period(box->arduino, box->sensors_period);
        }

        return err;
}

static int sensorbox_create_datastream(sensorbox_t* box, 
                                       const char* name, 
                                       const char* unit)
{
        double timezone = config_get_timezone(box->config);
        double latitude = config_get_latitude(box->config);
        double longitude = config_get_longitude(box->config);
        
        json_object_t d = new_osd_datastream(name, name, timezone,
                                             latitude, longitude, unit);

        int r = opensensordata_create_datastream(box->osd, name, d);
        if (r != 0) 
                return -1;

        json_unref(d);

        return 0;
}

static int sensorbox_create_photostream(sensorbox_t* box, const char* name)
{
        double timezone = config_get_timezone(box->config);
        double latitude = config_get_latitude(box->config);
        double longitude = config_get_longitude(box->config);
        
        json_object_t d = new_osd_photostream(name, name, timezone,
                                              latitude, longitude);

        int r = opensensordata_create_photostream(box->osd, name, d);
        if (r != 0) 
                return -1;

        json_unref(d);

        return 0;
}

int sensorbox_check_osd_definitions(sensorbox_t* box)
{
        for (int i = 0; i < DATASTREAM_COUNT; i++) {
                if (box->datastreams[i].enabled
                    && (box->datastreams[i].osd_id == -1)) 
                        return -1;
        }
        
        if (box->photostream.enabled
            && (box->photostream.osd_id == -1))
                return -1;

        json_object_t group = opensensordata_get_group(box->osd, 1);
        if (json_isnull(group)) 
                return -1;
        
        int id = osd_object_get_id(group);
        if (id == -1)
                return -1;

        const char* config_name = config_get_sensorbox_name(box->config);

        const char* group_name = osd_object_get_name(group);
        if (group_name == NULL)
                return -1;

        if (strcmp(config_name, group_name) != 0)
                return -1;

        for (int i = 0; i < DATASTREAM_COUNT; i++) {
                if (box->datastreams[i].enabled
                    && (osd_group_has_datastream(group, box->datastreams[i].osd_id) != 1))
                        return -1;
        }
        
        if (box->photostream.enabled
            && (osd_group_has_photostream(group, box->photostream.osd_id) != 1))
                return -1;

        return 0;
}

int sensorbox_create_osd_definitions(sensorbox_t* box)
{
        int r;

        for (int i = 0; i < DATASTREAM_COUNT; i++) {
                if (box->datastreams[i].enabled
                    && (box->datastreams[i].osd_id == -1)) {
                        log_info("Sensorbox: Creating datastream '%s'",
                                 box->datastreams[i].name);
                        r = sensorbox_create_datastream(box,
                                                        box->datastreams[i].name,
                                                        box->datastreams[i].unit);
                        if (r == 0) 
                                box->datastreams[i].osd_id =
                                        opensensordata_get_datastream_id(box->osd,
                                                                         box->datastreams[i].name);
                }
        }

        if (box->photostream.enabled
            && (box->photostream.osd_id == -1)) {
                log_info("Sensorbox: Creating photostream '%s'",
                         box->photostream.name);
                r = sensorbox_create_photostream(box,
                                                 box->photostream.name);
                if (r == 0) 
                        box->photostream.osd_id =
                                opensensordata_get_photostream_id(box->osd,
                                                                  box->photostream.name);
        }

        const char* name = config_get_sensorbox_name(box->config);

        json_object_t g = new_osd_group(name, "P2P Food Lab sensorbox");
        
        int id = opensensordata_get_group_id(box->osd);
        if (id > 0)
                json_object_setnum(g, "id", id);

        for (int i = 0; i < DATASTREAM_COUNT; i++) {
                if (box->datastreams[i].enabled
                    && (box->datastreams[i].osd_id != -1)) {
                        osd_group_add_datastream(g, box->datastreams[i].osd_id);
                }
        }

        if (box->photostream.enabled
            && (box->photostream.osd_id != -1)) {                        
                osd_group_add_photostream(g, box->photostream.osd_id);
        }

        log_info("Sensorbox: Creating group '%s'", name);

        r = opensensordata_create_group(box->osd, g);

        json_unref(g);

        return r;
}

int sensorbox_store_sensor_data(sensorbox_t* box, 
                                const char* filename)
{
        unsigned char sensors_a;
        unsigned char period_a;
        int err;

        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return -1;
        }

        err = arduino_get_sensors(box->arduino, &sensors_a);
        if (err != 0) 
                return err;

        err = arduino_get_period(box->arduino, &period_a);
        if (err != 0) 
                return err;

        if (filename == NULL) {
                filename = sensorbox_path(box, "datapoints.csv");
                box->datafp = fopen(filename, "a");

        } else if (strcmp(filename, "-") == 0) {
                box->datafp = stdout;

        } else {
                box->datafp = fopen(filename, "a");
        }

        if (box->datafp == NULL) {
                log_err("Arduino: Failed to open the output file: %s", 
                        filename);
                return -1;
        }

        int num_points;
        datapoint_t* datapoints = arduino_read_data(box->arduino, &num_points);

        for (int i = 0; i < num_points; i++) {
                struct tm r;
                char s[256];
                
                if (box->datastreams[datapoints[i].datastream].osd_id == -1)
                        continue;

                localtime_r(&datapoints[i].timestamp, &r);
                snprintf(s, 256, "%04d-%02d-%02dT%02d:%02d:%02d",
                         1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, 
                         r.tm_hour, r.tm_min, r.tm_sec);
                
                fprintf(box->datafp, "%d,%s,%f\n", 
                        box->datastreams[datapoints[i].datastream].osd_id, 
                        s, 
                        datapoints[i].value);
        } 

        if (datapoints)
                free(datapoints);

        if (box->datafp != stdout) {
                fclose(box->datafp);
                box->datafp = NULL;
        }

        return err;
}

void sensorbox_test_run(sensorbox_t* box)
{
        box->test = 1;
}

void sensorbox_handle_events(sensorbox_t* box)
{
        int err;
        time_t t;
        struct tm tm;

        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return;
        }

        err = arduino_get_time(box->arduino, &t);
        if (err != 0)
                return;
        //t = time(NULL);
        localtime_r(&t, &tm);
        log_debug("Current time (arduino): %02d:%02d.", tm.tm_hour, tm.tm_min);
        int cur_minute = tm.tm_hour * 60 + tm.tm_min;

        event_t* e = eventlist_get_next(box->events, cur_minute);
        if (e == NULL) {
                log_info("No more events for today."); 
                return;
        }

        while ((e != NULL) &&
               (e->minute == cur_minute)) {
                if (box->test) 
                        log_info("EXEC update %s", (e->type == UPDATE_SENSORS)? "sensors" : "camera");
                else
                        sensorbox_handle_event(box, e, t);
                e = e->next;
        }
}

void sensorbox_upload_data(sensorbox_t* box)
{
        struct stat buf;

        char* filename = sensorbox_path(box, "datapoints.csv");

        if (stat(filename, &buf) == -1) {
                log_debug("Sensorbox: No datapoints to upload");
                return;
        }
        if ((buf.st_mode & S_IFMT) != S_IFREG) {
                log_info("Sensorbox: Datapoints file is not a regular file!");
                return;
        }
        if (buf.st_size == 0) {
                log_debug("Sensorbox: Datapoints file is empty");
                return;
        }
        if (box->test) {
                log_debug("Sensorbox: Upload datapoints (filesize=%d)", (int) buf.st_size); 
                return;
        }

        log_info("Sensorbox: Uploading datapoints (filesize=%d)", (int) buf.st_size); 

        if (sensorbox_bring_network_up(box) != 0) {
                log_err("Sensorbox: Failed to bring the network up");
                return;
        }

        int ret = opensensordata_put_datapoints(box->osd, filename);
        if (ret != 0) {
                log_err("Sensorbox: Uploading of datapoints failed"); 
                char* resp = opensensordata_get_response(box->osd);
                if (resp) 
                        log_err("%s", resp); 
                return;
        } 

        log_info("Sensorbox: Upload successful"); 

        char backupfile[512];
        struct timeval tv;
        struct tm r;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &r);
        snprintf(backupfile, 512, "%s/backup/datapoints-%04d%02d%02d-%02d%02d%02d.csv",
                 box->home_dir,
                 1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, 
                 r.tm_hour, r.tm_min, r.tm_sec);
        backupfile[511] = 0;

        log_info("Sensorbox: Copying datapoints to %s", backupfile);
        
        if (rename(filename, backupfile) == -1) {
                log_err("Sensorbox: Failed to copy datapoints to %s", backupfile); 
        }        

        // FIXME: DEBUG!!!
        log_info("Sensorbox: Uploading log file");
        sensorbox_upload_status(box);
}

void sensorbox_upload_photos(sensorbox_t* box)
{
        DIR *dir;
        struct dirent *entry;
        struct stat buf;
        char filename[512];
        char backupfile[512];

        char* dirname = sensorbox_path(box, "photostream");
        
        dir = opendir(dirname);
        if (dir == NULL) {
                log_err("Sensorbox: Failed to open the photo directory '%s'", dirname);
                return;
        }

        int count = 0;
        while ((entry = readdir(dir)) != NULL) {
                snprintf(filename, 511, "%s/%s", dirname, entry->d_name);
                filename[511] = 0;
        
                if ((stat(filename, &buf) == -1)
                    || ((buf.st_mode & S_IFMT) != S_IFREG)
                    || (buf.st_size == 0))
                        continue;

                count++;
        }

        if (count == 0) {
                closedir(dir);
                return;
        }

        log_info("Sensorbox: Found %d %s on the disk", 
                 count, (count == 1)? "photo" : "photos");

        int photostream = opensensordata_get_datastream_id(box->osd, "webcam");
        
        if (sensorbox_bring_network_up(box) != 0) {
                log_err("Sensorbox: Failed to bring the network up");
                return;
        }

        rewinddir(dir);

        while ((entry = readdir(dir)) != NULL) {
                snprintf(filename, 511, "%s/%s", dirname, entry->d_name);
                filename[511] = 0;
        
                if ((stat(filename, &buf) == -1)
                    || ((buf.st_mode & S_IFMT) != S_IFREG)
                    || (buf.st_size == 0))
                        continue;

                log_info("Sensorbox: Uploading photo '%s'", filename);

                if (box->test)
                        continue;

                int err = opensensordata_put_photo(box->osd, photostream, 
                                                   entry->d_name, filename);
                if (err != 0) {
                        log_err("Sensorbox: Uploading of photo failed"); 
                        char* resp = opensensordata_get_response(box->osd);
                        if (resp) 
                                log_err("%s", resp); 
                        continue;
                }

                snprintf(backupfile, 512, "%s/backup/%s", box->home_dir, entry->d_name);
                backupfile[511] = 0;

                log_info("Sensorbox: Copying photo to %s", backupfile);
                
                if (rename(filename, backupfile) == -1) {
                        log_err("Sensorbox: Failed to copy photo to %s", backupfile); 
                }
        }
        
        closedir(dir);
}

int sensorbox_powersaving_enabled(sensorbox_t* box)
{
        return config_powersaving_enabled(box->config);
}

static void sensorbox_poweroff(sensorbox_t* box, int minutes)
{
        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return;
        }

        int err = arduino_set_poweroff(box->arduino, minutes);
        if (err != 0) 
                return;

        if (1) {
                log_info("Sensorbox: Powering off"); 
                char* argv[] = { "/usr/bin/sudo", "/sbin/poweroff", NULL};
                execv(argv[0], argv);
                log_err("Sensorbox: Failed to poweroff");         

        } else {
                log_info("Sensorbox: Powering off, NOT"); 
        }
}

void sensorbox_poweroff_maybe(sensorbox_t* box)
{
        int err;
        time_t t;
        struct tm tm;

        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return;
        }

        //t = time(NULL);
        err = arduino_get_time(box->arduino, &t);
        if (err != 0)
                return;

        localtime_r(&t, &tm);
        log_debug("Current time: %02d:%02d.", tm.tm_hour, tm.tm_min);
        int cur_minute = tm.tm_hour * 60 + tm.tm_min;

        event_t* e = eventlist_get_next(box->events, cur_minute + 1);
        if (e == NULL)
                e = eventlist_get_next(box->events, 0);
        if (e == NULL)
                return;

        int delta;
        if (e->minute < cur_minute) 
                delta = e->minute + 24 * 60 - cur_minute;
        else
                delta = e->minute - cur_minute;

        if (delta == 0) {
                return;
        } else if (delta == 1) {
                log_info("Next event in 1 minute");
                return;
        } else if (delta <= 5) {
                log_info("Next event in %d minute(s)", delta);
        } else if ((delta < 60) && ((delta % 10) == 0)) {
                log_info("Next event in %d minute(s)", delta);
        } else if ((delta >= 60) && (delta % 60) == 0) {
                log_info("Next event in %d minute(s)", delta);
        }  

        /* Power off if:
           1. poweroff enabled in config
           2. next event is at least 3 minutes in the future
           3. the system has been up for at least 3 minutes.

           The last criteria is to avoid that the sensorbox shuts down
           too quickly to allow people to connect to it.
         */
        int uptime = sensorbox_uptime(box);
        int enabled = sensorbox_powersaving_enabled(box);
        if (!enabled) 
                return;

        if ((delta > 3) && (uptime > 180)) {
                if (box->test) 
                        printf("POWEROFF %d\n", delta - 3);
                else
                        sensorbox_poweroff(box, delta - 3);
        } else if (delta <= 3) {
                log_info("Not powering off, next event is coming soon");
        } else if (uptime <= 180) {
                log_info("Not powering off, system just started (%ds < 180s)", uptime);
        }
}

int sensorbox_get_time(sensorbox_t* box, time_t* m) 
{
        if (arduino_get_time(box->arduino, m) != 0) {
                log_warn("Sensorbox: Using local system time"); 
                time(m);
        }
        return 0;
}

int sensorbox_set_time(sensorbox_t* box, time_t m) 
{
        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return -1;
        }
        return arduino_set_time(box->arduino, m);
}

const char* sensorbox_getdir(sensorbox_t* box)
{
        return box->home_dir;
}


int sensorbox_print_events(sensorbox_t* box)
{
        eventlist_print(box->events, stdout); 
        return 0;
}

int sensorbox_uptime(sensorbox_t* box)
{
        FILE* fp = fopen("/proc/uptime", "r");
        if (fp == NULL) {
                log_err("Failed to open /proc/uptime!");
                return -1;
        }
        double seconds;
        int n = fscanf(fp, "%lf", &seconds);
        if (n != 1) {
                log_err("Failed to read the system uptime!");
                fclose(fp);
                return -1;
        }
        fclose(fp);
        return (int) seconds;
}

int sensorbox_run_ntp(sensorbox_t* box)
{
        struct tm tm;
        struct timeval tv;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        log_debug("Time before NTP (raspberry): %02d:%02d.", tm.tm_hour, tm.tm_min);

        // Use the opportunity to update the clock
        log_info("Network: Running NTPD");
        char* const argv[] = { "/usr/bin/sudo", 
                               "/usr/sbin/ntpd", 
                               "-q", "-g", NULL};
        int ret = system_run(argv);

        log_debug("NTP returned %d", ret);

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        log_debug("Time after NTP (raspberry): %02d:%02d.", tm.tm_hour, tm.tm_min);

        return ret;
}

int sensorbox_bring_network_up(sensorbox_t* box)
{
        const char* iface = config_get_network_interface(box->config);
        int r = network_gogo(iface);
        
        if (r == 0) {
                // Use the opportunity to update the clock
                if (sensorbox_run_ntp(box) == 0)
                        sensorbox_set_time(box, time(NULL)); 
        }

        return r;
}

int sensorbox_bring_network_down(sensorbox_t* box)
{
        const char* iface = config_get_network_interface(box->config);
        return network_byebye(iface);
}

void sensorbox_bring_network_down_maybe(sensorbox_t* box)
{
        const char* iface = config_get_network_interface(box->config);

        if (strcmp(iface, "eth0") == 0) {
                log_info("Sensorbox: Bring eth0 down? No.");
                return;        
        } else if (strcmp(iface, "wlan0") == 0) {
                if (sensorbox_powersaving_enabled(box)) {
                        log_info("Sensorbox: Bring wlan0 down? Yes.");
                        sensorbox_bring_network_down(box);
                } else {
                        log_info("Sensorbox: Bring wlan0 down? No. "
                                 "Not in power saving mode.");
                }
        } else if (strcmp(iface, "ppp0") == 0) {
                log_info("Sensorbox: Bring ppp0 down? Yes.");
                sensorbox_bring_network_down(box);
        }
}

int sensorbox_lock(sensorbox_t* box) 
{
        int fd;
        if ((fd = open("/tmp/sensorbox.lock", O_CREAT | O_RDWR, 0666))  < 0)
                return -1;
        
        if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
                close(fd);
                return -1;
        }
        box->lock = fd;
        return 0;
}

void sensorbox_unlock(sensorbox_t* box) 
{
        if (box->lock == -1)
                return;
        flock(box->lock, LOCK_UN);
        close(box->lock);
        unlink("/tmp/sensorbox.lock");
        box->lock = -1;
}

/* void status_init(status_t* status) */
/* { */
/*         memset(status, 0, sizeof(status_t)); */
/* } */

/* void status_delete(status_t* status) */
/* { */
/*         if (status == NULL) */
/*                 return; */
/*         if (status->data_on_disk) */
/*                 free(status->data_on_disk); */
/*         if (status->datastreams) */
/*                 free(status->datastreams); */
/*         if (status->photos_on_disk) { */
/*                 for (int i = 0; i < status->count_photos_on_disk; i++) */
/*                         if (status->photos_on_disk[i] != NULL) */
/*                                 free(status->photos_on_disk[i]); */
/*         } */
/*         free(status); */
/* } */

/* static void sensorbox_status_load_data(sensorbox_t* box, status_t* status) */
/* { */
/*         struct stat buf; */
/*         char* filename = sensorbox_path(box, "datapoints.csv"); */

/*         if (stat(filename, &buf) == -1) */
/*                 return; */
/*         if ((buf.st_mode & S_IFMT) != S_IFREG) */
/*                 return; */
/*         if (buf.st_size == 0) */
/*                 return; */

/*         FILE* fp = fopen(filename, "r"); */
/*         if (fp == NULL) */
/*                 return; */

/*         status->data_on_disk = malloc(buf.st_size + 1); */
/*         if (status->data_on_disk == NULL) */
/*                 return; */

/*         int n = fread(status->data_on_disk, 1, buf.st_size, fp); */
/*         status->data_on_disk[n] = 0; */

/*         //if (ferror(fp)) ...  */

/*         fclose(fp); */
/* } */

/* static void sensorbox_status_load_photos(sensorbox_t* box, status_t* status) */
/* { */
/*         DIR *dir; */
/*         struct dirent *entry; */
/*         struct stat buf; */
/*         char filename[512]; */

/*         char* dirname = sensorbox_path(box, "photostream"); */
        
/*         dir = opendir(dirname); */
/*         if (dir == NULL) */
/*                 return; */

/*         status->count_photos_on_disk = 0; */
/*         while ((entry = readdir(dir)) != NULL) { */
/*                 snprintf(filename, 511, "%s/%s", dirname, entry->d_name); */
/*                 filename[511] = 0; */
        
/*                 if ((stat(filename, &buf) == -1) */
/*                     || ((buf.st_mode & S_IFMT) != S_IFREG) */
/*                     || (buf.st_size == 0)) */
/*                         continue; */

/*                 status->count_photos_on_disk++; */
/*         } */

/*         rewinddir(dir); */

/*         status->photos_on_disk = malloc(status->count_photos_on_disk * sizeof(char*)); */
/*         if (status->photos_on_disk == NULL) { */
/*                 status->photos_on_disk = 0; */
/*                 return; */
/*         } */

/*         int i = 0; */
/*         while ((entry = readdir(dir)) != NULL) { */
/*                 snprintf(filename, 511, "%s/%s", dirname, entry->d_name); */
/*                 filename[511] = 0; */
        
/*                 if ((stat(filename, &buf) == -1) */
/*                     || ((buf.st_mode & S_IFMT) != S_IFREG) */
/*                     || (buf.st_size == 0)) */
/*                         continue; */

/*                 status->photos_on_disk[i++] = strdup(filename); */
/*         } */
        
/*         closedir(dir); */
/* } */

/* int sensorbox_get_status(sensorbox_t* box, status_t* status) */
/* { */
/*         status_init(status); */

/*         status->network_connected = network_connected(); */

/*         sensorbox_status_load_data(box, status); */
/*         sensorbox_status_load_photos(box, status); */

/*         //sensorbox_status_load_datastreams(box, status); */

/*         return 0; */
/* } */

const char* sensorbox_config_getstr(sensorbox_t* box, const char* expr)
{
        return json_getstr(box->config, expr);
}

double sensorbox_config_getnum(sensorbox_t* box, const char* expr)
{
        return json_getnum(box->config, expr);
}

void sensorbox_measure(sensorbox_t* box)
{
        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return;
        }
        
        int num_points;
        datapoint_t* datapoints = arduino_measure(box->arduino, &num_points);

        printf("number of datapoints : %d\n", num_points);
                
        for (int i = 0; i < num_points; i++) {
                if (box->datastreams[datapoints[i].datastream].osd_id == -1)
                        printf("(%d),%f\n", 
                               datapoints[i].datastream, 
                               datapoints[i].value);
                else
                        printf("%s,%f\n", 
                               box->datastreams[datapoints[i].datastream].name, 
                               datapoints[i].value);
        } 
        
        if (datapoints)
                free(datapoints);
}

void sensorbox_reset_stack(sensorbox_t* box)
{
        if (box->arduino == NULL) {
                log_warn("Sensorbox: Failed to initialise Arduino"); 
                return;
        }
        arduino_reset_stack(box->arduino);        
}

static int sensorbox_generate_network_interfaces(sensorbox_t* box)
{
        char filename[512];

        snprintf(filename, 511, "%s/etc/interfaces", box->home_dir);
        filename[511] = 0;
        
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Failed to open temp network interfaces file: %s", filename);
                return -1;
        }

        fprintf(fp, 
                "auto lo\n"
                "iface lo inet loopback\n\n");

        if (json_streq(box->config, "wired.inet", "dhcp")) {
                fprintf(fp, 
                        "auto eth0\n"
                        "allow-hotplug eth0\n"
                        "iface eth0 inet dhcp\n\n");

        } else if (json_streq(box->config, "wired.inet", "static")) {
                fprintf(fp, 
                        "auto eth0\n"
                        "allow-hotplug eth0\n"
                        "iface eth0 inet static\n"
                        "        address %s\n" 
                        "        netmask %s\n\n",
                        json_getstr(box->config, "wired.static.address"),
                        json_getstr(box->config, "wired.static.netmask"));
        }

        if (json_streq(box->config, "wifi.enable", "yes")) {
                fprintf(fp, 
                        "auto wlan0\n"
                        "allow-hotplug wlan0\n"
                        "iface wlan0 inet dhcp\n"
                        "      wpa-ssid \"%s\"\n"
                        "      wpa-psk \"%s\"\n\n",
                        json_getstr(box->config, "wifi.ssid"),
                        json_getstr(box->config, "wifi.password"));
        }

        if (json_streq(box->config, "gsm.enable", "yes")) {
                fprintf(fp, 
                        "iface ppp0 inet wvdial\n"
                        "      provider gsmmodem\n\n");
        }

        fclose(fp);

        return 0;
}

static int sensorbox_generate_dhcpd_config(sensorbox_t* box)
{
        char filename[512];

        snprintf(filename, 511, "%s/etc/dhcpd.conf", box->home_dir);
        filename[511] = 0;
        
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Failed to open temp dhcpd config file: %s", filename);
                return -1;
        }

        if (json_streq(box->config, "wired.dhcpserver.enable", "yes")) {
                fprintf(fp,
                        "ddns-update-style none;\n"
                        "option domain-name \"p2pfoodlab.lan\";\n"
                        "option domain-name-servers %s\n"
                        "default-lease-time 3600;\n"
                        "max-lease-time 7200;\n"
                        "authoritative;\n"
                        "log-facility local7;\n"
                        "\n"
                        "subnet %s netmask %s {\n"
                        "  range %s %s\n"
                        "  option routers %s\n"
                        "}\n\n",
                        json_getstr(box->config, "wired.static.gateway"),
                        json_getstr(box->config, "wired.dhcpserver.subnet"),
                        json_getstr(box->config, "wired.static.netmask"),
                        json_getstr(box->config, "wired.dhcpserver.from"),
                        json_getstr(box->config, "wired.dhcpserver.to"),
                        json_getstr(box->config, "wired.static.gateway"));
        }
        fclose(fp);
        
        return 0;
}

static int sensorbox_generate_wvdial_config(sensorbox_t* box)
{
        char filename[512];

        snprintf(filename, 511, "%s/etc/wvdial.conf", box->home_dir);
        filename[511] = 0;
        
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Failed to open temp dhcpd config file: %s", filename);
                return -1;
        }

        if (json_streq(box->config, "gsm.enable", "yes")) {
                fprintf(fp,
                        "[Dialer gsmmodem]\n"
                        "Modem = /dev/ttyUSB0\n"
                        "Modem Type = Analog Modem\n"
                        "ISDN = 0\n"
                        "Baud = %s\n"
                        "Username = %s\n"
                        "Password = %s\n"
                        "Init1 = ATZ\n"
                        "Init2 = AT&F E1 V1 X1 &D2 &C1 S0=0\n"
                        "Dial Attempts = 1\n"
                        "Phone = %s\n"
                        "Stupid Mode = 1\n"
                        "Init3 = AT+CGDCONT=1,\"IP\",\"%s\"\n",
                        json_getstr(box->config, "gsm.baud"),
                        json_getstr(box->config, "gsm.username"),
                        json_getstr(box->config, "gsm.password"),
                        json_getstr(box->config, "gsm.phone"),
                        json_getstr(box->config, "gsm.apn"));
                
                if (!json_streq(box->config, "gsm.pin", ""))
                        fprintf(fp, "Init4 = AT+CPIN=%s\n", json_getstr(box->config, "gsm.pin"));
                
                fprintf(fp, "\n");
        }

        fclose(fp);
        
        return 0;
}

static int sensorbox_generate_authorized_keys(sensorbox_t* box)
{
        char filename[512];

        snprintf(filename, 511, "%s/etc/authorized_keys", box->home_dir);
        filename[511] = 0;

        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Failed to open temp dhcpd config file: %s", filename);
                return -1;
        }

        fprintf(fp, "%s\n%s\n%s\n",
                json_getstr(box->config, "ssh.key1"),
                json_getstr(box->config, "ssh.key2"),
                json_getstr(box->config, "ssh.key3"));

        fclose(fp);
        
        return 0;
}

static int sensorbox_generate_hostname(sensorbox_t* box)
{
        const char* name = json_getstr(box->config, "general.name");
        if (name == NULL) {
                log_err("Could not find value of 'general.name' setting!");
                return -1;
        }

        int len = strlen(name);
        if (len < 1) {
                log_err("Value of 'general.name' is too short: %s", name);
                return -1;
        }
        if (len > 63) {
                log_err("Value of 'general.name' is too long: %s", name);
                return -1;
        }
        for (int i = 0; i < len; i++) {
                char c = name[i];
                if (((c < 'a') || (c > 'z'))
                    && ((c < 'A') || (c > 'Z'))
                    && ((c < '0') || (c > '9'))
                    && (c != '-')) {
                        log_err("Invalid value of 'general.name': %s", name);
                        return -1;
                }
        }

        char filename[512];

        snprintf(filename, 511, "%s/etc/hostname", box->home_dir);
        filename[511] = 0;

        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Failed to open temp dhcpd config file: %s", filename);
                return -1;
        }

        fprintf(fp, "%s\n", name);

        fclose(fp);
        
        return 0;
}

static int sensorbox_mkdir(const char *path)
{
        struct stat st;
        
        if (stat(path, &st) != 0) {
                if (mkdir(path, 0777) != 0) {
                        log_err("Failed to create directory %s", path);
                        return -1;
                }
        } else if (!S_ISDIR(st.st_mode)) {
                log_err("Path exists but is not a directory: %s", path);
                return -1;
        }
        
        return 0;
}

static int sensorbox_mkdir_backup(sensorbox_t* box)
{
        char path[512];
        snprintf(path, 511, "%s/backup", box->home_dir);
        path[511] = 0;
        return sensorbox_mkdir(path);
}

static int sensorbox_install_file(sensorbox_t* box,
                                  const char* path,
                                  const char* filename)
{
        char new_file[512];
        char backup_file[512];

        if (sensorbox_mkdir_backup(box) != 0)
                return -1;

        snprintf(new_file, 511, "%s/etc/%s", box->home_dir, filename);
        new_file[511] = 0;

        snprintf(backup_file, 511, "%s/backup/%s-%lu", box->home_dir, filename, time(NULL));
        backup_file[511] = 0;

        if ((access(path, F_OK) == 0) && (access(path, W_OK) != 0)) {
                log_err("Write access denied to file: %s", path);
                return -1;
        }

        if ((access(backup_file, F_OK) == 0) && (access(backup_file, W_OK) != 0)) {
                log_err("Write access denied to file: %s", backup_file);
                return -1;
        }
        
        if (rename(path, backup_file) != 0) {
                log_err("Failed to create backup file: %s", backup_file);
                return -1;
        }
        
        if (rename(new_file, path) != 0) {
                log_err("Failed to install new system file: %s -> %s", new_file, path);
                return -1;
        }

        return 0;
}

static int sensorbox_install_hostname(sensorbox_t* box)
{
        return sensorbox_install_file(box, "/etc/hostname", "hostname");
}

static int sensorbox_install_network_interfaces(sensorbox_t* box)
{
        return sensorbox_install_file(box, "/etc/network/interfaces", "interfaces");
}

static int sensorbox_install_dhcpd_config(sensorbox_t* box)
{
        return sensorbox_install_file(box, "/etc/dhcp/dhcpd.conf", "dhcpd.conf");
}

static int sensorbox_install_wvdial_config(sensorbox_t* box)
{
        return sensorbox_install_file(box, "/etc/wvdial.conf", "wvdial.conf");
}

static int sensorbox_install_authorized_keys(sensorbox_t* box)
{
        return sensorbox_install_file(box, "/home/pi/.ssh/authorized_keys", "authorized_keys");
}

void sensorbox_generate_system_files(sensorbox_t* box)
{
        log_info("Updating system files");
        if (access("/boot/p2pfoodlab.json", F_OK) == 0) {
                log_info("Merging /boot/p2pfoodlab.json");
                config_check_boot_file(box->config, "/boot/p2pfoodlab.json");
                if (rename("/boot/p2pfoodlab.json", "/boot/p2pfoodlab-backup.json") != 0)
                        log_warn("Failed to copy /boot/p2pfoodlab.json to backup file");
                else log_info("/boot/p2pfoodlab.json renamed to /boot/p2pfoodlab-backup.json");
        }

        if (sensorbox_generate_hostname(box) == 0)
                sensorbox_install_hostname(box);

        if (sensorbox_generate_network_interfaces(box) == 0) 
                sensorbox_install_network_interfaces(box);

        if (sensorbox_generate_dhcpd_config(box) != 0) 
                sensorbox_install_dhcpd_config(box);

        if (sensorbox_generate_wvdial_config(box) != 0) 
                sensorbox_install_wvdial_config(box);

        if (sensorbox_generate_authorized_keys(box) != 0) 
                sensorbox_install_authorized_keys(box);        
}

void sensorbox_update_network(sensorbox_t* box, const char* iface)
{
        if (sensorbox_generate_network_interfaces(box) != 0) 
                return;

        if (sensorbox_install_network_interfaces(box) != 0)
                return;

        if (strcmp(iface, "eth0") == 0)
                network_stop_dhcpd();

        network_ifdown(iface);
        network_ifup(iface);

        if ((strcmp(iface, "eth0") == 0)
            && json_streq(box->config, "wired.dhcpserver.enable", "yes"))
                network_start_dhcpd();
}

void sensorbox_update_ssh(sensorbox_t* box)
{
        if (sensorbox_generate_authorized_keys(box) != 0) 
                sensorbox_install_authorized_keys(box);
}

static int sensorbox_upload_status(sensorbox_t* box)
{
        char remote_name[512];
        snprintf(remote_name, 512, 
                 "sensorbox@p2pfoodlab.net:log-%s-%s.txt",
                 json_getstr(box->config, "general.name"),
                 json_getstr(box->config, "opensensordata.key"));
        
        char* const argv[] = { "/usr/bin/rsync", 
                               "-oStrictHostKeyChecking=no",
                               "-oUserKnownHostsFile=/dev/null",
                               "/var/p2pfoodlab/log.txt", 
                               remote_name, 
                               NULL };

        return system_run(argv);        
}

