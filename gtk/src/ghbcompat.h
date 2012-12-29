#if !defined(_GHB_COMPAT_H_)
#define _GHB_COMPAT_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#if !GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms.h>
#endif

#if !GTK_CHECK_VERSION(2, 22, 0)
// Define any keys not defined by older GDK versions
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_Up GDK_Up
#endif

#if !GTK_CHECK_VERSION(2, 20, 0)
// Replace simple accessor functions added to newer gtk versions
static inline void gtk_widget_set_realized(GtkWidget *widget, gboolean realized)
{
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
}

static inline gboolean gtk_widget_get_realized(GtkWidget *widget)
{
    return GTK_WIDGET_REALIZED(widget);
}
#endif

#endif // _GHB_COMPAT_H_
