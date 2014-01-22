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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sensorbox_t sensorbox_t;

sensorbox_t* new_sensorbox(const char* dir);
int delete_sensorbox(sensorbox_t* sensorbox);

void sensorbox_test_run(sensorbox_t* sensorbox);
void sensorbox_handle_events(sensorbox_t* box);
void sensorbox_upload_data(sensorbox_t* box);
void sensorbox_upload_photos(sensorbox_t* box);
void sensorbox_poweroff_maybe(sensorbox_t* box);

#ifdef __cplusplus
}
#endif

#endif 
