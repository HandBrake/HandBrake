#if !defined(_GHB_COMPAT_H_)
#define _GHB_COMPAT_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

static inline PangoFontDescription* ghb_widget_get_font(GtkWidget *widget)
{
    PangoFontDescription *font = NULL;

#if GTK_CHECK_VERSION(3, 0, 0)
    GtkStyleContext *style;

    style = gtk_widget_get_style_context(widget);

#if GTK_CHECK_VERSION(3, 8, 0)
    gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL,
                          "font", &font, NULL);
#else
    font = gtk_style_context_get_font(style, GTK_STATE_FLAG_NORMAL);
#endif
#endif

    return font;
}

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
