/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#if !defined(_ICON_TOOLS_H_)
#define _ICON_TOOLS_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>

GdkPixbuf* icon_deserialize(const guint8 *sd, guint len);
guint8* icon_serialize(const GdkPixbuf *pixbuf, guint *len);
guint8* icon_file_serialize(const gchar *filename, guint *len);
GdkPixbuf* base64_to_icon(const gchar *bd);
gchar* icon_to_base64(const GdkPixbuf *pixbuf);
gchar* icon_file_to_base64(const gchar *filename);

#endif // _ICON_TOOLS_H_
