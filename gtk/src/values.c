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

