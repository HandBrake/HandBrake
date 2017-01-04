/*
 * ghbcompat.h
 * Copyright (C) John Stebbins 2008-2017 <stebbins@stebbins>
 *
 * ghbcompat.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * ghbcompat.h is distributed in the hope that it will be useful,
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

static inline void ghb_monitor_get_size(GdkWindow *window, gint *w, gint *h)
{
    *w = *h = 0;

#if GTK_CHECK_VERSION(3, 22, 0)
    if (window != NULL)
    {
        GdkMonitor *mm;
        GdkDisplay *dd;

        dd = gdk_display_get_default();
        if (dd != NULL)
        {
            mm = gdk_display_get_monitor_at_window(dd, window);
            if (mm != NULL)
            {
                GdkRectangle rr;

                gdk_monitor_get_geometry(mm, &rr);
                *w = rr.width;
                *h = rr.height;
            }
        }
    }
#else
    GdkScreen *ss;

    ss = gdk_screen_get_default();
    if (ss != NULL)
    {
        *w = gdk_screen_get_width(ss);
        *h = gdk_screen_get_height(ss);
    }
#endif
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
