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

#ifndef _ARDUINO_API_H_
#define _ARDUINO_API_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_TRH_ID      0
#define SENSOR_TRHX_ID     1
#define SENSOR_LUM_ID      2
#define SENSOR_USBBAT_ID   3
#define SENSOR_SOIL_ID     4
#define SENSOR_COUNT       5

#define SENSOR_TRH         (1 << 0)
#define SENSOR_TRHX        (1 << 1)
#define SENSOR_LUM         (1 << 2)
#define SENSOR_USBBAT      (1 << 3)
#define SENSOR_SOIL        (1 << 4)


#define DATASTREAM_T       1
#define DATASTREAM_RH      2
#define DATASTREAM_TX      3
#define DATASTREAM_RHX     4
#define DATASTREAM_LUM     5
#define DATASTREAM_SOIL    6
#define DATASTREAM_USBBAT  7
#define DATASTREAM_COUNT   8

typedef struct _datapoint_t {
        int datastream;
        time_t timestamp;
        float value;
} datapoint_t;

typedef struct _arduino_t arduino_t;

arduino_t* new_arduino(int bus, int address);
int delete_arduino(arduino_t* arduino);

int arduino_get_time(arduino_t* arduino, time_t *time);
int arduino_set_time(arduino_t* arduino, time_t time);

int arduino_set_sensors(arduino_t* arduino,
                        unsigned char sensors);

int arduino_get_sensors(arduino_t* arduino,
                        unsigned char* sensors);

int arduino_set_period(arduino_t* arduino,
                       unsigned char period);
        
int arduino_get_period(arduino_t* arduino,
                       unsigned char* period);
        
int arduino_get_frames(arduino_t* arduino,
                       int* frames);

int arduino_set_poweroff(arduino_t* arduino, int minutes);

int arduino_get_poweroff(arduino_t* arduino, int* minutes);

/* int arduino_set_pump(arduino_t* arduino, int seconds); */

/* int arduino_get_pump(arduino_t* arduino, int* seconds); */


/* typedef void (*arduino_data_callback_t)(void* ptr, */
/*                                         int datastream, */
/*                                         time_t timestamp, */
/*                                         float value); */

datapoint_t* arduino_read_data(arduino_t* arduino, int* num_points);

datapoint_t* arduino_measure(arduino_t* arduino, int* num_points);

#ifdef __cplusplus
}
#endif

#endif // _ARDUINO_API_H_
