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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define HASHTABLE_MIN_SIZE 7
#define HASHTABLE_MAX_SIZE 13845163

char* json_strdup(const char* s);

#define base_type(_b)      (_b)->type
#define base_get(_b,_type) ((_type*)(_b)->value.data)
#define base_getnum(_b)    (_b)->value.number

static base_t* _null = NULL;
static base_t* _true = NULL;
static base_t* _false = NULL;
static base_t* _undefined = NULL;

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

base_t* base_new(int type, int len)
{
        base_t* base = JSON_NEW(base_t);
        if (base == NULL)
                return NULL;
        base->refcount = 1;
        base->type = type;
        if (len > 0) {
                base->value.data = JSON_NEW_ARRAY(char, len);
                if (base->value.data == NULL) {
                        JSON_FREE(base);
                        return NULL;
                }
        }
        return base;
}

/******************************************************************************/

json_object_t json_null()
{
        if (_null == NULL) {
                _null = base_new(k_json_null, 0);
                _null->type = k_json_null;
                if (_null == NULL) {
                        fprintf(stderr, "Failed to allocate the null object\n");
                        exit(1);
                }
        }

	return _null;
}

json_object_t json_true()
{
        if (_true == NULL) {
                _true = base_new(k_json_true, 0);
                _true->type = k_json_true;
                if (_true == NULL) {
                        fprintf(stderr, "Failed to allocate the null object\n");
                        exit(1);
                }
        }

	return _true;
}

json_object_t json_false()
{
        if (_false == NULL) {
                _false = base_new(k_json_false, 0);
                _false->type = k_json_false;
                if (_false == NULL) {
                        fprintf(stderr, "Failed to allocate the null object\n");
                        exit(1);
                }
        }

	return _false;
}

json_object_t json_undefined()
{
        if (_undefined == NULL) {
                _undefined = base_new(k_json_undefined, 0);
                _undefined->type = k_json_undefined;
                if (_undefined == NULL) {
                        fprintf(stderr, "Failed to allocate the null object\n");
                        exit(1);
                }
        }

	return _undefined;
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

json_object_t json_number_create(double value)
{
	base_t *base;
        base = base_new(k_json_object, 0);
        if (base == NULL)
                return json_null();
	base->type = k_json_number;
	base->value.number = value;
	return base;
}

static void delete_number(base_t* base)
{
}

real_t json_number_value(json_object_t obj)
{
	return obj->value.number;
}

/******************************************************************************/

typedef struct _string_t {
	int length;
	char s[0];
} string_t;

json_object_t json_string_create(const char* s)
{
	base_t *base;
	string_t *string;
       	int len = strlen(s);

        base = base_new(k_json_string, sizeof(string_t) + len + 1);
        if (base == NULL)
                return json_null();
        base->type = k_json_string;

	string = base_get(base, string_t);
	string->length = len;
	memcpy(string->s, s, string->length);
	string->s[string->length] = 0;

	return base;
}

static void delete_string(base_t* base)
{
	string_t *string = base_get(base, string_t);
	JSON_FREE(string);
}

const char* json_string_value(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_string)
		return NULL;
	string_t* str = base_get(base, string_t);
	return str->s;
}

int32 json_string_length(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_string)
		return 0;
	string_t* str = base_get(base, string_t);
	return str->length;
}

int32 json_string_compare(json_object_t obj1, const char* s)
{
	if (base_type(obj1) != k_json_string)
		return -1;
	string_t* s1 = base_get(obj1, string_t);
        return strcmp(s1->s, s);
}

int32 json_string_equals(json_object_t obj1, const char* s)
{
        return json_string_compare(obj1, s) == 0;
}

/******************************************************************************/

typedef struct _array_t {
	int length;
	int datalen;
	json_object_t data[0];
} array_t;

json_object_t json_array_create()
{
	base_t *base;
	array_t *array;
       	int len = 4;
        int memlen = sizeof(array_t) + len * sizeof(json_object_t);

        base = base_new(k_json_array, memlen);
        if (base == NULL)
                return json_null();
        base->type = k_json_array;

	array = base_get(base, array_t);

	array->length = 0;
	array->datalen = len;

	return base;
}

static void delete_array(base_t* base)
{
	array_t *array = base_get(base, array_t);
        int i;

        for (i = 0; i < array->length; i++)
                json_unref(array->data[i]);
	JSON_FREE(array);
}

int32 json_array_length(json_object_t array)
{
	if (array->type != k_json_array)
		return 0;
	array_t* a = (array_t*) array->value.data;
	return a->length;	
}

json_object_t json_array_get(json_object_t obj, int32 index)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array)
		return json_null();
	array_t *array = base_get(base, array_t);
	if ((0 <= index) && (index < array->length)) {
		return array->data[index];
	}
	return json_null();
}

real_t json_array_getnum(json_object_t obj, int32 index)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array)
		return NAN;
	array_t *array = base_get(base, array_t);
	if ((0 <= index) && (index < array->length))
		return json_number_value(array->data[index]);
	return NAN;
}

int32 json_array_set(json_object_t obj, json_object_t value, int32 index)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array)
		return 0;
	array_t *array = base_get(base, array_t);
        // LOCK
	if (index >= array->datalen) {
		int newlen = 8;
		while (newlen <= index) {
			newlen *= 2;
		} 
                
                int memlen = sizeof(array_t) + newlen * sizeof(json_object_t);
                array_t *newarray = JSON_NEW_ARRAY(array_t, memlen);
		if (newarray == NULL) {
                        // UNLOCK
			return -1;
		}
		for (int i = 0; i < array->datalen; i++)
			newarray->data[i] = array->data[i];

		newarray->datalen = newlen;
		newarray->length = array->length;

                base->value.data = newarray;                

                JSON_FREE(array);
                array = newarray;
	}

        json_object_t old = array->data[index];
	array->data[index] = value;  // FIXME: write barrier
	if (index >= array->length) 
		array->length = index + 1;
	
        // UNLOCK

        json_ref(value);
        if (old) json_unref(old);

	return 0;
}

int32 json_array_push(json_object_t obj, json_object_t value)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array)
		return 0;
	array_t *array = base_get(base, array_t);
	return json_array_set(obj, value, array->length);	
}

int32 json_array_setnum(json_object_t obj, real_t value, int32 index)
{
        json_object_t num = json_number_create(value);
        json_array_set(obj, num, index);	
        json_unref(num);
        return 0;
}

int32 json_array_setstr(json_object_t obj, const char* value, int32 index)
{
        json_object_t s = json_string_create(value);
        json_array_set(obj, s, index);	
        json_unref(s);
        return 0;
}

const char* json_array_getstr(json_object_t array, int32 index)
{
        json_object_t val = json_array_get(array, index);
        if (val->type == k_json_string)
                return json_string_value(val);
        else
                return NULL;
}

/******************************************************************************/

typedef struct _hashnode_t hashnode_t;

struct _hashnode_t {
	char* key;
	json_object_t value;
	hashnode_t *next;
};

static hashnode_t* new_json_hashnode(const char* key, json_object_t* value);
static void delete_json_hashnode(hashnode_t *hash_node);
static void delete_json_hashnodes(hashnode_t *hash_node);
uint32 json_strhash(const char* v);

static hashnode_t* new_json_hashnode(const char* key, json_object_t* value)
{
	hashnode_t *hash_node = JSON_NEW(hashnode_t);
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

static void delete_json_hashnode(hashnode_t *hash_node)
{
	json_unref(hash_node->value);
	JSON_FREE(hash_node->key);
	JSON_FREE(hash_node);
}

static void delete_json_hashnodes(hashnode_t *hash_node)
{
	while (hash_node) {
		hashnode_t *next = hash_node->next;
		delete_json_hashnode(hash_node);
		hash_node = next;
	}  
}

/******************************************************************************/

typedef struct _hashtable_t {
	uint32 refcount;
        int32 size;
        int32 num_nodes;
	hashnode_t **nodes;
} hashtable_t;


static hashtable_t* new_hashtable();
static void delete_hashtable(hashtable_t *hashtable);
static int32 hashtable_set(hashtable_t *hashtable, const char* key, json_object_t value);
static json_object_t hashtable_get(hashtable_t *hashtable, const char* key);
static int32 hashtable_unset(hashtable_t *hashtable, const char* key);
static int32 hashtable_foreach(hashtable_t *hashtable, json_iterator_t func, void* data);
static int32 hashtable_size(hashtable_t *hashtable);
static void hashtable_resize(hashtable_t *hashtable);
static hashnode_t** hashtable_lookup_node(hashtable_t *hashtable, const char* key);

static hashtable_t* new_hashtable()
{
	hashtable_t *hashtable;
	
	hashtable = JSON_NEW(hashtable_t);
	if (hashtable == NULL) {
		return NULL;
	}
	hashtable->refcount = 0;
	hashtable->num_nodes = 0;
	hashtable->size = 0;
	hashtable->nodes = NULL;
	
	return hashtable;
}

static void delete_hashtable(hashtable_t *hashtable)
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

static hashnode_t** hashtable_lookup_node(hashtable_t* hashtable, const char* key)
{
	hashnode_t **node;
	
	if (hashtable->nodes == NULL) {
		hashtable->size = HASHTABLE_MIN_SIZE;
		hashtable->nodes = JSON_NEW_ARRAY(hashnode_t*, hashtable->size);	
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

static int32 hashtable_set(hashtable_t *hashtable, const char* key, json_object_t value)
{
  
	if ((key == NULL) || (JSON_STRLEN(key) == 0)) {
		return -1;
	}

	hashnode_t **node = hashtable_lookup_node(hashtable, key);

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
			hashtable_resize(hashtable);
		} 
	}

        char buffer[256];
        json_tostring(value, buffer, sizeof(buffer));
        //printf("hash node %s - %s\n", key, buffer);

	return 0;
}

static json_object_t hashtable_get(hashtable_t *hashtable, const char* key)
{
	hashnode_t *node;
	node = *hashtable_lookup_node(hashtable, key);
       	return (node)? node->value : json_null();
}

static int32 hashtable_unset(hashtable_t *hashtable, const char* key)
{
	hashnode_t **node, *dest;
  
	node = hashtable_lookup_node(hashtable, key);
	if (*node) {
		dest = *node;
		(*node) = dest->next;
		delete_json_hashnode(dest);
		hashtable->num_nodes--;
		return 0;
	}
	
	return -1;
}

static int32 hashtable_foreach(hashtable_t *hashtable, json_iterator_t func, void* data)
{
	hashnode_t *node = NULL;
	
	for (int32 i = 0; i < hashtable->size; i++) {
		for (node = hashtable->nodes[i]; node != NULL; node = node->next) {
			int32 r = (*func)(node->key, node->value, data);
                        if (r != 0) return r;
		}
	}
        return 0;
}

static int32 hashtable_size(hashtable_t *hashtable)
{
	return hashtable->num_nodes;
}


static void hashtable_resize(hashtable_t *hashtable)
{
	hashnode_t **new_nodes;
	hashnode_t *node;
	hashnode_t *next;
	uint32 hash_val;
	int32 new_size;
	
	new_size = 3 * hashtable->size + 1;
	new_size = (new_size > HASHTABLE_MAX_SIZE)? HASHTABLE_MAX_SIZE : new_size;
	
	new_nodes = JSON_NEW_ARRAY(hashnode_t*, new_size);
	
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

json_object_t json_object_create()
{
	base_t *base;
	hashtable_t* hashtable;

        base = base_new(k_json_object, 0);
        if (base == NULL)
                return json_null();
	base->type = k_json_object;
	hashtable = new_hashtable();
	if (hashtable == NULL) {
                JSON_FREE(base);
                return json_null();
	}
	
	base->value.data = hashtable;
	return base;
}

static void delete_object(base_t* base)
{
	hashtable_t *hashtable = base_get(base, hashtable_t);
        delete_hashtable(hashtable);
}

int32 json_object_set(json_object_t object, const char* key, json_object_t value)
{
	if (object->type != k_json_object) {
		return -1;
	}
	hashtable_t *hashtable = base_get(object, hashtable_t);
	return hashtable_set(hashtable, key, value);
}

json_object_t json_object_get(json_object_t object, const char* key)
{
	if (object->type != k_json_object) {
		return json_null();
	}
	hashtable_t *hashtable = base_get(object, hashtable_t);
	return hashtable_get(hashtable, key);
}

double json_object_getnum(json_object_t object, const char* key)
{
	if (object->type != k_json_object) {
		return NAN;
	}
	hashtable_t *hashtable = base_get(object, hashtable_t);
	json_object_t val = hashtable_get(hashtable, key);
        if (val->type == k_json_number) {
                return json_number_value(val);
        } else {
                return NAN;
        }
}

const char* json_object_getstr(json_object_t object, const char* key)
{
	if (object->type != k_json_object)
		return NULL;
	hashtable_t *hashtable = base_get(object, hashtable_t);
	json_object_t val = hashtable_get(hashtable, key);
        if (val->type == k_json_string) {
                return json_string_value(val);
        } else {
                return NULL;
        }
}

int32 json_object_unset(json_object_t object, const char* key)
{
	if (object->type != k_json_object)
		return -1;
	hashtable_t *hashtable = base_get(object, hashtable_t);
	return hashtable_unset(hashtable, key);
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
	if (object->type != k_json_object)
		return -1;
	hashtable_t *hashtable = base_get(object, hashtable_t);
	return hashtable_foreach(hashtable, func, data);
}

int32 json_object_length(json_object_t object)
{
	if (object->type != k_json_object)
		return 0;
	return hashtable_size(base_get(object, hashtable_t));
}

/******************************************************************************/

typedef struct _variable_t {
        json_object_t name;
} variable_t;

json_object_t variable_create(const char* s)
{
	base_t *base;
	variable_t *variable;

        base = base_new(k_json_variable, sizeof(variable_t));
        if (base == NULL)
                return json_null();

	variable = base_get(base, variable_t);
        variable->name = json_string_create(s);

	return base;
}

void delete_variable(base_t* base)
{
	variable_t* variable = base_get(base, variable_t);
	json_unref(variable->name);
	JSON_FREE(variable);
}

const char* variable_name(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_variable)
		return NULL;
	variable_t* variable = base_get(base, variable_t);
        return json_string_value(variable->name);
}

json_object_t variable_string_name(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_variable)
		return NULL;
	variable_t* variable = base_get(base, variable_t);
        return variable->name;
}

/******************************************************************************/

typedef struct _accessor_t {
	json_object_t context;
	json_object_t variable;
} accessor_t;

json_object_t accessor_create(json_object_t context, json_object_t variable)
{
	base_t *base;
	accessor_t *accessor;

        base = base_new(k_json_accessor, sizeof(accessor_t));
        if (base == NULL)
                return json_null();

	accessor = base_get(base, accessor_t);
        accessor->context = context; 
        accessor->variable = variable;
        json_ref(accessor->context);
        json_ref(accessor->variable);

	return base;
}

void delete_accessor(base_t* base)
{
	accessor_t* accessor = base_get(base, accessor_t);
        json_unref(accessor->context);
        json_unref(accessor->variable);
        JSON_FREE(accessor);
}

json_object_t accessor_context(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_accessor)
		return json_null();
	accessor_t* accessor = base_get(base, accessor_t);
        return accessor->context;
}

json_object_t accessor_variable(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_accessor)
		return json_null();
	accessor_t* accessor = base_get(base, accessor_t);
        return accessor->variable;
}

/******************************************************************************/

typedef struct _array_element_t {
	json_object_t accessor;
	json_object_t index;
} array_element_t;

json_object_t array_element_create(json_object_t accessor, json_object_t index)
{
	base_t *base;
	array_element_t *array_element;

        base = base_new(k_json_array_element, sizeof(array_element_t));
        if (base == NULL)
                return json_null();

	array_element = base_get(base, array_element_t);
        array_element->accessor = accessor;
        array_element->index = index;
        json_ref(array_element->accessor);
        json_ref(array_element->index);

	return base;
}

void delete_array_element(base_t* base)
{
	array_element_t* array_element = base_get(base, array_element_t);
        json_unref(array_element->accessor);
        json_unref(array_element->index);
        JSON_FREE(array_element);
}

json_object_t array_element_accessor(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array_element)
		return json_null();
	array_element_t* array_element = base_get(base, array_element_t);
        return array_element->accessor;
}

json_object_t array_element_index(json_object_t obj)
{
	base_t* base = (base_t*) obj;
	if (base_type(base) != k_json_array_element)
		return json_null();
	array_element_t* array_element = base_get(base, array_element_t);
        return array_element->index;
}

/******************************************************************************/

static void _delete(base_t *base)
{
        if (base == NULL)
                return;

	switch (base->type) {
	case k_json_number: delete_number(base); break;
	case k_json_string: delete_string(base); break;
	case k_json_array: delete_array(base); break;
	case k_json_object: delete_object(base); break;
	case k_json_variable: delete_variable(base); break;
        case k_json_accessor: delete_accessor(base); break;
        case k_json_array_element: delete_array_element(base); break;
	default: break;
	}

        JSON_FREE(base);
}

void json_refcount(json_object_t obj, int32 val)
{
	if ((obj == NULL) || (obj->type < 100)) {
                return;
        }
        obj->refcount += val;
        if (obj->refcount <= 0)
                _delete(obj);
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

	switch (object->type) {

	case k_json_number: {
		char buf[128];
		if (floor(object->value.number) == object->value.number) 
			snprintf(buf, 128, "%g", object->value.number);
		else 
			snprintf(buf, 128, "%e", object->value.number);
		
		buf[127] = 0;
		r = json_print(fun, userdata, buf);
		if (r != 0) return r;
	} break;
		
	case k_json_string: {
		string_t* string = (string_t*) object->value.data;
		r = json_print(fun, userdata, "\"");
		if (r != 0) return r;
		r = json_print(fun, userdata, string->s);
		if (r != 0) return r;
		r = json_print(fun, userdata, "\"");
		if (r != 0) return r;
	} break;

	case k_json_boolean: {
		if (object->value.number) {
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
		array_t* array = (array_t*) object->value.data;
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

		hashtable_t* hashtable = (hashtable_t*) object->value.data;
		hashnode_t *node = NULL;
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

/******************************************************************************/

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
// object_3    + (value)      -> object_4  & object->set(key,top_value)
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

json_object_t json_load(const char* filename, int* err, char* errmsg, int len)
{
        errmsg[0] = 0;
        *err = 0;

        json_parser_t* parser = json_parser_create();
        if (parser == NULL) {
                *err = 1;
                snprintf(errmsg, len, "json_load: Failed to create the file parser.");
                errmsg[len-1] = 0;
                return json_null();
        }
        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
                *err = 1;
                snprintf(errmsg, len, "json_load: Failed to open the file: %s", filename);
                errmsg[len-1] = 0;
                json_parser_destroy(parser);
                return json_null();
        }
        while (!json_parser_done(parser)) {
                int c = fgetc(fp);
                if (c == EOF) {
                        *err = 1;
                        snprintf(errmsg, len, "json_load: The file is corrupt.");
                        errmsg[len-1] = 0;
                        fclose(fp);
                        json_parser_destroy(parser);
                        return json_null();
                }
                int32 r = json_parser_feed_one(parser, c);
                if (r != 0) {
                        *err = 1;
                        snprintf(errmsg, len, 
                                 "json_load: Failed to parse the file: %s: %s.\n",
                                 filename, json_parser_errstr(parser));
                        errmsg[len-1] = 0;
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

/******************************************************************************/

enum {
        k_tokenizer_continue,
        k_tokenizer_newtoken,
        k_tokenizer_error,
        k_tokenizer_endofstring
};

typedef enum {
	k_tokenizer_start = 0,
	k_tokenizer_number,
	k_tokenizer_variable
} tokenizer_state_t;

typedef enum {
	k_token_dot,
	k_token_number,
	k_token_variable,
	k_token_bracketopen,
	k_token_bracketclose,
	k_token_end
} token_t;

typedef struct _tokenizer_t {
        char* s;
        int index;
	char buffer[256];
	int buflen;
	int bufindex;
	int state;
} tokenizer_t;

void tokenizer_reset(tokenizer_t* tokenizer);

tokenizer_t* new_tokenizer(const char* s)
{
        tokenizer_t* tokenizer = JSON_NEW(tokenizer_t);
        if (tokenizer == NULL)
                return NULL;

	memset(tokenizer, 0, sizeof(tokenizer_t));
        tokenizer->s = json_strdup(s);
        tokenizer->state = k_tokenizer_start;
        tokenizer->index = 0;
	tokenizer->bufindex = 0;
	tokenizer->buflen = sizeof(tokenizer->buffer);
        tokenizer_reset(tokenizer);

        return tokenizer;
}

void delete_tokenizer(tokenizer_t* tokenizer)
{
        JSON_FREE(tokenizer);
}

static int tokenizer_append(tokenizer_t* tokenizer, char c)
{
	if (tokenizer->bufindex >= tokenizer->buflen - 1) {
                return k_tokenizer_error;
        }
	tokenizer->buffer[tokenizer->bufindex++] = (char) (c & 0xff);
	return k_tokenizer_continue;
}

void tokenizer_reset(tokenizer_t* tokenizer)
{
	tokenizer->state = k_tokenizer_start;
	tokenizer->bufindex = 0;
}

static int tokenizer_getc(tokenizer_t* tokenizer)
{
        return tokenizer->s[tokenizer->index++];
}

static void tokenizer_ungetc(tokenizer_t* tokenizer, int c)
{
        tokenizer->index--;
}

static void tokenizer_reset_buffer(tokenizer_t* tokenizer)
{ 
	tokenizer->bufindex = 0;
}

char* tokenizer_get_data(tokenizer_t* tokenizer)
{
	tokenizer->buffer[tokenizer->bufindex] = 0;
        return tokenizer->buffer;
}

static int tokenizer_feed_number(tokenizer_t* tokenizer, int *token)
{
        int r;
        int c = tokenizer_getc(tokenizer);
        if (c == -1) {
                r = k_token_end;
                return k_tokenizer_endofstring;
        }

        if (('0' <= c) && (c <= '9')) { 
		r = tokenizer_append(tokenizer, c);
        } else {
                r = tokenizer_append(tokenizer, 0);
                tokenizer_ungetc(tokenizer, c);
                r = k_tokenizer_newtoken;
                *token = k_token_number;
        }	

	return r;
}

static inline int tokenizer_whitespace(int c)
{
	return ((c == ' ') || (c == '\r') || (c == '\n') || (c == '\t'));
}

static int tokenizer_feed_variable(tokenizer_t* tokenizer, 
                                   int* token)
{
        int c = tokenizer_getc(tokenizer);
        if (c == -1)  {
                return k_tokenizer_endofstring;
        }
        if ((('a' <= c) && (c <= 'z')) 
            || (('A' <= c) && (c <= 'Z')) 
            || (('0' <= c) && (c <= '9')) 
            || (c == '_')) {
                int r = tokenizer_append(tokenizer, c);
                if (r != k_tokenizer_continue) {
                        return r;
                }
                return k_tokenizer_continue;

        } else {
                tokenizer_ungetc(tokenizer, c);
                *token = k_token_variable;
                return k_tokenizer_newtoken;
        }
	return k_tokenizer_error;
}

static int tokenizer_start(tokenizer_t* tokenizer, int* token)
{
	int r = k_tokenizer_continue;

        int c = tokenizer_getc(tokenizer);
        if (c == -1)
                return k_tokenizer_endofstring;
        
	if (tokenizer_whitespace(c))
		return k_tokenizer_continue;

	switch (c) {
	case '[': 
		*token = k_token_bracketopen;
		r = k_tokenizer_newtoken;
		break;

	case ']': 
		*token = k_token_bracketclose;
		r = k_tokenizer_newtoken;
		break;

	case '.': 
		*token = k_token_dot;
		r = k_tokenizer_newtoken;
		break;
				
	case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9': 
		tokenizer->state = k_tokenizer_number;
                tokenizer_ungetc(tokenizer, c);
                r = tokenizer_feed_number(tokenizer, token);
		break;

	case '\0':
		r = k_tokenizer_endofstring;
		break;

	default: 
                if ((('a' <= c) && (c <= 'z'))
                    || (('A' <= c) && (c <= 'Z'))
                    || (c == '_')) {
                        tokenizer->state = k_tokenizer_variable;
                        tokenizer_ungetc(tokenizer, c);
                        r = tokenizer_feed_variable(tokenizer, token);
                } else {
                        r = k_tokenizer_error;
                }
                break;
	}

	return r;
}

int tokenizer_advance(tokenizer_t* tokenizer, int* token)
{
        int r = k_tokenizer_error;

	switch (tokenizer->state) {
	case k_tokenizer_start: 
                tokenizer_reset_buffer(tokenizer);
		r = tokenizer_start(tokenizer, token);
                break;

	case k_tokenizer_number:
		r = tokenizer_feed_number(tokenizer, token);
                break;

	case k_tokenizer_variable: 
		r = tokenizer_feed_variable(tokenizer, token);
                break;
	}
        if (r == k_tokenizer_newtoken)
		tokenizer->state = k_tokenizer_start;

	return r;
}

int tokenizer_get(tokenizer_t* tokenizer, int* token)
{
        while (1) {
                int ret = tokenizer_advance(tokenizer, token);
                if (ret == k_tokenizer_continue) 
                        continue;
                return ret;
        }
}

/******************************************************************************/

// expression: accessor
//           | array_element

// accessor:   variable                       # "foo"
//           | expression DOT variable        # "foo.bar", "foo.bar[1].x"

// array_element: "[" number "]"              # "foo.bar[1]"
//              | expression "[" number "]"   # "foo.bar[1]"

/*

0 [var,bracketopen]     --var----------> 1 [dot,end]              + object_get
                        --bracketopen--> 2 [number]
1 [dot,end]             --dot----------> 4 [var]
                        --end----------> 6 [DONE]
2 [number]              --number-------> 5 [bracketclose]
3 [dot,bracketopen,end] --dot----------> 4 [var]
                        --bracketopen--> 2 [number]
                        --end----------> 6 [DONE]
4 [var]                 --var----------> 3 [dot,bracketopen,end] + object_get
5 [bracketclose]        --bracketclose-> 3 [dot,bracketopen,end] + array_get
6 [DONE]

*/

typedef enum _evaluator_state_t {
        evaluator_state_0,
        evaluator_state_1,
        evaluator_state_2,
        evaluator_state_3,
        evaluator_state_4,
        evaluator_state_5,
        evaluator_state_6
} evaluator_state_t;

typedef struct _evaluator_t {
        tokenizer_t* tokenizer;
        evaluator_state_t state;
} evaluator_t;

void evaluator_reset(evaluator_t* evaluator);

evaluator_t* new_evaluator(const char* s)
{
        evaluator_t* evaluator = JSON_NEW(evaluator_t);
        if (evaluator == NULL)
                return NULL;

	memset(evaluator, 0, sizeof(evaluator_t));
        evaluator->tokenizer = new_tokenizer(s);
        evaluator->state = evaluator_state_0;
        return evaluator;
}

void delete_evaluator(evaluator_t* evaluator)
{
        delete_tokenizer(evaluator->tokenizer);
        JSON_FREE(evaluator);
}

json_object_t evaluator_do(evaluator_t* evaluator, json_object_t x)
{
        int token;

        while (1) {

                if (json_isnull(x))
                        return x;

                int r = tokenizer_get(evaluator->tokenizer, &token);
                if (r == k_tokenizer_error) 
                        return json_null();

                if (r == k_tokenizer_endofstring) { 
                        switch (evaluator->state) {
                        case evaluator_state_1:
                        case evaluator_state_3:
                                return x;
                        default:
                                return json_null();
                        }
                }

                switch (evaluator->state) {
                case evaluator_state_0:
                        if (token == k_token_variable) {
                                x = json_object_get(x, tokenizer_get_data(evaluator->tokenizer));
                                evaluator->state = evaluator_state_1;
                        } else if (token == k_token_bracketopen) {
                                evaluator->state = evaluator_state_2;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_1:
                        if (token == k_token_dot) {
                                evaluator->state = evaluator_state_4;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_2:
                        if (token == k_token_number) {
                                int index = atoi(tokenizer_get_data(evaluator->tokenizer));
                                x = json_array_get(x, index);
                                evaluator->state = evaluator_state_5;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_3:
                        if (token == k_token_dot) {
                                evaluator->state = evaluator_state_4;
                        } else if (token == k_token_bracketopen) {
                                evaluator->state = evaluator_state_2;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_4:
                        if (token == k_token_variable) {
                                x = json_object_get(x, tokenizer_get_data(evaluator->tokenizer));
                                evaluator->state = evaluator_state_1;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_5:
                        if (token == k_token_bracketclose) {
                                evaluator->state = evaluator_state_3;
                        } else {
                                return json_null();
                        }
                        break;

                case evaluator_state_6:
                        break;
                }
        }
}

json_object_t json_get(json_object_t obj, const char* expression)
{
        evaluator_t* e = new_evaluator(expression);
        json_object_t r = evaluator_do(e, obj);
        delete_evaluator(e);
        return r;
}

const char* json_getstr(json_object_t obj, const char* expression)
{
        json_object_t s = json_get(obj, expression);
        return json_string_value(s);
}

double json_getnum(json_object_t obj, const char* expression)
{
        json_object_t v = json_get(obj, expression);
        return json_number_value(v);
}

