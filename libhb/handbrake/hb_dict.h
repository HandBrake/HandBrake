/* hb_dict.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#if !defined(HANDBRAKE_DICT_H)
#define HANDBRAKE_DICT_H

#include "handbrake/hbtypes.h"
#include <jansson.h>

#define HB_VALUE_TYPE_DICT      JSON_OBJECT
#define HB_VALUE_TYPE_ARRAY     JSON_ARRAY
#define HB_VALUE_TYPE_STRING    JSON_STRING
#define HB_VALUE_TYPE_INT       JSON_INTEGER
#define HB_VALUE_TYPE_DOUBLE    JSON_REAL
#define HB_VALUE_TYPE_NULL      JSON_NULL
#define HB_VALUE_TYPE_BOOL      0xff

#define HB_DICT_ITER_DONE       NULL

typedef int        hb_value_type_t;
typedef json_t     hb_value_t;
typedef hb_value_t hb_dict_t;
typedef hb_value_t hb_value_array_t;
typedef void*      hb_dict_iter_t;

/* A dictionary implementation.
 *
 * an hb_dict_t must be initialized with hb_dict_init() before use.
 *
 * "key" must be a string with non-zero length (NULL and "" are invalid keys).
 * "value" must be an hb_value_t*
 */
hb_dict_t *       hb_dict_init(void);
void              hb_dict_clear(hb_dict_t *dict);
/* free dictionary and release references to all values it contains */
void              hb_dict_free(hb_dict_t ** dict_ptr);
/* return number of member elements in the dictionary */
int               hb_dict_elements(hb_dict_t * dict);
/* add value to dictionary.  dictionary takes ownership of value */
void              hb_dict_set(hb_dict_t * dict, const char * key,
                              hb_value_t * value);
void              hb_dict_merge(hb_dict_t * dict, hb_dict_t *value);
void              hb_dict_case_set(hb_dict_t * dict, const char *key,
                                   hb_value_t *value);
/* remove value from dictionary.  releases reference to value */
int               hb_dict_remove(hb_dict_t * dict, const char * key);
/* get value from dictionary.  value has borrowed reference */
hb_value_t *      hb_dict_get(const hb_dict_t * dict, const char * key);
int               hb_dict_extract_int(int *dst,
                                      const hb_dict_t * dict,
                                      const char * key);
int               hb_dict_extract_double(double *dst,
                                         const hb_dict_t * dict,
                                         const char * key);
int               hb_dict_extract_bool(int *dst,
                                       const hb_dict_t * dict,
                                       const char * key);
int               hb_dict_extract_string(char **dst,
                                         const hb_dict_t * dict,
                                         const char * key);
int               hb_dict_extract_rational(hb_rational_t *dst,
                                           const hb_dict_t * dict,
                                           const char * key);
int               hb_dict_extract_int_array(int *dst, int count,
                                            const hb_dict_t * dict,
                                            const char * key);

/* dict iterator
 * hb_dict_iter_init(dict) returns an iter to the first key/value in the dict
 * hb_dict_iter_next(dict, iter) returns an iter to the next key/value
 * HB_DICT_ITER_DONE if the end of the dictionary was reached.
 */
hb_dict_iter_t    hb_dict_iter_init(const hb_dict_t *dict);
hb_dict_iter_t    hb_dict_iter_next(const hb_dict_t *dict, hb_dict_iter_t iter);
int               hb_dict_iter_next_ex(const hb_dict_t *dict,
                                       hb_dict_iter_t *iter,
                                       const char **key, hb_value_t **val);
/* get key from iter */
const char *      hb_dict_iter_key(const hb_dict_iter_t iter);
/* get value from iter.  value has borrowed reference */
hb_value_t *      hb_dict_iter_value(const hb_dict_iter_t iter);

/* hb_value_array_t */
hb_value_array_t * hb_value_array_init(void);
/* remove all elements of array */
void               hb_value_array_clear(hb_value_array_t *array);
/* get value from array.  value has borrowed reference */
hb_value_t *       hb_value_array_get(const hb_value_array_t *array, int index);
/* replace value at index in array.  array takes ownership of new value */
void               hb_value_array_set(hb_value_array_t *array, int index,
                                      hb_value_t *value);
/* insert value at index in array.  values move up.
 * array takes ownership of new value */
void               hb_value_array_insert(hb_value_array_t *array, int index,
                                         hb_value_t *value);
/* append value to array.  array takes ownership of new value */
void               hb_value_array_append(hb_value_array_t *array,
                                         hb_value_t *value);
/* remove value from array.  releases reference to value */
void               hb_value_array_remove(hb_value_array_t *array, int index);
/* clears dst and performs a deep copy */
void               hb_value_array_copy(hb_value_array_t *dst,
                                       const hb_value_array_t *src, int count);
/* appends copy of value to array.  if value is an array, appends a copy of
 * each element to array */
void               hb_value_array_concat(hb_value_array_t *array,
                                         hb_value_t *value);
size_t             hb_value_array_len(const hb_value_array_t *array);

/* hb_value_t */
int          hb_value_type(const hb_value_t *value);
int          hb_value_is_number(const hb_value_t *value);
hb_value_t * hb_value_dup(const hb_value_t *value);
hb_value_t * hb_value_incref(hb_value_t *value);
void         hb_value_decref(hb_value_t *value);
void         hb_value_free(hb_value_t **value);

/* Create new hb_value_t */
hb_value_t * hb_value_null(void);
hb_value_t * hb_value_string(const char *value);
hb_value_t * hb_value_int(json_int_t value);
hb_value_t * hb_value_double(double value);
hb_value_t * hb_value_bool(int value);
hb_value_t * hb_value_json(const char *json);
hb_value_t * hb_value_read_json(const char *path);

/* Transform hb_value_t from one type to another */
hb_value_t * hb_value_xform(const hb_value_t *value, int type);

/* Extract values */
/* hb_value_t must be of type HB_VALUE_TYPE_STRING */
const char * hb_value_get_string(const hb_value_t *value);
/* hb_value_t may be of any type, automatic conversion performed */
json_int_t   hb_value_get_int(const hb_value_t *value);
double       hb_value_get_double(const hb_value_t *value);
int          hb_value_get_bool(const hb_value_t *value);
/* converts value type and returns an allocated string representation.
 * caller must free the returned string */
char *       hb_value_get_string_xform(const hb_value_t *value);
/* converts value to json string */
char *       hb_value_get_json(const hb_value_t *value);
/* write json representation to a file */
int          hb_value_write_file_json(hb_value_t *value, FILE *file);
int          hb_value_write_json(hb_value_t *value, const char *path);

/* specialized dict functions */
/*
 * hb_encopts_to_dict() converts an op1=val1:opt2=val2:opt3=val3 type string to
 * an hb_dict_t dictionary. */
hb_dict_t * hb_encopts_to_dict(const char * encopts, int encoder);
char      * hb_dict_to_encopts(const hb_dict_t * dict);

/* convenience macros */
#define hb_dict_get_string(dict, key) hb_value_get_string(hb_dict_get(dict, key))
#define hb_dict_get_int(dict, key) hb_value_get_int(hb_dict_get(dict, key))
#define hb_dict_get_double(dict, key) hb_value_get_double(hb_dict_get(dict, key))
#define hb_dict_get_bool(dict, key) hb_value_get_bool(hb_dict_get(dict, key))

#define hb_dict_set_string(dict, key, val) hb_dict_set(dict, key, hb_value_string(val))
#define hb_dict_set_int(dict, key, val) hb_dict_set(dict, key, hb_value_int(val))
#define hb_dict_set_double(dict, key, val) hb_dict_set(dict, key, hb_value_double(val))
#define hb_dict_set_bool(dict, key, val) hb_dict_set(dict, key, hb_value_bool(val))


#endif // !defined(HANDBRAKE_DICT_H)
