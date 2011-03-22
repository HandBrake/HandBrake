#if !defined(_GHB_COMPAT_H_)
#define _GHB_COMPAT_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#if !GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms.h>
#endif

// Define any keys not defined by older GDK versions
#if !defined(GDK_KEY_Delete)
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_Up GDK_Up
#endif

#endif // _GHB_COMPAT_H_
