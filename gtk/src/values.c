/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * values.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * values.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * values.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "compat.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "values.h"

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
ghb_int_value(gint64 ival)
{
    static GhbValue *gval = NULL;
    if (gval == NULL)
        gval = json_integer(ival);
    else
        json_integer_set(gval, ival);
    return gval;
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

void
ghb_string_value_set(GhbValue *gval, const gchar *str)
{
    json_string_set(gval, str);
}

void
ghb_dict_set_string(GhbValue *dict, const gchar *key, const gchar *sval)
{
    GhbValue *value;
    value = ghb_string_value_new(sval);
    ghb_dict_set(dict, key, value);
}

void
ghb_dict_set_double(GhbValue *dict, const gchar *key, gdouble dval)
{
    GhbValue *value;
    value = ghb_double_value_new(dval);
    ghb_dict_set(dict, key, value);
}

void
ghb_dict_set_int(GhbValue *dict, const gchar *key, gint64 ival)
{
    GhbValue *value;
    value = ghb_int_value_new(ival);
    ghb_dict_set(dict, key, value);
}

void
ghb_dict_set_bool(GhbValue *dict, const gchar *key, gboolean bval)
{
    GhbValue *value;
    value = ghb_bool_value_new(bval);
    ghb_dict_set(dict, key, value);
}

GhbValue*
ghb_dict_get_value(const GhbValue *dict, const gchar *key)
{
    GhbValue *value;
    value = ghb_dict_get(dict, key);
    if (value == NULL)
        g_debug("returning null (%s)", key);
    return value;
}

gboolean
ghb_dict_get_bool(const GhbValue *dict, const gchar *key)
{
    const GhbValue* value;
    value = ghb_dict_get_value(dict, key);
    if (value == NULL) return FALSE;
    return ghb_value_get_bool(value);
}

gint64
ghb_dict_get_int(const GhbValue *dict, const gchar *key)
{
    const GhbValue* value;
    value = ghb_dict_get_value(dict, key);
    if (value == NULL) return 0;
    return ghb_value_get_int(value);
}

gdouble
ghb_dict_get_double(const GhbValue *dict, const gchar *key)
{
    const GhbValue* value;
    value = ghb_dict_get_value(dict, key);
    if (value == NULL) return 0;
    return ghb_value_get_double(value);
}

const gchar*
ghb_dict_get_string(const GhbValue *dict, const gchar *key)
{
    const GhbValue* value;
    value = ghb_dict_get_value(dict, key);
    return ghb_value_get_string(value);
}

gchar*
ghb_dict_get_string_xform(const GhbValue *dict, const gchar *key)
{
    const GhbValue* value;
    value = ghb_dict_get_value(dict, key);
    if (value == NULL) return g_strdup("");
    return ghb_value_get_string_xform(value);
}

void
ghb_dict_copy(GhbValue *dst, const GhbValue *src)
{
    GhbDictIter iter;
    const char *key;
    GhbValue *val, *dst_val;

    iter = ghb_dict_iter_init(src);
    while (ghb_dict_iter_next(src, &iter, &key, &val))
    {
        dst_val = ghb_dict_get(dst, key);
        if (ghb_value_type(val) == GHB_DICT)
        {
            if (dst_val == NULL || ghb_value_type(dst_val) != GHB_DICT)
            {
                dst_val = ghb_value_dup(val);
                ghb_dict_set(dst, key, dst_val);
            }
            else if (ghb_value_type(dst_val) == GHB_DICT)
            {
                ghb_dict_copy(dst_val, val);
            }
        }
        else
        {
            ghb_dict_set(dst, key, ghb_value_dup(val));
        }
    }
}
