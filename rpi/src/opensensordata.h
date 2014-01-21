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
#ifndef _OPENSENSORDATA_H_
#define _OPENSENSORDATA_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _opensensordata_t opensensordata_t;

opensensordata_t* new_opensensordata(const char* url);
void delete_opensensordata(opensensordata_t* osd);

void opensensordata_set_cache_dir(opensensordata_t* osd, const char* dir);
void opensensordata_set_key(opensensordata_t* osd, const char* key);
int opensensordata_put_datapoints(opensensordata_t* osd, const char* filename);
int opensensordata_put_photo(opensensordata_t* osd, int photostream, const char* id, const char* filename);
int opensensordata_map_datastream(opensensordata_t* osd, const char* name);
char* opensensordata_get_response(opensensordata_t* osd);


#ifdef __cplusplus
}
#endif

#endif
