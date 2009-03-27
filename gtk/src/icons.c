#include <gtk/gtk.h>
#include "icon_tools.h"
#include "values.h"
#include "resources.h"

#if 0
void
ghb_load_icons()
{
	GHashTableIter iter;
	gchar *name;
	GValue *gval;

	GValue *icons = ghb_resource_get("icons");
	ghb_dict_iter_init(&iter, icons);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&name, (gpointer*)(void*)&gval))
	{
		gint colorspace, bps, width, height, rowstride;
		gboolean alpha;
		ghb_rawdata_t *rd;
		gint size;
		GdkPixbuf *pb;

		colorspace = ghb_value_int(ghb_dict_lookup(gval, "colorspace"));
		alpha = ghb_value_boolean(ghb_dict_lookup(gval, "alpha"));
		bps = ghb_value_int(ghb_dict_lookup(gval, "bps"));
		width = ghb_value_int(ghb_dict_lookup(gval, "width"));
		height = ghb_value_int(ghb_dict_lookup(gval, "height"));
		rowstride = ghb_value_int(ghb_dict_lookup(gval, "rowstride"));
		rd = g_value_get_boxed(ghb_dict_lookup(gval, "data"));
		pb = gdk_pixbuf_new_from_data(
				rd->data, colorspace, alpha, bps,
				width, height, rowstride,
				NULL, NULL);
		size = gdk_pixbuf_get_height(pb);
		gtk_icon_theme_add_builtin_icon(name, size, pb);
		gdk_pixbuf_unref(pb);
	}
}

#else

void
ghb_load_icons()
{
	GdkPixbuf *pb;
	GHashTableIter iter;
	gchar *name;
	GValue *gval;
	ghb_rawdata_t *rd;
	gint size;

	GValue *icons = ghb_resource_get("icons");
	ghb_dict_iter_init(&iter, icons);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&name, (gpointer*)(void*)&gval))
	{
		rd = g_value_get_boxed(gval);
		pb = icon_deserialize(rd->data, rd->size);
		size = gdk_pixbuf_get_height(pb);
		gtk_icon_theme_add_builtin_icon(name, size, pb);
		gdk_pixbuf_unref(pb);
	}
}
#endif
