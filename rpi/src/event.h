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

#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

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

event_t* new_event(int minute, event_type_t type);
void event_print(event_t* e);
event_t* eventlist_insert(event_t* events, event_t* e);
void eventlist_print(event_t* events);
void eventlist_delete_all(event_t* events);
event_t* eventlist_get_next(event_t* events, int minute);

#ifdef __cplusplus
}
#endif

#endif 
