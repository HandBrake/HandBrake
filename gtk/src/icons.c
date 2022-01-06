/*
 * icons.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * icons.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * icons.c is distributed in the hope that it will be useful,
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
#include "icon_res.h"

void
ghb_load_icons()
{
#if GTK_CHECK_VERSION(3, 14, 0)
    ghb_icons_register_resource();
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(),
                                     "/fr/handbrake/ghb/icons");
#else
    ghb_icons_register_resource();
    GResource *icon_res = ghb_icons_get_resource();

    char ** children = g_resource_enumerate_children(icon_res,
                            "/fr/handbrake/ghb/icons/scalable/apps", 0, NULL);

    if (children == NULL)
    {
        g_warning("No icons in resources!");
        return;
    }
    int ii;
    for (ii = 0; children[ii] != NULL; ii++)
    {
        char * path;

        path = g_strdup_printf("/fr/handbrake/ghb/icons/scalable/apps/%s",
                               children[ii]);
        GBytes *gbytes = g_resource_lookup_data(icon_res, path, 0, NULL);
        gsize data_size;
        gconstpointer data = g_bytes_get_data(gbytes, &data_size);
        g_free(path);

        char *pos;
        char *name = g_strdup(children[ii]);
        pos = g_strstr_len(name, -1, ".");
        if (pos != NULL)
            *pos = '\0';

        int jj;
        int sizes[] = {16, 22, 24, 32, 48, 64, 128, 256, 0};
        for (jj = 0; sizes[jj]; jj++)
        {
            GdkPixbuf *pb;
            GInputStream *gis;
            int size;

            gis = g_memory_input_stream_new_from_data(data, data_size,
                                                      NULL);
            pb = gdk_pixbuf_new_from_stream_at_scale(gis,
                                                     sizes[jj], sizes[jj],
                                                     TRUE, NULL, NULL);
            g_input_stream_close(gis, NULL, NULL);
            size = gdk_pixbuf_get_height(pb);
            gtk_icon_theme_add_builtin_icon(name, size, pb);
            g_object_unref(pb);
        }
        g_bytes_unref(gbytes);
    }
    g_strfreev(children);
#endif
}
