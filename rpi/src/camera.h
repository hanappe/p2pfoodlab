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

#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
} io_method;

typedef struct _camera_t camera_t;

camera_t* new_camera(const char* dev, 
                       io_method io,
                       unsigned int width, 
                       unsigned int height, 
                       int jpeg_quality);

int delete_camera(camera_t* camera);

int camera_capture(camera_t* camera);

int camera_getimagesize(camera_t* camera);
unsigned char* camera_getimagebuffer(camera_t* camera);

#ifdef __cplusplus
}
#endif

#endif 
