/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * resources.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * resources.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * resources.c is distributed in the hope that it will be useful,
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

#include "ghbcompat.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>

#include "settings.h"
#include "resources.h"
#include "values.h"
#include "data_res.h"

static GhbValue *resources;

void
ghb_resource_init (void)
{
    GhbValue *val;
    gsize data_size;
    GBytes *gbytes;
    gconstpointer data;

    resources = ghb_dict_new();

    GResource *data_res = ghb_data_get_resource();

    gbytes = g_resource_lookup_data(data_res,
                    "/fr/handbrake/ghb/data/internal_defaults.json", 0, NULL);
    data = g_bytes_get_data(gbytes, &data_size);
    val = ghb_json_parse(data);
    g_bytes_unref(gbytes);
    ghb_dict_set(resources, "internal-defaults", val);
}

GhbValue*
ghb_resource_get(const gchar *name)
{
    GhbValue *result;
    result = ghb_dict_get(resources, name);
    return result;
}

void
ghb_resource_free (void)
{
    ghb_value_free(&resources);
}
