#include <gtk/gtk.h>
#include "icon_tools.h"
#include "values.h"
#include "resources.h"

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
