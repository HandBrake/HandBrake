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

#if !GTK_CHECK_VERSION(3, 10, 0)
#define gtk_image_set_from_icon_name gtk_image_set_from_stock
#define GHB_PLAY_ICON "gtk-media-play"
#define GHB_PAUSE_ICON "gtk-media-pause"
#else
#define GHB_PLAY_ICON "media-playback-start"
#define GHB_PAUSE_ICON "media-playback-pause"
#endif

#if !GTK_CHECK_VERSION(3, 10, 0)
#define GHB_STOCK_OPEN      GTK_STOCK_OPEN
#define GHB_STOCK_CANCEL    GTK_STOCK_CANCEL
#define GHB_STOCK_SAVE      GTK_STOCK_SAVE
#else
#define GHB_STOCK_OPEN      "_Open"
#define GHB_STOCK_CANCEL    "_Cancel"
#define GHB_STOCK_SAVE      "_Save"
#endif

#endif // _GHB_COMPAT_H_
