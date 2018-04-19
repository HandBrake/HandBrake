/*
 * ghbcompat.h
 * Copyright (C) John Stebbins 2008-2018 <stebbins@stebbins>
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
#include <string.h>

static inline void ghb_widget_get_preferred_width(
    GtkWidget *widget, gint *min_width, gint * natural_width)
{
#if GTK_CHECK_VERSION(3, 90, 0)
    GtkRequisition min_req, nat_req;

    gtk_widget_get_preferred_size(widget, &min_req, &nat_req);
    if (min_width != NULL)
    {
        *min_width = min_req.width;
    }
    if (natural_width != NULL)
    {
        *natural_width = nat_req.width;
    }
#else
    gtk_widget_get_preferred_width(widget, min_width, natural_width);
#endif
}

static inline void ghb_widget_get_preferred_height(
    GtkWidget *widget, gint *min_height, gint * natural_height)
{
#if GTK_CHECK_VERSION(3, 90, 0)
    GtkRequisition min_req, nat_req;

    gtk_widget_get_preferred_size(widget, &min_req, &nat_req);
    if (min_height != NULL)
    {
        *min_height = min_req.height;
    }
    if (natural_height != NULL)
    {
        *natural_height = nat_req.height;
    }
#else
    gtk_widget_get_preferred_height(widget, min_height, natural_height);
#endif
}

static inline void ghb_button_set_icon_name(GtkButton *button,
                                            const char * name)
{
#if GTK_CHECK_VERSION(3, 90, 0)
    gtk_button_set_icon_name(button, name);
#else
    GtkImage *image;

    image = GTK_IMAGE(gtk_image_new_from_icon_name(name, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_image(button, GTK_WIDGET(image));
#endif
}

static inline void ghb_get_expand_fill(GtkBox * box, GtkWidget * child,
                                       gboolean *expand, gboolean *fill)
{
    if (gtk_orientable_get_orientation(GTK_ORIENTABLE(box)) ==
        GTK_ORIENTATION_HORIZONTAL)
    {
        *expand = gtk_widget_get_hexpand(child);
        *fill   = gtk_widget_get_halign(child) == GTK_ALIGN_FILL;
    }
    else
    {
        *expand = gtk_widget_get_vexpand(child);
        *fill   = gtk_widget_get_valign(child) == GTK_ALIGN_FILL;
    }
}

static inline void ghb_box_pack_start(GtkBox * box, GtkWidget * child)
{
#if GTK_CHECK_VERSION(3, 90, 0)
    gtk_box_pack_start(box, child);
#else
    gboolean expand, fill;

    ghb_get_expand_fill(box, child, &expand, &fill);
    gtk_box_pack_start(box, child, expand, fill, 0);
#endif
}

static inline void ghb_css_provider_load_from_data(GtkCssProvider *provider,
                                                   const gchar *data,
                                                   gssize length)
{
#if GTK_CHECK_VERSION(3, 90, 0)
    gtk_css_provider_load_from_data(provider, data, length);
#else
    gtk_css_provider_load_from_data(provider, data, length, NULL);
#endif
}

static inline GdkEventType ghb_event_get_event_type(const GdkEvent *event)
{
#if GTK_CHECK_VERSION(3, 10, 0)
    return gdk_event_get_event_type(event);
#else
    return event->type;
#endif
}

static inline gboolean ghb_event_get_keyval(const GdkEvent *event,
                                            guint *keyval)
{
#if GTK_CHECK_VERSION(3, 2, 0)
    return gdk_event_get_keyval(event, keyval);
#else
    *keyval = ((GdkEventKey*)event)->keyval;
    return TRUE;
#endif
}

static inline gboolean ghb_event_get_button(const GdkEvent *event,
                                            guint *button)
{
#if GTK_CHECK_VERSION(3, 2, 0)
    return gdk_event_get_button(event, button);
#else
    *keyval = ((GdkEventButton*)event)->button;
    return TRUE;
#endif
}

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

static inline gboolean ghb_strv_contains(const char ** strv, const char * str)
{
#if GLIB_CHECK_VERSION(2, 44, 0)
    return g_strv_contains(strv, str);
#else
    int ii;

    if (strv == NULL)
    {
        return FALSE;
    }
    for (ii = 0; strv[ii] != NULL; ii++)
    {
        if (!strcmp(strv[ii], str))
        {
            return TRUE;
        }
    }
    return FALSE;
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
