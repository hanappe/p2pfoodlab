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

#ifndef _STATUS_H_
#define _STATUS_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

        typedef struct _status_t status_t;

        /*
        struct _status_t {
                int network_connected;
                int network_connection_successes;
                int network_connection_failures;

                int arduino_count_connections;
                int arduino_count_errors;

                time_t last_data_upload;
                time_t last_photo_upload;

                int num_datastreams;
                datastream_t* datastreams;
        };
        */

        status_t* new_status();
        void delete_status(status_t* status);

        int status_load(status_t* status, const char* file);
        int status_store(status_t* status, const char* file);

        void status_set_network_connected(status_t* status, int value);
        int status_get_network_connected(status_t* status);

        void status_network_connection_ok(status_t* status, time_t timestamp);
        void status_network_connection_failed(status_t* status);
        time_t status_network_connection_last(status_t* status);
        int status_network_connection_successes(status_t* status);
        int status_network_connection_failures(status_t* status);

        void status_ntp_connection_ok(status_t* status, time_t timestamp);
        void status_ntp_connection_failed(status_t* status);
        time_t status_ntp_connection_last(status_t* status);
        int status_ntp_connection_successes(status_t* status);
        int status_ntp_connection_failures(status_t* status);

        void status_arduino_connection_ok(status_t* status, time_t timestamp);
        void status_arduino_connection_failed(status_t* status);
        time_t status_arduino_connection_last(status_t* status);
        int status_arduino_connection_successes(status_t* status);
        int status_arduino_connection_failures(status_t* status);

        void status_data_upload_ok(status_t* status, time_t timestamp);
        void status_data_upload_failed(status_t* status);
        time_t status_data_upload_last(status_t* status);
        int status_data_upload_successes(status_t* status);
        int status_data_upload_failures(status_t* status);

        void status_photo_upload_ok(status_t* status, time_t timestamp);
        void status_photo_upload_failed(status_t* status);
        time_t status_photo_upload_last(status_t* status);
        int status_photo_upload_successes(status_t* status);
        int status_photo_upload_failures(status_t* status);

#ifdef __cplusplus
}
#endif

#endif 
