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

#ifdef __cplusplus
extern "C" {
#endif

#define RHT03_1_FLAG (1 << 0)
#define RHT03_2_FLAG (1 << 1)
#define SHT15_1_FLAG (1 << 2)
#define SHT15_2_FLAG (1 << 3)
#define LUMINOSITY_FLAG (1 << 4)
#define SOIL_FLAG (1 << 5)

typedef struct _arduino_t arduino_t;

arduino_t* new_arduino(int bus, int address);
int delete_arduino(arduino_t* arduino);

int arduino_millis(arduino_t* arduino, unsigned long* m);

int arduino_set_sensors(arduino_t* arduino,
                        unsigned char enabled, 
                        unsigned char period);

int arduino_get_sensors(arduino_t* arduino,
                        unsigned char* enabled, 
                        unsigned char* period);

int arduino_store_data(arduino_t* arduino,
                       int* datastreams,
                       int num_datastreams,
                       const char* output_file);

int arduino_set_poweroff(arduino_t* arduino, int minutes);

int arduino_get_poweroff(arduino_t* arduino, unsigned long* off, unsigned long* on);

int arduino_pump(arduino_t* arduino, int seconds);

#ifdef __cplusplus
}
#endif

#endif // _ARDUINO_API_H_
