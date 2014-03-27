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

#ifndef _SENSORBOX_H_
#define _SENSORBOX_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

        typedef struct _sensor_t
        {
                int index;
                int flag;
                int enabled;
                const char* name;
        } sensor_t;

        typedef struct _datastream_t
        {
                int index;
                int enabled;
                int sensor;
                const char* name;
                const char* unit;
                int osd_id;

                time_t timestamp;
                short value;
        } datastream_t;

        typedef struct _photostream_t
        {
                int enabled;
                const char* name;
                int osd_id;
        } photostream_t;

        /* typedef struct _status_t */
        /* { */
        /*         int network_connected; */
        /*         int network_count_connections; */
        /*         int network_count_errors; */
        /*         int arduino_count_connections; */
        /*         int arduino_count_errors; */
        /*         char* data_on_disk; */
        /*         time_t last_data_upload; */
        /*         time_t last_photo_upload; */
        /*         int count_photos_on_disk; */
        /*         char** photos_on_disk; */
        /*         int num_datastreams; */
        /*         datastream_t* datastreams; */
        /* } status_t; */

        /* void status_init(status_t* status); */
        /* void status_delete(status_t* status); */

        typedef struct _sensorbox_t sensorbox_t;

        sensorbox_t* new_sensorbox(const char* dir);
        int delete_sensorbox(sensorbox_t* sensorbox);

        const char* sensorbox_getdir(sensorbox_t* box);
        void sensorbox_test_run(sensorbox_t* sensorbox);
        void sensorbox_handle_events(sensorbox_t* box);
        void sensorbox_upload_data(sensorbox_t* box);
        void sensorbox_upload_photos(sensorbox_t* box);
        void sensorbox_poweroff_maybe(sensorbox_t* box);
        int sensorbox_powersaving_enabled(sensorbox_t* box);
        int sensorbox_check_sensors(sensorbox_t* box);
        int sensorbox_check_osd_definitions(sensorbox_t* box);
        int sensorbox_create_osd_definitions(sensorbox_t* box);
        int sensorbox_print_events(sensorbox_t* box);

        /* If filename is NULL, the default output file will be
           used. If filename equals '-', the data will be printed to
           the console (stdout). */
        int sensorbox_store_sensor_data(sensorbox_t* box, const char* filename);

        void sensorbox_update_camera(sensorbox_t* box, time_t t);
        void sensorbox_grab_image(sensorbox_t* box, const char* filename);
        int sensorbox_get_time(sensorbox_t* box, time_t* m);
        int sensorbox_set_time(sensorbox_t* box, time_t m);

        /* Number of seconds since system start. */
        int sensorbox_uptime(sensorbox_t* box);

        int sensorbox_run_ntp(sensorbox_t* box);

        int sensorbox_bring_network_up(sensorbox_t* box);
        int sensorbox_bring_network_down(sensorbox_t* box);
        void sensorbox_bring_network_down_maybe(sensorbox_t* box);

        int sensorbox_lock(sensorbox_t* box);
        void sensorbox_unlock(sensorbox_t* box);

        /* int sensorbox_get_status(sensorbox_t* box, status_t* status); */

        const char* sensorbox_config_getstr(sensorbox_t* box, const char* expr);
        double sensorbox_config_getnum(sensorbox_t* box, const char* expr);

        void sensorbox_measure(sensorbox_t* box);

#ifdef __cplusplus
}
#endif

#endif 
