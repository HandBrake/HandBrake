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
