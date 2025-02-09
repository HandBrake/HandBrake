/* hb_dict.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <ctype.h>
#include <stdio.h>
#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"

hb_value_type_t hb_value_type(const hb_value_t *value)
{
   if (value == NULL)
        return HB_VALUE_TYPE_NULL;
    hb_value_type_t type = json_typeof(value);
    if (type == JSON_TRUE || type == JSON_FALSE)
        return HB_VALUE_TYPE_BOOL;
    return type;
}

int hb_value_is_number(const hb_value_t *value)
{
    return json_is_number(value);
}

hb_value_t * hb_value_dup(const hb_value_t *value)
{
    if (value == NULL) return NULL;
    return json_deep_copy(value);
}

hb_value_t* hb_value_incref(hb_value_t *value)
{
    return json_incref(value);
}

void hb_value_decref(hb_value_t *value)
{
    if (value == NULL) return;
    json_decref(value);
}

void hb_value_free(hb_value_t **_value)
{
    hb_value_decref(*_value);
    *_value = NULL;
}

hb_value_t * hb_value_null()
{
    return json_null();
}

hb_value_t * hb_value_string(const char * value)
{
    // json_string does not create a value for NULL strings.
    // So create JSON_NULL in this case
    if (value == NULL)
        return json_null();
    return json_string(value);
}

hb_value_t * hb_value_int(json_int_t value)
{
    return json_integer(value);
}

hb_value_t * hb_value_double(double value)
{
    return json_real(value);
}

hb_value_t * hb_value_bool(int value)
{
    return json_boolean(value);
}

hb_value_t * hb_value_json(const char *json)
{
    json_error_t error;
    hb_value_t *val = json_loads(json, 0, &error);
    if (val == NULL)
    {
        hb_error("hb_value_json: Failed, error %s", error.text);
    }
    return val;
}

hb_value_t * hb_value_read_json(const char *path)
{
    FILE * fp;
    json_error_t error;

    fp = hb_fopen(path, "r");
    if (fp == NULL)
    {
        return NULL;
    }
    hb_value_t *val = json_loadf(fp, 0, &error);
    fclose(fp);
    return val;
}

static hb_value_t* xform_null(hb_value_type_t type)
{
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
            return json_null();
        case HB_VALUE_TYPE_BOOL:
            return json_false();
        case HB_VALUE_TYPE_INT:
            return json_integer(0);
        case HB_VALUE_TYPE_DOUBLE:
            return json_real(0.0);
        case HB_VALUE_TYPE_STRING:
            return json_string("");
    }
}

static hb_value_t* xform_bool(const hb_value_t *value, hb_value_type_t type)
{
    json_int_t b = json_is_true(value);
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
            return json_null();
        case HB_VALUE_TYPE_BOOL:
            return json_boolean(b);
        case HB_VALUE_TYPE_INT:
            return json_integer(b);
        case HB_VALUE_TYPE_DOUBLE:
            return json_real(b);
        case HB_VALUE_TYPE_STRING:
        {
            char *s = hb_strdup_printf("%"JSON_INTEGER_FORMAT, b);
            hb_value_t *v = json_string(s);
            free(s);
            return v;
        }
    }
}

static hb_value_t* xform_int(const hb_value_t *value, hb_value_type_t type)
{
    json_int_t i = json_integer_value(value);
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
            return json_null();
        case HB_VALUE_TYPE_BOOL:
            return json_boolean(i);
        case HB_VALUE_TYPE_INT:
            return json_integer(i);
        case HB_VALUE_TYPE_DOUBLE:
            return json_real(i);
        case HB_VALUE_TYPE_STRING:
        {
            char *s = hb_strdup_printf("%"JSON_INTEGER_FORMAT, i);
            hb_value_t *v = json_string(s);
            free(s);
            return v;
        }
    }
}

static hb_value_t* xform_double(const hb_value_t *value, hb_value_type_t type)
{
    double d = json_real_value(value);
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
            return json_null();
        case HB_VALUE_TYPE_BOOL:
            return json_boolean((int)d != 0);
        case HB_VALUE_TYPE_INT:
            return json_integer(d);
        case HB_VALUE_TYPE_DOUBLE:
            return json_real(d);
        case HB_VALUE_TYPE_STRING:
        {
            char *s = hb_strdup_printf("%g", d);
            hb_value_t *v = json_string(s);
            free(s);
            return v;
        }
    }
}

static hb_value_t* xform_string(const hb_value_t *value, hb_value_type_t type)
{
    const char *s = json_string_value(value);
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
        {
            return json_null();
        }
        case HB_VALUE_TYPE_BOOL:
        {
            if (!strcasecmp(s, "true") ||
                !strcasecmp(s, "yes")  ||
                !strcasecmp(s, "1"))
            {
                return json_true();
            }
            return json_false();
        }
        case HB_VALUE_TYPE_INT:
        {
            return json_integer(strtoll(s, NULL, 0));
        }
        case HB_VALUE_TYPE_DOUBLE:
        {
            return json_real(strtod(s, NULL));
        }
        case HB_VALUE_TYPE_STRING:
        {
            return json_string(s);
        }
    }
}

static hb_value_t* xform_array(const hb_value_t *value, hb_value_type_t type)
{
    hb_value_t *first = NULL;
    int count = hb_value_array_len(value);

    if (count > 0)
        first = hb_value_array_get(value, 0);
    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
        case HB_VALUE_TYPE_BOOL:
        case HB_VALUE_TYPE_INT:
        case HB_VALUE_TYPE_DOUBLE:
            return hb_value_xform(first, type);
        case HB_VALUE_TYPE_STRING:
        {
            char *r = strdup("");
            int ii;
            for (ii = 0; ii < count; ii++)
            {
                hb_value_t *v = hb_value_array_get(value, ii);
                hb_value_t *x = hb_value_xform(v, type);
                const char *s = hb_value_get_string(x);
                if (s != NULL)
                {
                    char *tmp = r;
                    r = hb_strdup_printf("%s%s,", tmp, s);
                    free(tmp);
                }
                hb_value_free(&x);
            }
            int len = strlen(r);
            hb_value_t *v;
            if (len > 0)
            {
                // Removing trailing ','
                r[len - 1] = 0;
                v = json_string(r);
            }
            else
            {
                free(r);
                r = NULL;
                v = json_null();
            }
            free(r);
            return v;
        }
    }
}

static hb_value_t* xform_dict(const hb_value_t *dict, hb_value_type_t type)
{
    hb_value_t *first = NULL;
    hb_dict_iter_t iter = hb_dict_iter_init(dict);

    if (iter != HB_DICT_ITER_DONE)
        first = hb_dict_iter_value(iter);

    switch (type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
        case HB_VALUE_TYPE_BOOL:
        case HB_VALUE_TYPE_INT:
        case HB_VALUE_TYPE_DOUBLE:
            return hb_value_xform(first, type);
        case HB_VALUE_TYPE_STRING:
        {
            char *r = strdup("");
            hb_dict_iter_t iter;
            for (iter  = hb_dict_iter_init(dict);
                 iter != HB_DICT_ITER_DONE;
                 iter  = hb_dict_iter_next(dict, iter))
            {
                const char *k = hb_dict_iter_key(iter);
                hb_value_t *v = hb_dict_iter_value(iter);
                hb_value_t *x = hb_value_xform(v, type);
                const char *s = hb_value_get_string(x);

                char *tmp = r;
                r = hb_strdup_printf("%s%s%s%s:",
                                     r,
                                     k,
                                     s  ? "=" : "",
                                     s  ? s   : "");
                free(tmp);
                hb_value_free(&x);
            }
            int len = strlen(r);
            hb_value_t *v;
            if (len > 0)
            {
                // Removing trailing ':'
                r[len - 1] = 0;
                v = json_string(r);
            }
            else
            {
                free(r);
                r = NULL;
                v = json_null();
            }
            free(r);
            return v;
        }
    }
}

hb_value_t* hb_value_xform(const hb_value_t *value, hb_value_type_t type)
{
    hb_value_type_t src_type = hb_value_type(value);
    if (src_type == type && value != NULL)
    {
        json_incref((hb_value_t*)value);
        return (hb_value_t*)value;
    }
    switch (src_type)
    {
        default:
        case HB_VALUE_TYPE_NULL:
        {
            return xform_null(type);
        }
        case HB_VALUE_TYPE_BOOL:
        {
            return xform_bool(value, type);
        }
        case HB_VALUE_TYPE_INT:
        {
            return xform_int(value, type);
        }
        case HB_VALUE_TYPE_DOUBLE:
        {
            return xform_double(value, type);
        }
        case HB_VALUE_TYPE_STRING:
        {
            return xform_string(value, type);
        }
        case HB_VALUE_TYPE_ARRAY:
        {
            return xform_array(value, type);
        }
        case HB_VALUE_TYPE_DICT:
        {
            return xform_dict(value, type);
        }
    }
}

const char * hb_value_get_string(const hb_value_t *value)
{
    if (hb_value_type(value) != HB_VALUE_TYPE_STRING) return NULL;
    return json_string_value(value);
}

json_int_t hb_value_get_int(const hb_value_t *value)
{
    json_int_t result;
    hb_value_t *v = hb_value_xform(value, HB_VALUE_TYPE_INT);
    result = json_integer_value(v);
    json_decref(v);
    return result;
}

double hb_value_get_double(const hb_value_t *value)
{
    double result;
    hb_value_t *v = hb_value_xform(value, HB_VALUE_TYPE_DOUBLE);
    result = json_real_value(v);
    json_decref(v);
    return result;
}

int hb_value_get_bool(const hb_value_t *value)
{
    int result;
    hb_value_t *v = hb_value_xform(value, HB_VALUE_TYPE_BOOL);
    result = json_is_true(v);
    json_decref(v);
    return result;
}

char*
hb_value_get_string_xform(const hb_value_t *value)
{
    char *result;
    if (hb_value_type(value) == HB_VALUE_TYPE_NULL)
        return NULL;
    hb_value_t *v = hb_value_xform(value, HB_VALUE_TYPE_STRING);
    if (hb_value_type(v) == HB_VALUE_TYPE_NULL)
        return NULL;
    result = strdup(json_string_value(v));
    json_decref(v);
    return result;
}

char * hb_value_get_json(const hb_value_t *value)
{
    return json_dumps(value, JSON_INDENT(4) | JSON_SORT_KEYS);
}

int  hb_value_write_file_json(hb_value_t *value, FILE *file)
{
    return json_dumpf(value, file, JSON_INDENT(4) | JSON_SORT_KEYS);
}

int  hb_value_write_json(hb_value_t *value, const char *path)
{
    return json_dump_file(value, path, JSON_INDENT(4) | JSON_SORT_KEYS);
}

void hb_dict_free(hb_dict_t **_dict)
{
    hb_value_free(_dict);
}

hb_dict_t * hb_dict_init()
{
    return json_object();
}

void
hb_dict_clear(hb_dict_t *dict)
{
    json_object_clear(dict);
}

int hb_dict_elements(hb_dict_t * dict)
{
    return json_object_size(dict);
}

static char * makelower(const char *key)
{
    int    ii, len = strlen(key);
    char * lower = malloc(len + 1);

    for (ii = 0; ii < len; ii++)
    {
        lower[ii] = tolower(key[ii]);
    }
    lower[ii] = '\0';
    return lower;
}

void hb_dict_set(hb_dict_t * dict, const char *key, hb_value_t *value)
{
    json_object_set_new(dict, key, value);
}

void hb_dict_merge(hb_dict_t * dict, hb_dict_t *value)
{
    json_object_update(dict, value);
}

void hb_dict_case_set(hb_dict_t * dict, const char *key, hb_value_t *value)
{
    char * lower = makelower(key);
    json_object_set_new(dict, lower, value);
    free(lower);
}

int hb_dict_remove(hb_dict_t * dict, const char * key)
{
    int    result;

    // First try case sensitive lookup
    result = json_object_del(dict, key) == 0;
    if (!result)
    {
        // If not found, try case insensitive lookup
        char * lower = makelower(key);
        result = json_object_del(dict, lower) == 0;
        free(lower);
    }
    return result;
}

hb_value_t * hb_dict_get(const hb_dict_t * dict, const char * key)
{
    hb_value_t * result;

    // First try case sensitive lookup
    result = json_object_get(dict, key);
    if (result == NULL)
    {
        // If not found, try case insensitive lookup
        char * lower = makelower(key);
        result = json_object_get(dict, lower);
        free(lower);
    }
    return result;
}

//  Dictionary extraction helpers
//
// Extract the given key from the dict and assign to dst *only*
// if key is found in dict.  Values are converted to the requested
// data type.
//
// return: 1 - key is in dict
//         0 - key is not in dict
int hb_dict_extract_int(int *dst, const hb_dict_t * dict, const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (val == NULL)
    {
        return 0;
    }

    *dst = hb_value_get_int(val);
    return 1;
}

int hb_dict_extract_double(double *dst, const hb_dict_t * dict,
                                        const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (val == NULL)
    {
        return 0;
    }

    *dst = hb_value_get_double(val);
    return 1;
}

int hb_dict_extract_bool(int *dst, const hb_dict_t * dict, const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (val == NULL)
    {
        return 0;
    }

    *dst = hb_value_get_bool(val);
    return 1;
}

int hb_dict_extract_string(char **dst, const hb_dict_t * dict, const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (val == NULL)
    {
        return 0;
    }

    *dst = hb_value_get_string_xform(val);
    return 1;
}

int hb_dict_extract_rational(hb_rational_t *dst, const hb_dict_t * dict,
                                                 const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (val == NULL)
    {
        return 0;
    }

    if (hb_value_type(val) == HB_VALUE_TYPE_DICT)
    {
        hb_value_t * num_val = hb_dict_get(val, "Num");
        if (num_val == NULL)
        {
            return 0;
        }
        hb_value_t * den_val = hb_dict_get(val, "Den");
        if (den_val == NULL)
        {
            return 0;
        }

        dst->num = hb_value_get_int(num_val);
        dst->den = hb_value_get_int(den_val);
        return 1;
    }
    else if (hb_value_type(val) == HB_VALUE_TYPE_STRING)
    {
        const char * str = hb_value_get_string(val);
        char ** rational = hb_str_vsplit(str, '/');
        if (rational[0] != NULL && rational[1] != NULL &&
            isdigit(rational[0][0]) && isdigit(rational[1][0]))
        {
            char *num_end, *den_end;

            // found rational format value
            int num = strtol(rational[0], &num_end, 0);
            int den = strtol(rational[1], &den_end, 0);
            // confirm that the 2 components were entirely numbers
            if (num_end[0] == 0 && den_end[0] == 0)
            {
                dst->num = num;
                dst->den = den;
                hb_str_vfree(rational);
                return 1;
            }
        }
        hb_str_vfree(rational);
    }

    return 0;
}

int hb_dict_extract_int_array(int *dst, int count,
                              const hb_dict_t * dict, const char * key)
{
    if (dict == NULL || key == NULL || dst == NULL)
    {
        return 0;
    }

    hb_value_t *val = hb_dict_get(dict, key);
    if (hb_value_type(val) != HB_VALUE_TYPE_ARRAY)
    {
        return 0;
    }

    int len = hb_value_array_len(val);
    count = count < len ? count : len;

    int ii;
    for (ii = 0; ii < count; ii++)
    {
        dst[ii] = hb_value_get_int(hb_value_array_get(val, ii));
    }
    return 1;
}

hb_dict_iter_t hb_dict_iter_init(const hb_dict_t *dict)
{
    if (dict == NULL)
        return HB_DICT_ITER_DONE;
    return json_object_iter((hb_dict_t*)dict);
}

hb_dict_iter_t hb_dict_iter_next(const hb_dict_t *dict, hb_dict_iter_t iter)
{
    return json_object_iter_next((hb_dict_t*)dict, iter);
}

const char * hb_dict_iter_key(const hb_dict_iter_t iter)
{
    return json_object_iter_key(iter);
}

hb_value_t * hb_dict_iter_value(const hb_dict_iter_t iter)
{
    return json_object_iter_value(iter);
}

int
hb_dict_iter_next_ex(const hb_dict_t *dict, hb_dict_iter_t *iter,
                     const char **key, hb_value_t **val)
{
    if (*iter == NULL)
        return 0;
    if (key != NULL)
        *key = json_object_iter_key(*iter);
    if (val != NULL)
        *val = json_object_iter_value(*iter);
    *iter = json_object_iter_next((hb_dict_t*)dict, *iter);
    return 1;
}

hb_value_array_t*
hb_value_array_init()
{
    return json_array();
}

void
hb_value_array_clear(hb_value_array_t *array)
{
    json_array_clear(array);
}

hb_value_t*
hb_value_array_get(const hb_value_array_t *array, int index)
{
    return json_array_get(array, index);
}

void
hb_value_array_set(hb_value_array_t *array, int index, hb_value_t *value)
{
    if (index < 0 || index >= json_array_size(array))
    {
        hb_error("hb_value_array_set: invalid index %d size %zu",
                 index, json_array_size(array));
        return;
    }
    json_array_set_new(array, index, value);
}

void
hb_value_array_insert(hb_value_array_t *array, int index, hb_value_t *value)
{
    json_array_insert_new(array, index, value);
}

void
hb_value_array_append(hb_value_array_t *array, hb_value_t *value)
{
    json_array_append_new(array, value);
}

void
hb_value_array_concat(hb_value_array_t *array, hb_value_t *value)
{
    if (hb_value_type(value) == HB_VALUE_TYPE_ARRAY)
    {
        int ii;
        int len = hb_value_array_len(value);

        for (ii = 0; ii < len; ii++)
        {
            hb_value_t * val = hb_value_array_get(value, ii);
            json_array_append_new(array, hb_value_dup(val));
        }
    }
    else
    {
        json_array_append_new(array, hb_value_dup(value));
    }
}

void
hb_value_array_remove(hb_value_array_t *array, int index)
{
    json_array_remove(array, index);
}

void
hb_value_array_copy(hb_value_array_t *dst,
                    const hb_value_array_t *src, int count)
{
    size_t len;
    int ii;

    // empty the first array if it is not already empty
    json_array_clear(dst);

    len = hb_value_array_len(src);
    count = MIN(count, len);
    for (ii = 0; ii < count; ii++)
        hb_value_array_append(dst, hb_value_dup(hb_value_array_get(src, ii)));
}

size_t
hb_value_array_len(const hb_value_array_t *array)
{
    return json_array_size(array);
}

hb_dict_t * hb_encopts_to_dict(const char * encopts, int encoder)
{
    hb_dict_t * dict = NULL;

    if (encopts && *encopts)
    {
        char *cur_opt, *opts_start, *value;
        const char *name;
        dict = hb_dict_init();
        if( !dict )
            return NULL;
        cur_opt = opts_start = strdup(encopts);
        if (opts_start)
        {
            while (*cur_opt)
            {
                name = cur_opt;
                cur_opt += strcspn(cur_opt, ":");
                if (*cur_opt)
                {
                    *cur_opt = 0;
                    cur_opt++;
                }
                value = strchr(name, '=');
                if (value)
                {
                    *value = 0;
                    value++;
                }
                // x264 has multiple names for some options
                if (encoder & HB_VCODEC_X264_MASK)
                    name = hb_x264_encopt_name(name);
#if HB_PROJECT_FEATURE_X265
                // x265 has multiple names for some options
                if (encoder & HB_VCODEC_X265_MASK)
                    name = hb_x265_encopt_name(name);
#endif
                if (name != NULL)
                {
                    hb_dict_set(dict, name, hb_value_string(value));
                }
            }
        }
        free(opts_start);
    }
    return dict;
}

char * hb_dict_to_encopts(const hb_dict_t * dict)
{
    return hb_value_get_string_xform(dict);
}

