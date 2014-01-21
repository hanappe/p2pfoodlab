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
#include <unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "json.h"
#include "log_message.h"
#include "arduino-api.h"
#include "camera.h"
#include "config.h"
#include "opensensordata.h"

static char* _config_file = "/var/p2pfoodlab/etc/config.json";
static char* _log_file = "/var/p2pfoodlab/log.txt";
static char* _osd_dir = "/var/p2pfoodlab/etc/opensensordata";
static char* _data_file = "/var/p2pfoodlab/datapoints.csv";
static char* _img_dir = "/var/p2pfoodlab/photostream";
static int _test = 0;

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

typedef enum _event_type_t {
        UPDATE_SENSORS = 0,
        UPDATE_CAMERA = 1
} event_type_t;

typedef struct _event_t event_t;

struct _event_t {
        int minute;
        event_type_t type;
        event_t* next;
};

static event_t* new_event(int minute, event_type_t type)
{
        event_t* e = (event_t*) malloc(sizeof(event_t));
        if (e == NULL) 
                return NULL;
        e->minute = minute;
        e->type = type;
        e->next = NULL;
        return e;
}

static void event_print(event_t* e)
{
        log_debug("%02d:%02d %s", 
                  e->minute / 60, 
                  e->minute % 60, 
                  (e->type == UPDATE_SENSORS)? "sensors" : "camera");
}

static int map_datastreams(const char* _osd_dir,
                           unsigned char enabled, 
                           int* ids)
{
        opensensordata_t* osd = new_opensensordata("");
        opensensordata_set_cache_dir(osd, _osd_dir);

        int count = 0;
        if (enabled & RHT03_1_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "t");
                ids[count++] = opensensordata_map_datastream(osd, "rh");
        }
        if (enabled & RHT03_2_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "tx");
                ids[count++] = opensensordata_map_datastream(osd, "rhx");
        }
        if (enabled & SHT15_1_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "t2");
                ids[count++] = opensensordata_map_datastream(osd, "rh2");
        }
        if (enabled & SHT15_2_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "tx2");
                ids[count++] = opensensordata_map_datastream(osd, "rhx2");
        }
        if (enabled & LUMINOSITY_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "lum");
        }
        if (enabled & SOIL_FLAG) {
                ids[count++] = opensensordata_map_datastream(osd, "soil");
        }

        delete_opensensordata(osd);
        return count;
}

static void event_update_sensors()
{
        unsigned char enabled;
        unsigned char period;

        int err = arduino_get_sensors(&enabled, &period);
        if (err != 0) 
                return;

        int datastreams[16];
        int num_datastreams = map_datastreams(_osd_dir, enabled, datastreams);
        if (num_datastreams == -1) 
                return;

        log_info("Found %d datastreams", num_datastreams); 
        if (num_datastreams == 0)
                return;

        err = arduino_store_data(datastreams,
                                 num_datastreams,
                                 _data_file);

        return;
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

static void event_update_camera(json_object_t config)
{
        json_object_t camera_obj = json_object_get(config, "camera");
        if (json_isnull(camera_obj)) {
                log_err("Could not find the camera configuration"); 
                return;
        }

        json_object_t device_str = json_object_get(camera_obj, "device");
        if (!json_isstring(device_str)) {
                log_err("Invalid device configuration"); 
                return;
        }        
        const char* device = json_string_value(device_str);

        json_object_t size_str = json_object_get(camera_obj, "size");
        if (!json_isstring(size_str)) {
                log_err("Image size is not a string"); 
                return;
        }        

        unsigned int width, height;

        if (get_image_size(json_string_value(size_str), &width, &height) != 0) {
                log_err("Invalid image size: %s", 
                        json_string_value(size_str)); 
                return;
        }

        log_info("Grabbing %s image from %s", 
                   json_string_value(size_str), device);

        camera_t* camera = new_camera(device, IO_METHOD_MMAP,
                                      width, height, 90);

        int error = camera_capture(camera);
        if (error) {
                delete_camera(camera);
                log_err("Failed to grab the image"); 
                return;
        }

        char filename[512];
        struct timeval tv;
        struct tm r;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &r);
        snprintf(filename, 512, "%s/%04d%02d%02d-%02d%02d%02d.jpg",
                 _img_dir,
                 1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, 
                 r.tm_hour, r.tm_min, r.tm_sec);

        log_info("Storing image in %s", filename);

        int size = camera_getimagesize(camera);
        unsigned char* buffer = camera_getimagebuffer(camera);

        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                delete_camera(camera);
                log_info("Failed to open file '%s'", filename);
                return;
        }

        size_t n = 0;
        while (n < size) {
                size_t m = fwrite(buffer + n, 1, size - n, fp);
                if ((m == 0) && (ferror(fp) != 0)) { 
                        delete_camera(camera);
                        fclose(fp);
                        log_info("Failed to write to file '%s'", 
                                 filename);
                        return;
                        
                }
                n += m;
        }

        fclose(fp);

        log_info("Finished");

        return;
}

static void event_exec(event_t* e, json_object_t config)
{
        if (e->type == UPDATE_SENSORS) {
                event_update_sensors();
        } else if (e->type == UPDATE_CAMERA) {
                event_update_camera(config);
        }
}

static event_t* eventlist_insert(event_t* events, event_t* e) 
{
        if (events == NULL)
                return e;
        if (events->minute > e->minute) {
                e->next = events;
                return e;
        }

        event_t* prev = events;
        event_t* next = events->next;
        while ((next != NULL) 
               && (next->minute < e->minute)) {
                prev = next;
                next = next->next;
        }

        prev->next = e;
        e->next = next;

        return events;
}

static void eventlist_print(event_t* events) 
{
        event_t* e = events;
        while (e) {
                event_print(e);
                e = e->next;
        }
}

static void eventlist_delete_all(event_t* events) 
{
        while (events) {
                event_t* e = events;
                events = events->next;
                free(e);
        }
}

static event_t* eventlist_get_next(event_t* events, int minute)
{
        if (events == NULL)
                return NULL;
        if (minute <= events->minute)
                return events;
        event_t* prev = events;
        event_t* next = events->next;
        while (next) {
                if ((prev->minute < minute) 
                    && (minute <= next->minute) )
                        return next;
                prev = next;
                next = next->next;
        }
        return NULL;
}

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-c | --config        Configuration file\n"
                 "-l | --log           Log file ('-' for stderr)\n"
                 "-t | --test          Test run\n"
                 "-D | --debug         Print debug message\n"
                  "",
                 argv[0]);
}

static void print_version(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  update\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

static void parse_arguments(int argc, char **argv)
{
        static const char short_options [] = "hvtDc:l:";

        static const struct option
                long_options [] = {
                { "help",        no_argument, NULL, 'h' },
                { "version",     no_argument, NULL, 'v' },
                { "config",      required_argument, NULL, 'c' },
                { "log",         required_argument, NULL, 'l' },
                { "test",        no_argument, NULL, 't' },
                { "debug",       no_argument, NULL, 'D' },
                { 0, 0, 0, 0 }
        };

        for (;;) {
                int index, c = 0;
                
                c = getopt_long(argc, argv, short_options, long_options, &index);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);
                case 'v':
                        print_version(stdout);
                        exit(EXIT_SUCCESS);
                case 't':
                        _test = 1;
                        break;
                case 'D':
                        log_set_level(LOG_DEBUG);
                        break;
                case 'c':
                        _config_file = optarg;
                        break;
                case 'l':
                        _log_file = optarg;
                        break;
                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
}

static event_t* add_periodic_events(int period, event_t* list, int type)
{
        log_debug("Add periodic events (period %d).", period);

        int minutes_day = 24 * 60;
        int minute = 0;
        while (minute < minutes_day) {
                event_t* e = new_event(minute, type);
                list = eventlist_insert(list, e);
                minute += period;
        }
        return list;
}

static event_t* add_fixed_events(json_object_t fixed, event_t* list, int type)
{
        log_debug("Add fixed events");

        int num = json_array_length(fixed);
        for (int i = 0; i < num; i++) {
                json_object_t time = json_array_get(fixed, i);
                if (!json_isobject(time)) {
                        log_err("Camera fixed update time setting is invalid"); 
                        continue;
                }
                json_object_t hs = json_object_get(time, "h");
                if (!json_isstring(hs)) {
                        log_err("Camera fixed update time setting is invalid"); 
                        continue;
                }
                if (json_string_equals(hs, "")) {
                        continue;
                }
                int h = atoi(json_string_value(hs));
                if ((h < 0) || (h > 23)) {
                        log_err("Invalid camera update hour: %d", h); 
                        continue;
                }
                json_object_t ms = json_object_get(time, "m");
                if (!json_isstring(ms)) {
                        log_err("Camera fixed update time setting is invalid"); 
                        continue;
                }
                if (json_string_equals(ms, "")) {
                        continue;
                }
                int m = atoi(json_string_value(ms));
                if ((m < 0) || (m > 59)) {
                        log_err("Invalid camera update minute: %d", m); 
                        continue;
                }
                event_t* e = new_event(h * 60 + m, type);
                list = eventlist_insert(list, e);                
        }
        return list;
}

static event_t* add_camera_events(json_object_t config, event_t* list)
{
        log_debug("Camera events");

        json_object_t camera = json_object_get(config, "camera");
        if (!json_isobject(camera)) {
                log_err("Camera settings are not a JSON object, as expected"); 
                return list;
        }
        json_object_t enabled = json_object_get(camera, "enable");
        if (!json_isstring(enabled)) {
                log_err("Camera enabled setting is not a JSON string, as expected"); 
                return list;
        }
        if (!json_string_equals(enabled, "yes")) {
                log_debug("Camera not enabled");
                return list;
        }
        json_object_t update = json_object_get(camera, "update");
        if (!json_isstring(update)) {
                log_err("Camera update setting is not a JSON string, as expected"); 
                return list;
        }

        if (json_string_equals(update, "fixed")) {
                json_object_t fixed = json_object_get(camera, "fixed");
                if (!json_isarray(fixed)) {
                        log_err("Camera fixed settings are not a JSON array, as expected"); 
                        return list;
                }
                return add_fixed_events(fixed, list, UPDATE_CAMERA);

        } else if (json_string_equals(update, "periodical")) {
                json_object_t period = json_object_get(camera, "period");
                if (!json_isstring(period)) {
                        log_err("Camera period setting is not a JSON string, as expected"); 
                        return list;
                }
                int value = atoi(json_string_value(period));
                if (value <= 0) {
                        log_err("Invalid camera period setting: %d", value); 
                        return list;
                }
                return add_periodic_events(value, list, UPDATE_CAMERA);

        } else {
                log_err("Invalid camera update setting: '%s'", 
                           json_string_value(update)); 
                return list;
        }        
        return list; // not reached
}

static event_t* add_sensor_events(json_object_t config, event_t* list)
{
        log_debug("Sensor events");

        json_object_t sensors = json_object_get(config, "sensors");
        if (!json_isobject(sensors)) {
                log_err("Sensors settings are not a JSON object, as expected"); 
                return list;
        }
        json_object_t upload = json_object_get(sensors, "upload");
        if (!json_isstring(upload)) {
                log_err("Sensors upload setting is not a JSON string, as expected"); 
                return list;
        }

        if (json_string_equals(upload, "fixed")) {
                json_object_t fixed = json_object_get(sensors, "fixed");
                if (!json_isarray(fixed)) {
                        log_err("Sensors fixed settings are not a JSON array, as expected"); 
                        return list;
                }
                return add_fixed_events(fixed, list, UPDATE_SENSORS);

        } else if (json_string_equals(upload, "periodical")) {
                json_object_t period = json_object_get(sensors, "period");
                if (!json_isstring(period)) {
                        log_err("Sensors period setting is not a JSON string, as expected"); 
                        return list;
                }
                int value = atoi(json_string_value(period));
                if (value <= 0) {
                        log_err("Invalid sensors period setting: %d", value); 
                        return list;
                }
                return add_periodic_events(value, list, UPDATE_SENSORS);

        } else {
                log_err("Invalid sensors update setting: '%s'", 
                        json_string_value(upload)); 
                return list;
        }        

        return list; // not reached
}

static void handle_events(json_object_t config, event_t* events, int test)
{
        time_t t;
        struct tm tm;

        t = time(NULL);
        localtime_r(&t, &tm);
        log_debug("current time: %02d:%02d.", tm.tm_hour, tm.tm_min);
        int cur_minute = tm.tm_hour * 60 + tm.tm_min;

        event_t* e = eventlist_get_next(events, cur_minute);
        if (e == NULL) {
                log_err("No events!"); 
                return;
        }

        while ((e != NULL) &&
               (e->minute == cur_minute)) {
                if (test) 
                        printf("EXEC %s\n", (e->type == UPDATE_SENSORS)? "update sensors" : "update camera");
                else
                        event_exec(e, config);
                e = e->next;
        }
}

static int poweroff_enabled(json_object_t config)
{
        json_object_t power = json_object_get(config, "power");
        if (!json_isobject(power)) {
                log_err("Power settings are not a JSON object, as expected"); 
                return 0;
        }
        json_object_t poweroff = json_object_get(power, "poweroff");
        if (!json_isstring(poweroff)) {
                log_err("Poweroff setting is not a JSON string, as expected"); 
                return 0;
        }
        if (json_string_equals(poweroff, "yes")) {
                return 1;
        }
        return 0;
}

static void do_poweroff(int minutes)
{
        int err = arduino_set_poweroff(minutes);
        if (err != 0) return;
        log_info("Poweroff"); 
        char* argv[] = { "/usr/bin/sudo", "/sbin/poweroff", NULL};
        execv(argv[0], argv);
        log_err("Failed to poweroff");         
}

static void poweroff_maybe(json_object_t config, event_t* events, int test)
{
        time_t t;
        struct tm tm;

        t = time(NULL);
        localtime_r(&t, &tm);
        log_debug("current time: %02d:%02d.", tm.tm_hour, tm.tm_min);
        int cur_minute = tm.tm_hour * 60 + tm.tm_min;

        event_t* e = eventlist_get_next(events, cur_minute);
        if (e == NULL)
                e = eventlist_get_next(events, 0);
        if (e == NULL)
                return;

        int delta;
        if (e->minute < cur_minute) 
                delta = e->minute + 24 * 60 - cur_minute;
        else
                delta = e->minute - cur_minute;

        log_debug("next event in %d minute(s)", delta);

        if ((delta > 2) && poweroff_enabled(config)) {
                if (test) 
                        printf("POWEROFF %d\n", delta - 2);
                else
                        do_poweroff(delta - 3);
        }
}

static void upload_data(json_object_t config, const char* filename)
{
        struct stat buf;
        if (stat(filename, &buf) == -1) {
                return;
        }
        if (((buf.st_mode & S_IFMT) != S_IFREG) 
            || (buf.st_size == 0)) {
                return;
        }

        json_object_t osd_config = json_object_get(config, "opensensordata");
        if (!json_isobject(osd_config)) {
                log_err("OpenSensorData settings are not a JSON object, as expected"); 
                return;
        }
        json_object_t server = json_object_get(osd_config, "server");
        if (!json_isstring(server)) {
                log_err("OpenSensorData server setting is not a JSON string, as expected"); 
                return;
        }
        json_object_t key = json_object_get(osd_config, "key");
        if (!json_isstring(key)) {
                log_err("OpenSensorData key is not a JSON string, as expected"); 
                return;
        }

        opensensordata_t* osd = new_opensensordata(json_string_value(server));
        if (osd == NULL) {
                log_err("Out of memory"); 
                return;
        }

        opensensordata_set_key(osd, json_string_value(server));

        int ret = opensensordata_put_datapoints(osd, filename);
        if (ret != 0) {
                log_err("Upload failed"); 
        }

        delete_opensensordata(osd);
}

static event_t* build_eventlist(json_object_t config)
{
        event_t* events = NULL;
        events = add_camera_events(config, events);
        events = add_sensor_events(config, events);
        return events;
}

static void init_log(const char* file)
{
        FILE* log = NULL;
        if (strcmp(file, "-") == 0) {
                log = stderr;
        } else {
                log = fopen(file, "a");
        }
        if (log == NULL) {
                fprintf(stderr, "Failed to open the log file ('%s')", file);
                log_set_filep(stderr);
        } else {
                log_set_filep(log);
        }
}

int main(int argc, char **argv)
{
        parse_arguments(argc, argv);

        init_log(_log_file);

        json_object_t config = config_load(_config_file);
        if (json_isnull(config)) {
                log_err("Failed to load the config file"); 
                exit(1);
        } 

        event_t* events = build_eventlist(config);
        eventlist_print(events);

        handle_events(config, events, _test);

        upload_data(config, _data_file);

        poweroff_maybe(config, events, _test);

        eventlist_delete_all(events);

        return 0;
}
