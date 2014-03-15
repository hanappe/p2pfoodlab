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
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "json.h"
#include "config.h"
#include "arduino.h"
#include "opensensordata.h"
#include "log_message.h"

struct _opensensordata_t {
        char* url;
        char* cache;
        char* key;

        char* response;
        int response_length;
        int response_index;

        char* request;
        int request_length;
        int request_write;
        int request_read;

        CURL* curl;
};

opensensordata_t* new_opensensordata(const char* url)
{
        opensensordata_t* osd = (opensensordata_t*) malloc(sizeof(opensensordata_t));
        if (osd == NULL) 
                return NULL;
        osd->url = strdup(url);
        osd->cache = NULL;
        osd->key = NULL;
        osd->curl = NULL;
        osd->response = NULL;
        osd->response_index = 0;
        osd->response_length = 0;
        osd->request = NULL;
        osd->request_read = 0;
        osd->request_write = 0;
        osd->request_length = 0;

        return osd;
}

void delete_opensensordata(opensensordata_t* osd)
{
        if (osd->url)
                free(osd->url);
        if (osd->cache)
                free(osd->cache);
        if (osd->key)
                free(osd->key);
        if (osd->response)
                free(osd->response);
        if (osd->request)
                free(osd->request);
        free(osd);
}

void opensensordata_set_cache_dir(opensensordata_t* osd, const char* cache)
{
        if (osd->cache)
                free(osd->cache);
        osd->cache = strdup(cache);
}

void opensensordata_set_key(opensensordata_t* osd, const char* key)
{
        if (osd->key)
                free(osd->key);
        osd->key = strdup(key);
}

static void opensensordata_response_append(opensensordata_t* osd, char c)
{
	if (osd->response_index >= osd->response_length) {
		int newlen = 2 * osd->response_length;
		if (newlen == 0) {
			newlen = 128;
		}
		char* newbuf = (char*) calloc(newlen, sizeof(char));
		if (newbuf == NULL) {
			return;
		}
		if (osd->response != NULL) {
			memcpy(newbuf, osd->response, osd->response_length);
			free(osd->response);
		}
		osd->response = newbuf;
		osd->response_length = newlen;
	}

	osd->response[osd->response_index++] = (char) (c & 0xff);
}

static void opensensordata_request_append(opensensordata_t* osd, char c)
{
	if (osd->request_write >= osd->request_length) {
		int newlen = 2 * osd->request_length;
		if (newlen == 0) {
			newlen = 128;
		}
		char* newbuf = (char*) calloc(newlen, sizeof(char));
		if (newbuf == NULL) {
			return;
		}
		if (osd->request != NULL) {
			memcpy(newbuf, osd->request, osd->request_length);
			free(osd->request);
		}
		osd->request = newbuf;
		osd->request_length = newlen;
	}

	osd->request[osd->request_write++] = (char) (c & 0xff);
}

static size_t opensensordata_memorise_function(void *ptr, size_t size, size_t nmemb, void* data)
{
        opensensordata_t* osd = (opensensordata_t*) data;
        int len = size * nmemb;
        char* s = (char*) ptr;
        for (int i = 0; i < len; i++) 
                opensensordata_response_append(osd, s[i]);
        return len;
}

static int opensensordata_put_file(opensensordata_t* osd, 
                                   const char* path,
                                   const char* filename,
                                   const char* mime_type)
{
        osd->response_index = 0;

        if ((osd->url == NULL) || (strlen(osd->url) == 0)) {
                log_err("OpenSensorData: Invalid URL.");
                return -1;
        }
        if ((osd->key == NULL) || (strlen(osd->key) == 0)) {
                log_err("OpenSensorData: Invalid key.");
                return -1;
        }

        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
                log_err("OpenSensorData: Failed to open the file '%s'.", filename);
                return -1;
        }

        osd->curl = curl_easy_init();
        if (osd->curl == NULL) {
                log_err("OpenSensorData: Failed to initialise curl.");
                fclose(fp);
                return -1;
        }
        
        char url[2048];
        snprintf(url, 2048, "%s/%s", osd->url, path);
        url[2047] = 0;

        char key_header[2048];
        snprintf(key_header, 2048, "X-OpenSensorData-Key: %s", osd->key);
        key_header[2047] = 0;

        char mime_header[2048];
        snprintf(mime_header, 2048, "Content-Type: %s", mime_type);
        mime_header[2047] = 0;

        struct curl_slist* header_lines = NULL;
        header_lines = curl_slist_append(header_lines, mime_header);
        header_lines = curl_slist_append(header_lines, key_header);

        curl_easy_setopt(osd->curl, CURLOPT_HTTPHEADER, header_lines);
        curl_easy_setopt(osd->curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(osd->curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(osd->curl, CURLOPT_URL, url);
        curl_easy_setopt(osd->curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(osd->curl, CURLOPT_READDATA, fp);
        curl_easy_setopt(osd->curl, CURLOPT_WRITEFUNCTION, opensensordata_memorise_function);
        curl_easy_setopt(osd->curl, CURLOPT_WRITEDATA, osd);
        curl_easy_setopt(osd->curl, CURLOPT_TIMEOUT, 300L); // 5 minutes
        
        CURLcode res = curl_easy_perform(osd->curl);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to upload the data.");
                fclose(fp);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return res;
        }
        fclose(fp);

        long response_code = -1;
        res = curl_easy_getinfo(osd->curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to obtain curl's response code.");
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }
        if (response_code != 200) {
                log_err("OpenSensorData: HTTP PUT request failed (HTTP code %d).", response_code);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }
        log_debug("OpenSensorData: HTTP PUT request successful (HTTP code %d).", response_code);
                
        curl_slist_free_all(header_lines);
        curl_easy_cleanup(osd->curl);
        osd->curl = NULL;

        return 0;
}

static int32 opensensordata_json_to_request(opensensordata_t* osd, const char* s, int32 len)
{
        for (int i =0; i < len; i++)
                opensensordata_request_append(osd, s[i]); 
        return 0;
}

static size_t opensensordata_request_to_curl(void *ptr, 
                                             size_t size, 
                                             size_t nmemb, 
                                             opensensordata_t* osd)
{
        int len = size * nmemb;
        int available = osd->request_write - osd->request_read;
        
        if (len > available) 
                len = available;
        memcpy(ptr, osd->request + osd->request_read, len);
        osd->request_read += len;

        return len; 
}

static char* opensensordata_get_cache_file(opensensordata_t* osd, 
                                           const char* name, 
                                           char* filename, 
                                           int len)
{
        snprintf(filename, len, "%s/%s.json", osd->cache, name);
        filename[len-1] = 0;
        return filename;
}

static json_object_t opensensordata_cache_get(opensensordata_t* osd, const char* name)
{
        int err;
        char buffer[512];
        char filename[512];

        if ((osd->cache == NULL) || (strlen(osd->cache) == 0)) {
                log_err("OpenSensorData: Invalid cache dir.");
                return json_null();
        }

        opensensordata_get_cache_file(osd, name, filename, 512);

        json_object_t def = json_load(filename, &err, buffer, 512);
        if (err) {
                log_err("%s", buffer); 
                json_unref(def);
                return json_null();
        } else if (!json_isobject(def)) {
                log_err("OpenSensorData: Invalid definition: %s", filename); 
                json_unref(def);
                return json_null();
        }

        // Check whether the response was an error object.
        json_object_t e = json_object_get(def, "error");
        if (!json_isnull(e)) {
                const char* msg = json_object_getstr(def, "msg");
                log_err("OpenSensorData: Server returned an error: %s", msg);
                json_unref(def);
                return json_null();
        }
        
        return def;
}

static int opensensordata_put_object(opensensordata_t* osd, 
                                     const char* path,
                                     json_object_t object,
                                     const char* name)
{
        osd->response_index = 0;
        osd->request_write = 0;
        osd->request_read = 0;

        if ((osd->url == NULL) || (strlen(osd->url) == 0)) {
                log_err("OpenSensorData: Invalid URL.");
                return -1;;
        }
        if ((osd->key == NULL) || (strlen(osd->key) == 0)) {
                log_err("OpenSensorData: Invalid key.");
                return -1;;
        }

        int32 r = json_serialise(object, 
                                 k_json_pretty, 
                                 (json_writer_t) opensensordata_json_to_request, 
                                 osd);
        if (r != 0) {
                log_err("OpenSensorData: Failed to serialise the json object.");
                return -1;;
        }

        char cache_file[512];
        opensensordata_get_cache_file(osd, name, cache_file, 512);

        FILE* fp = fopen(cache_file, "w");
        if (fp == NULL) {
                log_err("OpenSensorData: Failed to open the file '%s'.", cache_file);
                return -1;;
        }

        osd->curl = curl_easy_init();
        if (osd->curl == NULL) {
                log_err("OpenSensorData: Failed to initialise curl.");
                fclose(fp);
                return -1;;
        }
        
        char url[2048];
        snprintf(url, 2048, "%s/%s", osd->url, path);
        url[2047] = 0;

        char key_header[2048];
        snprintf(key_header, 2048, "X-OpenSensorData-Key: %s", osd->key);
        key_header[2047] = 0;

        char* mime_header = "Content-Type: application/json";

        struct curl_slist* header_lines = NULL;
        header_lines = curl_slist_append(header_lines, mime_header);
        header_lines = curl_slist_append(header_lines, key_header);

        curl_easy_setopt(osd->curl, CURLOPT_HTTPHEADER, header_lines);
        curl_easy_setopt(osd->curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(osd->curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(osd->curl, CURLOPT_URL, url);
        curl_easy_setopt(osd->curl, CURLOPT_READFUNCTION, opensensordata_request_to_curl);
        curl_easy_setopt(osd->curl, CURLOPT_READDATA, osd);
        curl_easy_setopt(osd->curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(osd->curl, CURLOPT_TIMEOUT, 300L); // 5 minutes
        
        CURLcode res = curl_easy_perform(osd->curl);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to upload the data.");
                fclose(fp);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;;
        }
        fclose(fp);

        long response_code = -1;
        res = curl_easy_getinfo(osd->curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to obtain curl's response code.");
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;;
        }
        if (response_code != 200) {
                log_err("OpenSensorData: HTTP PUT request failed (HTTP code %d).", response_code);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;;
        }
        log_debug("OpenSensorData: HTTP PUT request successful (HTTP code %d).", 
                  response_code);
                
        curl_slist_free_all(header_lines);
        curl_easy_cleanup(osd->curl);
        osd->curl = NULL;


        return 0;
}

/*
static int opensensordata_get(opensensordata_t* osd, 
                              const char* path,
                              const char* filename)
{
        osd->response_index = 0;

        if ((osd->url == NULL) || (strlen(osd->url) == 0)) {
                log_err("OpenSensorData: Invalid URL.");
                return -1;
        }
        if ((osd->key == NULL) || (strlen(osd->key) == 0)) {
                log_err("OpenSensorData: Invalid key.");
                return -1;
        }

        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("OpenSensorData: Failed to open the file '%s'.", filename);
                return -1;
        }

        osd->curl = curl_easy_init();
        if (osd->curl == NULL) {
                log_err("OpenSensorData: Failed to initialise curl.");
                fclose(fp);
                return -1;
        }
        
        char url[2048];
        snprintf(url, 2048, "%s/%s", osd->url, path);
        url[2047] = 0;

        char key_header[2048];
        snprintf(key_header, 2048, "X-OpenSensorData-Key: %s", osd->key);
        key_header[2047] = 0;

        char errmsg[CURL_ERROR_SIZE];

        struct curl_slist* header_lines = NULL;
        header_lines = curl_slist_append(header_lines, key_header);

        curl_easy_setopt(osd->curl, CURLOPT_HTTPHEADER, header_lines);
        curl_easy_setopt(osd->curl, CURLOPT_URL, url);
        curl_easy_setopt(osd->curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(osd->curl, CURLOPT_TIMEOUT, 300L); // 5 minutes
        curl_easy_setopt(osd->curl, CURLOPT_ERRORBUFFER, errmsg);
        
        CURLcode res = curl_easy_perform(osd->curl);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to get the data: %s", errmsg);
                fclose(fp);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return res;
        }
        fclose(fp);

        long response_code = -1;
        res = curl_easy_getinfo(osd->curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (res != CURLE_OK) {
                log_err("OpenSensorData: Failed to obtain curl's response code.");
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }
        if (response_code != 200) {
                log_err("OpenSensorData: HTTP PUT request failed (HTTP code %d).", response_code);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }
        log_debug("OpenSensorData: HTTP PUT request successful (HTTP code %d).", response_code);
                
        curl_slist_free_all(header_lines);
        curl_easy_cleanup(osd->curl);
        osd->curl = NULL;

        return 0;
}
*/

int opensensordata_put_datapoints(opensensordata_t* osd, const char* filename)
{
        return opensensordata_put_file(osd, "upload", filename, "text/csv");
}

int opensensordata_put_photo(opensensordata_t* osd, 
                              int photostream,
                              const char* id, 
                              const char* filename)
{
        char path[512];
        snprintf(path, 512, "/photo/%d/%s", photostream, id);
        path[511] = 0;
        return opensensordata_put_file(osd, path, filename, "image/jpeg");
}

int osd_object_get_id(json_object_t obj)
{
        int id = -1;
        json_object_t v = json_object_get(obj, "id");
        if (json_isnull(v)) {
                log_err("OpenSensorData: Object has no ID"); 
                return -1;
        }
        if (json_isstring(v)) {
                id = atoi(json_string_value(v));
        } else if (json_isnumber(v)) {
                id = json_number_value(v);
        } else {
                log_err("OpenSensorData: ID value is not a string nor a number!"); 
                return -1;
        }
        
        return id;
}

const char* osd_object_get_name(json_object_t obj)
{
        json_object_t v = json_object_get(obj, "name");
        if (json_isstring(v))
                return json_string_value(v);

        if (json_isnull(v)) {
                log_err("OpenSensorData: Object has no name!"); 
        } else {
                log_err("OpenSensorData: Object name is not a string!"); 
        }
        return NULL;
}

static int opensensordata_get_cached_id(opensensordata_t* osd, const char* name)
{
        json_object_t def = opensensordata_cache_get(osd, name);
        if (json_isnull(def)) {
                return -1;
        }

        int id = osd_object_get_id(def);
        json_unref(def);

        if (id == -1) 
                log_err("OpenSensorData: Invalid cache object: %s", name); 
        
        return id;
}

json_object_t opensensordata_get_group(opensensordata_t* osd, 
                                       int cache_ok)
{
        if (cache_ok)
                return opensensordata_cache_get(osd, "group");
        log_err("OpenSensorData: get group not yet implemented");
        return json_null();
}

int opensensordata_get_group_id(opensensordata_t* osd)
{
        return opensensordata_get_cached_id(osd, "group");
}

json_object_t opensensordata_get_datastream(opensensordata_t* osd, 
                                            const char* name, 
                                            int cache_ok)
{
        if (cache_ok)
                return opensensordata_cache_get(osd, name);
        log_err("OpenSensorData: get datastream not yet implmented");
        return json_null();
}

int opensensordata_get_datastream_id(opensensordata_t* osd, 
                                     const char* name)
{
        return opensensordata_get_cached_id(osd, name);
}

json_object_t opensensordata_get_photostream(opensensordata_t* osd, 
                                             const char* name, 
                                             int cache_ok)
{
        if (cache_ok)
                return opensensordata_cache_get(osd, name);
        log_err("OpenSensorData: get photostream not yet implmented");
        return json_null();
}

int opensensordata_get_photostream_id(opensensordata_t* osd, 
                                      const char* name)
{
        return opensensordata_get_cached_id(osd, name);
}

char* opensensordata_get_response(opensensordata_t* osd)
{
        if ((osd->response == NULL)
            || (osd->response_length == 0))
                return NULL;
        opensensordata_response_append(osd, '\0');
        return osd->response;
}

int opensensordata_create_datastream(opensensordata_t* osd, 
                                     const char* name,
                                     json_object_t datastream)
{
        return opensensordata_put_object(osd, "/datastreams/",
                                         datastream, name);
}

int opensensordata_create_photostream(opensensordata_t* osd, 
                                                const char* name,
                                                json_object_t photostream)
{
        return opensensordata_put_object(osd, "/photostreams/",
                                         photostream, name);
}

int opensensordata_create_group(opensensordata_t* osd, 
                                json_object_t g)
{
        return opensensordata_put_object(osd, "/groups/", g, "group");
}

json_object_t new_osd_group(const char* name, 
                            const char* description)
{
        json_object_t object = json_object_create();
        json_object_setstr(object, "name", name);
        json_object_setstr(object, "description", description);
        json_object_set(object, "datastreams", json_array_create());
        json_object_set(object, "photostreams", json_array_create());
        return object;
}

json_object_t new_osd_datastream(const char* name, 
                                 const char* description, 
                                 double timezone,
                                 double latitude,
                                 double longitude,
                                 const char* unit)
{
        json_object_t object = json_object_create();
        json_object_setstr(object, "name", name);
        json_object_setstr(object, "description", description);
        json_object_setnum(object, "timezone", timezone);
        json_object_setnum(object, "latitude", latitude);
        json_object_setnum(object, "longitude", longitude);
        json_object_setstr(object, "unit", unit);
        return object;
}

json_object_t new_osd_photostream(const char* name, 
                                  const char* description, 
                                  double timezone,
                                  double latitude,
                                  double longitude)
{
        json_object_t object = json_object_create();
        json_object_setstr(object, "name", name);
        json_object_setstr(object, "description", description);
        json_object_setnum(object, "timezone", timezone);
        json_object_setnum(object, "latitude", latitude);
        json_object_setnum(object, "longitude", longitude);
        return object;        
}

/*
  Returns: 
   -1: group object is corrupted
    0: didn't find datastream
    1: found datastream
*/
int osd_group_has_datastream(json_object_t g, int id)
{
        json_object_t a = json_object_get(g, "datastreams");
        if (!json_isarray(a))
                return -1;
        int len = json_array_length(a);
        for (int i = 0; i < len; i++) {
                json_object_t d = json_array_get(a, i);
                if (!json_isobject(d))
                        return -1;
                int x = osd_object_get_id(d);
                if (x == -1) 
                        return -1;
                if (x == id)
                        return 1;
        }
        return 0;
}

/*
  Returns: 
   -1: group object is corrupted
    0: didn't find datastream
    1: found datastream
*/
int osd_group_has_photostream(json_object_t g, int id)
{
        json_object_t a = json_object_get(g, "photostreams");
        if (!json_isarray(a))
                return -1;
        int len = json_array_length(a);
        for (int i = 0; i < len; i++) {
                json_object_t d = json_array_get(a, i);
                if (!json_isobject(d))
                        return -1;
                int x = osd_object_get_id(d);
                if (x == -1) 
                        return -1;
                if (x == id)
                        return 1;
        }
        return 0;
}

void osd_group_add_datastream(json_object_t g, int id)
{
        json_object_t a = json_object_get(g, "datastreams");
        json_object_t d = json_object_create();
        json_object_setnum(d, "id", id);
        json_array_push(a, d);
        json_unref(d);
}

void osd_group_add_photostream(json_object_t g, int id)
{
        json_object_t a = json_object_get(g, "photostreams");
        json_object_t d = json_object_create();
        json_object_setnum(d, "id", id);
        json_array_push(a, d);
        json_unref(d);
}

