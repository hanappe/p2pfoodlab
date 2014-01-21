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
#include "arduino-api.h"
#include "opensensordata.h"
#include "log_message.h"

struct _opensensordata_t {
        char* url;
        char* cache;
        char* key;
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

static size_t opensensordata_write_function(void *ptr, size_t size, size_t nmemb, void* data)
{
        //opensensordata_t* osd = (opensensordata_t*) data;
        return size * nmemb;
}

static int opensensordata_put_file(opensensordata_t* osd, 
                                   const char* path,
                                   const char* filename,
                                   const char* mime_type)
{
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
        curl_easy_setopt(osd->curl, CURLOPT_WRITEFUNCTION, opensensordata_write_function);
        curl_easy_setopt(osd->curl, CURLOPT_WRITEDATA, osd);
        
        CURLcode res = curl_easy_perform(osd->curl);
        if (res != CURLE_OK) {
                log_err("Failed to upload the data.");
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
                log_err("Failed to obtain curl's response code.");
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }
        if (response_code != 200) {
                log_err("Upload failed (HTTP code %d).", response_code);
                curl_slist_free_all(header_lines);
                curl_easy_cleanup(osd->curl);
                osd->curl = NULL;
                return -1;
        }

        curl_slist_free_all(header_lines);
        curl_easy_cleanup(osd->curl);
        osd->curl = NULL;

        return 0;
}

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

int opensensordata_map_datastream(opensensordata_t* osd, const char* name)
{
        char buffer[512];
        char filename[512];

        if ((osd->cache == NULL) || (strlen(osd->cache) == 0)) {
                log_err("OpenSensorData: Invalid cache dir.");
                return -1;
        }

        snprintf(filename, 511, "%s/%s.json", osd->cache, name);
        filename[511] = 0;

        json_object_t def = json_load(filename, buffer, 512);
        if (json_isnull(def)) {
                log_err("%s", buffer); 
                return -1;
        } else if (!json_isobject(def)) {
                log_err("Invalid datastream definition: %s", filename); 
                return -1;
        }

        int id = -1;
        json_object_t v = json_object_get(def, "id");
        if (json_isnull(v)) {
                log_err("Invalid datastream id: %s", filename); 
                return -1;
        }
        if (json_isstring(v)) {
                id = atoi(json_string_value(v));
        } else if (json_isnumber(v)) {
                id = json_number_value(v);
        } else {
                log_err("Invalid datastream id: %s", filename); 
                return -1;
        }
        
        return id;
}
