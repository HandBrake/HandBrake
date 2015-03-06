/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * presets.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * presets.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "values.h"

GhbType
ghb_value_type(const GhbValue *val)
{
    if (val == NULL)
        return GHB_NULL;
    GhbType type = json_typeof(val);
    if (type == JSON_TRUE || type == JSON_FALSE)
        return GHB_BOOL;
    return type;
}

GhbValue*
ghb_value_new(GhbType type)
{
    GhbValue *val = NULL;
    switch (type)
    {
        case GHB_DICT:
            val = json_object();
            break;
        case GHB_ARRAY:
            val = json_array();
            break;
        case GHB_STRING:
            val = json_string("");
            break;
        case GHB_INT:
            val = json_integer(0);
            break;
        case GHB_DOUBLE:
            val = json_real(0.0);
            break;
        case GHB_BOOL:
            val = json_false();
            break;
        default:
            g_warning("Unrecognized GHB value type %d", type);
            break;
    }
    return val;
}

void
ghb_value_incref(GhbValue *gval)
{
    if (gval == NULL) return;
    json_incref(gval);
}

void
ghb_value_decref(GhbValue *gval)
{
    if (gval == NULL) return;
    json_decref(gval);
}

void
ghb_value_free(GhbValue *gval)
{
    if (gval == NULL) return;
    json_decref(gval);
}

GhbValue*
ghb_value_dup(const GhbValue *gval)
{
    if (gval == NULL) return NULL;
    return json_deep_copy(gval);
}

void
debug_show_type(GhbType tp)
{
    const gchar *str = "unknown";
    if (tp == GHB_STRING)
    {
        str ="string";
    }
    else if (tp == GHB_INT)
    {
        str ="int";
    }
    else if (tp == GHB_DOUBLE)
    {
        str ="double";
    }
    else if (tp == GHB_BOOL)
    {
        str ="bool";
    }
    else if (tp == GHB_ARRAY)
    {
        str ="array";
    }
    else if (tp == GHB_DICT)
    {
        str ="dict";
    }
    g_debug("Type %s", str);
}

void
debug_show_value(GhbValue *gval)
{
    GhbType tp;

    tp = ghb_value_type(gval);
    if (tp == GHB_STRING)
    {
        g_message("Type %s value %s", "string", json_string_value(gval));
    }
    else if (tp == GHB_INT)
    {
        g_message("Type %s value %"JSON_INTEGER_FORMAT, "int",
                  json_integer_value(gval));
    }
    else if (tp == GHB_DOUBLE)
    {
        g_message("Type %s value %f", "double", json_real_value(gval));
    }
    else if (tp == GHB_BOOL)
    {
        g_message("Type %s value %d", "boolean", json_is_true(gval));
    }
    else if (tp == GHB_ARRAY)
    {
        g_message("Type %s", "array");
    }
    else if (tp == GHB_DICT)
    {
        g_message("Type %s", "dict");
    }
}

static GhbValue* xform_int(json_int_t i, GhbType type)
{
    switch (type)
    {
        default:
        case GHB_NULL:
            return json_null();
        case GHB_BOOL:
            return json_boolean(i);
        case GHB_INT:
            return json_integer(i);
        case GHB_DOUBLE:
            return json_real(i);
        case GHB_STRING:
        {
            char *s = g_strdup_printf("%"JSON_INTEGER_FORMAT, i);
            GhbValue *v = json_string(s);
            g_free(s);
            return v;
        }
    }
}

static GhbValue* xform_double(double d, GhbType type)
{
    switch (type)
    {
        default:
        case GHB_NULL:
            return json_null();
        case GHB_BOOL:
            return json_boolean((int)d != 0);
        case GHB_INT:
            return json_integer(d);
        case GHB_DOUBLE:
            return json_real(d);
        case GHB_STRING:
        {
            char *s = g_strdup_printf("%g", d);
            GhbValue *v = json_string(s);
            g_free(s);
            return v;
        }
    }
}

static GhbValue* xform_string(const char * s, GhbType type)
{
    switch (type)
    {
        default:
        case GHB_NULL:
        {
            return json_null();
        }
        case GHB_BOOL:
        {
            if (!strcasecmp(s, "true") ||
                !strcasecmp(s, "yes")  ||
                !strcasecmp(s, "1"))
            {
                return json_true();
            }
            return json_false();
        }
        case GHB_INT:
        {
            double d = g_strtod(s, NULL);
            return json_integer(d);
        }
        case GHB_DOUBLE:
        {
            double d = g_strtod(s, NULL);
            return json_real(d);
        }
        case GHB_STRING:
        {
            return json_string(s);
        }
    }
}

static GhbValue* xform_null(GhbType type)
{
    switch (type)
    {
        default:
        case GHB_NULL:
            return json_null();
        case GHB_BOOL:
            return json_false();
        case GHB_INT:
            return json_integer(0);
        case GHB_DOUBLE:
            return json_real(0.0);
        case GHB_STRING:
            return json_string("");
    }
}

GhbValue* ghb_value_xform(const GhbValue *val, GhbType type)
{
    GhbType src_type = ghb_value_type(val);
    if (src_type == type && val != NULL)
    {
        json_incref((GhbValue*)val);
        return (GhbValue*)val;
    }
    switch (src_type)
    {
        default:
        case GHB_NULL:
        {
            return xform_null(type);
        }
        case GHB_BOOL:
        {
            json_int_t b = json_is_true(val);
            return xform_int(b, type);
        }
        case GHB_INT:
        {
            json_int_t i = json_integer_value(val);
            return xform_int(i, type);
        }
        case GHB_DOUBLE:
        {
            double d = json_real_value(val);
            return xform_double(d, type);
        }
        case GHB_STRING:
        {
            const char *s = json_string_value(val);
            return xform_string(s, type);
        }
    }
}

gint
ghb_value_int(const GhbValue *val)
{
    gint result;
    GhbValue *v = ghb_value_xform(val, GHB_INT);
    result = json_integer_value(v);
    json_decref(v);
    return result;
}

gint64
ghb_value_int64(const GhbValue *val)
{
    gint64 result;
    GhbValue *v = ghb_value_xform(val, GHB_INT);
    result = json_integer_value(v);
    json_decref(v);
    return result;
}

gdouble
ghb_value_double(const GhbValue *val)
{
    gdouble result;
    GhbValue *v = ghb_value_xform(val, GHB_DOUBLE);
    result = json_real_value(v);
    json_decref(v);
    return result;
}

gchar*
ghb_value_string(const GhbValue *val)
{
    gchar *result;
    GhbValue *v = ghb_value_xform(val, GHB_STRING);
    result = g_strdup(json_string_value(v));
    json_decref(v);
    return result;
}

const gchar*
ghb_value_const_string(const GhbValue *val)
{
    if (ghb_value_type(val) != GHB_STRING)
        return NULL;
    return json_string_value(val);
}

gboolean
ghb_value_boolean(const GhbValue *val)
{
    gboolean result;
    GhbValue *v = ghb_value_xform(val, GHB_BOOL);
    result = json_is_true(v);
    json_decref(v);
    return result;
}

gint
ghb_value_cmp(const GhbValue *vala, const GhbValue *valb)
{
    return !json_equal((GhbValue*)vala, (GhbValue*)valb);
}

GhbValue*
ghb_string_value(const gchar *str)
{
    static GhbValue *gval = NULL;
    if (gval == NULL)
        gval = json_string(str);
    else
        json_string_set(gval, str);
    return gval;
}

GhbValue*
ghb_int64_value(gint64 ival)
{
    static GhbValue *gval = NULL;
    if (gval == NULL)
        gval = json_integer(ival);
    else
        json_integer_set(gval, ival);
    return gval;
}

GhbValue*
ghb_int_value(gint ival)
{
    return ghb_int64_value(ival);
}

GhbValue*
ghb_double_value(gdouble dval)
{
    static GhbValue *gval = NULL;
    if (gval == NULL)
        gval = json_real(dval);
    else
        json_real_set(gval, dval);
    return gval;
}

GhbValue*
ghb_boolean_value(gboolean bval)
{
    // Jansson boolean is singleton, no need for local static
    GhbValue *gval = json_boolean(bval);
    json_decref(gval);
    return gval;
}

GhbValue*
ghb_string_value_new(const gchar *str)
{
    return json_string(str);
}

void
ghb_string_value_set(GhbValue *gval, const gchar *str)
{
    json_string_set(gval, str);
}

GhbValue*
ghb_int64_value_new(gint64 ival)
{
    return json_integer(ival);
}

GhbValue*
ghb_int_value_new(gint ival)
{
    return json_integer(ival);
}

GhbValue*
ghb_double_value_new(gdouble dval)
{
    return json_real(dval);
}

GhbValue*
ghb_boolean_value_new(gboolean bval)
{
    return json_boolean(bval);
}

GhbValue*
ghb_dict_value_new()
{
    return json_object();
}

GhbValue*
ghb_array_value_new()
{
    return json_array();
}

void
ghb_array_value_reset(GhbValue *array)
{
    json_array_clear(array);
}

void
ghb_dict_insert(GhbValue *dict, const gchar *key, GhbValue *val)
{
    json_object_set_new(dict, key, val);
}

void
ghb_dict_iter_init(GhbValue *dict, GhbDictIter *iter)
{
    *iter = json_object_iter(dict);
}

int
ghb_dict_iter_next(GhbValue *dict, GhbDictIter *iter,
                   const char **key, GhbValue **val)
{
    if (*iter == NULL)
        return 0;
    if (key != NULL)
        *key = json_object_iter_key(*iter);
    if (val != NULL)
        *val = json_object_iter_value(*iter);
    *iter = json_object_iter_next(dict, *iter);
    return 1;
}

GhbValue*
ghb_dict_lookup(const GhbValue *dict, const gchar *key)
{
    return json_object_get(dict, key);
}

gboolean
ghb_dict_remove(GhbValue *dict, const gchar *key)
{
    return json_object_del(dict, key) == 0;
}

GhbValue*
ghb_array_get_nth(const GhbValue *array, gint ii)
{
    return json_array_get(array, ii);
}

void
ghb_array_insert(GhbValue *array, guint ii, GhbValue *val)
{
    json_array_insert_new(array, ii, val);
}

void
ghb_array_append(GhbValue *array, GhbValue *val)
{
    json_array_append_new(array, val);
}

void
ghb_array_remove(GhbValue *array, guint ii)
{
    json_array_remove(array, ii);
}

void
ghb_array_replace(GhbValue *array, guint ii, GhbValue *val)
{
    if (ii < 0 || ii >= json_array_size(array))
    {
        g_warning("ghb_array_replace: invalid index");
        return;
    }
    json_array_set_new(array, ii, val);
}

void
ghb_array_copy(GhbValue *arr1, GhbValue *arr2, gint count)
{
    gint len, ii;

    // empty the first array if it is not already empty
    json_array_clear(arr1);

    len = ghb_array_len(arr2);
    count = MIN(count, len);
    for (ii = 0; ii < count; ii++)
        ghb_array_append(arr1, ghb_value_dup(ghb_array_get_nth(arr2, ii)));
}

gint
ghb_array_len(const GhbValue *array)
{
    return json_array_size(array);
}

void
ghb_json_write(FILE *file, GhbValue *gval)
{
    char * json = json_dumps(gval, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    fprintf(file, "%s", json);
    free(json);
}

void
ghb_json_write_file(const char *path, GhbValue *gval)
{
    FILE *file = g_fopen(path, "w");
    if (file == NULL)
        return;
    ghb_json_write(file, gval);
    fclose(file);
}

GhbValue*
ghb_json_parse(const char *json, size_t size)
{
    return json_loadb(json, size, JSON_REJECT_DUPLICATES, NULL);
}

GhbValue*
ghb_json_parse_file(const char *path)
{
    return json_load_file(path, JSON_REJECT_DUPLICATES, NULL);
}

