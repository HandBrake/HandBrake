/* compat.h
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#define G_LOG_USE_STRUCTURED
#define G_LOG_DOMAIN "ghb"

#define ghb_log_func(x)
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

G_BEGIN_DECLS

int ghb_dialog_run (GtkDialog *dialog);

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

static inline void
gtkc_button_set_icon_name (GtkButton *button, const char *name)
{
#if GTK_CHECK_VERSION(4, 4, 0)
    gtk_button_set_icon_name(button, name);
#else
    GtkImage *image;

    image = GTK_IMAGE(gtk_image_new_from_icon_name(name, GHB_ICON_SIZE_BUTTON));
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
    gtk_box_append(box, child);
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

static inline GdkEventType ghb_event_get_event_type(GdkEvent *event)
{
    return gdk_event_get_event_type(event);
}

#if GTK_CHECK_VERSION(4, 4, 0)
typedef GdkSurface      GhbSurface;

static inline GhbSurface * ghb_widget_get_surface(GtkWidget * widget)
{
    GtkRoot *root = gtk_widget_get_root(widget);
    return gtk_native_get_surface(GTK_NATIVE(root));
}

static inline GdkMonitor *
ghb_display_get_monitor_at_surface(GdkDisplay * display, GhbSurface * surface)
{
    return gdk_display_get_monitor_at_surface(display, surface);
}
#else
typedef GdkWindow       GhbSurface;

static inline GhbSurface * ghb_widget_get_surface(GtkWidget * widget)
{
    return gtk_widget_get_window(widget);
}

static inline GdkMonitor *
ghb_display_get_monitor_at_surface(GdkDisplay * display, GhbSurface * surface)
{
    return gdk_display_get_monitor_at_window(display, surface);
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
#define ghb_window_destroy(w) gtk_window_destroy(GTK_WINDOW(w))
#define ghb_file_chooser_set_current_folder(c,f,e) gtk_file_chooser_set_current_folder((c), (f), (e))
#define ghb_tool_button_set_icon_name(b,n) ghb_button_set_icon_name(GHB_BUTTON(b), (n))
#define ghb_tool_button_set_label(b,l) ghb_button_set_label(GHB_BUTTON(b), (l))
#define ghb_tool_item_set_tooltip_text(i,t) gtk_widget_set_tooltip_text(GTK_WIDGET(i), (t))
#define ghb_check_button_get_active(b) gtk_check_button_get_active(GTK_CHECK_BUTTON(b))
#define ghb_check_button_set_active(b,a) gtk_check_button_set_active(GTK_CHECK_BUTTON(b), (a))

#else

#define ghb_editable_get_text(e) gtk_entry_get_text(GTK_ENTRY(e))
#define ghb_editable_set_text(e,t) gtk_entry_set_text(GTK_ENTRY(e), (t))
#define ghb_window_destroy(w) gtk_widget_destroy(GTK_WIDGET(w))
#define ghb_file_chooser_set_current_folder(c,f,e) gtk_file_chooser_set_current_folder_file((c), (f), (e))
#define ghb_tool_button_set_icon_name(b,n) gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(b), (n))
#define ghb_tool_button_set_label(b,l) gtk_tool_button_set_label(GTK_TOOL_BUTTON(b), (l))
#define ghb_tool_item_set_tooltip_text(i,t) gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(i), (t))
#define ghb_check_button_get_active(b) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))
#define ghb_check_button_set_active(b,a) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), (a))

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
    gdk_drop_status(ctx, action, GDK_ACTION_MOVE);
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

#if !GTK_CHECK_VERSION(4, 4, 0)
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

static inline void
ghb_file_chooser_set_initial_file (GtkFileChooser *chooser, const char *file)
{
    if (!file || !file[0])
    {
        return;
    }
    else if (g_file_test(file, G_FILE_TEST_IS_DIR))
    {
#if GTK_CHECK_VERSION(4, 4, 0)
        GFile *gfile = g_file_new_for_path(file);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), gfile, NULL);
        g_object_unref(gfile);
#else
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), file);
#endif
    }
    else
    {
        char *dir = g_path_get_dirname(file);
        if (g_file_test(dir, G_FILE_TEST_IS_DIR))
        {
#if GTK_CHECK_VERSION(4, 4, 0)
            GFile *gfile = g_file_new_for_path(dir);
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), gfile, NULL);
            g_object_unref(gfile);
#else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
#endif
        }
        g_free(dir);
        if (gtk_file_chooser_get_action(chooser) == GTK_FILE_CHOOSER_ACTION_SAVE)
        {
            char *base = g_path_get_basename(file);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), base);
            g_free(base);
        }
    }
}

static inline char *
ghb_file_chooser_get_filename (GtkFileChooser *chooser)
{
#if GTK_CHECK_VERSION(4, 4, 0)
    g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
    return g_file_get_path(file);
#else
    return gtk_file_chooser_get_filename(chooser);
#endif
}

static inline char *
ghb_file_chooser_get_current_folder (GtkFileChooser *chooser)
{
#if GTK_CHECK_VERSION(4, 4, 0)
    g_autoptr(GFile) folder = gtk_file_chooser_get_current_folder(chooser);
    return g_file_get_path(folder);
#else
    return gtk_file_chooser_get_current_folder(chooser);
#endif
}

G_END_DECLS

