/*
 * icons.c
 * Copyright (C) John Stebbins 2008-2016 <stebbins@stebbins>
 *
 * icons.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * icons.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "ghbcompat.h"
#include "values.h"
#include "resources.h"

void
ghb_load_icons()
{
    GHashTableIter iter;
    gchar *key;
    GValue *gval;

    GValue *icons = ghb_resource_get("icons");
    ghb_dict_iter_init(&iter, icons);
    // middle (void*) cast prevents gcc warning "defreferencing type-punned
    // pointer will break strict-aliasing rules"
    while (g_hash_table_iter_next(
            &iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
    {
        ghb_rawdata_t *rd;
        gint size;
        GdkPixbuf *pb;
        gboolean svg;
        char *name = g_strdup(key);
        char *pos;

        pos = g_strstr_len(name, -1, ".");
        if (pos != NULL)
            *pos = '\0';

        GInputStream *gis;
        svg = ghb_value_boolean(ghb_dict_lookup(gval, "svg"));
        rd = g_value_get_boxed(ghb_dict_lookup(gval, "data"));
        if (svg)
        {
            int ii;
            int sizes[] = {16, 22, 24, 32, 48, 64, 128, 256, 0};
            for (ii = 0; sizes[ii]; ii++)
            {
                gis = g_memory_input_stream_new_from_data(rd->data, rd->size,
                                                          NULL);
                pb = gdk_pixbuf_new_from_stream_at_scale(gis,
                                                         sizes[ii], sizes[ii],
                                                         TRUE, NULL, NULL);
                g_input_stream_close(gis, NULL, NULL);
                size = gdk_pixbuf_get_height(pb);
                gtk_icon_theme_add_builtin_icon(name, size, pb);
                g_object_unref(pb);
            }
        }
        else
        {
            gis = g_memory_input_stream_new_from_data(rd->data, rd->size, NULL);
            pb = gdk_pixbuf_new_from_stream(gis, NULL, NULL);
            g_input_stream_close(gis, NULL, NULL);
            size = gdk_pixbuf_get_height(pb);
            gtk_icon_theme_add_builtin_icon(name, size, pb);
            g_object_unref(pb);
        }
        g_free(name);
    }
}
