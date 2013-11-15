/*
    json.c A small JSON parser.

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

#include "json.h"

#if !defined(JSON_EMBEDDED)
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#endif

#define HASHTABLE_MIN_SIZE 7
#define HASHTABLE_MAX_SIZE 13845163

char* json_strdup(const char* s);

#define CHECK_MEMORY 0

#if CHECK_MEMORY
#include <sgc.h>
#define JSON_NEW(_type)            (_type*) sgc_new_object(sizeof(_type), SGC_ZERO, 0)
#define JSON_NEW_ARRAY(_type, _n)  (_type*) sgc_new_object(_n * sizeof(_type), SGC_ZERO, 0)
#define JSON_FREE(_p)              sgc_free_object((void*)_p)
#define JSON_MEMCPY(_dst,_src,_n)  memcpy(_dst,_src,_n)
#define JSON_MEMSET(_ptr,_c,_n)    memset(_ptr,_c,_n)
#define JSON_MEMCMP(_s,_t,_n)      memcmp(_s,_t,_n)
#define JSON_STRLEN(_s)            strlen(_s)
#define JSON_STRCMP(_s,_t)         strcmp(_s,_t)
#define JSON_STRCPY(_dst,_src)     strcpy(_dst,_src)

#elif defined(JSON_EMBEDDED)
#define JSON_NEW(_type)            (_type*) json_alloc(1, sizeof(_type))
#define JSON_NEW_ARRAY(_type, _n)  (_type*) json_alloc(_n, sizeof(_type))
#define JSON_FREE(_p)              json_free((char*)_p)
#define JSON_MEMCPY(_dst,_src,_n)  json_memcpy((char*)_dst,(char*)_src,_n)
#define JSON_MEMSET(_ptr,_c,_n)    json_memset((char*)_ptr,_c,_n)
#define JSON_MEMCMP(_s,_t,_n)      json_memcmp(_s,_t,_n)
#define JSON_STRLEN(_s)            json_strlen((char*)_s)
#define JSON_STRCMP(_s,_t)         json_strcmp((const unsigned char*)_s,(const unsigned char*)_t)
#define JSON_STRCPY(_dst,_src)     json_strcpy(_dst,_src)

#else
#define JSON_NEW(_type)            (_type*) calloc(1, sizeof(_type))
#define JSON_NEW_ARRAY(_type, _n)  (_type*) calloc(_n, sizeof(_type))
#define JSON_FREE(_p)              free(_p)
#define JSON_MEMCPY(_dst,_src,_n)  memcpy(_dst,_src,_n)
#define JSON_MEMSET(_ptr,_c,_n)    memset(_ptr,_c,_n)
#define JSON_MEMCMP(_s,_t,_n)      memcmp(_s,_t,_n)
#define JSON_STRLEN(_s)            strlen(_s)
#define JSON_STRCMP(_s,_t)         strcmp(_s,_t)
#define JSON_STRCPY(_dst,_src)     strcpy(_dst,_src)

#endif


#if JSON_EMBEDDED
#if !defined(NULL)
#define NULL ((void*)0)
#endif
static char* json_memory = NULL;
static int32 json_memory_size = 0;
static int32 json_memory_used = 0;

void json_init_memory(char* ptr, int32 size)
{
        json_memory = ptr;
        json_memory_size = size;
        json_memory_used = 0;
}

char* json_alloc(int32 count, int32 size) 
{
        size *= count;
        if (size < json_memory_size - json_memory_used) 
                return NULL;
        char* p = json_memory + json_memory_used;
        json_memory_used += size;
        return p;
}

void json_free(char* p)
{
}

void json_memcpy(char* dst, char* src, uint32 n)
{
        for (uint32 i = 0; i < n; i++) 
                *dst++ = *src++;
}

void json_memset(char* p, char c, uint32 n)
{
        for (uint32 i = 0; i < n; i++) 
                *p++ = c;
}

int32 json_memcmp(const char* s1, const char* s2, int32 n)
{
        int32 ret = 0;
        int32 i = 0;
        while (!(ret = *s1 - *s2) && (i < n)) 
                ++s1, ++s2, ++i;

        if (ret < 0)
                ret = -1;
        else if (ret > 0)
                ret = 1 ;
        
        return ret;
}

int32 json_strlen(char* s)
{
        int32 len = 0;
        while (*s++) 
                len++;
        return len;
}

int32 json_strcmp(const unsigned char* s1, const unsigned char* s2)
{
        int32 ret = 0;

        while (!(ret = *s1 - *s2) && *s2) 
                ++s1, ++s2;

        if (ret < 0)
                ret = -1;
        else if (ret > 0)
                ret = 1 ;
        
        return ret;
}

void json_strcpy(char* dst, const char* src)
{
        while (*src) 
                *dst++ = *src++;
}

#endif

#if CHECK_MEMORY
static int32 memory_leak_callback(int32 op, void* ptr, int32 type, int32 counter, int32 size)
{
        if (op == 3) {
                printf("Memory leak: counter=%d, size=%d\n", counter, size);
        } 
        return 1;
}

void __attribute__ ((constructor))  json_init()
{
        int32 dummy;
	if (sgc_init(0, &dummy, 0, 0) != 0) {
                fprintf(stderr, "Failed the initialise the SGC memory heap\n");
                exit(1);
        }
}

void __attribute__ ((destructor)) json_cleanup()
{
        sgc_search_memory_leaks(memory_leak_callback);
        sgc_cleanup();
}
#endif

/******************************************************************************/

typedef struct _string_t {
	uint32 refcount;
	int32 length;
	char* s;
} string_t;

static string_t* new_string(const char* s)
{
	string_t *string;

        //printf("new string %s\n", s);

	string = JSON_NEW(string_t);
	if (string == NULL) {
		return NULL;
	}

	string->refcount = 0;
	string->length = JSON_STRLEN(s);
	string->s = JSON_NEW_ARRAY(char, string->length+1);
	if (string->s == NULL) {
		JSON_FREE(string);
		return NULL;
	}
	JSON_MEMCPY(string->s, s, string->length);
	string->s[string->length] = 0;

	return string;
}

static void delete_string(string_t* string)
{
	JSON_FREE(string->s);
	JSON_FREE(string);
}

/******************************************************************************/

typedef struct _array_t {
	uint32 refcount;
	int32 length;
	int32 datalen;
	json_object_t* data;
} array_t;

static array_t* new_array()
{
	array_t *array;
	
	array = JSON_NEW(array_t);
	if (array == NULL) {
		return NULL;
	}

	array->refcount = 0;
	array->length = 0;
	array->datalen = 0;
	array->data = NULL;

	return array;
}

static void delete_array(array_t* array)
{
        for (int32 i = 0; i < array->datalen; i++) 
                json_unref(array->data[i]);

	if (array->data) {
		JSON_FREE(array->data);
	}
	JSON_FREE(array);
}

static int32 array_set(array_t* array, json_object_t object, int32 index)
{
	if (index >= array->datalen) {
		int32 newlen = 8;
		while (newlen <= index) {
			newlen *= 2;
		} 
		json_object_t* data = JSON_NEW_ARRAY(json_object_t, newlen);
		if (data == NULL) {
			return -1;
		}
		for (int32 i = 0; i < array->datalen; i++) {
			data[i] = array->data[i];
		}
		for (int32 i = array->datalen; i < newlen; i++) {
			data[i].type = k_json_null;
		}

		if (array->data) {
			JSON_FREE(array->data);
		}
		array->data = data;
		array->datalen = newlen;
	}

	array->data[index] = object;
        json_ref(array->data[index]);
	if (index >= array->length) {
		array->length = index+1;
	}

	return 0;
}

static json_object_t array_get(array_t* array, int32 index)
{
	if ((0 <= index) && (index < array->length)) {
		return array->data[index];
	}
	return json_null();
}

static int32 array_length(array_t* array)
{
	return array->length;
}

/******************************************************************************/

typedef struct _json_hashnode_t json_hashnode_t;

struct _json_hashnode_t {
	char* key;
	json_object_t value;
	json_hashnode_t *next;
};

static json_hashnode_t* new_json_hashnode(const char* key, json_object_t* value);
static void delete_json_hashnode(json_hashnode_t *hash_node);
static void delete_json_hashnodes(json_hashnode_t *hash_node);
uint32 json_strhash(const char* v);


typedef struct _json_hashtable_t {
	uint32 refcount;
        int32 size;
        int32 num_nodes;
	json_hashnode_t **nodes;
} json_hashtable_t;


static json_hashtable_t* new_json_hashtable();
static void delete_json_hashtable(json_hashtable_t *hashtable);
static int32 json_hashtable_set(json_hashtable_t *hashtable, const char* key, json_object_t value);
static json_object_t json_hashtable_get(json_hashtable_t *hashtable, const char* key);
static int32 json_hashtable_unset(json_hashtable_t *hashtable, const char* key);
static int32 json_hashtable_foreach(json_hashtable_t *hashtable, json_iterator_t func, void* data);
static int32 json_hashtable_size(json_hashtable_t *hashtable);
static void json_hashtable_resize(json_hashtable_t *hashtable);
static json_hashnode_t** json_hashtable_lookup_node(json_hashtable_t *hashtable, const char* key);

static json_hashtable_t* new_json_hashtable()
{
	json_hashtable_t *hashtable;
	
	hashtable = JSON_NEW(json_hashtable_t);
	if (hashtable == NULL) {
		return NULL;
	}
	hashtable->refcount = 0;
	hashtable->num_nodes = 0;
	hashtable->size = 0;
	hashtable->nodes = NULL;
	
	return hashtable;
}


static void delete_json_hashtable(json_hashtable_t *hashtable)
{
	if (hashtable == NULL) {
		return;
	}

	if (hashtable->nodes) {
		for (int32 i = 0; i < hashtable->size; i++) {
			delete_json_hashnodes(hashtable->nodes[i]);
		}
		JSON_FREE(hashtable->nodes);
	}

	JSON_FREE(hashtable);
}

static json_hashnode_t** json_hashtable_lookup_node(json_hashtable_t* hashtable, const char* key)
{
	json_hashnode_t **node;
	
	if (hashtable->nodes == NULL) {
		hashtable->size = HASHTABLE_MIN_SIZE;
		hashtable->nodes = JSON_NEW_ARRAY(json_hashnode_t*, hashtable->size);	
		for (int32 i = 0; i < hashtable->size; i++) {
			hashtable->nodes[i] = NULL;
		}
	}
	
	node = &hashtable->nodes[json_strhash(key) % hashtable->size];
	
	while (*node && (JSON_STRCMP((*node)->key, key) != 0)) {
		node = &(*node)->next;
	}
	
	return node;
}


static int32 json_hashtable_set(json_hashtable_t *hashtable, const char* key, json_object_t value)
{
  
	if ((key == NULL) || (JSON_STRLEN(key) == 0)) {
		return -1;
	}

	json_hashnode_t **node = json_hashtable_lookup_node(hashtable, key);

	if (*node) {
		json_object_t oldvalue = (*node)->value;
		(*node)->value = value;
		json_ref((*node)->value);
		json_unref(oldvalue);

	} else {
		*node = new_json_hashnode(key, &value);
		if (*node == NULL) {
			return -1;
		}
		hashtable->num_nodes++;

		if ((3 * hashtable->size <= hashtable->num_nodes)
		    && (hashtable->size < HASHTABLE_MAX_SIZE)) {
			json_hashtable_resize(hashtable);
		} 
	}

        char buffer[256];
        json_tostring(value, buffer, sizeof(buffer));
        //printf("hash node %s - %s\n", key, buffer);

	return 0;
}

static json_object_t json_hashtable_get(json_hashtable_t *hashtable, const char* key)
{
	json_hashnode_t *node;
	node = *json_hashtable_lookup_node(hashtable, key);
       	return (node)? node->value : json_null();
}

static int32 json_hashtable_unset(json_hashtable_t *hashtable, const char* key)
{
	json_hashnode_t **node, *dest;
  
	node = json_hashtable_lookup_node(hashtable, key);
	if (*node) {
		dest = *node;
		(*node) = dest->next;
		delete_json_hashnode(dest);
		hashtable->num_nodes--;
		return 0;
	}
	
	return -1;
}

static int32 json_hashtable_foreach(json_hashtable_t *hashtable, json_iterator_t func, void* data)
{
	json_hashnode_t *node = NULL;
	
	for (int32 i = 0; i < hashtable->size; i++) {
		for (node = hashtable->nodes[i]; node != NULL; node = node->next) {
			int32 r = (*func)(node->key, &node->value, data);
                        if (r != 0) return r;
		}
	}
        return 0;
}

static int32 json_hashtable_size(json_hashtable_t *hashtable)
{
	return hashtable->num_nodes;
}


static void json_hashtable_resize(json_hashtable_t *hashtable)
{
	json_hashnode_t **new_nodes;
	json_hashnode_t *node;
	json_hashnode_t *next;
	uint32 hash_val;
	int32 new_size;
	
	new_size = 3 * hashtable->size + 1;
	new_size = (new_size > HASHTABLE_MAX_SIZE)? HASHTABLE_MAX_SIZE : new_size;
	
	new_nodes = JSON_NEW_ARRAY(json_hashnode_t*, new_size);
	
	for (int32 i = 0; i < hashtable->size; i++) {
		for (node = hashtable->nodes[i]; node; node = next) {
			next = node->next;
			hash_val = json_strhash(node->key) % new_size;      
			node->next = new_nodes[hash_val];
			new_nodes[hash_val] = node;
		}
	}
	
	JSON_FREE(hashtable->nodes);
	hashtable->nodes = new_nodes;
	hashtable->size = new_size;
}

/******************************************************************************/


static json_hashnode_t* new_json_hashnode(const char* key, json_object_t* value)
{
	json_hashnode_t *hash_node = JSON_NEW(json_hashnode_t);
	if (hash_node == NULL) {
		return NULL;
	}

	hash_node->key = json_strdup(key);
	if (hash_node->key == NULL) {
		JSON_FREE(hash_node);
		return NULL;
	}
	hash_node->value = *value;
	json_ref(hash_node->value);
	hash_node->next = NULL;
	
	return hash_node;
}


static void delete_json_hashnode(json_hashnode_t *hash_node)
{
	json_unref(hash_node->value);
	JSON_FREE(hash_node->key);
	JSON_FREE(hash_node);
}


static void delete_json_hashnodes(json_hashnode_t *hash_node)
{
	while (hash_node) {
		json_hashnode_t *next = hash_node->next;
		delete_json_hashnode(hash_node);
		hash_node = next;
	}  
}

/******************************************************************************/

/* 31 bit hash function */
uint32 json_strhash(const char* key)
{
  const char *p = key;
  uint32 h = *p;

  if (h) {
    for (p += 1; *p != '\0'; p++) {
      h = (h << 5) - h + *p;
    }
  }

  return h;
}


char* json_strdup(const char* s)
{
	int32 len = JSON_STRLEN(s) + 1;
	char* t = JSON_NEW_ARRAY(char, len);
	if (t == NULL) {
		return NULL;
	}
	JSON_STRCPY(t, s);
	return t;
}

/******************************************************************************/

json_object_t json_object_create()
{
	json_object_t object;
	json_hashtable_t* hashtable;

	object.type = k_json_object;
	hashtable = new_json_hashtable();
	if (hashtable == NULL) {
		object.type = k_json_null;
		return object;
	}
	
	object.value.data = hashtable;
	json_ref(object);
	return object;
}

int32 json_object_set(json_object_t object, const char* key, json_object_t value)
{
	if (object.type != k_json_object) {
		return -1;
	}
	json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
	return json_hashtable_set(hashtable, key, value);
}

json_object_t json_object_get(json_object_t object, const char* key)
{
	if (object.type != k_json_object) {
		return json_null();
	}
	json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
	return json_hashtable_get(hashtable, key);
}

const char* json_object_getstr(json_object_t object, const char* key)
{
	if (object.type != k_json_object) {
		return NULL;
	}
	json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
	json_object_t val = json_hashtable_get(hashtable, key);
        if (val.type == k_json_string) {
                return json_string_value(val);
        } else {
                return NULL;
        }
}

int32 json_object_unset(json_object_t object, const char* key)
{
	if (object.type != k_json_object) {
		return -1;
	}
	json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
	return json_hashtable_unset(hashtable, key);
}


int32 json_object_setnum(json_object_t object, const char* key, double value)
{
	return json_object_set(object, key, json_number_create(value));
}

int32 json_object_setstr(json_object_t object, const char* key, const char* value)
{
	return json_object_set(object, key, json_string_create(value));
}

int32 json_object_foreach(json_object_t object, json_iterator_t func, void* data)
{
	if (object.type != k_json_object) {
		return -1;
	}
	json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
	return json_hashtable_foreach(hashtable, func, data);
}

int32 json_object_length(json_object_t object)
{
	if (object.type != k_json_object) {
		return 0;
	}
	return json_hashtable_size((json_hashtable_t*) object.value.data);
}

/******************************************************************************/

json_object_t json_number_create(double value)
{
	json_object_t number;
	number.type = k_json_number;
	number.value.number = value;
	json_ref(number);
	return number;
}

float64 json_number_value(json_object_t obj)
{
	return obj.value.number;
}

/******************************************************************************/

json_object_t json_string_create(const char* s)
{
	json_object_t string;
	string.type = k_json_string;
	string.value.data = new_string(s);
	if (string.value.data == NULL) {
		string.type = k_json_null;
	}
	json_ref(string);
	return string;
}

const char* json_string_value(json_object_t string)
{
	if (string.type != k_json_string) {
		return NULL;
	}
	string_t* str = (string_t*) string.value.data;
	return str->s;
}

int32 json_string_length(json_object_t string)
{
	if (string.type != k_json_string) {
		return 0;
	}
	string_t* str = (string_t*) string.value.data;
	return str->length;
}

int32 json_string_equals(json_object_t string, const char* s)
{
	if (string.type != k_json_string) {
		return 0;
	}
	string_t* str = (string_t*) string.value.data;
	return (JSON_STRCMP(str->s, s) == 0);
}

/******************************************************************************/

void json_refcount(json_object_t *object, int32 val)
{
        switch (object->type) {
                
        case k_json_object: {
                json_hashtable_t* hashtable = (json_hashtable_t*) object->value.data;
                //printf("delete object\n");
                hashtable->refcount += val;
                if (hashtable->refcount <= 0) {
                        delete_json_hashtable(hashtable);
                        object->value.data = NULL;
                        object->type = k_json_null; }} 
                break;
        case k_json_string: {
                string_t* string = (string_t*) object->value.data;
                //printf("delete string %s\n", ((string_t*)object->value.data)->s);
                string->refcount += val;
                if (string->refcount <= 0) {
                        delete_string(string);
                        object->value.data = NULL;
                        object->type = k_json_null; }}
                break;
                
        case k_json_array: {
                array_t* array = (array_t*) object->value.data;
                array->refcount += val;
                if (array->refcount <= 0) {
                        delete_array(array);
                        object->value.data = NULL;
                        object->type = k_json_null; }}
                break;
        default:
                break;
        }
}

/******************************************************************************/

json_object_t json_null()
{
	json_object_t object;
	object.type = k_json_null;
	json_ref(object);
	return object;
}

json_object_t json_true()
{
	json_object_t object;
	object.type = k_json_boolean;
	object.value.number = 1.0;
	json_ref(object);
	return object;
}

json_object_t json_false()
{
	json_object_t object;
	object.type = k_json_boolean;
	object.value.number = 0.0;
	json_ref(object);
	return object;
}

/******************************************************************************/

json_object_t json_array_create()
{
	json_object_t object;
	array_t* array;
	object.type = k_json_array;
	array = new_array();
	if (array == NULL) {
		return json_null();
	}
	
	object.value.data = array;
	json_ref(object);
	return object;
}

int32 json_array_length(json_object_t array)
{
	if (array.type != k_json_array) {
		return 0;
	}
	array_t* a = (array_t*) array.value.data;
	return array_length(a);	
}

json_object_t json_array_get(json_object_t array, int32 index)
{
	if (array.type != k_json_array) {
		return json_null();
	}
	array_t* a = (array_t*) array.value.data;
	return array_get(a, index);	
}

int32 json_array_set(json_object_t array, json_object_t value, int32 index)
{
	if (array.type != k_json_array) {
		return 0;
	}
	array_t* a = (array_t*) array.value.data;
	return array_set(a, value, index);	
}

int32 json_array_push(json_object_t array, json_object_t value)
{
	if (array.type != k_json_array) {
		return 0;
	}
	array_t* a = (array_t*) array.value.data;
	int32 index = a->length;
	return array_set(a, value, index);	
}

int32 json_array_setnum(json_object_t array, double value, int32 index)
{
	return json_array_set(array, json_number_create(value), index);
}

int32 json_array_setstr(json_object_t array, char* value, int32 index)
{
	return json_array_set(array, json_string_create(value), index);
}

float64 json_array_getnum(json_object_t array, int32 index)
{
        json_object_t val = json_array_get(array, index);
        if (val.type == k_json_number) {
                return json_number_value(val);
        } else {
                return 0.0;
        }
}

const char* json_array_getstr(json_object_t array, int32 index)
{
        json_object_t val = json_array_get(array, index);
        if (val.type == k_json_string) {
                return json_string_value(val);
        } else {
                return NULL;
        }
}

int32 json_array_gettype(json_object_t array, int32 index)
{
        json_object_t val = json_array_get(array, index);
        return val.type;
}

/******************************************************************************/

typedef struct _json_serialise_t {
        int pretty;
        int indent;
} json_serialise_t;

int32 json_serialise_text(json_serialise_t* serialise, 
                          json_object_t object, 
                          json_writer_t fun, 
                          void* userdata);

typedef struct _json_strbuf_t {
        char* s;
        int32 len;
        int32 index;
} json_strbuf_t;

static int32 json_strwriter(void* userdata, const char* s, int32 len)
{
        json_strbuf_t* strbuf = (json_strbuf_t*) userdata;
        while (len-- > 0) {
                if (strbuf->index >= strbuf->len - 1) {
                        return -1;
                }
                strbuf->s[strbuf->index++] = *s++;
        } 
        return 0;
}

int32 json_tostring(json_object_t object, char* buffer, int32 buflen)
{
        json_strbuf_t strbuf = { buffer, buflen, 0 };
        
        int32 r = json_serialise(object, 0, json_strwriter, (void*) &strbuf);
        strbuf.s[strbuf.index] = 0;
        return r;
}

/* typedef struct _json_filebuf_t { */
/*         FILE* fp; */
/*         int32 linelen; */
/* } json_strbuf_t; */

#if !defined(JSON_EMBEDDED)
static int32 json_file_writer(void* userdata, const char* s, int32 len)
{
        if (len == 0) return 0;
        FILE* fp = (FILE*) userdata;
	int32 n = fwrite(s, len, 1, fp);
	return (n != 1);
}

int32 json_tofilep(json_object_t object, int32 flags, FILE* fp)
{
        int32 res = json_serialise(object, flags, json_file_writer, (void*) fp);
        fprintf(fp, "\n");
        return res;
}

int32 json_tofile(json_object_t object, int32 flags, const char* path)
{
        FILE* fp = fopen(path, "w");
        if (fp == NULL)
                return -1;

        int32 r = json_serialise(object, flags, json_file_writer, (void*) fp);
        fclose(fp);

        return r;
}
#endif

int32 json_serialise(json_object_t object, 
                     int32 flags, 
                     json_writer_t fun, 
                     void* userdata)
{
        json_serialise_t serialise;
        serialise.pretty = flags & k_json_pretty;
        serialise.indent = 0;

	if (flags & k_json_binary) {
		return -1;
	} else {
		return json_serialise_text(&serialise, object, fun, userdata);
	}
}

int32 json_print(json_writer_t fun, void* userdata, const char* s)
{
	return (*fun)(userdata, s, JSON_STRLEN(s));
}

int32 json_serialise_text(json_serialise_t* serialise, 
                          json_object_t object, 
                          json_writer_t fun, 
                          void* userdata)
{
	int32 r;

	switch (object.type) {

	case k_json_number: {
		char buf[128];
#if !defined(JSON_EMBEDDED)
		if (floor(object.value.number) == object.value.number) 
			snprintf(buf, 128, "%g", object.value.number);
		else 
#endif
			snprintf(buf, 128, "%e", object.value.number);
		
		buf[127] = 0;
		r = json_print(fun, userdata, buf);
		if (r != 0) return r;
	} break;
		
	case k_json_string: {
		string_t* string = (string_t*) object.value.data;
		r = json_print(fun, userdata, "\"");
		if (r != 0) return r;
		r = json_print(fun, userdata, string->s);
		if (r != 0) return r;
		r = json_print(fun, userdata, "\"");
		if (r != 0) return r;
	} break;

	case k_json_boolean: {
		if (object.value.number) {
			r = json_print(fun, userdata, "true");
		} else {
			r = json_print(fun, userdata, "false");
		}
		if (r != 0) return r;
	} break;

	case k_json_null: {
		r = json_print(fun, userdata, "null");
		if (r != 0) return r;
	} break;

	case k_json_array: {
		r = json_print(fun, userdata, "[");
		if (r != 0) return r;
		array_t* array = (array_t*) object.value.data;
		for (int32 i = 0; i < array->length; i++) {
			r = json_serialise_text(serialise, array->data[i], fun, userdata);
			if (r != 0) return r;
			if (i < array->length - 1) {
				r = json_print(fun, userdata, ", ");
				if (r != 0) return r;
                                if ((i+1) % 10 == 0) // FIXME
                                        r = json_print(fun, userdata, "\n"); // FIXME
				if (r != 0) return r;
			}
		}
		r = json_print(fun, userdata, "]");
		if (r != 0) return r;

	} break;
 
	case k_json_object: {
		r = json_print(fun, userdata, "{");
                if (serialise->pretty) {
                        r = json_print(fun, userdata, "\n");
                        serialise->indent += 4;
                }
		if (r != 0) return r;

		json_hashtable_t* hashtable = (json_hashtable_t*) object.value.data;
		json_hashnode_t *node = NULL;
		int32 count = 0;
		for (int32 i = 0; i < hashtable->size; i++) {
			for (node = hashtable->nodes[i]; node != NULL; node = node->next) {
                                if (serialise->pretty) {
                                        for (int ii = 0; ii < serialise->indent; ii++) {
                                                r = json_print(fun, userdata, " ");
                                                if (r != 0) return r;
                                        }
                                }
				r = json_print(fun, userdata, "\"");
				if (r != 0) return r;
				r = json_print(fun, userdata, node->key);
				if (r != 0) return r;
				r = json_print(fun, userdata, "\": ");
				if (r != 0) return r;
				r = json_serialise_text(serialise, node->value, fun, userdata);
				if (r != 0) return r;
				if (++count < hashtable->num_nodes) {
					r = json_print(fun, userdata, ", ");
				}
                                if (serialise->pretty) {
                                        r = json_print(fun, userdata, "\n");
                                }
                                if (r != 0) return r;
			}
		}

                if (serialise->pretty) {
                        serialise->indent -= 4;
                        for (int ii = 0; ii < serialise->indent; ii++) {
                                r = json_print(fun, userdata, " ");
                                if (r != 0) return r;
                        }
                }
		r = json_print(fun, userdata, "}");
		if (r != 0) return r;
	} break; }

	return 0;
}

typedef enum {
	k_end_of_string = -5,
	k_stack_overflow = -4,
	k_out_of_memory = -3,
	k_token_error = -2,
	k_parsing_error = -1,
	k_continue = 0
} json_error_t;

typedef enum {
	k_object_start,
	k_object_end,
	k_array_start,
	k_array_end,
	k_string,
	k_number,
	k_true,
	k_false,
	k_null,
	k_colon,
	k_comma
} json_token_t;

typedef enum {
	k_do,
	k_parsing_string,
	k_parsing_number,
	k_parsing_true,
	k_parsing_false,
	k_parsing_null
} json_token_state_t;

		    
struct _json_parser_t {
	char* buffer;
	int32 buflen;
	int32 bufindex;
	json_token_state_t state;
	char backslash;
	char unicode;
	int32 unihex;
	int32 numstate;
	int32 error_code;
	char* error_message;

	int32 stack_top;
	int32 stack_depth;
	json_object_t value_stack[256];
	int32 state_stack[256];
};

static inline int32 whitespace(int32 c)
{
	return ((c == ' ') || (c == '\r') || (c == '\n') || (c == '\t'));
}

json_parser_t* json_parser_create()
{
	json_parser_t* parser = JSON_NEW(json_parser_t);
	if (parser == NULL) {
		return NULL;
	}
        json_parser_init(parser);
	return parser;
}

void json_parser_destroy(json_parser_t* parser)
{
	if (parser != NULL) {
                json_parser_cleanup(parser);
		JSON_FREE(parser);
	}
}

void json_parser_init(json_parser_t* parser)
{
	JSON_MEMSET(parser, 0, sizeof(*parser));
        json_parser_reset(parser);
}

void json_parser_cleanup(json_parser_t* parser)
{
        if (parser->buffer != NULL) {
                JSON_FREE(parser->buffer);
        }
        if (parser->error_message != NULL) {
                JSON_FREE(parser->error_message);
        }
}

int32 json_parser_errno(json_parser_t* parser)
{
	return parser->error_code;
}

char* json_parser_errstr(json_parser_t* parser)
{
	return parser->error_message;
}

static void json_parser_set_error(json_parser_t* parser, int32 error, const char* message)
{
	parser->error_code = error;
	parser->error_message = json_strdup(message);
}

static int32 json_parser_append(json_parser_t* parser, char c)
{
	if (parser->bufindex >= parser->buflen) {
		int32 newlen = 2 * parser->buflen;
		if (newlen == 0) {
			newlen = 128;
		}
		char* newbuf = JSON_NEW_ARRAY(char, newlen);
		if (newbuf == NULL) {
			return k_out_of_memory;
		}
		if (parser->buffer != NULL) {
			JSON_MEMCPY(newbuf, parser->buffer, parser->buflen);
			JSON_FREE(parser->buffer);
		}
		parser->buffer = newbuf;
		parser->buflen = newlen;
	}

	parser->buffer[parser->bufindex++] = (char) (c & 0xff);

	return k_continue;
}


enum { k_parse_done = -1, k_parse_value = 0, k_object_1, k_object_2, k_object_3, k_object_4, k_array_1, k_array_2 }; 

void json_parser_reset(json_parser_t* parser)
{
	parser->stack_top = 0;
	parser->stack_depth = 256;
	parser->state_stack[0] = k_parse_value;
	parser->state = k_do;
	parser->bufindex = 0;
	parser->unicode = 0;
	parser->unihex = 0;
	parser->backslash = 0;
	parser->error_code = 0;
	if (parser->error_message != NULL) {
		JSON_FREE(parser->error_message);
		parser->error_message = NULL;
	}
}

void json_parser_reset_buffer(json_parser_t* parser)
{
	parser->bufindex = 0;
}

//
// value = object | array | number | string | true | false | null
// object = object_start key_value object_rest object_end  
// key_value = string : value 
// object_rest = , key_value object_rest
// array = array_start value array_rest array_end  
// array_rest = , array_rest

// parse_value + object_start -> object_1  & top_value = new object()
// object_1    + string       -> object_2
// object_2    + colon        -> object_3  & top_state = parse_value & parse
// object_3    + (value)      -> object_4  & object.set(key,top_value)
// object_4    + object_end   -> (top_state)
// object_4    + comma        -> object_1
// 
// parse_value + array_start  -> array_1  & top_value = new array()  
// array_1     + (value)      -> array_2  & array.push(top_value)
// array_2     + colon        -> array_1  & top_state = parse_value & parse 
// array_2     + array_end    -> (top_state)


static int32 json_parser_token(json_parser_t* parser, int32 token)
{
	int32 state = parser->state_stack[parser->stack_top];
	double d;
	json_object_t value;
	json_object_t key;
	json_object_t object;
	int32 r = k_continue;

        if (0) {
                switch (token) {
                case k_object_start: printf("k_object_start\n"); break;
                case k_object_end: printf("k_object_end\n"); break;
                case k_array_start: printf("k_array_start\n"); break;
                case k_array_end: printf("k_array_end\n"); break;
                case k_string: printf("k_string\n"); break;
                case k_number: printf("k_number\n"); break;
                case k_true: printf("k_true\n"); break;
                case k_false: printf("k_false\n"); break;
                case k_null: printf("k_null\n"); break;
                case k_colon: printf("k_colon\n"); break;
                case k_comma: printf("k_comma\n"); break;
                default: printf("unknown token\n"); break;
                }
        }

	switch (state) {

	case k_parse_done:
		r = k_parsing_error;
		break;

	case k_parse_value:

		switch (token) {
			
		case k_object_start:
			parser->value_stack[parser->stack_top] = json_object_create();
			parser->state_stack[parser->stack_top] = k_object_1;
			parser->state_stack[++parser->stack_top] = k_parse_value;
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
			break;

		case k_array_start:
			parser->value_stack[parser->stack_top] = json_array_create();
			parser->state_stack[parser->stack_top] = k_array_1;
			parser->state_stack[++parser->stack_top] = k_parse_value;
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
			break;

		case k_string:
			parser->value_stack[parser->stack_top] = json_string_create(parser->buffer);
			json_parser_reset_buffer(parser);
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		case k_number: 
			d = atof(parser->buffer); // FIXME: use strtod
			parser->value_stack[parser->stack_top] = json_number_create(d);
			json_parser_reset_buffer(parser);
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
		
			break;

		case k_true:
			parser->value_stack[parser->stack_top] = json_true();
			json_parser_reset_buffer(parser);
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		case k_false:
			parser->value_stack[parser->stack_top] = json_false();
			json_parser_reset_buffer(parser);
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		case k_null:
			parser->value_stack[parser->stack_top] = json_null();
			json_parser_reset_buffer(parser);
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		default:
			r = k_parsing_error;
			break;		
		}
		break;

	case k_object_1:
		if (token == k_string) {
			// keep the key on the stack by increase the stack top.
			parser->stack_top++; 
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
			parser->state_stack[parser->stack_top] = k_object_2; 
		} else {
			r = k_parsing_error;
		}
		break;

	case k_object_2:
		if (token == k_colon) {
			parser->state_stack[parser->stack_top] = k_object_3;
			parser->state_stack[++parser->stack_top] = k_parse_value; 
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
		} else {
			r = k_parsing_error;
		}
		break;

	case k_object_3:
		switch (token) {
		case k_object_end:
		case k_array_end:
		case k_string:
		case k_number:
		case k_true:
		case k_false:
		case k_null:
			value = parser->value_stack[parser->stack_top+1];
			key = parser->value_stack[parser->stack_top--];
			object = parser->value_stack[parser->stack_top];
			json_object_set(object, json_string_value(key), value);
			json_unref(key);
			json_unref(value);
			parser->state_stack[parser->stack_top] = k_object_4;
			break;
			
		default:
			r = k_parsing_error;
			break;
		}
		break;

	case k_object_4:
		switch (token) {
		case k_object_end:
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		case k_comma:
			parser->state_stack[parser->stack_top] = k_object_1;
			parser->state_stack[++parser->stack_top] = k_parse_value;
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
			break;

		default:
			r = k_parsing_error;
			break;
		}
		break;

	case k_array_1:
		switch (token) {
		case k_object_end:
		case k_array_end:
		case k_string:
		case k_number:
		case k_true:
		case k_false:
		case k_null:
			value = parser->value_stack[parser->stack_top+1];
			object = parser->value_stack[parser->stack_top];
			json_array_push(object, value);
                        json_unref(value);
			parser->state_stack[parser->stack_top] = k_array_2;
			break;
			
		default:
			r = k_parsing_error;
			break;
		}
		break;

	case k_array_2:
		switch (token) {
		case k_comma:
			parser->state_stack[parser->stack_top] = k_array_1;
			parser->state_stack[++parser->stack_top] = k_parse_value;
			if (parser->stack_top >= parser->stack_depth) {
				r = k_stack_overflow;
			}
			break;
			
		case k_array_end:
			if (parser->stack_top > 0) {
				parser->stack_top--;
				r = json_parser_token(parser, token);
			} else {
				parser->state_stack[parser->stack_top] = k_parse_done;
			}
			break;

		default:
			r = k_parsing_error;
			break;
		}
		break;
	};

	return r;
}

static int32 json_parser_unicode(json_parser_t* parser, char c)
{
	int32 r = k_continue;
	char v = 0;
	if (('0' <= c) && (c <= '9')) {
		v = c - '0';
	} else if (('a' <= c) && (c <= 'f')) {
		v = 10 + c - 'a';
	} else if (('A' <= c) && (c <= 'F')) {
		v = 10 + c - 'A';
	}
	switch (parser->unicode) {
	case 1: parser->unihex = (v & 0x0f); 
		break;

	case 2: parser->unihex = (parser->unihex << 4) | (v & 0x0f); 
		break;

	case 3: parser->unihex = (parser->unihex << 4) | (v & 0x0f); 
		break;

	case 4: parser->unihex = (parser->unihex << 4) | (v & 0x0f); 
		if ((0 <= parser->unihex) && (parser->unihex <= 0x007f)) {
			r = json_parser_append(parser, (char) (parser->unihex & 0x007f));
		} else if (parser->unihex <= 0x07ff) {
			uint8 b1 = 0xc0 | (parser->unihex & 0x07c0) >> 6;
			uint8 b2 = 0x80 | (parser->unihex & 0x003f);
			r = json_parser_append(parser, (char) b1);
			if (r != k_continue) return r;
			r = json_parser_append(parser, (char) b2);
		} else if (parser->unihex <= 0xffff) {
			uint8 b1 = 0xe0 | (parser->unihex & 0xf000) >> 12;
			uint8 b2 = 0x80 | (parser->unihex & 0x0fc0) >> 6;
			uint8 b3 = 0x80 | (parser->unihex & 0x003f);
			r = json_parser_append(parser, (char) b1);
			if (r != k_continue) return r;
			r = json_parser_append(parser, (char) b2);
			if (r != k_continue) return r;
			r = json_parser_append(parser, (char) b3);
		} 
		break;		
	}
	return r;
}

static int32 json_parser_feed_string(json_parser_t* parser, char c)
{
	int32 r = k_continue;

	if (parser->unicode > 0) {
		r = json_parser_unicode(parser, c);
		if (parser->unicode++ == 4) {
			parser->unicode = 0;
		}

	} else if (parser->backslash) {
		parser->backslash = 0;
		switch (c) {
		case 'n': r = json_parser_append(parser, '\n');
			break; 
		case 'r': r = json_parser_append(parser, '\r');
			break; 
		case 't': r = json_parser_append(parser, '\t');
			break; 
		case 'b': r = json_parser_append(parser, '\b');
			break; 
		case 'f': r = json_parser_append(parser, '\f');
			break; 
		case 'u': parser->unicode = 1; 
			break;
		default: r = json_parser_append(parser, c);
			break;
		}

	} else if (c == '\\') {
		parser->backslash = 1;

	} else if (c == '"') {
		r = json_parser_append(parser, 0);
		if (r != k_continue) {
			return r;
		}
		r = json_parser_token(parser, k_string);
		parser->state = k_do;

	} else {
		r = json_parser_append(parser, c);
	}

	return r;
}

///////////////////////////////////////////////////////////////////
//
//                  +------------+
//     0     1     2|    4     5 |6               10
//  ()-+---+-+-[0]-++-[.]+[0-9]+-+----------------+--()
//     |   | |     |     +-----+ |                |
//     |   | |     |            [e]               |
//     +[-]+ +[1-9]+[0-9]+       |                |
//                 |     |       |  +[+]+         |
//                 +-----+       +--+---+---+[0-9]+ 
//                 3             7  +[-]+ 8 +-----+
//                                                9
//
//-----------------------------------------------------------------
//
//   0 + ''    -> 1
//   0 + '-'   -> 1
//   1 + '0'   -> 2
//   1 + [1-9] -> 3
//   3 + [0-9] -> 3
//   3 + ''    -> 2
//   2 + ''    -> 6
//   2 + '.'   -> 4
//   4 + [0-9] -> 5
//   5 + ''    -> 4
//   5 + ''    -> 6
//   6 + ''    -> 10
//   6 + 'e'   -> 7
//   6 + 'E'   -> 7
//   7 + ''    -> 8
//   7 + '-'   -> 8
//   7 + '+'   -> 8
//   8 + [0-9] -> 9
//   9 + ''    -> 10
//   9 + [0-9] -> 9
//
//-----------------------------------------------------------------
//
//   0_1      + '-'   -> 1
//   1        + '0'   -> 2_6_10
//   1        + [1-9] -> 2_3_6_10
//   0_1      + '0'   -> 2_6_10
//   0_1      + [1-9] -> 2_3_6_10
//   2_6_10   + '.'   -> 4
//   2_6_10   + 'e'   -> 7_8
//   2_3_6_10 + [0-9] -> 2_3_6_10
//   2_3_6_10 + '.'   -> 4
//   2_3_6_10 + 'e'   -> 7_8
//   7_8      + '+'   -> 7
//   7_8      + '-'   -> 7
//   7_8      + [0-9] -> 9_10
//   7        + [0-9] -> 9_10
//   9_10     + [0-9] -> 9_10
//   4        + [0-9] -> 5_6_10
//   5_6_10   + [0-9] -> 5_6_10
//   5_6_10   + 'e'   -> 7_8
//
//-----------------------------------------------------------------

enum { _error = 0, _0_1, _1, _2_6_10, _2_3_6_10, _7_8, _7, _9_10, _4, _5_6_10, _state_last };

enum { _other = 0, _min, _plus, _zero, _d19, _dot, _e, _input_last };

char _numtrans[_state_last][_input_last] = {
//        _other, _min,   _plus,  _zero,     _d19,      _dot,   _e,   
	{ _error, _error, _error, _error,    _error,    _error, _error },   // _error
	{ _error, _1,     _error, _2_6_10,   _2_3_6_10, _error, _error },   // _0_1
	{ _error, _error, _error, _2_6_10,   _2_3_6_10, _error, _error },   // _1
	{ _error, _error, _error, _error,    _error,    _4,     _7_8,  },   // _2_6_10
	{ _error, _error, _error, _2_3_6_10, _2_3_6_10, _4,     _7_8,  },   // _2_3_6_10
	{ _error, _7,     _7,     _9_10,     _9_10,     _error, _error },   // _7_8
	{ _error, _error, _error, _9_10,     _9_10,     _error, _error },   // _7
	{ _error, _error, _error, _9_10,     _9_10,     _error, _error },   // _9_10
	{ _error, _error, _error, _5_6_10,   _5_6_10,   _error, _error },   // _4
	{ _error, _error, _error, _5_6_10,   _5_6_10,   _error, _7_8,  }};  // _5_6_10

char _endstate[_state_last] = { 0, 0, 0, 1, 1, 0, 0, 1, 0, 1 };

static int32 json_numinput(json_parser_t* parser, char c)
{
        (void) parser;
	if (('1' <= c) && (c <= '9')) return _d19;
	if (c == '0') return _zero;
	if (c == '.') return _dot;
	if (c == '-') return _min;
	if (c == '+') return _plus;
	if (c == 'e') return _e;
	if (c == 'E') return _e;
	return _other;
}

static int32 json_parser_feed_number(json_parser_t* parser, char c)
{
	int32 r = k_token_error;
	
	int32 input = json_numinput(parser, c);

	int32 prevstate = parser->numstate;

	parser->numstate = _numtrans[prevstate][input];

	if (parser->numstate != _error) {
		r = json_parser_append(parser, c);

	} else {
		if (_endstate[prevstate]) {
			r = json_parser_append(parser, 0);
			if (r != k_continue) {
				return r;
			}
			r = json_parser_token(parser, k_number);
			if (r != k_continue) {
				return r;
			}
			//
			parser->state = k_do;
			r = json_parser_feed_one(parser, c);
		} else {
			r = k_token_error;
		}
	}

	return r;
}

static int32 json_parser_feed_true(json_parser_t* parser, char c)
{
	int32 r = json_parser_append(parser, c);
	if (r != k_continue) {
		return r;
	}
	if (parser->bufindex < 4) {
		if (JSON_MEMCMP(parser->buffer, "true", parser->bufindex) != 0) {
			return k_token_error;
		}
	} else if (parser->bufindex == 4) {
		if (JSON_MEMCMP(parser->buffer, "true", 4) == 0) {
			parser->state = k_do;
			return json_parser_token(parser, k_true);
		} else {
			return k_token_error;
		}
	}
	return k_continue;
}

static int32 json_parser_feed_false(json_parser_t* parser, char c)
{
	int32 r = json_parser_append(parser, c);
	if (r != k_continue) {
		return r;
	}
	if (parser->bufindex < 5) {
		if (JSON_MEMCMP(parser->buffer, "false", parser->bufindex) != 0) {
			return k_token_error;
		}
	} else if (parser->bufindex == 5) {
		if (JSON_MEMCMP(parser->buffer, "false", 5) == 0) {
			parser->state = k_do;
			return json_parser_token(parser, k_false);
		} else {
			return k_token_error;
		}
	}
	return k_continue;
}

static int32 json_parser_feed_null(json_parser_t* parser, char c)
{
	int32 r = json_parser_append(parser, c);
	if (r != k_continue) {
		return r;
	}
	if (parser->bufindex < 4) {
		if (JSON_MEMCMP(parser->buffer, "null", parser->bufindex) != 0) {
			return k_token_error;
		}
	} else if (parser->bufindex == 4) {
		if (JSON_MEMCMP(parser->buffer, "null", 4) == 0) {
			parser->state = k_do;
			return json_parser_token(parser, k_null);
		} else {
			return k_token_error;
		}
	}
	return k_continue;
}

static int32 json_parser_do(json_parser_t* parser, char c)
{
	int32 r = k_continue;

	if (whitespace(c)) {
		return k_continue;
	} 

	switch (c) {
	case '{': 
		parser->state = k_do;
		r = json_parser_token(parser, k_object_start);
		break;

	case '}': 
		parser->state = k_do;
		r = json_parser_token(parser, k_object_end);
		break;

	case '[': 
		parser->state = k_do;
		r = json_parser_token(parser, k_array_start);
		break;

	case ']': 
		parser->state = k_do;
		r = json_parser_token(parser, k_array_end);
		break;

	case '"': 
		parser->state = k_parsing_string;
		break;

	case 't': 
		parser->state = k_parsing_true;
		r = json_parser_feed_true(parser, c);
		break;

	case 'f': 
		parser->state = k_parsing_false;
		r = json_parser_feed_false(parser, c);
		return k_continue;

	case 'n': 
		parser->state = k_parsing_null;
		r = json_parser_feed_null(parser, c);
		return k_continue;
		
	case '-': case 'O': case '0': case '1': 
	case '2': case '3': case '4': case '5': 
	case '6': case '7': case '8': case '9': 
		parser->state = k_parsing_number;
		parser->numstate = _0_1;
		r = json_parser_feed_number(parser, c);
		break;

	case ',': 
		parser->state = k_do;
		r = json_parser_token(parser, k_comma);
		break;	

	case ':': 
		parser->state = k_do;
		r = json_parser_token(parser, k_colon);
		break;

	case '\0':
		r = k_end_of_string;
		break;

	default: 
		r = k_token_error;
		break;
	}

        if (r != k_continue) {
                char message[256];
                snprintf(message, 255, "invalid char: %c", c);
                message[255] = 0;
                json_parser_set_error(parser, 1, message);
        }

	return r;
}

int32 json_parser_feed_one(json_parser_t* parser, char c)
{
        int32 ret = k_token_error;

	switch (parser->state) {
	case k_do: 
		ret = json_parser_do(parser, c);
                break;

	case k_parsing_string: 
		ret = json_parser_feed_string(parser, c);
                break;

	case k_parsing_number:
		ret = json_parser_feed_number(parser, c);
                break;

	case k_parsing_false: 
		ret = json_parser_feed_false(parser, c);
                break;

	case k_parsing_true: 
		ret = json_parser_feed_true(parser, c);
                break;

	case k_parsing_null: 
		ret = json_parser_feed_null(parser, c);
                break;
	}

	return ret;
}

int32 json_parser_feed(json_parser_t* parser, const char* buffer, int32 len)
{
	int32 r = 0;
	while (len-- > 0) {
		r = json_parser_feed_one(parser, *buffer++);
		if (r != 0) {
			break;
		}
	}
	return r;
}

int32 json_parser_done(json_parser_t* parser)
{
	return parser->state_stack[parser->stack_top] == k_parse_done;
}

json_object_t json_parser_eval(json_parser_t* parser, const char* s)
{
	int32 r = 0;

	json_parser_reset(parser);

	do {
		r = json_parser_feed_one(parser, *s);
		if (r != k_continue) {
			switch (r) {
			case k_out_of_memory: 
				json_parser_set_error(parser, -1, "Out of memory");
				return json_null();
				
			case k_token_error: 
				json_parser_set_error(parser, -2, "Unexpected character");
				return json_null();

			case k_parsing_error: 
				json_parser_set_error(parser, -2, "Parse error");
				return json_null();
			}
		}

	} while (*s++);

	if (!json_parser_done(parser)) {
		json_parser_set_error(parser, -1, "Unexpected end while parsing");
		return json_null();
	}

	return parser->value_stack[0];
}

json_object_t json_parser_result(json_parser_t* parser)
{
	return parser->value_stack[0];
}

json_object_t json_load(const char* filename, char* err, int len)
{
        if (err) err[0] = 0;

        json_parser_t* parser = json_parser_create();
        if (parser == NULL) {
                snprintf(err, len, "json_load: Failed to create the file parser.");
                err[len-1] = 0;
                return json_null();
        }
        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
                snprintf(err, len, "json_load: Failed to open the file: %s", filename);
                err[len-1] = 0;
                json_parser_destroy(parser);
                return json_null();
        }
        while (!json_parser_done(parser)) {
                int c = fgetc(fp);
                if (c == EOF) {
                        snprintf(err, len, "json_load: The file is corrupt.");
                        err[len-1] = 0;
                        fclose(fp);
                        json_parser_destroy(parser);
                        return json_null();
                }
                int32 r = json_parser_feed_one(parser, c);
                if (r != 0) {
                        snprintf(err, len, 
                                 "json_load: Failed to parse the file: %s: %s.\n",
                                 filename, json_parser_errstr(parser));
                        err[len-1] = 0;
                        fclose(fp);
                        json_parser_destroy(parser);
                        return json_null();
                } 
        }
        json_object_t obj = json_parser_result(parser);

        fclose(fp);
        json_parser_destroy(parser);

        return obj;
}
