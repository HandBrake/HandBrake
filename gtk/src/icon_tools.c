/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * icon_tools.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * icon_tools.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 */
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "icon_tools.h"

GdkPixbuf*
icon_deserialize(const guint8 *sd, guint len)
{
	GdkPixdata pd;
	GdkPixbuf *pb;
	GError *err = NULL;

	gdk_pixdata_deserialize(&pd, len, sd, &err);
	pb = gdk_pixbuf_from_pixdata(&pd, TRUE, &err);
	return pb;
}

guint8*
icon_serialize(const GdkPixbuf *pixbuf, guint *len)
{
	GdkPixdata pd;
	guint8 *sd;

	gdk_pixdata_from_pixbuf(&pd, pixbuf, FALSE);
	sd = gdk_pixdata_serialize(&pd, len);
	return sd;
}

guint8*
icon_file_serialize(const gchar *filename, guint *len)
{
	GdkPixbuf *pb;
	GError *err = NULL;

	pb = gdk_pixbuf_new_from_file(filename, &err);
	if (pb == NULL)
	{
		g_warning("Failed to open icon file %s: %s", filename, err->message);
		return NULL;
	}
	return icon_serialize(pb, len);
}

GdkPixbuf*
base64_to_icon(const gchar *bd)
{
	guchar *sd;
	gsize len;
	GdkPixdata pd;
	GdkPixbuf *pb;
	GError *err = NULL;

	sd = g_base64_decode(bd, &len);
	gdk_pixdata_deserialize(&pd, len, sd, &err);
	pb = gdk_pixbuf_from_pixdata(&pd, TRUE, &err);
	g_free(sd);
	return pb;
}

gchar*
icon_to_base64(const GdkPixbuf *pixbuf)
{
	GdkPixdata pd;
	guint len;
	guint8 *sd;
	gchar *bd;

	gdk_pixdata_from_pixbuf(&pd, pixbuf, FALSE);
	sd = gdk_pixdata_serialize(&pd, &len);
	bd = g_base64_encode(sd, len);
	g_free(sd);
	return bd;
}

gchar*
icon_file_to_base64(const gchar *filename)
{
	GdkPixbuf *pb;
	GError *err = NULL;

	pb = gdk_pixbuf_new_from_file(filename, &err);
	if (pb == NULL)
	{
		g_warning("Failed to open icon file %s: %s", filename, err->message);
		return NULL;
	}
	return icon_to_base64(pb);
}

