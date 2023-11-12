/*
 * ghbcompat.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * ghbcompat.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
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

#define G_LOG_USE_STRUCTURED
#define G_LOG_DOMAIN "ghb"

#define ghb_log_func(x) g_debug("Function: %s", __func__)
#define ghb_log_func_str(x) g_debug("Function: %s (%s)", __func__, (x))

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#if defined(_WIN32)
#define GHB_UNSAFE_FILENAME_CHARS "/:<>\"\\|?*"
#else
#define GHB_UNSAFE_FILENAME_CHARS "/"
#endif

#if GTK_CHECK_VERSION(4, 4, 0)
#define GHB_ICON_SIZE_BUTTON GTK_ICON_SIZE_NORMAL
#else
#define GHB_ICON_SIZE_BUTTON GTK_ICON_SIZE_BUTTON
#endif

#define GHB_PLAY_ICON "media-playback-start"
#define GHB_PAUSE_ICON "media-playback-pause"

#define GHB_STOCK_OPEN      _("_Open")
#define GHB_STOCK_CANCEL    _("_Cancel")
#define GHB_STOCK_SAVE      _("_Save")

static inline void ghb_widget_get_preferred_width(
    GtkWidget *widget, gint *min_width, gint * natural_width)
{
#if GTK_CHECK_VERSION(4, 4, 0)
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
#if GTK_CHECK_VERSION(4, 4, 0)
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
#if GTK_CHECK_VERSION(4, 4, 0)
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

static inline void ghb_box_append_child(GtkBox * box, GtkWidget * child)
{
#if GTK_CHECK_VERSION(4, 4, 0)
    GtkWidget * sibling = NULL;

    sibling = gtk_widget_get_last_child(GTK_WIDGET(box));
    gtk_box_insert_child_after(box, child, sibling);
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
#if GTK_CHECK_VERSION(4, 4, 0)
    gtk_css_provider_load_from_data(provider, data, length);
#else
    gtk_css_provider_load_from_data(provider, data, length, NULL);
#endif
}

static inline GdkEventType ghb_event_get_event_type(const GdkEvent *event)
{
    return gdk_event_get_event_type(event);
}

static inline gboolean ghb_event_get_keyval(const GdkEvent *event,
                                            guint *keyval)
{
    return gdk_event_get_keyval(event, keyval);
}

static inline gboolean ghb_event_get_button(const GdkEvent *event,
                                            guint *button)
{
    return gdk_event_get_button(event, button);
}

static inline PangoFontDescription* ghb_widget_get_font(GtkWidget *widget)
{
    PangoFontDescription *font = NULL;

    GtkStyleContext *style;

    style = gtk_widget_get_style_context(widget);

    gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL,
                          "font", &font, NULL);
    return font;
}

#if GTK_CHECK_VERSION(4, 4, 0)
typedef GdkSurface      GhbSurface;
typedef GdkSurfaceHints GhbSurfaceHints;

static inline GhbSurface * ghb_widget_get_surface(GtkWidget * widget)
{
    return gtk_widget_get_surface(widget);
}

static inline GdkMonitor *
ghb_display_get_monitor_at_surface(GdkDisplay * display, GhbSurface * surface)
{
    return gdk_display_get_monitor_at_surface(display, surface);
}

static inline void
ghb_surface_set_geometry_hints(GhbSurface * surface,
                               const GdkGeometry * geometry,
                               GhbSurfaceHints geom_mask)
{
    gdk_surface_set_geometry_hints(surface, geometry, geom_mask);
}
#else
typedef GdkWindow       GhbSurface;
typedef GdkWindowHints  GhbSurfaceHints;

static inline GhbSurface * ghb_widget_get_surface(GtkWidget * widget)
{
    return gtk_widget_get_window(widget);
}

static inline GdkMonitor *
ghb_display_get_monitor_at_surface(GdkDisplay * display, GhbSurface * surface)
{
    return gdk_display_get_monitor_at_window(display, surface);
}

static inline void
ghb_surface_set_geometry_hints(GhbSurface * surface,
                               const GdkGeometry * geometry,
                               GhbSurfaceHints geom_mask)
{
    gdk_window_set_geometry_hints(surface, geometry, geom_mask);
}
#endif

static inline void ghb_monitor_get_size(GtkWidget *widget, gint *w, gint *h)
{
    *w = *h = 0;

    GdkDisplay * display = gtk_widget_get_display(widget);
    GhbSurface * surface = ghb_widget_get_surface(widget);
    if (surface != NULL && display != NULL)
    {
        GdkMonitor * monitor;

        monitor = ghb_display_get_monitor_at_surface(display, surface);
        if (monitor != NULL)
        {
            GdkRectangle rect;

            gdk_monitor_get_geometry(monitor, &rect);
            *w = rect.width;
            *h = rect.height;
        }
    }
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

#if GTK_CHECK_VERSION(4, 4, 0)

#define ghb_editable_get_text(e) gtk_editable_get_text(GTK_EDITABLE(e))
#define ghb_editable_set_text(e,t) gtk_editable_set_text(GTK_EDITABLE(e), (t))

#else

#define ghb_editable_get_text(e) gtk_entry_get_text(GTK_ENTRY(e))
#define ghb_editable_set_text(e,t) gtk_entry_set_text(GTK_ENTRY(e), (t))

#endif

#if GTK_CHECK_VERSION(4, 4, 0)
static inline void
ghb_image_set_from_icon_name(GtkImage * image, const gchar * name,
                             GtkIconSize size)
{
    gtk_image_set_from_icon_name(image, name);
    gtk_image_set_icon_size(image, size);
}

static inline GtkWidget *
ghb_image_new_from_icon_name(const gchar * name, GtkIconSize size)
{
    GtkWidget * image;

    image = gtk_image_new_from_icon_name(name);
    gtk_image_set_icon_size(GTK_IMAGE(image), size);

    return image;
}

static inline GtkWidget *
ghb_button_new_from_icon_name(const gchar * name)
{
    return gtk_button_new_from_icon_name(name);
}

static inline GtkWidget *
ghb_scale_button_new(gdouble min, gdouble max, gdouble step,
                     const gchar ** icons)
{
    return gtk_scale_button_new(min, max, step, icons);
}
#else
static inline void
ghb_image_set_from_icon_name(GtkImage * image, const gchar * name,
                             GtkIconSize size)
{
    gtk_image_set_from_icon_name(image, name, size);
}

static inline GtkWidget *
ghb_image_new_from_icon_name(const gchar * name, GtkIconSize size)
{
    return gtk_image_new_from_icon_name(name, size);
}

static inline GtkWidget *
ghb_button_new_from_icon_name(const gchar * name)
{
    return gtk_button_new_from_icon_name(name, GHB_ICON_SIZE_BUTTON);
}

static inline GtkWidget *
ghb_scale_button_new(gdouble min, gdouble max, gdouble step,
                     const gchar ** icons)
{
    return gtk_scale_button_new(GHB_ICON_SIZE_BUTTON, min, max, step, icons);
}
#endif

#if GTK_CHECK_VERSION(4, 4, 0)
static inline void ghb_drag_status(GdkDrop * ctx, GdkDragAction action,
                                   guint32 time)
{
    gdk_drop_status(ctx, action);
}
#else
static inline void ghb_drag_status(GdkDragContext * ctx, GdkDragAction action,
                                   guint32 time)
{
    gdk_drag_status(ctx, action, time);
}
#endif

#if GTK_CHECK_VERSION(4, 4, 0)
static inline void ghb_entry_set_width_chars(GtkEntry * entry, gint n_chars)
{
    gtk_editable_set_width_chars(GTK_EDITABLE(entry), n_chars);
}
#else
static inline void ghb_entry_set_width_chars(GtkEntry * entry, gint n_chars)
{
    gtk_entry_set_width_chars(entry, n_chars);
}
#endif

#if GTK_CHECK_VERSION(4, 4, 0)
static inline GdkAtom ghb_atom_string(const char * str)
{
    return g_intern_static_string(str);
}
#else
static inline GdkAtom ghb_atom_string(const char * str)
{
    return gdk_atom_intern_static_string(str);
}
#endif

static inline GtkGesture *ghb_gesture_click_new (GtkWidget *widget)
{
    GtkGesture *gesture;

#if GTK_CHECK_VERSION(4, 4, 0)
    gesture = gtk_gesture_click_new();
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
#else
    gesture = gtk_gesture_multi_press_new(widget);
#endif
    return gesture;
}

#endif // _GHB_COMPAT_H_
