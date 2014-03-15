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

#include "json.h"

#ifdef __cplusplus
extern "C" {
#endif


        int osd_object_get_id(json_object_t obj);
        const char* osd_object_get_name(json_object_t obj);

        json_object_t new_osd_group(const char* name, 
                                    const char* description);

        /*
          Returns: 
          -1: group object is corrupted
          0: didn't find datastream
          1: found datastream
        */
        int osd_group_has_datastream(json_object_t g, int id);
        int osd_group_has_photostream(json_object_t g, int id);

        void osd_group_add_datastream(json_object_t g, int id);
        void osd_group_add_photostream(json_object_t g, int id);

        json_object_t new_osd_datastream(const char* name, 
                                         const char* description, 
                                         double timezone,
                                         double latitude,
                                         double longitude,
                                         const char* unit);

        json_object_t new_osd_photostream(const char* name, 
                                          const char* description, 
                                          double timezone,
                                          double latitude,
                                          double longitude);

        /* opensensordata */

        typedef struct _opensensordata_t opensensordata_t;

        opensensordata_t* new_opensensordata(const char* url);
        void delete_opensensordata(opensensordata_t* osd);

        void opensensordata_set_cache_dir(opensensordata_t* osd, const char* dir);
        void opensensordata_set_key(opensensordata_t* osd, const char* key);

        char* opensensordata_get_response(opensensordata_t* osd);

        /* groups */

        json_object_t opensensordata_get_group(opensensordata_t* osd, int cache_ok);

        int opensensordata_get_group_id(opensensordata_t* osd);

        int opensensordata_create_group(opensensordata_t* osd, 
                                        json_object_t group);

        int opensensordata_update_group(opensensordata_t* osd, json_object_t g);


        /* datastream */

        json_object_t opensensordata_get_datastream(opensensordata_t* osd, 
                                                    const char* name, 
                                                    int cache_ok);

        int opensensordata_create_datastream(opensensordata_t* osd, 
                                             const char* name,
                                             json_object_t datastream);

        int opensensordata_get_datastream_id(opensensordata_t* osd, const char* name);

        int opensensordata_put_datapoints(opensensordata_t* osd, const char* filename);

        /* photostream */

        json_object_t opensensordata_get_photostream(opensensordata_t* osd, 
                                                     const char* name, 
                                                     int cache_ok);

        int opensensordata_get_photostream_id(opensensordata_t* osd, const char* name);

        int opensensordata_create_photostream(opensensordata_t* osd,  
                                              const char* name,
                                              json_object_t photostream);

        int opensensordata_put_photo(opensensordata_t* osd, 
                                     int photostream, 
                                     const char* id, 
                                     const char* filename);

#ifdef __cplusplus
}
#endif

#endif
