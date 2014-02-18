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
#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

        
        typedef struct _process_t
        {
                pid_t id;
                int in;
                int out;
                int err;
                int exited;
                int ret;
                int status;
        } process_t;
        
        process_t* new_process(pid_t id,
                               int in,
                               int out,
                               int err);
        
        void delete_process(process_t* p);
        void process_wait(process_t* p);

        process_t* system_exec(const char* path, char* const argv[], int wait);

#ifdef __cplusplus
}
#endif

#endif


