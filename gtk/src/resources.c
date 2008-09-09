/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * resources.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * resources.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 */
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "settings.h"
#include "plist.h"
#include "resources.h"
#include "values.h"

static const gchar resource_str[] =
#include "resource_data.h"
;

static GValue *resources;

void
ghb_resource_init()
{
	resources = ghb_plist_parse(resource_str, sizeof(resource_str)-1);
}

GValue*
ghb_resource_get(const gchar *name)
{
	GValue *result;
	result = ghb_dict_lookup(resources, name);
	return result;
}
