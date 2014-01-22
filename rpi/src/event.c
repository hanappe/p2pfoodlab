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

#include <stdlib.h>
#include "log_message.h"
#include "event.h"

event_t* new_event(int minute, event_type_t type)
{
        event_t* e = (event_t*) malloc(sizeof(event_t));
        if (e == NULL) {
                log_err("Event: out of memory");
                return NULL;
        }
        e->minute = minute;
        e->type = type;
        e->next = NULL;
        return e;
}

void event_print(event_t* e)
{
        log_debug("Event: %02d:%02d %s", 
                  e->minute / 60, 
                  e->minute % 60, 
                  (e->type == UPDATE_SENSORS)? "sensors" : "camera");
}

event_t* eventlist_insert(event_t* events, event_t* e) 
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

void eventlist_print(event_t* events) 
{
        event_t* e = events;
        while (e) {
                event_print(e);
                e = e->next;
        }
}

void eventlist_delete_all(event_t* events) 
{
        while (events) {
                event_t* e = events;
                events = events->next;
                free(e);
        }
}

event_t* eventlist_get_next(event_t* events, int minute)
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
