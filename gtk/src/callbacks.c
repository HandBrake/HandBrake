/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * callbacks.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * callbacks.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include "ghbcompat.h"

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#if !defined(_WIN32)
#include <poll.h>
#define G_UDEV_API_IS_SUBJECT_TO_CHANGE 1
#if defined(__linux__) && defined(_HAVE_GUDEV)
#include <gudev/gudev.h>
#endif

#if defined( __FreeBSD__ ) || defined(__OpenBSD__)
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netdb.h>

#if !defined(_NO_UPDATE_CHECK)
#if defined(_OLD_WEBKIT)
#include <webkit.h>
#else
#include <webkit/webkit.h>
#endif
#endif

#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif

#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif
#else
#include <winsock2.h>
#include <dbt.h>
#endif

#include "handbrake/handbrake.h"
#include "callbacks.h"
#include "chapters.h"
#include "queuehandler.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "resources.h"
#include "settings.h"
#include "jobdict.h"
#include "presets.h"
#include "preview.h"
#include "values.h"
#include "plist.h"
#include "appcast.h"
#include "hb-backend.h"
#include "ghb-dvd.h"
#include "libavutil/parseutils.h"


static void add_video_file_filters(GtkFileChooser *chooser, signal_user_data_t *ud);
static void update_queue_labels(signal_user_data_t *ud);
static void load_all_titles(signal_user_data_t *ud, int titleindex);
static GList* dvd_device_list(void);
static void prune_logs(signal_user_data_t *ud);
static gboolean can_suspend_logind(void);
static void suspend_logind(void);
static gboolean appcast_busy = FALSE;
static gboolean has_drive = FALSE;

#if !defined(_WIN32)
#define DBUS_LOGIND_SERVICE         "org.freedesktop.login1"
#define DBUS_LOGIND_PATH            "/org/freedesktop/login1"
#define DBUS_LOGIND_INTERFACE       "org.freedesktop.login1.Manager"
#define GPM_DBUS_PM_SERVICE         "org.freedesktop.PowerManagement"
#define GPM_DBUS_INHIBIT_PATH       "/org/freedesktop/PowerManagement/Inhibit"
#define GPM_DBUS_INHIBIT_INTERFACE  "org.freedesktop.PowerManagement.Inhibit"
#endif

#if !defined(_WIN32)
static GDBusProxy *
ghb_get_dbus_proxy(GBusType type, const gchar *name, const gchar *path, const gchar *interface)
{
    GDBusConnection *conn;
    GDBusProxy *proxy;
    GError *error = NULL;

    ghb_log_func();
    conn = g_bus_get_sync(type, NULL, &error);
    if (conn == NULL)
    {
        if (error != NULL)
        {
            g_warning("DBUS cannot connect: %s", error->message);
            g_error_free(error);
        }
        return NULL;
    }

    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  name, path, interface, NULL, NULL);
    if (proxy == NULL)
        g_warning("Could not get DBUS proxy: %s", name);

    g_object_unref(conn);
    return proxy;
}
#endif

static gboolean
can_suspend_logind (void)
{
    gboolean can_suspend = FALSE;
#if !defined(_WIN32)
    char *str = NULL;
    GDBusProxy  *proxy;
    GError *error = NULL;
    GVariant *res;

    ghb_log_func();
    proxy = ghb_get_dbus_proxy(G_BUS_TYPE_SYSTEM, DBUS_LOGIND_SERVICE,
                            DBUS_LOGIND_PATH, DBUS_LOGIND_INTERFACE);
    if (proxy == NULL)
        return FALSE;

    res = g_dbus_proxy_call_sync(proxy, "CanSuspend", NULL,
                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (!res)
    {
        if (error != NULL)
        {
            g_warning("CanSuspend failed: %s", error->message);
            g_error_free(error);
        }
        else
            g_warning("CanSuspend failed");
        // Try to shutdown anyway
        can_suspend = TRUE;
    }
    else
    {
        g_variant_get(res, "(s)", &str);
        g_variant_unref(res);
        if (g_strcmp0(str, "yes") == 0)
            can_suspend = TRUE;

        g_free(str);
    }
    g_object_unref(G_OBJECT(proxy));
#endif
    return can_suspend;
}

static void
suspend_logind (void)
{
#if !defined(_WIN32)
    GDBusProxy  *proxy;
    GError *error = NULL;
    GVariant *res;

    ghb_log_func();
    proxy = ghb_get_dbus_proxy(G_BUS_TYPE_SYSTEM, DBUS_LOGIND_SERVICE,
                            DBUS_LOGIND_PATH, DBUS_LOGIND_INTERFACE);
    if (proxy == NULL)
        return;

    res = g_dbus_proxy_call_sync(proxy, "Suspend", g_variant_new("(b)", FALSE),
                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (!res)
    {
        if (error != NULL)
        {
            g_warning("Suspend failed: %s", error->message);
            g_error_free(error);
        }
        else
            g_warning("Suspend failed");
    }
    else
    {
        g_variant_unref(res);
    }
    g_object_unref(G_OBJECT(proxy));
#endif
}

static gboolean
can_shutdown_logind (void)
{
    gboolean can_shutdown = FALSE;
#if !defined(_WIN32)
    char *str = NULL;
    GDBusProxy  *proxy;
    GError *error = NULL;
    GVariant *res;


    ghb_log_func();
    proxy = ghb_get_dbus_proxy(G_BUS_TYPE_SYSTEM, DBUS_LOGIND_SERVICE,
                            DBUS_LOGIND_PATH, DBUS_LOGIND_INTERFACE);
    if (proxy == NULL)
        return FALSE;

    res = g_dbus_proxy_call_sync(proxy, "CanPowerOff", NULL,
                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (!res)
    {
        if (error != NULL)
        {
            g_warning("CanPowerOff failed: %s", error->message);
            g_error_free(error);
        }
        else
            g_warning("CanPowerOff failed");
        // Try to shutdown anyway
        can_shutdown = TRUE;
    }
    else
    {
        g_variant_get(res, "(s)", &str);
        g_variant_unref(res);
        if (g_strcmp0(str, "yes") == 0)
            can_shutdown = TRUE;

        g_free(str);
    }
    g_object_unref(G_OBJECT(proxy));
#endif
    return can_shutdown;
}

static void
shutdown_logind (void)
{
#if !defined(_WIN32)
    GDBusProxy  *proxy;
    GError *error = NULL;
    GVariant *res;


    ghb_log_func();
    proxy = ghb_get_dbus_proxy(G_BUS_TYPE_SYSTEM, DBUS_LOGIND_SERVICE,
                            DBUS_LOGIND_PATH, DBUS_LOGIND_INTERFACE);
    if (proxy == NULL)
        return;

    res = g_dbus_proxy_call_sync(proxy, "PowerOff", g_variant_new("(b)", FALSE),
                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (!res)
    {
        if (error != NULL)
        {
            g_warning("PowerOff failed: %s", error->message);
            g_error_free(error);
        }
        else
            g_warning("PowerOff failed");
    }
    else
    {
        g_variant_unref(res);
    }
    g_object_unref(G_OBJECT(proxy));
#endif
}

#if !defined(_WIN32)
// For inhibit and shutdown
#define GPM_DBUS_SM_SERVICE         "org.gnome.SessionManager"
#define GPM_DBUS_SM_PATH            "/org/gnome/SessionManager"
#define GPM_DBUS_SM_INTERFACE       "org.gnome.SessionManager"
#endif

enum {
    GHB_SUSPEND_UNINHIBITED = 0,
    GHB_SUSPEND_INHIBITED_GPM,
    GHB_SUSPEND_INHIBITED_GSM,
    GHB_SUSPEND_INHIBITED_GTK
};
static int suspend_inhibited = GHB_SUSPEND_UNINHIBITED;
static guint suspend_cookie;

static void
inhibit_suspend (signal_user_data_t *ud)
{
    if (suspend_inhibited != GHB_SUSPEND_UNINHIBITED)
    {
        // Already inhibited
        return;
    }
    suspend_cookie = gtk_application_inhibit(ud->app, NULL,
                            GTK_APPLICATION_INHIBIT_SUSPEND, "Encoding");
    if (suspend_cookie != 0)
    {
        suspend_inhibited = GHB_SUSPEND_INHIBITED_GTK;
        return;
    }
}

static void
uninhibit_suspend (signal_user_data_t *ud)
{
    switch (suspend_inhibited)
    {
        case GHB_SUSPEND_INHIBITED_GTK:
            gtk_application_uninhibit(ud->app, suspend_cookie);
            break;
        default:
            break;
    }
    suspend_inhibited = GHB_SUSPEND_UNINHIBITED;
}

// This is a dependency map used for greying widgets
// that are dependent on the state of another widget.
// The enable_value comes from the values that are
// obtained from ghb_widget_value().  For combo boxes
// you will have to look further to combo box options
// maps in hb-backend.c

GhbValue *dep_map;
GhbValue *rev_map;

void
ghb_init_dep_map (void)
{
    dep_map = ghb_resource_get("widget-deps");
    rev_map = ghb_resource_get("widget-reverse-deps");
}

static gboolean
dep_check(signal_user_data_t *ud, const gchar *name, gboolean *out_hide)
{
    GtkWidget *widget;
    GObject *dep_object;
    gint ii;
    gint count;
    gboolean result = TRUE;
    GhbValue *array, *data;
    const gchar *widget_name;

    ghb_log_func_str(name);

    if (rev_map == NULL) return TRUE;
    array = ghb_dict_get(rev_map, name);
    count = ghb_array_len(array);
    *out_hide = FALSE;
    for (ii = 0; ii < count; ii++)
    {
        data = ghb_array_get(array, ii);
        widget_name = ghb_value_get_string(ghb_array_get(data, 0));
        widget = GHB_WIDGET(ud->builder, widget_name);
        dep_object = gtk_builder_get_object(ud->builder, name);
        if (widget != NULL && !gtk_widget_is_sensitive(widget))
        {
            continue;
        }
        if (dep_object == NULL)
        {
            g_warning("Failed to find widget");
        }
        else
        {
            gchar *value;
            gint jj = 0;
            gchar **values;
            gboolean sensitive = FALSE;
            gboolean die, hide;

            die = ghb_value_get_bool(ghb_array_get(data, 2));
            hide = ghb_value_get_bool(ghb_array_get(data, 3));
            const char *tmp = ghb_value_get_string(ghb_array_get(data, 1));
            values = g_strsplit(tmp, "|", -1);

            if (widget)
                value = ghb_widget_string(widget);
            else
                value = ghb_dict_get_string_xform(ud->settings, widget_name);
            while (values && values[jj])
            {
                if (values[jj][0] == '>')
                {
                    gdouble dbl = g_strtod (&values[jj][1], NULL);
                    gdouble dvalue = ghb_widget_double(widget);
                    if (dvalue > dbl)
                    {
                        sensitive = TRUE;
                        break;
                    }
                }
                else if (values[jj][0] == '<')
                {
                    gdouble dbl = g_strtod (&values[jj][1], NULL);
                    gdouble dvalue = ghb_widget_double(widget);
                    if (dvalue < dbl)
                    {
                        sensitive = TRUE;
                        break;
                    }
                }
                if (strcmp(values[jj], value) == 0)
                {
                    sensitive = TRUE;
                    break;
                }
                jj++;
            }
            sensitive = die ^ sensitive;
            if (!sensitive)
            {
                result = FALSE;
                *out_hide |= hide;
            }
            g_strfreev (values);
            g_free(value);
        }
    }
    return result;
}

void
ghb_check_dependency(
    signal_user_data_t *ud,
    GtkWidget *widget,
    const char *alt_name)
{
    GObject *dep_object;
    const gchar *name;
    GhbValue *array, *data;
    gint count, ii;
    const gchar *dep_name;
    GType type;

    if (widget != NULL)
    {
        type = G_OBJECT_TYPE(widget);
        if (type == GTK_TYPE_COMBO_BOX)
            if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) < 0) return;
        name = ghb_get_setting_key(widget);
    }
    else
        name = alt_name;

    ghb_log_func_str(name);

    if (dep_map == NULL) return;
    array = ghb_dict_get(dep_map, name);
    count = ghb_array_len(array);
    for (ii = 0; ii < count; ii++)
    {
        gboolean sensitive;
        gboolean hide;

        data = ghb_array_get(array, ii);
        dep_name = ghb_value_get_string(data);
        dep_object = gtk_builder_get_object(ud->builder, dep_name);
        if (dep_object == NULL)
        {
            g_warning("Failed to find dependent widget %s", dep_name);
            continue;
        }
        sensitive = dep_check(ud, dep_name, &hide);
        gtk_widget_set_sensitive(GTK_WIDGET(dep_object), sensitive);
        gtk_widget_set_can_focus(GTK_WIDGET(dep_object), sensitive);
        if (!sensitive && hide)
        {
            if (gtk_widget_get_visible(GTK_WIDGET(dep_object)))
            {
                gtk_widget_hide(GTK_WIDGET(dep_object));
            }
        }
        else
        {
            if (!gtk_widget_get_visible(GTK_WIDGET(dep_object)))
            {
                gtk_widget_show(GTK_WIDGET(dep_object));
            }
        }
    }
}

void
ghb_check_all_dependencies(signal_user_data_t *ud)
{
    GhbDictIter iter;
    const gchar *dep_name;
    GhbValue *value;
    GObject *dep_object;

    ghb_log_func();
    if (rev_map == NULL) return;
    iter = ghb_dict_iter_init(rev_map);
    while (ghb_dict_iter_next(rev_map, &iter, &dep_name, &value))
    {
        gboolean sensitive;
        gboolean hide;

        dep_object = gtk_builder_get_object (ud->builder, dep_name);
        if (dep_object == NULL)
        {
            g_warning("Failed to find dependent widget %s", dep_name);
            continue;
        }
        sensitive = dep_check(ud, dep_name, &hide);
        gtk_widget_set_sensitive(GTK_WIDGET(dep_object), sensitive);
        gtk_widget_set_can_focus(GTK_WIDGET(dep_object), sensitive);
        if (!sensitive && hide)
        {
            gtk_widget_hide(GTK_WIDGET(dep_object));
        }
        else
        {
            gtk_widget_show(GTK_WIDGET(dep_object));
        }
    }
}

G_MODULE_EXPORT void
quit_action_cb(GSimpleAction *action, GVariant *param, signal_user_data_t *ud)
{
    gint state = ghb_get_queue_state();
    if (state & (GHB_STATE_WORKING|GHB_STATE_SEARCHING))
    {
        if (ghb_cancel_encode2(ud, _("Closing HandBrake will terminate encoding.\n")))
        {
            ghb_hb_cleanup(FALSE);
            prune_logs(ud);
            g_application_quit(G_APPLICATION(ud->app));
            return;
        }
        return;
    }
    ghb_hb_cleanup(FALSE);
    prune_logs(ud);
    g_application_quit(G_APPLICATION(ud->app));
}

static gboolean
uppers_and_unders(gchar *str)
{
    if (str == NULL) return FALSE;
    str = g_strchomp(g_strchug(str));
    while (*str)
    {
        if (*str == ' ')
        {
            return FALSE;
        }
        if (*str >= 'a' && *str <= 'z')
        {
            return FALSE;
        }
        str++;
    }
    return TRUE;
}

enum
{
    CAMEL_FIRST_UPPER,
    CAMEL_OTHER
};

static void
camel_convert(gchar *str)
{
    gint state = CAMEL_OTHER;

    if (str == NULL) return;
    while (*str)
    {
        if (*str == '_') *str = ' ';
        switch (state)
        {
            case CAMEL_OTHER:
            {
                if (*str >= 'A' && *str <= 'Z')
                    state = CAMEL_FIRST_UPPER;
                else
                    state = CAMEL_OTHER;

            } break;
            case CAMEL_FIRST_UPPER:
            {
                if (*str >= 'A' && *str <= 'Z')
                    *str = *str - 'A' + 'a';
                else
                    state = CAMEL_OTHER;
            } break;
        }
        str++;
    }
}

#if defined(_WIN32)
static gchar*
get_dvd_device_name(gchar *device)
{
    return g_strdup(device);
}
#else
static gchar*
get_dvd_device_name(GDrive *gd)
{
    return g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
}
#endif

static GHashTable *volname_hash = NULL;
#if GLIB_CHECK_VERSION(2, 32, 0)
static GMutex     volname_mutex_static;
#endif
static GMutex     *volname_mutex;

static void
free_volname_key(gpointer data)
{
    if (data != NULL)
        g_free(data);
}

static void
free_volname_value(gpointer data)
{
    if (data != NULL)
        g_free(data);
}

#if defined(_WIN32)
static gchar*
get_direct_dvd_volume_name(const gchar *drive)
{
    gchar *result = NULL;
    gchar vname[51], fsname[51];

    if (GetVolumeInformation(drive, vname, 50, NULL, NULL, NULL, fsname, 50))
    {
        result = g_strdup_printf("%s", vname);
    }
    return result;
}
#else
static gchar*
get_direct_dvd_volume_name(const gchar *drive)
{
    gchar *result;

    result = ghb_dvd_volname (drive);
    return result;
}
#endif

static gchar*
get_dvd_volume_name(gpointer gd)
{
    gchar *label = NULL;
    gchar *result;
    gchar *drive;

    drive = get_dvd_device_name(gd);
    g_mutex_lock(volname_mutex);
    label = g_strdup(g_hash_table_lookup(volname_hash, drive));
    g_mutex_unlock(volname_mutex);
    if (label != NULL)
    {
        if (uppers_and_unders(label))
        {
            camel_convert(label);
        }
        result = g_strdup_printf("%s (%s)", label, drive);
        g_free(label);
    }
    else
    {
        result = g_strdup_printf("%s", drive);
    }
    g_free(drive);
    return result;
}

void
ghb_volname_cache_init(void)
{
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_mutex_init(&volname_mutex_static);
    volname_mutex = &volname_mutex_static;
#else
    volname_mutex = g_mutex_new();
#endif
    volname_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                        free_volname_key, free_volname_value);
}

static void
free_drive(gpointer drive)
{
#if defined(_WIN32)
        g_free(drive);
#else
        g_object_unref(drive);
#endif
}

gpointer
ghb_cache_volnames(signal_user_data_t *ud)
{
    GList *link, *drives;

    ghb_log_func();
    link = drives = dvd_device_list();
    if (drives == NULL)
        return NULL;

    g_mutex_lock(volname_mutex);
    g_hash_table_remove_all(volname_hash);
    while (link != NULL)
    {
        gchar *name, *drive;

#if !defined(_WIN32)
        if (!g_drive_has_media (link->data))
        {
            g_object_unref(link->data);
            link = link->next;
            continue;
        }
#endif
        drive = get_dvd_device_name(link->data);
        name = get_direct_dvd_volume_name(drive);

        if (drive != NULL && name != NULL)
        {
            g_hash_table_insert(volname_hash, drive, name);
        }
        else
        {
            if (drive != NULL)
                g_free(drive);
            if (name != NULL)
                g_free(name);
        }

        free_drive(link->data);
        link = link->next;
    }
    g_mutex_unlock(volname_mutex);

    g_list_free(drives);

    g_idle_add((GSourceFunc)ghb_file_menu_add_dvd, ud);

    return NULL;
}

static const gchar*
get_extension(signal_user_data_t *ud, GhbValue *settings)
{
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    if ((mux->format & HB_MUX_MASK_MP4) &&
        ghb_dict_get_bool(ud->prefs, "UseM4v"))
    {
        return "m4v";
    }
    return mux->default_extension;
}

gboolean
ghb_check_name_template (signal_user_data_t *ud, const char *str)
{
    if (ghb_dict_get_bool(ud->prefs, "auto_name"))
    {
        const gchar *template;

        template = ghb_dict_get_string(ud->prefs, "auto_name_template");
        if (strstr(template, str) != NULL)
            return TRUE;
    }
    return FALSE;
}

typedef struct {
    const char *pattern;
    const char *format;
} datemap;

static int
parse_datestring(const char *src, struct tm *tm)
{
    datemap ymdThmsZ = {"[0-9]{4}-[0-1]?[0-9]-[0-3]?[0-9]T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", "%Y-%m-%dT%H:%M:%SZ"};

    datemap maps[1] = { ymdThmsZ };

    for (int i = 0; i < sizeof(maps); i++)
    {
        if (hb_validate_param_string(maps[i].pattern, src))
        {
            av_small_strptime(src, maps[i].format, tm);
            return 1;
        }
    }
    return 0;
}

static char*
get_creation_date(const char *pattern, const char *metaValue, const char *file)
{
    char date[11] = "";
    if (metaValue != NULL && strlen(metaValue) > 1)
    {
        struct tm tm;
        if (parse_datestring(metaValue, &tm))
        {
            strftime(date, 11, pattern, &tm);
        }
    }
    else
    {
        struct stat stbuf;
        if (g_stat(file, &stbuf) == 0){
            struct tm *tm;
            tm = localtime(&(stbuf.st_mtime));
            strftime(date, 11, pattern, tm);
        }
    }
    return strdup(date);
}

static void
make_unique_dest(const gchar *dest_dir, GString *str, const gchar * extension)
{
    GString * uniq = g_string_new(str->str);
    int       copy = 0;

    g_string_printf(uniq, "%s/%s.%s", dest_dir, str->str, extension);
    while (g_file_test(uniq->str, G_FILE_TEST_EXISTS))
    {
        g_string_printf(uniq, "%s/%s (%d).%s", dest_dir, str->str, ++copy, extension);
    }
    if (copy)
    {
        g_string_append_printf(str, " (%d)", copy);
    }
    g_string_free(uniq, TRUE);
}

/* TODO: It would be better to cache this rather than query
 * the file system every time the output name changes */
static GDateTime *
get_file_modification_date_time (const char *filename)
{
    GFile *file;
    GFileInfo *info;
    GDateTime *datetime = NULL;

    file = g_file_new_for_path(filename);
    info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                             G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (info != NULL)
    {
#if GLIB_CHECK_VERSION(2, 62, 0)
        datetime = g_file_info_get_modification_date_time(info);
#else
        GTimeVal tv;
        g_file_info_get_modification_time(info, &tv);
        datetime = g_date_time_new_from_timeval_utc(&tv);
#endif
        g_object_unref(info);
    }
    g_object_unref(file);
    return datetime;
}

static char *
get_file_modification_date (const char *filename)
{
    GDateTime *datetime;
    char *result = NULL;

    datetime = get_file_modification_date_time(filename);
    if (datetime != NULL)
    {
        result = g_date_time_format(datetime, "%Y-%m-%d");
        g_date_time_unref(datetime);
    }
    return result;
}

static char *
get_file_modification_time (const char *filename)
{
    char *result = NULL;
    GDateTime *datetime;

    datetime = get_file_modification_date_time(filename);
    if (datetime != NULL)
    {
        result = g_date_time_format(datetime, "%H:%M");
        g_date_time_unref(datetime);
    }
    return result;
}

static void
set_destination_settings(signal_user_data_t *ud, GhbValue *settings)
{
    const gchar *extension, *dest_file, *dest_dir;
    gchar *filename;

    extension = get_extension(ud, settings);

    ghb_log_func();
    dest_file = ghb_dict_get_string(ud->settings, "dest_file");
    if (dest_file == NULL)
    {
        // Initialize destination filename if it has no value yet.
        // If auto-naming is disabled, this will be the default filename.
        GString *str = g_string_new("");
        const gchar *vol_name;
        vol_name = ghb_dict_get_string(settings, "volume");
        g_string_append_printf(str, "%s", vol_name);
        g_string_append_printf(str, ".%s", extension);
        filename = g_string_free(str, FALSE);
        ghb_dict_set_string(settings, "dest_file", filename);
        g_free(filename);
    }
    ghb_dict_set(settings, "dest_dir", ghb_value_dup(
                 ghb_dict_get_value(ud->prefs, "destination_dir")));
    if (ghb_dict_get_bool(ud->prefs, "auto_name"))
    {
        GString *str = g_string_new("");
        const gchar *p;

        p = ghb_dict_get_string(ud->prefs, "auto_name_template");
        // {source-path} is only allowed as the first element of the
        // template since the path must come first in the filename
        if (p != NULL &&
            (!strncasecmp(p, "{source-path}", strlen("{source-path}")) ||
             !strncasecmp(p, "{source_path}", strlen("{source_path}"))))
        {
            const gchar * source;

            source = ghb_dict_get_string(ud->globals, "scan_source");
            if (source != NULL)
            {
                char * dirname = g_path_get_dirname(source);
                // if dirname is a directory and it is writable...
                if (dirname != NULL &&
                    g_file_test(dirname, G_FILE_TEST_IS_DIR) &&
                    access(dirname, W_OK) == 0)
                {
                    ghb_dict_set_string(settings, "dest_dir", dirname);
                }
                g_free(dirname);
            }
            p += strlen("{source-path}");
        }
        while (*p)
        {
            if (!strncasecmp(p, "{source}", strlen("{source}")))
            {
                const gchar *vol_name;
                vol_name = ghb_dict_get_string(settings, "volume");
                g_string_append_printf(str, "%s", vol_name);
                p += strlen("{source}");
            }
            else if (!strncasecmp(p, "{title}", strlen("{title}")))
            {
                gint title = ghb_dict_get_int(settings, "title");
                if (title >= 0)
                    g_string_append_printf(str, "%d", title);
                p += strlen("{title}");
            }
            else if (!strncasecmp(p, "{width}", strlen("{width}")))
            {
                int width = ghb_dict_get_int(settings, "scale_width");
                g_string_append_printf(str, "%d", width);
                p += strlen("{width}");
            }
            else if (!strncasecmp(p, "{height}", strlen("{height}")))
            {
                int height = ghb_dict_get_int(settings, "scale_height");
                g_string_append_printf(str, "%d", height);
                p += strlen("{height}");
            }
            else if (!strncasecmp(p, "{preset}", strlen("{preset}")))
            {
                const gchar *preset_name;
                preset_name = ghb_dict_get_string(settings, "PresetName");
                g_string_append_printf(str, "%s", preset_name);
                p += strlen("{preset}");
            }
            else if (!strncasecmp(p, "{chapters}", strlen("{chapters}")))
            {
                if (ghb_settings_combo_int(settings, "PtoPType") == 0)
                {
                    gint start, end;
                    start = ghb_dict_get_int(settings, "start_point");
                    end = ghb_dict_get_int(settings, "end_point");
                    if (start == end)
                        g_string_append_printf(str, "%d", start);
                    else
                        g_string_append_printf(str, "%d-%d", start, end);
                }
                p += strlen("{chapters}");
            }
            else if (!strncasecmp(p, "{time}", strlen("{time}")))
            {
                char st[6];
                struct tm *lt;
                time_t t = time(NULL);
                lt = localtime(&t);
                st[0] = 0;
                strftime(st, 6, "%H:%M", lt);
                g_string_append_printf(str, "%s", st);
                p += strlen("{time}");
            }
            else if (!strncasecmp(p, "{date}", strlen("{date}")))
            {
                char dt[11];
                struct tm *lt;
                time_t t = time(NULL);
                lt = localtime(&t);
                dt[0] = 0;
                strftime(dt, 11, "%Y-%m-%d", lt);
                g_string_append_printf(str, "%s", dt);
                p += strlen("{date}");
            }
            else if (!strncasecmp(p, "{creation-date}", strlen("{creation-date}")))
            {
                gchar *val;
                const gchar *source = ghb_dict_get_string(ud->globals, "scan_source");
                val = get_creation_date("%Y-%m-%d", ghb_dict_get_string(settings, "MetaReleaseDate"), source);
                g_string_append_printf(str, "%s", val);
                p += strlen("{creation-date}");
                g_free(val);
            }
            else if (!strncasecmp(p, "{creation-time}", strlen("{creation-time}")))
            {
                gchar *val;
                const gchar *source = ghb_dict_get_string(ud->globals, "scan_source");
                val = get_creation_date("%H:%M", ghb_dict_get_string(settings, "MetaReleaseDate"), source);
                g_string_append_printf(str, "%s", val);
                p += strlen("{creation-time}");
                g_free(val);
            }
            else if (!strncasecmp(p, "{modification-date}", strlen("{modification-date}")))
            {
                gchar *val;
                const gchar *source = ghb_dict_get_string(ud->globals, "scan_source");
                val = get_file_modification_date(source);
                if (val != NULL)
                    g_string_append_printf(str, "%s", val);
                p += strlen("{modification-date}");
                g_free(val);
            }
            else if (!strncasecmp(p, "{modification-time}", strlen("{modification-time}")))
            {
                gchar *val;
                const gchar *source = ghb_dict_get_string(ud->globals, "scan_source");
                val = get_file_modification_time(source);
                if (val != NULL)
                    g_string_append_printf(str, "%s", val);
                p += strlen("{modification-time}");
                g_free(val);
            }
            else if (!strncasecmp(p, "{quality}", strlen("{quality}")))
            {
                if (ghb_dict_get_bool(settings, "vquality_type_constant"))
                {
                    gint vcodec;
                    const char *vqname;
                    double vquality;
                    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
                    vqname = hb_video_quality_get_name(vcodec);
                    vquality = ghb_dict_get_double(settings, "VideoQualitySlider");
                    g_string_append_printf(str, "%s%.3g", vqname, vquality);
                }
                p += strlen("{quality}");
            }
            else if (!strncasecmp(p, "{bitrate}", strlen("{bitrate}")))
            {
                if (ghb_dict_get_bool(settings, "vquality_type_bitrate"))
                {
                    int vbitrate;
                    vbitrate = ghb_dict_get_int(settings, "VideoAvgBitrate");
                    g_string_append_printf(str, "%dkbps", vbitrate);
                }
                p += strlen("{bitrate}");
            }
            else if (!strncasecmp(p, "{codec}", strlen("{codec}")))
            {
                int vcodec;
                const char *codec;
                vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
                codec = hb_video_encoder_get_name(vcodec);
                g_string_append_len(str, codec, strcspn(codec, " ("));

                p += strlen("{codec}");
            }
            else if (!strncasecmp(p, "{bit-depth}", strlen("{bit-depth}")))
            {
                int vcodec, bit_depth;
                vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
                bit_depth = hb_video_encoder_get_depth(vcodec);
                g_string_append_printf(str, "%d", bit_depth);

                p += strlen("{bit-depth}");
            }
            else
            {
                g_string_append_printf(str, "%c", *p);
                p++;
            }
        }
        dest_dir = ghb_dict_get_string(settings, "dest_dir");
        make_unique_dest(dest_dir, str, extension);
        g_string_append_printf(str, ".%s", extension);
        filename = g_string_free(str, FALSE);
        ghb_dict_set_string(settings, "dest_file", filename);
        g_free(filename);
    }
}

void
ghb_set_destination (signal_user_data_t *ud)
{
    set_destination_settings(ud, ud->settings);
    ghb_ui_update(ud, "dest_file",
        ghb_dict_get_value(ud->settings, "dest_file"));
}

static void
source_dialog_drive_list (GtkFileChooser *chooser, signal_user_data_t *ud)
{
    GList *drives, *link;
    guint length, ii = 1;
    gchar **entries;

    ghb_log_func();
    link = drives = dvd_device_list();
    if (!link)
    {
        has_drive = FALSE;
        return;
    }
    has_drive = TRUE;

    length = g_list_length(drives);
    entries = g_malloc0_n(length + 2, sizeof(char*));
    entries[0] = g_strdup(_("Not Selected"));
    while (link && ii <= length)
    {
        gchar *name = get_dvd_device_name(link->data);
        entries[ii] = name;
        link = link->next;
        ii++;
    }
    g_list_free(drives);
    gtk_file_chooser_add_choice(chooser, "drive", _("Detected DVD devices:"),
                                (const gchar **) entries, (const gchar **) entries);
    gtk_file_chooser_set_choice(chooser, "drive", _("Not Selected"));
    g_strfreev(entries);
}

void ghb_break_pts_duration(gint64 ptsDuration, gint *hh, gint *mm, gdouble *ss)
{
    *hh = ptsDuration / (90000 * 60 * 60);
    ptsDuration -= *hh * 90000 * 60 * 60;
    *mm = ptsDuration / (90000 * 60);
    ptsDuration -= *mm * 90000 * 60;
    *ss = (gdouble)ptsDuration / 90000;
}

void ghb_break_duration(gint64 duration, gint *hh, gint *mm, gint *ss)
{
    *hh = duration / (60*60);
    *mm = (duration / 60) % 60;
    *ss = duration % 60;
}

static gint64
title_range_get_duration (GhbValue * settings, const hb_title_t * title)
{
    gint64 start, end;

    if (ghb_settings_combo_int(settings, "PtoPType") == 0)
    {
        start = ghb_dict_get_int(settings, "start_point");
        end = ghb_dict_get_int(settings, "end_point");
        return ghb_chapter_range_get_duration(title, start, end) / 90000;
    }
    else if (ghb_settings_combo_int(settings, "PtoPType") == 1)
    {
        start = ghb_dict_get_int(settings, "start_point");
        end = ghb_dict_get_int(settings, "end_point");
        return end - start;
    }
    else if (ghb_settings_combo_int(settings, "PtoPType") == 2)
    {
        if (title != NULL)
        {
            gint64 frames;

            start = ghb_dict_get_int(settings, "start_point");
            end = ghb_dict_get_int(settings, "end_point");
            frames = end - start + 1;
            return frames * title->vrate.den / title->vrate.num;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}

static void
update_title_duration(signal_user_data_t *ud)
{
    gint hh, mm, ss;
    gint64 duration;
    gchar *text;
    GtkWidget *widget;
    int title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    widget = GHB_WIDGET (ud->builder, "title_duration");

    duration = title_range_get_duration(ud->settings, title);
    ghb_break_duration(duration, &hh, &mm, &ss);

    text = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);
}

void ghb_show_container_options(signal_user_data_t *ud)
{
    GtkWidget *w1, *w2, *w3;
    w1 = GHB_WIDGET(ud->builder, "AlignAVStart");
    w2 = GHB_WIDGET(ud->builder, "Mp4HttpOptimize");
    w3 = GHB_WIDGET(ud->builder, "Mp4iPodCompatible");

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    gint enc = ghb_settings_video_encoder_codec(ud->settings, "VideoEncoder");

    gtk_widget_set_visible(w1, (mux->format & HB_MUX_MASK_MP4));
    gtk_widget_set_visible(w2, (mux->format & HB_MUX_MASK_MP4));
    gtk_widget_set_visible(w3, (mux->format & HB_MUX_MASK_MP4) &&
                               (enc == HB_VCODEC_X264_8BIT));
}

static void
adjustment_configure(
    GtkAdjustment *adj,
    double val,
    double min, double max,
    double step, double page, double page_sz)
{
    gtk_adjustment_configure(adj, val, min, max, step, page, page_sz);
}

static void
spin_configure(signal_user_data_t *ud, char *name, double val, double min, double max)
{
    GtkSpinButton *spin;
    GtkAdjustment *adj;
    double step, page, page_sz;

    spin = GTK_SPIN_BUTTON(GHB_WIDGET(ud->builder, name));

    adj = gtk_spin_button_get_adjustment(spin);
    step = gtk_adjustment_get_step_increment(adj);
    page = gtk_adjustment_get_page_increment(adj);
    page_sz = gtk_adjustment_get_page_size(adj);

    adjustment_configure(adj, val, min, max, step, page, page_sz);
}

void
ghb_scale_configure(
    signal_user_data_t *ud,
    char *name,
    double val, double min, double max,
    double step, double page,
    int digits, gboolean inverted)
{
    GtkScale *scale;
    GtkAdjustment *adj;
    double page_sz;

    scale = GTK_SCALE(GHB_WIDGET(ud->builder, name));

    gtk_scale_set_draw_value(scale, FALSE);
    adj = gtk_range_get_adjustment(GTK_RANGE(scale));
    page_sz = gtk_adjustment_get_page_size(adj);

    adjustment_configure(adj, val, min, max, step, page, page_sz);

    gtk_scale_set_digits(scale, digits);
    gtk_range_set_inverted(GTK_RANGE(scale), inverted);
    gtk_scale_set_draw_value(scale, TRUE);
}

static void
set_widget_ranges (signal_user_data_t *ud, GhbValue *settings)
{
    int title_id, titleindex;
    const hb_title_t * title;
    double val;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    // Reconfigure the UI combo boxes
    ghb_update_ui_combo_box(ud, "AudioTrack", title, FALSE);
    ghb_update_ui_combo_box(ud, "SubtitleTrack", title, FALSE);

    if (title != NULL)
    {

        // Set the limits of cropping.  hb_set_anamorphic_size crashes if
        // you pass it a cropped width or height == 0.
        gint vbound, hbound;
        vbound = title->geometry.height;
        hbound = title->geometry.width;

        val = ghb_dict_get_int(ud->settings, "PictureTopCrop");
        spin_configure(ud, "PictureTopCrop", val, 0, vbound);
        val = ghb_dict_get_int(ud->settings, "PictureBottomCrop");
        spin_configure(ud, "PictureBottomCrop", val, 0, vbound);
        val = ghb_dict_get_int(ud->settings, "PictureLeftCrop");
        spin_configure(ud, "PictureLeftCrop", val, 0, hbound);
        val = ghb_dict_get_int(ud->settings, "PictureRightCrop");
        spin_configure(ud, "PictureRightCrop", val, 0, hbound);

        gint duration = title->duration / 90000;

        if (ghb_settings_combo_int(ud->settings, "PtoPType") == 0)
        {
            GhbValue *chapter_list = ghb_get_job_chapter_list(settings);
            gint num_chapters = ghb_array_len(chapter_list);

            val = ghb_dict_get_int(ud->settings, "start_point");
            spin_configure(ud, "start_point", val, 1, num_chapters);
            val = ghb_dict_get_int(ud->settings, "end_point");
            spin_configure(ud, "end_point", val, 1, num_chapters);
        }
        else if (ghb_settings_combo_int(ud->settings, "PtoPType") == 1)
        {
            val = ghb_dict_get_int(ud->settings, "start_point");
            spin_configure(ud, "start_point", val, 0, duration * 2 - 1);
            val = ghb_dict_get_int(ud->settings, "end_point");
            spin_configure(ud, "end_point", val, 0, duration * 2);
        }
        else if (ghb_settings_combo_int(ud->settings, "PtoPType") == 2)
        {
            gdouble max_frames;
            max_frames = (gdouble)duration *
                         title->vrate.num / title->vrate.den;

            val = ghb_dict_get_int(ud->settings, "start_point");
            spin_configure(ud, "start_point", val, 1, max_frames * 2);
            val = ghb_dict_get_int(ud->settings, "end_point");
            spin_configure(ud, "end_point", val, 1, max_frames * 2);
        }

        val = ghb_dict_get_int(ud->settings, "angle");
        spin_configure(ud, "angle", val, 1, title->angle_count);
    }

    float vqmin, vqmax, step, page;
    int inverted, digits;

    ghb_vquality_range(ud, &vqmin, &vqmax, &step, &page, &digits, &inverted);
    val = ghb_dict_get_double(ud->settings, "VideoQualitySlider");
    ghb_scale_configure(ud, "VideoQualitySlider", val, vqmin, vqmax,
                        step, page, digits, inverted);
}

static void
check_chapter_markers(signal_user_data_t *ud)
{
    GtkWidget *widget;
    gint start, end;

    if (ghb_settings_combo_int(ud->settings, "PtoPType") == 0)
    {
        start = ghb_dict_get_int(ud->settings, "start_point");
        end = ghb_dict_get_int(ud->settings, "end_point");
        widget = GHB_WIDGET (ud->builder, "ChapterMarkers");
        gtk_widget_set_sensitive(widget, end > start);
        widget = GHB_WIDGET (ud->builder, "chapters_list");
        gtk_widget_set_sensitive(widget, end > start);
    }
}

void
ghb_load_settings(signal_user_data_t * ud)
{
    const char      * fullname;
    int               type;
    gboolean          preset_modified;
    static gboolean   busy = FALSE;

    if (busy)
        return;
    busy = TRUE;

    fullname        = ghb_dict_get_string(ud->settings, "PresetFullName");
    type            = ghb_dict_get_int(ud->settings, "Type");
    preset_modified = ghb_dict_get_bool(ud->settings, "preset_modified");
    if (preset_modified)
    {
        ghb_clear_presets_selection(ud);
        ghb_preset_menu_button_refresh(ud, fullname, type);
    }
    else
    {
        ghb_dict_set_bool(ud->settings, "preset_reload", TRUE);
        ghb_select_preset(ud, fullname, type);
        ghb_dict_set_bool(ud->settings, "preset_reload", FALSE);
    }

    busy = FALSE;

    ghb_load_post_settings(ud);
}

void
ghb_load_post_settings(signal_user_data_t * ud)
{
    static gboolean busy = FALSE;
    if (busy)
        return;
    busy = TRUE;

    ud->dont_clear_presets = TRUE;
    ud->scale_busy = TRUE;

    set_widget_ranges(ud, ud->settings);
    ghb_check_all_dependencies(ud);
    ghb_show_container_options(ud);
    check_chapter_markers(ud);

    ghb_clear_audio_selection(ud->builder);
    ghb_clear_subtitle_selection(ud->builder);
    ghb_settings_to_ui(ud, ud->settings);
    ghb_audio_defaults_to_ui(ud);
    ghb_subtitle_defaults_to_ui(ud);
    ghb_audio_list_refresh_all(ud);
    ghb_subtitle_list_refresh_all(ud);
    ghb_chapter_list_refresh_all(ud);
    update_title_duration(ud);
    ghb_update_title_info(ud);

    ud->dont_clear_presets = FALSE;
    ud->scale_busy = FALSE;
    busy = FALSE;

    ghb_picture_settings_deps(ud);
}

static void
show_scan_progress(signal_user_data_t *ud)
{
    GtkWidget      * widget;
    GtkProgressBar * progress;
    GtkLabel       * label;

    widget = GHB_WIDGET(ud->builder, "SourceInfoBox");
    gtk_widget_hide(widget);

    widget = GHB_WIDGET(ud->builder, "SourceScanBox");
    gtk_widget_show(widget);

    progress = GTK_PROGRESS_BAR(GHB_WIDGET(ud->builder, "scan_prog"));
    gtk_progress_bar_set_fraction(progress, 0);

    label = GTK_LABEL(GHB_WIDGET(ud->builder, "source_scan_label"));
    gtk_label_set_text( label, _("Scanning ...") );

}

static void
hide_scan_progress(signal_user_data_t *ud)
{
    GtkWidget      * widget;
    GtkProgressBar * progress;

    progress = GTK_PROGRESS_BAR(GHB_WIDGET(ud->builder, "scan_prog"));
    gtk_progress_bar_set_fraction(progress, 1.0);

    widget = GHB_WIDGET(ud->builder, "SourceScanBox");
    gtk_widget_hide(widget);

    widget = GHB_WIDGET(ud->builder, "SourceInfoBox");
    gtk_widget_show(widget);
}

static void
start_scan(
    signal_user_data_t *ud,
    const gchar *path,
    gint title_id,
    gint preview_count)
{
    GtkWidget *widget;
    ghb_status_t status;

    ghb_get_status(&status);
    if (status.scan.state != GHB_STATE_IDLE)
        return;

    widget = GHB_WIDGET(ud->builder, "sourcetoolbutton");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop Scan"));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Scan"));
    ghb_backend_scan(path, title_id, preview_count,
            90000L * ghb_dict_get_int(ud->prefs, "MinTitleDuration"));
}

gboolean
ghb_idle_scan(signal_user_data_t *ud)
{
    gchar *path;
    // ghb_do_scan replaces "scan_source" key in dict, so we must
    // make a copy of the string.
    path = g_strdup(ghb_dict_get_string(ud->globals, "scan_source"));
    ghb_do_scan(ud, path, 0, TRUE);
    g_free(path);
    return FALSE;
}

extern GhbValue *ghb_queue_edit_settings;
static gchar *last_scan_file = NULL;

void
ghb_do_scan(
    signal_user_data_t *ud,
    const gchar *filename,
    gint title_id,
    gboolean force)
{
    int titleindex;
    const hb_title_t *title;

    (void)title; // Silence "unused variable" warning

    ghb_log_func();
    if (!force && last_scan_file != NULL &&
        strcmp(last_scan_file, filename) == 0)
    {
        if (ghb_queue_edit_settings != NULL)
        {
            title_id = ghb_dict_get_int(ghb_queue_edit_settings, "title");
            title = ghb_lookup_title(title_id, &titleindex);
            ghb_array_replace(ud->settings_array, titleindex,
                              ghb_queue_edit_settings);
            ud->settings = ghb_queue_edit_settings;
            ghb_load_settings(ud);
            ghb_queue_edit_settings = NULL;
        }
        else
        {
            title = ghb_lookup_title(title_id, &titleindex);
            load_all_titles(ud, titleindex);
        }
        return;
    }
    if (last_scan_file != NULL)
        g_free(last_scan_file);
    last_scan_file = NULL;
    if (filename != NULL)
    {
        const gchar *path;
        gint preview_count;

        last_scan_file = g_strdup(filename);
        ghb_dict_set_string(ud->globals, "scan_source", filename);

        show_scan_progress(ud);
        path = ghb_dict_get_string(ud->globals, "scan_source");
        prune_logs(ud);

        preview_count = ghb_dict_get_int(ud->prefs, "preview_count");
        start_scan(ud, path, title_id, preview_count);
    }
}

static gint
single_title_dialog (signal_user_data_t *ud)
{
    GtkWidget *dialog, *spin, *msg;
    GtkAdjustment *adj;
    int result;

    dialog = gtk_message_dialog_new(GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window")),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_OK,
                                    "Title Number:");

    adj = gtk_adjustment_new(1, 0, 100, 1, 10, 10);
    spin = gtk_spin_button_new(adj, 1, 0);
    gtk_widget_show(spin);
    msg = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
    gtk_box_pack_end(GTK_BOX(msg), spin, FALSE, FALSE, 0);
    gtk_dialog_run(GTK_DIALOG(dialog));
    result = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    gtk_widget_destroy(dialog);
    return result;
}

static void
source_dialog_response_cb(GtkFileChooser *chooser,
                          GtkResponseType response, signal_user_data_t *ud)
{
    const gchar *sourcename;
    const gchar *drivename = NULL;
    GFile *file;
    gchar *filename;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        if (has_drive)
            drivename = gtk_file_chooser_get_choice(chooser, "drive");

        if (drivename && g_strcmp0(drivename, _("Not Selected")))
            filename = g_strdup(drivename);
        else
        {
            file = gtk_file_chooser_get_file(chooser);
            filename = g_file_get_path(file);
            g_object_unref(file);
        }

        if (filename != NULL)
        {
            gint title_id = 0;

            if (g_strcmp0(gtk_file_chooser_get_choice(chooser, "single"), "true") == 0)
                title_id = single_title_dialog(ud);
            sourcename = ghb_dict_get_string(ud->globals, "scan_source");

            // ghb_do_scan replaces "scan_source" key in dict, so we must
            // be finished with sourcename before calling ghb_do_scan
            // since the memory it references will be freed
            if (strcmp(sourcename, filename) != 0)
            {
                ghb_dict_set_string(ud->prefs, "default_source", filename);
                ghb_pref_save(ud->prefs, "default_source");
                ghb_dvd_set_current(filename, ud);
            }
            ghb_do_scan(ud, filename, title_id, TRUE);
            g_free(filename);
        }
    }
    ud->source_dialog = NULL;
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

static void
do_source_dialog(gboolean dir, signal_user_data_t *ud)
{
    GtkFileChooserNative *chooser;
    GtkWindow *hb_window;
    const gchar *sourcename;

    if (ud->source_dialog)
    return;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    chooser = ud->source_dialog = gtk_file_chooser_native_new(
                dir ? _("Open Source Directory") : _("Open Source"),
                hb_window,
                dir ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
                GHB_STOCK_OPEN,
                GHB_STOCK_CANCEL);

    ghb_log_func();
    sourcename = ghb_dict_get_string(ud->globals, "scan_source");

    if (!dir)
        add_video_file_filters(GTK_FILE_CHOOSER(chooser), ud);

    source_dialog_drive_list(GTK_FILE_CHOOSER(chooser), ud);

    gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(chooser), "single",
                                _("Single Title"), NULL, NULL);

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    gtk_native_dialog_set_transient_for(GTK_NATIVE_DIALOG(chooser), hb_window);
    g_signal_connect(chooser, "response", G_CALLBACK(source_dialog_response_cb), ud);
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(chooser), sourcename);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

#if 0
G_MODULE_EXPORT void
source_button_clicked_cb(GtkButton *button, signal_user_data_t *ud)
{
    ghb_status_t status;
    ghb_get_status(&status);
    if (status.scan.state & GHB_STATE_SCANNING)
    {
        ghb_backend_scan_stop();
    }
    else
    {
        do_source_dialog(FALSE, ud);
    }
}
#else
G_MODULE_EXPORT void
source_action_cb(GSimpleAction *action, GVariant *param,
                 signal_user_data_t *ud)
{
    ghb_status_t status;
    ghb_get_status(&status);
    if (status.scan.state & GHB_STATE_SCANNING)
    {
        ghb_backend_scan_stop();
    }
    else
    {
        do_source_dialog(FALSE, ud);
    }
}
#endif

G_MODULE_EXPORT void
source_dir_action_cb(GSimpleAction *action, GVariant * param,
                       signal_user_data_t *ud)
{
    do_source_dialog(TRUE, ud);
}

G_MODULE_EXPORT void
dvd_source_activate_cb(GSimpleAction *action, GVariant *param,
                       signal_user_data_t *ud)
{
    const gchar *filename;
    const gchar *sourcename;

    // ghb_do_scan replaces "scan_source" key in dict, so we must
    // be finished with sourcename before calling ghb_do_scan
    // since the memory it references will be freed
    sourcename = ghb_dict_get_string(ud->globals, "scan_source");
    filename = g_variant_get_string(param, NULL);
    if (strcmp(sourcename, filename) != 0)
    {
        ghb_dict_set_string(ud->prefs, "default_source", filename);
        ghb_pref_save(ud->prefs, "default_source");
        ghb_dvd_set_current(filename, ud);
    }
    ghb_do_scan(ud, filename, 0, TRUE);
}

void
ghb_update_destination_extension(signal_user_data_t *ud)
{
    static gchar *containers[] = {".mkv", ".mp4", ".m4v", ".webm", ".error", NULL};
    gchar *filename;
    const gchar *extension;
    gint ii;
    GtkEntry *entry;
    static gboolean busy = FALSE;

    ghb_log_func();
    // Since this function modifies the thing that triggers it's
    // invocation, check to see if busy to prevent accidental infinite
    // recursion.
    if (busy)
        return;
    busy = TRUE;
    extension = get_extension(ud, ud->settings);
    entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "dest_file"));
    filename = g_strdup(ghb_editable_get_text(entry));
    for (ii = 0; containers[ii] != NULL; ii++)
    {
        if (g_str_has_suffix(filename, containers[ii]))
        {
            gchar *pos;
            gchar *new_name;

            pos = g_strrstr( filename, "." );
            if (pos == NULL)
            {
                // No period? shouldn't happen
                break;
            }
            *pos = 0;
            if (strcmp(extension, &pos[1]) == 0)
            {
                // Extension is already correct
                break;
            }
            new_name = g_strjoin(".", filename, extension, NULL);
            ghb_ui_update(ud, "dest_file", ghb_string_value(new_name));
            g_free(new_name);
            break;
        }
    }
    g_free(filename);
    busy = FALSE;
}

static void
destination_select_title(GtkEntry *entry)
{
    const gchar *dest;
    gint start, end;

    dest = ghb_editable_get_text(entry);
    for (end = strlen(dest)-1; end > 0; end--)
    {
        if (dest[end] == '.')
        {
            break;
        }
    }
    for (start = end; start >= 0; start--)
    {
        if (dest[start] == G_DIR_SEPARATOR)
        {
            start++;
            break;
        }
    }
    if (start < 0) start = 0;
    if (start < end)
    {
        gtk_editable_select_region(GTK_EDITABLE(entry), start, end);
    }
}

G_MODULE_EXPORT gboolean
destination_grab_cb(
    GtkEntry *entry,
    signal_user_data_t *ud)
{
    destination_select_title(entry);
    return FALSE;
}

static void
update_default_destination(signal_user_data_t *ud)
{
    const gchar *dest_dir, *def_dest;

    dest_dir = ghb_dict_get_string(ud->settings, "dest_dir");
    def_dest = ghb_dict_get_string(ud->prefs, "destination_dir");
    if (dest_dir != NULL && def_dest != NULL && dest_dir[0] != 0 &&
        strcmp(dest_dir, def_dest) != 0)
    {
        ghb_dict_set_string(ud->prefs, "destination_dir", dest_dir);
        ghb_pref_save(ud->prefs, "destination_dir");
    }
}

G_MODULE_EXPORT void
dest_dir_set_cb(GtkFileChooserButton *dest_chooser, signal_user_data_t *ud)
{
    const gchar *dest_file, *dest_dir;
    gchar *dest;

    ghb_log_func();
    ghb_widget_to_setting(ud->settings, (GtkWidget*)dest_chooser);
    dest_file = ghb_dict_get_string(ud->settings, "dest_file");
    dest_dir = ghb_dict_get_string(ud->settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(ud->settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(ud->settings);
    ghb_dict_set_string(dest_dict, "File", dest);
    g_free(dest);
    update_default_destination(ud);
}

G_MODULE_EXPORT void
dest_file_changed_cb(GtkEntry *entry, signal_user_data_t *ud)
{
    const gchar *dest_file, *dest_dir;
    gchar *dest;

    ghb_log_func();
    ghb_update_destination_extension(ud);
    ghb_widget_to_setting(ud->settings, (GtkWidget*)entry);
    // This signal goes off with ever keystroke, so I'm putting this
    // update on the timer.
    dest_file = ghb_dict_get_string(ud->settings, "dest_file");
    dest_dir = ghb_dict_get_string(ud->settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(ud->settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(ud->settings);
    ghb_dict_set_string(dest_dict, "File", dest);
    g_free(dest);
}

static void
destination_response_cb(GtkFileChooserNative *chooser,
                        GtkResponseType response, signal_user_data_t *ud)
{
    GtkEntry *entry;
    gchar *basename;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        GFile *file;
        char *filename, *dirname;
        GtkFileChooser *dest_chooser;

        file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER (chooser));
        filename = g_file_get_path(file);
        basename = g_path_get_basename(filename);
        dirname = g_path_get_dirname(filename);
        entry = (GtkEntry*)GHB_WIDGET(ud->builder, "dest_file");
        ghb_editable_set_text(entry, basename);
        dest_chooser = GTK_FILE_CHOOSER(GHB_WIDGET(ud->builder, "dest_dir"));
        gtk_file_chooser_set_filename(dest_chooser, dirname);
        g_object_unref(file);
        g_free (dirname);
        g_free (basename);
        g_free (filename);
    }
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

G_MODULE_EXPORT void
destination_action_cb(GSimpleAction *action, GVariant *param,
                      signal_user_data_t *ud)
{
    GtkFileChooserNative *chooser;
    GtkWindow *hb_window;
    const gchar *destname;
    gchar *basename;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    destname = ghb_dict_get_string(ud->settings, "destination");
    chooser = gtk_file_chooser_native_new("Choose Destination",
                                          hb_window,
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GHB_STOCK_SAVE,
                                          GHB_STOCK_CANCEL);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), destname);
    basename = g_path_get_basename(destname);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), basename);
    g_free(basename);

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    g_signal_connect(chooser, "response", G_CALLBACK(destination_response_cb), ud);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

G_MODULE_EXPORT gboolean
window_destroy_event_cb(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    ghb_hb_cleanup(FALSE);
    prune_logs(ud);
    g_application_quit(G_APPLICATION(ud->app));
    return FALSE;
}

G_MODULE_EXPORT gboolean
window_delete_event_cb(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    gint state = ghb_get_queue_state();
    if (state & (GHB_STATE_WORKING|GHB_STATE_SEARCHING))
    {
        if (ghb_cancel_encode2(ud,
            _("Closing HandBrake will terminate encoding.\n")))
        {
            ghb_hb_cleanup(FALSE);
            prune_logs(ud);
            g_application_quit(G_APPLICATION(ud->app));
            return FALSE;
        }
        return TRUE;
    }
    ghb_hb_cleanup(FALSE);
    prune_logs(ud);
    g_application_quit(G_APPLICATION(ud->app));
    return FALSE;
}

static void
update_acodec(signal_user_data_t *ud)
{
    ghb_audio_list_refresh_all(ud);
    ghb_grey_combo_options(ud);
}

G_MODULE_EXPORT void
container_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    const char * mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
    GhbValue *dest_dict = ghb_get_job_dest_settings(ud->settings);
    ghb_dict_set_string(dest_dict, "Mux", mux_id);

    const hb_container_t *mux = ghb_lookup_container_by_name(mux_id);
    if (!(mux->format & HB_MUX_MASK_MP4))
    {
        ghb_ui_update(ud, "AlignAVStart", ghb_boolean_value(FALSE));
    }

    ghb_check_dependency(ud, widget, NULL);
    ghb_show_container_options(ud);
    update_acodec(ud);
    ghb_update_destination_extension(ud);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    ghb_subtitle_prune(ud);
    ghb_subtitle_list_refresh_all(ud);
    ghb_audio_list_refresh_selected(ud);
}

static gchar*
get_aspect_string(gint aspect_n, gint aspect_d)
{
    gchar *aspect;

    if (aspect_d < 10)
    {
        aspect = g_strdup_printf("%d:%d", aspect_n, aspect_d);
    }
    else
    {
        gdouble aspect_nf = (gdouble)aspect_n / aspect_d;
        aspect = g_strdup_printf("%.2f:1", aspect_nf);
    }
    return aspect;
}

void
ghb_update_title_info(signal_user_data_t *ud)
{
    GtkWidget           * widget;
    GString             * info;
    gchar               * text;
    gchar               * aspect;
    int                   title_id, titleindex;
    int                   audio_count, subtitle_count;
    const hb_title_t    * title;
    const hb_geometry_t * geo;
    gint                  aspect_n, aspect_d;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
        return;

    update_title_duration(ud);

    geo = &title->geometry;
    hb_reduce(&aspect_n, &aspect_d, geo->width * geo->par.num,
              geo->height * geo->par.den);
    aspect = get_aspect_string(aspect_n, aspect_d);
    audio_count = hb_list_count(title->list_audio);
    subtitle_count = hb_list_count(title->list_subtitle);

    info = g_string_sized_new(256);

    g_string_append_printf(info, ", %dx%d", geo->width, geo->height);
    int screen_width =  geo->width * geo->par.num / geo->par.den;
    if (geo->width != screen_width)
        g_string_append_printf(info, " (%dx%d)", screen_width, geo->height);

    g_string_append_printf(info, ", %s, %.6g %s, ", aspect,
                           (double) title->vrate.num / title->vrate.den,
                           _("FPS"));

    if (title->dovi.dv_profile)
    {
        g_string_append_printf(info, _("Dolby Vision %d.%d"),
                               title->dovi.dv_profile,
                               title->dovi.dv_bl_signal_compatibility_id);
        if (title->hdr_10_plus)
            g_string_append_printf(info, " %s", _("HDR10+"));
    }
    else if (title->hdr_10_plus)
        g_string_append(info, _("HDR10+"));
    else if (title->mastering.has_primaries && title->mastering.has_luminance)
        g_string_append(info, _("HDR10"));
    else if (title->color_transfer == 16 || title->color_transfer == 18)
        g_string_append(info, _("HDR"));
    else
        g_string_append(info, _("SDR"));

    g_string_append(info, " (");

    int bit_depth = hb_get_bit_depth(title->pix_fmt);
    if (bit_depth)
    {
        // TRANSLATORS: This is the bit depth of the video
        g_string_append_printf(info, _("%d-bit"), bit_depth);
        g_string_append_c(info, ' ');
    }

    int h_shift, v_shift, chroma_available;
    chroma_available = hb_get_chroma_sub_sample(title->pix_fmt, &h_shift, &v_shift);
    if (chroma_available == 0)
    {
        int h_value = 4 >> h_shift;
        int v_value = v_shift ? 0 : h_value;
        g_string_append_printf(info, "4:%d:%d", h_value, v_value);
    }

    if (bit_depth || chroma_available == 0)
    {
        g_string_append(info, ", ");
    }

    g_string_append_printf(info, "%d-%d-%d)", title->color_prim,
                           title->color_transfer, title->color_matrix);

    g_string_append_printf(info, ", %d %s", audio_count,
                           ngettext("Audio Track", "Audio Tracks", audio_count));
    if (subtitle_count)
        g_string_append_printf(info, ", %d %s", subtitle_count,
                               ngettext("Subtitle Track", "Subtitle Tracks", subtitle_count));

    widget = GHB_WIDGET(ud->builder, "source_info_label");
    gtk_label_set_text(GTK_LABEL(widget), info->str);
    g_string_free(info, TRUE);

    text = g_strdup_printf("%d x %d", geo->width, geo->height);
    ghb_ui_update(ud, "source_storage_size", ghb_string_value(text));
    g_free(text);

    text = g_strdup_printf("%d x %d", geo->width * geo->par.num / geo->par.den,
                           geo->height);
    ghb_ui_update(ud, "source_display_size", ghb_string_value(text));
    g_free(text);

    ghb_ui_update(ud, "source_aspect_ratio", ghb_string_value(aspect));
    free(aspect);
}

static void update_meta(GhbValue *settings, const char *name, const char *val)
{
    GhbValue *metadata = ghb_get_job_metadata_settings(settings);

    if (val == NULL || val[0] == 0)
        ghb_dict_remove(metadata, name);
    else
        ghb_dict_set_string(metadata, name, val);
}

static void
mini_preview_update (gboolean has_preview, signal_user_data_t *ud)
{
    GtkWidget *widget;

    if (ghb_dict_get_bool(ud->prefs, "ShowMiniPreview") && has_preview)
    {
        widget = GHB_WIDGET(ud->builder, "summary_image");
        gtk_widget_hide(widget);
        widget = GHB_WIDGET(ud->builder, "preview_button_image");
        gtk_widget_show(widget);
    }
    else
    {
        widget = GHB_WIDGET(ud->builder, "summary_image");
        gtk_widget_show(widget);
        widget = GHB_WIDGET(ud->builder, "preview_button_image");
        gtk_widget_hide(widget);
    }
}

void
ghb_update_summary_info(signal_user_data_t *ud)
{
    GString            * str;
    char               * text;
    int                  title_id;
    GtkWidget          * widget;
    GhbValue           * titleDict;

    title_id  = ghb_dict_get_int(ud->settings, "title");
    titleDict = ghb_get_title_dict(title_id);
    if (titleDict == NULL)
    {
        // No title, clear summary
        widget = GHB_WIDGET(ud->builder, "tracks_summary");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "filters_summary");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "dimensions_summary");
        gtk_label_set_text(GTK_LABEL(widget), "--");
        mini_preview_update(FALSE, ud);
        return;
    }
    mini_preview_update(TRUE, ud);

    // Video Track
    const hb_encoder_t * video_encoder;
    const hb_rate_t    * fps;
    hb_rational_t        vrate;
    char               * rate_str;

    str = g_string_new("");
    video_encoder = ghb_settings_video_encoder(ud->settings, "VideoEncoder");
    fps = ghb_settings_video_framerate(ud->settings, "VideoFramerate");
    if (fps->rate == 0)
    {
        hb_dict_extract_rational(&vrate, titleDict, "FrameRate");
    }
    else
    {
        vrate.num = 27000000;
        vrate.den = fps->rate;
    }
    rate_str = g_strdup_printf("%.6g", (gdouble)vrate.num / vrate.den);
    g_string_append_printf(str, "%s, %s %s", video_encoder->name, rate_str, _("FPS"));
    g_free(rate_str);
    if (ghb_dict_get_bool(ud->settings, "VideoFramerateCFR"))
    {
        g_string_append_printf(str, " %s", _("CFR"));
    }
    else if (ghb_dict_get_bool(ud->settings, "VideoFrameratePFR"))
    {
        g_string_append_printf(str, " %s", _("PFR"));
    }
    else if (ghb_dict_get_bool(ud->settings, "VideoFramerateVFR"))
    {
        g_string_append_printf(str, " %s", _("VFR"));
    }

    // Audio Tracks (show at most 3 tracks)
    GhbValue * audioList;
    GhbValue * sourceAudioList;
    int        ii, count, show;

    sourceAudioList = ghb_dict_get(titleDict, "AudioList");
    audioList    = ghb_get_job_audio_list(ud->settings);
    show = count = ghb_array_len(audioList);
    if (count > 3)
    {
        show = 2;
    }
    for (ii = 0; ii < show; ii++)
    {
        GhbValue           * asettings, * asource;
        const hb_mixdown_t * audio_mix;
        const hb_encoder_t * audio_encoder;
        const char         * lang;
        int                  track;

        asettings     = ghb_array_get(audioList, ii);
        track         = ghb_dict_get_int(asettings, "Track");
        asource       = ghb_array_get(sourceAudioList, track);
        lang          = ghb_dict_get_string(asource, "Language");
        audio_encoder = ghb_settings_audio_encoder(asettings, "Encoder");
        if (audio_encoder->codec & HB_ACODEC_PASS_FLAG)
        {
            g_string_append_printf(str, "\n%s, %s", lang, audio_encoder->name);
        }
        else
        {
            audio_mix = ghb_settings_mixdown(asettings, "Mixdown");
            g_string_append_printf(str, "\n%s, %s, %s", lang,
                                   audio_encoder->name, audio_mix->name);
        }
    }
    if (show < count)
    {
        g_string_append_printf(str, "\n");
        g_string_append_printf(str, ngettext("+ %d more audio track",
                                             "+ %d more audio tracks",
                                             count - show),
                               count - show);
    }

    // Subtitle Tracks (show at most 3 tracks)
    GhbValue * subtitleDict;
    GhbValue * searchDict;
    GhbValue * subtitleList;
    GhbValue * sourceSubtitleList;
    gboolean   search;

    sourceSubtitleList = ghb_dict_get(titleDict, "SubtitleList");
    subtitleDict       = ghb_get_job_subtitle_settings(ud->settings);
    subtitleList       = ghb_dict_get(subtitleDict, "SubtitleList");
    searchDict         = ghb_dict_get(subtitleDict, "Search");
    search             = ghb_dict_get_bool(searchDict, "Enable");
    show = count       = ghb_array_len(subtitleList) + search;
    if (count > 3)
    {
        show = 2;
    }
    if (search)
    {
        gboolean force, burn, def;

        force = ghb_dict_get_bool(searchDict, "Forced");
        burn  = ghb_dict_get_bool(searchDict, "Burn");
        def   = ghb_dict_get_bool(searchDict, "Default");

        g_string_append_printf(str, "\n%s", _("Foreign Audio Scan"));
        if (force)
        {
            g_string_append_printf(str, _(", Forced Only"));
        }
        if (burn)
        {
            g_string_append_printf(str, _(", Burned"));
        }
        else if (def)
        {
            g_string_append_printf(str, _(", Default"));
        }
        show--;
        count--;
    }
    for (ii = 0; ii < show; ii++)
    {
        GhbValue           * subsettings, * subsource;
        int                  track;
        char               * desc;
        gboolean             force, burn, def;

        subsettings = ghb_array_get(subtitleList, ii);
        track       = ghb_dict_get_int(subsettings, "Track");
        subsource   = ghb_array_get(sourceSubtitleList, track);
        desc        = ghb_subtitle_short_description(subsource, subsettings);
        force       = ghb_dict_get_bool(subsettings, "Forced");
        burn        = ghb_dict_get_bool(subsettings, "Burn");
        def         = ghb_dict_get_bool(subsettings, "Default");

        g_string_append_printf(str, "\n%s", desc);
        free(desc);
        if (force)
        {
            g_string_append_printf(str, _(", Forced Only"));
        }
        if (burn)
        {
            g_string_append_printf(str, _(", Burned"));
        }
        else if (def)
        {
            g_string_append_printf(str, _(", Default"));
        }
    }
    if (show < count)
    {
        g_string_append_printf(str, "\n");
        g_string_append_printf(str, ngettext("+ %d more subtitle track",
                                       "+ %d more subtitle tracks",
                                       count - show),
                               count - show);
    }

    if (ghb_dict_get_bool(ud->settings, "ChapterMarkers"))
    {
        g_string_append_printf(str, "\n%s", _("Chapter Markers"));
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "tracks_summary");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    // Filters
    gboolean detel, comb_detect, yadif, decomb, deblock, nlmeans, denoise, bwdif;
    gboolean unsharp, lapsharp, hflip, rot, gray, colorspace, chroma_smooth;
    const char * sval;

    sval        = ghb_dict_get_string(ud->settings, "PictureDetelecine");
    detel       = sval != NULL && !!strcasecmp(sval, "off");
    sval        = ghb_dict_get_string(ud->settings, "PictureCombDetectPreset");
    comb_detect = sval != NULL && !!strcasecmp(sval, "off");
    sval        = ghb_dict_get_string(ud->settings, "PictureDeinterlaceFilter");
    yadif       = sval != NULL && !strcasecmp(sval, "deinterlace");
    bwdif       = sval != NULL && !strcasecmp(sval, "bwdif");
    decomb      = sval != NULL && !strcasecmp(sval, "decomb");
    sval        = ghb_dict_get_string(ud->settings, "PictureDeblockPreset");
    deblock     = sval != NULL && !!strcasecmp(sval, "off");
    sval        = ghb_dict_get_string(ud->settings, "PictureDenoiseFilter");
    nlmeans     = sval != NULL && !strcasecmp(sval, "nlmeans");
    denoise     = sval != NULL && !strcasecmp(sval, "hqdn3d");
    sval        = ghb_dict_get_string(ud->settings, "PictureSharpenFilter");
    unsharp     = sval != NULL && !strcasecmp(sval, "unsharp");
    lapsharp    = sval != NULL && !strcasecmp(sval, "lapsharp");
    hflip       = ghb_dict_get_bool(ud->settings, "hflip");
    sval        = ghb_dict_get_string(ud->settings, "rotate");
    rot         = sval != NULL && !!strcasecmp(sval, "0");
    gray        = ghb_dict_get_bool(ud->settings, "VideoGrayScale");
    sval        = ghb_dict_get_string(ud->settings, "PictureColorspacePreset");
    colorspace  = sval != NULL && !!strcasecmp(sval, "off");
    sval        = ghb_dict_get_string(ud->settings, "PictureChromaSmoothPreset");
    chroma_smooth  = sval != NULL && !!strcasecmp(sval, "off");

    str = g_string_new("");
    sval = "";
    if (detel)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DETELECINE);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (comb_detect)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COMB_DETECT);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (yadif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_YADIF);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (bwdif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_BWDIF);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (decomb)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DECOMB);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (deblock)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DEBLOCK);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (nlmeans)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_NLMEANS);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (denoise)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DENOISE);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (chroma_smooth)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_CHROMA_SMOOTH);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (unsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_UNSHARP);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (rot || hflip)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_ROTATE);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (lapsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_LAPSHARP);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (gray)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_GRAYSCALE);
        g_string_append_printf(str, "%s%s", sval, ghb_get_filter_name(filter));
        sval = ", ";
    }
    if (colorspace)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COLORSPACE);
        g_string_append_printf(str, "%s%s", sval,ghb_get_filter_name(filter));
        sval = ", ";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "filters_summary");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    double display_width;
    int    width, height, display_height, par_width, par_height;
    char * display_aspect;

    width          = ghb_dict_get_int(ud->settings, "scale_width");
    height         = ghb_dict_get_int(ud->settings, "scale_height");
    display_width  = ghb_dict_get_int(ud->settings, "PictureDARWidth");
    display_height = ghb_dict_get_int(ud->settings, "DisplayHeight");
    par_width      = ghb_dict_get_int(ud->settings, "PicturePARWidth");
    par_height     = ghb_dict_get_int(ud->settings, "PicturePARHeight");

    display_width = (double)width * par_width / par_height;
    display_aspect = ghb_get_display_aspect_string(display_width,
                                                   display_height);

    display_width  = ghb_dict_get_int(ud->settings, "PictureDARWidth");
    text = g_strdup_printf("%dx%d %s, %dx%d %s\n%d:%d %s\n%s %s",
                           width, height, _("storage"),
                           (int)display_width, display_height, _("display"),
                           par_width, par_height,_("Pixel Aspect Ratio"),
                           display_aspect, _("Display Aspect Ratio"));
    widget = GHB_WIDGET(ud->builder, "dimensions_summary");
    gtk_label_set_text(GTK_LABEL(widget), text);

    g_free(text);
    g_free(display_aspect);

    ghb_value_free(&titleDict);
}

void
ghb_set_title_settings(signal_user_data_t *ud, GhbValue *settings)
{
    int title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    ghb_subtitle_set_pref_lang(settings);
    if (title != NULL)
    {
        GhbValue * job_dict, * title_dict;
        char     * label;

        job_dict = hb_preset_job_init(ghb_scan_handle(), title_id, settings);
        ghb_dict_set(settings, "Job", job_dict);
        title_dict = hb_title_to_dict(ghb_scan_handle(), title_id);
        ghb_dict_set(settings, "Title", title_dict);

        GhbValue *chapter_list = ghb_get_job_chapter_list(settings);
        gint num_chapters = ghb_array_len(chapter_list);

        ghb_dict_set_int(settings, "angle", 1);
        ghb_dict_set_string(settings, "PtoPType", "chapter");
        ghb_dict_set_int(settings, "start_point", 1);
        ghb_dict_set_int(settings, "end_point", num_chapters);
        ghb_dict_set_int(settings, "source_width", title->geometry.width);
        ghb_dict_set_int(settings, "source_height", title->geometry.height);
        ghb_dict_set_string(settings, "source", title->path);
        label = ghb_create_source_label(title);
        ghb_dict_set_string(settings, "source_label", label);
        g_free(label);
        label = ghb_create_volume_label(title);
        ghb_dict_set_string(settings, "volume", label);
        g_free(label);


        int                angle, hflip;
        hb_geometry_crop_t srcGeo;

        srcGeo.geometry = title->geometry;
        memcpy(srcGeo.crop, &title->crop, 4 * sizeof(int));
        angle = ghb_dict_get_int(settings, "rotate");
        hflip = ghb_dict_get_int(settings, "hflip");
        hb_rotate_geometry(&srcGeo, &srcGeo, angle, hflip);
        ghb_apply_crop(settings, &srcGeo, title);

        int crop[4];

        crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
        crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
        crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
        crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");
        ghb_dict_set_int(settings, "scale_width",
                 srcGeo.geometry.width - crop[2] - crop[3]);

        // If anamorphic or keep_aspect, the height will
        // be automatically calculated
        gboolean keep_aspect;
        gint pic_par;
        keep_aspect = ghb_dict_get_bool(settings, "PictureKeepRatio");
        pic_par = ghb_settings_combo_int(settings, "PicturePAR");
        if (!keep_aspect ||
            pic_par == HB_ANAMORPHIC_NONE ||
            pic_par == HB_ANAMORPHIC_AUTO ||
            pic_par == HB_ANAMORPHIC_CUSTOM)
        {
            ghb_dict_set_int(settings, "scale_height",
                srcGeo.geometry.height - crop[0] - crop[1]);
        }

        ghb_set_scale_settings(ud, settings, GHB_PIC_USE_MAX);
        ghb_dict_set_int(settings, "angle_count", title->angle_count);


        // Clear UI settings for new title
        ghb_dict_set_string(settings, "MetaName", "");
        ghb_dict_set_string(settings, "MetaArtist", "");
        ghb_dict_set_string(settings, "MetaReleaseDate", "");
        ghb_dict_set_string(settings, "MetaComment", "");
        ghb_dict_set_string(settings, "MetaAlbumArtist", "");
        ghb_dict_set_string(settings, "MetaGenre", "");
        ghb_dict_set_string(settings, "MetaDescription", "");
        ghb_dict_set_string(settings, "MetaLongDescription", "");

        if (ghb_dict_get_bool(settings, "MetadataPassthrough"))
        {
            ghb_dict_set_string(settings, "MetaName", title->name);
            update_meta(settings, "Name", title->name);
            if (title->metadata && title->metadata->dict)
            {

                hb_dict_iter_t iter = hb_dict_iter_init(title->metadata->dict);

                while (iter != HB_DICT_ITER_DONE)
                {
                    const char * key;
                    hb_value_t * val;

                    hb_dict_iter_next_ex(title->metadata->dict, &iter, &key, &val);
                    if (key != NULL && val != NULL)
                    {
                        const char * str = hb_value_get_string(val);

                        update_meta(settings, key, str);

                        if (!strcmp(key, "Album"))
                        {
                            key = "Name";
                        }
                        char * ui_key = g_strdup_printf("Meta%s", key);
                        ghb_dict_set_string(settings, ui_key, str);
                        free(ui_key);
                    }
                }
            }
        }
        else
        {
            GhbValue * meta = ghb_get_job_metadata_settings(settings);
            hb_dict_clear(meta);
        }
        ghb_sanitize_audio_track_settings(settings);
    }
    else
    {
        ghb_dict_set_string(settings, "source_label", _("No Title Found"));
        ghb_dict_set_string(settings, "volume", _("New Video"));
        ghb_set_scale_settings(ud, settings, GHB_PIC_USE_MAX);
    }

    set_destination_settings(ud, settings);

    const char *dest_file, *dest_dir;
    char *dest;
    dest_file = ghb_dict_get_string(settings, "dest_file");
    dest_dir = ghb_dict_get_string(settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(ud->settings);
    ghb_dict_set_string(dest_dict, "File", dest);
    g_free(dest);

    ghb_dict_set_int(settings, "preview_frame", 2);
}

void
ghb_set_current_title_settings(signal_user_data_t *ud)
{
    ghb_set_title_settings(ud, ud->settings);
    ghb_update_summary_info(ud);
}

static void
load_all_titles(signal_user_data_t *ud, int titleindex)
{
    gint ii, count;
    GhbValue *preset;
    GhbValue *settings_array;
    const hb_title_t *title;

    hb_list_t *list = ghb_get_title_list();
    count = hb_list_count(list);

    if (count == 0)
        count = 1;

    settings_array = ghb_array_new();

    // Start with a clean job
    ghb_dict_remove(ud->settings, "Job");

    preset = ghb_get_current_preset(ud);
    if (preset != NULL)
    {
        ghb_preset_to_settings(ud->settings, preset);
        ghb_value_free(&preset);
    }
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings = ghb_value_dup(ud->settings);

        title = hb_list_item(list, ii);
        ghb_dict_set_int(settings, "title", title ? title->index : -1);
        ghb_set_title_settings(ud, settings);
        ghb_array_append(settings_array, settings);
    }
    if (titleindex < 0 || titleindex >= count)
    {
        titleindex = 0;
    }
    ghb_value_free(&ud->settings_array);
    ud->settings_array = settings_array;
    ud->settings = ghb_array_get(ud->settings_array, titleindex);
    ghb_update_summary_info(ud);
}

static gboolean update_preview = FALSE;

G_MODULE_EXPORT void
title_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint               title_id, titleindex, count;
    const hb_title_t * title;
    GtkEntry         * title_label;
    char             * label;

    title_id = ghb_widget_int(widget);
    title = ghb_lookup_title(title_id, &titleindex);

    label = ghb_create_title_label(title);
    title_label = GTK_ENTRY(GHB_WIDGET(ud->builder, "title_label"));
    gtk_entry_set_text(title_label, label);
    g_free(label);

    count = ghb_array_len(ud->settings_array);
    int idx = (titleindex >= 0 && titleindex < count) ? titleindex : 0;
    if (ghb_dict_get_bool(ud->prefs, "SyncTitleSettings"))
    {
        GhbValue * preset   = ghb_settings_to_preset(ud->settings);
        GhbValue * settings = ghb_array_get(ud->settings_array, idx);
        if (preset != NULL)
        {
            ghb_preset_to_settings(settings, preset);
            ghb_set_title_settings(ud, settings);
        }
        ghb_value_free(&preset);
    }
    ud->settings = ghb_array_get(ud->settings_array, idx);
    ghb_load_settings(ud);

    ghb_audio_title_change(ud, title != NULL);
    ghb_subtitle_title_change(ud, title != NULL);
    ghb_grey_combo_options(ud);

    if (title != NULL)
    {
        gint preview_count;
        preview_count = title->preview_count;
        if (preview_count < 1)
        {
            preview_count = 1;
        }
        widget = GHB_WIDGET(ud->builder, "preview_frame");
        gtk_range_set_range(GTK_RANGE(widget), 1, preview_count);

        ghb_reset_preview_image(ud);
    }
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
ptop_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint title_id, titleindex;
    const hb_title_t * title;
    gboolean numeric = TRUE;
    GtkSpinButton *spin;
    GhbValue *range;

    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_live_reset(ud);

    // Update type in Job
    range = ghb_get_job_range_settings(ud->settings);
    ghb_dict_set_string(range, "Type",
                        ghb_dict_get_string(ud->settings, "PtoPType"));

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
        return;

    if (ghb_settings_combo_int(ud->settings, "PtoPType") == 1)
        numeric = FALSE;

    spin = GTK_SPIN_BUTTON(GHB_WIDGET(ud->builder, "start_point"));
    gtk_spin_button_set_numeric(spin, numeric);
    spin = GTK_SPIN_BUTTON(GHB_WIDGET(ud->builder, "end_point"));
    gtk_spin_button_set_numeric(spin, numeric);

    gint duration = title->duration / 90000;
    if (ghb_settings_combo_int(ud->settings, "PtoPType") == 0)
    {
        GhbValue *chapter_list = ghb_get_job_chapter_list(ud->settings);
        gint num_chapters = ghb_array_len(chapter_list);
        spin_configure(ud, "start_point", 1, 1, num_chapters);
        spin_configure(ud, "end_point", num_chapters, 1, num_chapters);
    }
    else if (ghb_settings_combo_int(ud->settings, "PtoPType") == 1)
    {
        spin_configure(ud, "start_point", 0, 0, duration * 2 - 1);
        spin_configure(ud, "end_point", duration, 0, duration * 2);
    }
    else if (ghb_settings_combo_int(ud->settings, "PtoPType") == 2)
    {
        gdouble max_frames = (gdouble)duration *
                             title->vrate.num / title->vrate.den;
        spin_configure(ud, "start_point", 1, 1, max_frames * 2);
        spin_configure(ud, "end_point", max_frames, 1, max_frames * 2);
    }
}

G_MODULE_EXPORT void
setting_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_update_summary_info(ud);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
meta_pass_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    setting_widget_changed_cb(widget, ud);

    int title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    if (title != NULL &&
        ghb_dict_get_bool(ud->settings, "MetadataPassthrough"))
    {
        // Reload metadata from title
        ghb_dict_set_string(ud->settings, "MetaName", title->name);
        update_meta(ud->settings, "Name", title->name);
        if (title->metadata && title->metadata->dict)
        {

            hb_dict_iter_t iter = hb_dict_iter_init(title->metadata->dict);

            while (iter != HB_DICT_ITER_DONE)
            {
                const char * key;
                hb_value_t * val;

                hb_dict_iter_next_ex(title->metadata->dict, &iter, &key, &val);
                if (key != NULL && val != NULL)
                {
                    const char * str = hb_value_get_string(val);

                    update_meta(ud->settings, key, str);

                    if (!strcmp(key, "Album"))
                    {
                        key = "Name";
                    }
                    char * ui_key = g_strdup_printf("Meta%s", key);
                    ghb_dict_set_string(ud->settings, ui_key, str);
                    free(ui_key);
                }
            }
        }
    }
    else
    {
        // Clear metadata
        ghb_dict_set_string(ud->settings, "MetaName", "");
        ghb_dict_set_string(ud->settings, "MetaArtist", "");
        ghb_dict_set_string(ud->settings, "MetaReleaseDate", "");
        ghb_dict_set_string(ud->settings, "MetaComment", "");
        ghb_dict_set_string(ud->settings, "MetaAlbumArtist", "");
        ghb_dict_set_string(ud->settings, "MetaGenre", "");
        ghb_dict_set_string(ud->settings, "MetaDescription", "");
        ghb_dict_set_string(ud->settings, "MetaLongDescription", "");

        GhbValue * meta = ghb_get_job_metadata_settings(ud->settings);
        hb_dict_clear(meta);
    }

    // Update UI
    ghb_ui_update_from_settings(ud, "MetaName", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaArtist", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaReleaseDate", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaComment", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaAlbumArtist", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaGenre", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaDescription", ud->settings);
    ghb_ui_update_from_settings(ud, "MetaLongDescription", ud->settings);
}

G_MODULE_EXPORT void
filter_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    setting_widget_changed_cb(widget, ud);
    update_preview = TRUE;
}

G_MODULE_EXPORT void
nonsetting_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_update_summary_info(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
comb_detect_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    const char * comb_detect;
    comb_detect = ghb_dict_get_string(ud->settings, "PictureCombDetectPreset");
    if (strcasecmp(comb_detect, "off"))
    {
        const char * deint;
        deint = ghb_dict_get_string(ud->settings, "PictureDeinterlaceFilter");
        if (!strcasecmp(deint, "off"))
        {
            ghb_ui_update(ud, "PictureDeinterlaceFilter",
                          ghb_string_value("decomb"));
        }
    }
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
deint_filter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    ghb_update_ui_combo_box(ud, "PictureDeinterlacePreset", NULL, FALSE);
    ghb_ui_update(ud, "PictureDeinterlacePreset",
                  ghb_dict_get(ud->settings, "PictureDeinterlacePreset"));

    const char * deint;
    deint = ghb_dict_get_string(ud->settings, "PictureDeinterlaceFilter");
    if (!strcasecmp(deint, "off"))
    {
        ghb_ui_update(ud, "PictureCombDetectPreset",
                      ghb_string_value("off"));
    }
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
denoise_filter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    ghb_update_ui_combo_box(ud, "PictureDenoisePreset", NULL, FALSE);
    ghb_ui_update(ud, "PictureDenoisePreset",
                  ghb_dict_get(ud->settings, "PictureDenoisePreset"));
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
sharpen_filter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    ghb_update_ui_combo_box(ud, "PictureSharpenPreset", NULL, FALSE);
    ghb_update_ui_combo_box(ud, "PictureSharpenTune", NULL, FALSE);
    ghb_ui_update(ud, "PictureSharpenPreset",
                  ghb_dict_get(ud->settings, "PictureSharpenPreset"));
    ghb_ui_update(ud, "PictureSharpenTune", ghb_string_value("none"));
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
title_angle_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_live_reset(ud);

    GhbValue *source = ghb_get_job_source_settings(ud->settings);
    ghb_dict_set_int(source, "Angle", ghb_dict_get_int(ud->settings, "angle"));
}

G_MODULE_EXPORT void
meta_name_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaName");
    update_meta(ud->settings, "Name", val);
}

G_MODULE_EXPORT void
meta_artist_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaArtist");
    update_meta(ud->settings, "Artist", val);
}

G_MODULE_EXPORT void
meta_album_artist_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaAlbumArtist");
    update_meta(ud->settings, "AlbumArtist", val);
}

G_MODULE_EXPORT void
meta_release_date_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaReleaseDate");
    update_meta(ud->settings, "ReleaseDate", val);
}

G_MODULE_EXPORT void
meta_comment_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaComment");
    update_meta(ud->settings, "Comment", val);
}

G_MODULE_EXPORT void
meta_genre_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaGenre");
    update_meta(ud->settings, "Genre", val);
}

G_MODULE_EXPORT void
meta_description_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    const char *val;

    ghb_widget_to_setting(ud->settings, widget);
    val = ghb_dict_get_string(ud->settings, "MetaDescription");
    update_meta(ud->settings, "Description", val);
}

G_MODULE_EXPORT void
plot_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkWidget  * textview;
    const char * val;

    textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "MetaLongDescription"));
    ghb_widget_to_setting(ud->settings, textview);
    val = ghb_dict_get_string(ud->settings, "MetaLongDescription");
    update_meta(ud->settings, "LongDescription", val);
}

G_MODULE_EXPORT void
chapter_markers_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    GhbValue *dest;
    int start, end;
    gboolean markers;
    dest     = ghb_get_job_dest_settings(ud->settings);
    markers  = ghb_dict_get_bool(ud->settings, "ChapterMarkers");
    start    = ghb_dict_get_int(ud->settings, "start_point");
    end      = ghb_dict_get_int(ud->settings, "end_point");
    markers &= (end > start);
    ghb_dict_set_bool(dest, "ChapterMarkers", markers);
    ghb_update_summary_info(ud);
}

G_MODULE_EXPORT void
vquality_type_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (ghb_check_name_template(ud, "{quality}") ||
        ghb_check_name_template(ud, "{bitrate}"))
        ghb_set_destination(ud);
}

G_MODULE_EXPORT void
vbitrate_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (ghb_check_name_template(ud, "{bitrate}"))
        ghb_set_destination(ud);
}

G_MODULE_EXPORT void
vquality_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    gint vcodec;
    double vquality;

    vcodec = ghb_settings_video_encoder_codec(ud->settings, "VideoEncoder");
    vquality = ghb_dict_get_double(ud->settings, "VideoQualitySlider");
    if (vcodec == HB_VCODEC_X264_8BIT && vquality < 1.0)
    {
        // Set Profile to auto for lossless x264
        ghb_ui_update(ud, "VideoProfile", ghb_string_value("auto"));
    }

    gdouble step;
    float min, max, min_step;
    int direction;

    hb_video_quality_get_limits(vcodec, &min, &max, &min_step, &direction);
    step = ghb_settings_combo_double(ud->prefs, "VideoQualityGranularity");
    if (step < min_step)
    {
        step = min_step;
    }
    gdouble val = gtk_range_get_value(GTK_RANGE(widget));
    if (val < 0)
    {
        val = ((int)((val - step / 2) / step)) * step;
    }
    else
    {
        val = ((int)((val + step / 2) / step)) * step;
    }
    if (val < min)
    {
        val = min;
    }
    if (val > max)
    {
        val = max;
    }
    gtk_range_set_value(GTK_RANGE(widget), val);
    if (ghb_check_name_template(ud, "{quality}"))
        ghb_set_destination(ud);
}

static void
set_has_chapter_markers (gboolean markers, signal_user_data_t *ud)
{
    GhbValue *dest = ghb_get_job_dest_settings(ud->settings);

    if (ghb_check_name_template(ud, "{chapters}"))
        ghb_set_destination(ud);
    GtkWidget *widget = GHB_WIDGET (ud->builder, "ChapterMarkers");
    gtk_widget_set_sensitive(widget, markers);
    update_title_duration(ud);

    markers &= ghb_dict_get_int(ud->settings, "ChapterMarkers");
    ghb_dict_set_bool(dest, "ChapterMarkers", markers);
}

enum {
    PTOP_NONE,
    PTOP_START,
    PTOP_END
};

/*
 * Reads the text entered in the spin button text field and converts
 * it to the correct format for the current mode.
 */
G_MODULE_EXPORT gboolean
ptop_read_value_cb (GtkWidget *widget, gdouble *val, signal_user_data_t *ud)
{
    const gchar *text;
    int64_t result;
    gdouble ss = 0;
    int hh = 0, mm = 0;

    text = ghb_editable_get_text(widget);
    if (ghb_settings_combo_int(ud->settings, "PtoPType") != 1)
    {
        result = strtol(text, NULL, 10);
        *val = (gdouble) result;
        return TRUE;
    }

    result = sscanf(text, "%2d:%2d:%lf", &hh, &mm, &ss);
    if (result != 1 && result != 3)
        return FALSE;
    if (result == 1)
    {
        result = sscanf(text, "%lf", val);
        return TRUE;
    }
    *val = hh * 3600 + mm * 60 + ss;
    return TRUE;
}

/*
 * Formats the current start or end point into a string which is then
 * displayed in the spin button text entry.
 */
G_MODULE_EXPORT gboolean
ptop_format_value_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    if (ghb_settings_combo_int(ud->settings, "PtoPType") != 1)
        return FALSE;

    GtkAdjustment *adjustment;
    gchar *text;
    double ss;
    int hh, mm;

    adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(widget));
    ss = gtk_adjustment_get_value(adjustment);
    hh = (int) (ss / 3600);
    ss = ss - hh * 3600;
    mm = (int) (ss / 60);
    ss = ss - mm * 60;
    text = g_strdup_printf ("%02d:%02d:%05.2f", hh, mm, ss);
    ghb_editable_set_text(widget, text);
    g_free (text);

    return TRUE;
}

/*
 * Updates the settings of the current job with the values currently being
 * entered by the user. This function is called every time the text in the
 * spin button changes but doesn't change the values currently displayed.
 * However, if the user starts the encode, the updated value will be used.
 */
static void
ptop_update_bg (int side_changed, double new_val, signal_user_data_t *ud)
{
    double start_val = 0.0, end_val = 0.0, min_val, max_val;
    int64_t start_int = 0, end_int = 0;
    GtkAdjustment *start_adj, *end_adj;
    GhbValue *range = ghb_get_job_range_settings(ud->settings);

    start_adj = GTK_ADJUSTMENT(gtk_builder_get_object(ud->builder,
                                                      "start_point_adj"));
    end_adj = GTK_ADJUSTMENT(gtk_builder_get_object(ud->builder,
                                                    "end_point_adj"));
    start_val = gtk_adjustment_get_value(start_adj);
    end_val = gtk_adjustment_get_value(end_adj);

    if (side_changed == PTOP_START)
    {
        min_val = gtk_adjustment_get_lower(start_adj);
        max_val = gtk_adjustment_get_upper(start_adj);

        if (new_val < min_val) start_val = min_val;
        else if (new_val > max_val) start_val = max_val;
        else start_val = new_val;

        end_val = (end_val > start_val) ? end_val : start_val;
    }
    else
    {
        min_val = gtk_adjustment_get_lower(end_adj);
        max_val = gtk_adjustment_get_upper(end_adj);

        if (new_val < min_val) end_val = min_val;
        else if (new_val > max_val) end_val = max_val;
        else end_val = new_val;

        start_val = (end_val > start_val) ? start_val : end_val;
    }

    if (ghb_settings_combo_int(ud->settings, "PtoPType") == 1)
    {
        start_int = (int64_t) nearbyint(start_val * 90000);
        end_int = (int64_t) nearbyint(end_val * 90000);

        if (start_int == end_int)
        {
            if (side_changed == PTOP_START && start_int > 0)
                start_int -= 1;
            else
                end_int += 1;
        }
        ghb_dict_set_double(ud->settings, "start_point", start_val);
        ghb_dict_set_double(ud->settings, "end_point", end_val);
    }
    else
    {
        start_int = (int64_t) nearbyint(start_val);
        end_int = (int64_t) nearbyint(end_val);

        ghb_dict_set_int(ud->settings, "start_point", start_int);
        ghb_dict_set_int(ud->settings, "end_point", end_int);
    }
    ghb_dict_set_int(range, "Start", start_int);
    ghb_dict_set_int(range, "End", end_int);

    if (ghb_settings_combo_int(ud->settings, "PtoPType") == 0)
        set_has_chapter_markers (end_int > start_int, ud);
}

G_MODULE_EXPORT void
start_point_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    double new_val = 0.0;

    ptop_read_value_cb(widget, &new_val, ud);
    ptop_update_bg(PTOP_START, new_val, ud);
    ghb_check_dependency(ud, widget, NULL);
}

G_MODULE_EXPORT void
end_point_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    double new_val = 0.0;

    ptop_read_value_cb(widget, &new_val, ud);
    ptop_update_bg(PTOP_END, new_val, ud);
    ghb_check_dependency(ud, widget, NULL);
}

/*
 * Refreshes the UI by updating the underlying GtkAdjustments with
 * the values set by the user. This function is only called when the
 * focus changes to avoid changing the text while the user is typing.
 */
G_MODULE_EXPORT void
ptop_update_ui_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    GtkAdjustment *adj;
    double value;

    adj = GTK_ADJUSTMENT(gtk_builder_get_object(ud->builder,
                                                "start_point_adj"));
    value = ghb_dict_get_double(ud->settings, "start_point");
    gtk_adjustment_set_value(adj, value);

    adj = GTK_ADJUSTMENT(gtk_builder_get_object(ud->builder,
                                                "end_point_adj"));
    value = ghb_dict_get_double(ud->settings, "end_point");
    gtk_adjustment_set_value(adj, value);
}

G_MODULE_EXPORT void
scale_width_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, GHB_PIC_KEEP_WIDTH);
    update_preview = TRUE;
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
scale_height_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, GHB_PIC_KEEP_HEIGHT);

    update_preview = TRUE;
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
crop_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, 0);
    update_preview = TRUE;
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
pad_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, 0);

    update_preview = TRUE;
}

G_MODULE_EXPORT void
display_width_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, GHB_PIC_KEEP_DISPLAY_WIDTH);

    update_preview = TRUE;
}

G_MODULE_EXPORT void
display_height_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, GHB_PIC_KEEP_DISPLAY_HEIGHT);

    update_preview = TRUE;
}

G_MODULE_EXPORT void
par_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, GHB_PIC_KEEP_HEIGHT);

    update_preview = TRUE;
}

G_MODULE_EXPORT void
scale_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, 0);
    update_preview = TRUE;
}

G_MODULE_EXPORT void
rotate_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    int angle, hflip, prev_angle, prev_hflip;

    ghb_log_func();
    prev_angle = ghb_dict_get_int(ud->settings, "rotate");
    prev_hflip = ghb_dict_get_int(ud->settings, "hflip");

    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    hb_geometry_crop_t   geo;

    // First normalize current settings to 0 angle 0 hflip
    geo.geometry.width   = ghb_dict_get_int(ud->settings, "scale_width");
    geo.geometry.height  = ghb_dict_get_int(ud->settings, "scale_height");
    geo.geometry.par.num =
        ghb_dict_get_int(ud->settings, "PicturePARWidth");
    geo.geometry.par.den =
        ghb_dict_get_int(ud->settings, "PicturePARHeight");
    geo.crop[0] = ghb_dict_get_int(ud->settings, "PictureTopCrop");
    geo.crop[1] = ghb_dict_get_int(ud->settings, "PictureBottomCrop");
    geo.crop[2] = ghb_dict_get_int(ud->settings, "PictureLeftCrop");
    geo.crop[3] = ghb_dict_get_int(ud->settings, "PictureRightCrop");

    geo.pad[0] = ghb_dict_get_int(ud->settings, "PicturePadTop");
    geo.pad[1] = ghb_dict_get_int(ud->settings, "PicturePadBottom");
    geo.pad[2] = ghb_dict_get_int(ud->settings, "PicturePadLeft");
    geo.pad[3] = ghb_dict_get_int(ud->settings, "PicturePadRight");
    // Normally hflip is applied, then rotation.
    // To revert, must apply rotation then hflip.
    hb_rotate_geometry(&geo, &geo, 360 - prev_angle, 0);
    hb_rotate_geometry(&geo, &geo, 0, prev_hflip);

    const hb_title_t   * title;
    int                  title_id, titleindex;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title != NULL)
    {
        // If there is a title, reset to title width/height so that
        // dimension limits get properly re-applied
        geo.geometry.width = title->geometry.width;
        geo.geometry.height = title->geometry.height;
    }

    // rotate dimensions to new angle and hflip
    angle = ghb_dict_get_int(ud->settings, "rotate");
    hflip = ghb_dict_get_int(ud->settings, "hflip");
    hb_rotate_geometry(&geo, &geo, angle, hflip);

    ghb_dict_set_int(ud->settings, "scale_width",
             geo.geometry.width - geo.crop[2] - geo.crop[3]);
    ghb_dict_set_int(ud->settings, "scale_height",
             geo.geometry.height - geo.crop[0] - geo.crop[1]);
    ghb_dict_set_int(ud->settings, "PicturePARWidth",   geo.geometry.par.num);
    ghb_dict_set_int(ud->settings, "PicturePARHeight",  geo.geometry.par.den);
    ghb_dict_set_int(ud->settings, "PictureTopCrop",    geo.crop[0]);
    ghb_dict_set_int(ud->settings, "PictureBottomCrop", geo.crop[1]);
    ghb_dict_set_int(ud->settings, "PictureLeftCrop",   geo.crop[2]);
    ghb_dict_set_int(ud->settings, "PictureRightCrop",  geo.crop[3]);

    ghb_dict_set_int(ud->settings, "PicturePadTop",    geo.pad[0]);
    ghb_dict_set_int(ud->settings, "PicturePadBottom", geo.pad[1]);
    ghb_dict_set_int(ud->settings, "PicturePadLeft",   geo.pad[2]);
    ghb_dict_set_int(ud->settings, "PicturePadRight",  geo.pad[3]);

    ghb_set_scale(ud, 0);
    update_preview = TRUE;
}

G_MODULE_EXPORT void
resolution_limit_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    const gchar * resolution_limit;
    int           width, height;

    resolution_limit = ghb_dict_get_string(ud->settings, "resolution_limit");
    ghb_lookup_resolution_limit_dimensions(resolution_limit, &width, &height);
    if (width >= 0 && height >= 0)
    {
        ghb_dict_set_int(ud->settings, "PictureWidth", width);
        ghb_dict_set_int(ud->settings, "PictureHeight", height);
    }

    ghb_set_scale(ud, GHB_PIC_KEEP_HEIGHT);

    update_preview = TRUE;
}

G_MODULE_EXPORT void
generic_entry_changed_cb(GtkEntry *entry, signal_user_data_t *ud)
{
    // Normally (due to user input) I only want to process the entry
    // when editing is done and the focus-out signal is sent.
    // But... there's always a but.
    // If the entry is changed by software, the focus-out signal is not sent.
    // The changed signal is sent ... so here we are.
    // I don't want to process upon every keystroke, so I prevent processing
    // while the widget has focus.
    ghb_log_func();
    if (!gtk_widget_has_focus((GtkWidget*)entry))
    {
        ghb_widget_to_setting(ud->settings, (GtkWidget*)entry);
    }
}

gboolean prefs_require_restart = FALSE;

G_MODULE_EXPORT void
preferences_action_cb(GSimpleAction *action, GVariant *param,
                      signal_user_data_t *ud)
{
    GtkWidget *dialog;

    prefs_require_restart = FALSE;
    dialog = GHB_WIDGET(ud->builder, "prefs_dialog");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    ghb_prefs_store();

    if (prefs_require_restart)
    {
        GtkWindow *hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));

        // Toss up a warning dialog
        ghb_message_dialog(hb_window, GTK_MESSAGE_WARNING,
                           "You must restart HandBrake now",
                           "Exit HandBrake", NULL);
        ghb_hb_cleanup(FALSE);
        prune_logs(ud);
        g_application_quit(G_APPLICATION(ud->app));
    }
}

typedef struct
{
    GtkMessageDialog *dlg;
    const gchar *msg;
    const gchar *action;
    gint timeout;
    signal_user_data_t *ud;
} countdown_t;

static gboolean
quit_cb(countdown_t *cd)
{
    gchar *str;

    cd->timeout--;
    if (cd->timeout == 0)
    {
        ghb_hb_cleanup(FALSE);
        prune_logs(cd->ud);

        gtk_widget_destroy (GTK_WIDGET(cd->dlg));
        g_application_quit(G_APPLICATION(cd->ud->app));
        return FALSE;
    }
    str = g_strdup_printf(_("%s\n\n%s in %d seconds ..."),
                            cd->msg, cd->action, cd->timeout);
    gtk_message_dialog_set_markup(cd->dlg, str);
    g_free(str);
    return TRUE;
}

static gboolean
shutdown_cb(countdown_t *cd)
{
    gchar *str;

    cd->timeout--;
    str = g_strdup_printf(_("%s\n\n%s in %d seconds ..."),
                            cd->msg, cd->action, cd->timeout);
    gtk_message_dialog_set_markup(cd->dlg, str);
    if (cd->timeout == 0)
    {
        ghb_hb_cleanup(FALSE);
        prune_logs(cd->ud);

        shutdown_logind();
        g_application_quit(G_APPLICATION(cd->ud->app));
        return FALSE;
    }
    g_free(str);
    return TRUE;
}

static gboolean
suspend_cb(countdown_t *cd)
{
    gchar *str;

    cd->timeout--;
    str = g_strdup_printf(_("%s\n\n%s in %d seconds ..."),
                            cd->msg, cd->action, cd->timeout);
    gtk_message_dialog_set_markup(cd->dlg, str);
    if (cd->timeout == 0)
    {
        gtk_widget_destroy (GTK_WIDGET(cd->dlg));
        suspend_logind();
        return FALSE;
    }
    g_free(str);
    return TRUE;
}

void
ghb_countdown_dialog(
    GtkMessageType type,
    const gchar *message,
    const gchar *action,
    const gchar *cancel,
    GSourceFunc action_func,
    signal_user_data_t *ud,
    gint timeout)
{
    GtkWindow *hb_window;
    GtkWidget *dialog;
    GtkResponseType response;
    guint timeout_id;
    countdown_t cd;

    cd.msg = message;
    cd.action = action;
    cd.timeout = timeout;
    cd.ud = ud;

    // Toss up a warning dialog
    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
                            type, GTK_BUTTONS_NONE,
                            _("%s\n\n%s in %d seconds ..."),
                            message, action, timeout);
    gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                           cancel, GTK_RESPONSE_CANCEL,
                           NULL);

    cd.dlg = GTK_MESSAGE_DIALOG(dialog);
    timeout_id = g_timeout_add(1000, action_func, &cd);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_CANCEL)
    {
        GMainContext *mc;
        GSource *source;

        mc = g_main_context_default();
        source = g_main_context_find_source_by_id(mc, timeout_id);
        if (source != NULL)
            g_source_destroy(source);
        gtk_widget_destroy (dialog);
    }
}

gboolean
ghb_title_message_dialog(GtkWindow *parent, GtkMessageType type, const gchar *title,
                         const gchar *message, const gchar *no, const gchar *yes)
{
    GtkWidget *dialog, *yes_button;
    GtkResponseType response;
    GtkStyleContext *yes_style;

    // Toss up a warning dialog
    dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, type,
                                    GTK_BUTTONS_NONE, "%s", title);
    if (message)
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           no, GTK_RESPONSE_NO,
                           yes, GTK_RESPONSE_YES, NULL);

    if (yes != NULL)
    {
        yes_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                                        GTK_RESPONSE_YES);
        yes_style = gtk_widget_get_style_context(yes_button);

        // Use GTK_MESSAGE_QUESTION for a neutral dialog,
        // GTK_MESSAGE_INFO for a blue 'suggested-action' confirm button, or
        // GTK_MESSAGE_WARNING for a red 'destructive-action' confirm button.
        switch (type)
        {
        case GTK_MESSAGE_INFO:
            gtk_style_context_add_class(yes_style, "suggested-action");
            break;
        case GTK_MESSAGE_WARNING:
        case GTK_MESSAGE_ERROR:
            gtk_style_context_add_class(yes_style, "destructive-action");
        default:
            break;
        }
    }
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
    if (response == GTK_RESPONSE_NO)
    {
        return FALSE;
    }
    return TRUE;
}

gboolean
ghb_message_dialog(GtkWindow *parent, GtkMessageType type, const gchar *message,
                   const gchar *no, const gchar *yes)
{
    return ghb_title_message_dialog(parent, type, message, NULL, no, yes);
}


void
ghb_error_dialog(GtkWindow *parent, GtkMessageType type, const gchar *message, const gchar *cancel)
{
    ghb_message_dialog(parent, type, message, cancel, NULL);
}

void
ghb_cancel_encode(signal_user_data_t *ud, const gchar *extra_msg)
{
    GtkWindow *hb_window;
    GtkWidget *dialog, *cancel;
    GtkResponseType response;
    GtkStyleContext *style;

    if (extra_msg == NULL) extra_msg = "";
    // Toss up a warning dialog
    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                _("%sYour movie will be lost if you don't continue encoding."),
                extra_msg);
    gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                           _("Cancel Current and Stop"), 1,
                           _("Cancel Current, Start Next"), 2,
                           _("Finish Current, then Stop"), 3,
                           _("Continue Encoding"), 4,
                           NULL);

    cancel = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), 1);
    style = gtk_widget_get_style_context(cancel);
    gtk_style_context_add_class(style, "destructive-action");
    cancel = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), 2);
    style = gtk_widget_get_style_context(cancel);
    gtk_style_context_add_class(style, "destructive-action");

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
    switch ((int)response)
    {
        case 1:
            ghb_stop_queue();
            ud->cancel_encode = GHB_CANCEL_ALL;
            break;
        case 2:
            ghb_stop_queue();
            ud->cancel_encode = GHB_CANCEL_CURRENT;
            break;
        case 3:
            ud->cancel_encode = GHB_CANCEL_FINISH;
            break;
        case 4:
        default:
            ud->cancel_encode = GHB_CANCEL_NONE;
            break;
    }
}

gboolean
ghb_cancel_encode2(signal_user_data_t *ud, const gchar *extra_msg)
{
    GtkWindow *hb_window;
    GtkWidget *dialog, *cancel;
    GtkResponseType response;
    GtkStyleContext *style;

    if (extra_msg == NULL) extra_msg = "";
    // Toss up a warning dialog
    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                _("%sYour movie will be lost if you don't continue encoding."),
                extra_msg);
    gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                           _("Cancel Current and Stop"), 1,
                           _("Continue Encoding"), 4,
                           NULL);

    cancel = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), 1);
    style = gtk_widget_get_style_context(cancel);
    gtk_style_context_add_class(style, "destructive-action");

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
    switch ((int)response)
    {
        case 1:
            ghb_stop_queue();
            ud->cancel_encode = GHB_CANCEL_ALL;
            return TRUE;
        case 4:
        default:
            break;
    }
    return FALSE;
}

static void
start_new_log(signal_user_data_t *ud, GhbValue *uiDict)
{
    time_t  _now;
    struct tm *now;
    gchar *log_path, *pos, *basename, *dest_dir;
    const gchar *destname;

    // queue_activity_buffer is about to be reused, make sure
    // queue is showing the correct buffer
    ghb_queue_select_log(ud);
    // Erase current contents of queue activity
    gtk_text_buffer_set_text(ud->queue_activity_buffer, "", 0);

    _now = time(NULL);
    now = localtime(&_now);
    destname = ghb_dict_get_string(uiDict, "destination");
    basename = g_path_get_basename(destname);
    if (ghb_dict_get_bool(ud->prefs, "EncodeLogLocation"))
    {
        dest_dir = g_path_get_dirname (destname);
    }
    else
    {
        dest_dir = ghb_get_user_config_dir("EncodeLogs");
    }
    pos = g_strrstr( basename, "." );
    if (pos != NULL)
    {
        *pos = 0;
    }
    log_path = g_strdup_printf("%s/%s %d-%02d-%02d %02d-%02d-%02d.log",
        dest_dir,
        basename,
        now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
        now->tm_hour, now->tm_min, now->tm_sec);
    g_free(basename);
    g_free(dest_dir);
    if (ud->job_activity_log)
        g_io_channel_unref(ud->job_activity_log);
    ud->job_activity_log = g_io_channel_new_file (log_path, "w", NULL);
    if (ud->job_activity_log)
    {
        gchar *ver_str;

        ver_str = g_strdup_printf("Handbrake Version: %s (%d)\n",
                                    hb_get_version(NULL), hb_get_build(NULL));
        g_io_channel_write_chars (ud->job_activity_log, ver_str,
                                    -1, NULL, NULL);
        g_free(ver_str);
        ghb_dict_set_string(uiDict, "ActivityFilename", log_path);
    }
    g_free(log_path);
}

static void
submit_job(signal_user_data_t *ud, GhbValue *queueDict)
{
    gchar *type, *modified;
    const char *name;
    GhbValue *uiDict;
    gboolean preset_modified;

    ghb_log_func();
    if (queueDict == NULL) return;
    uiDict = ghb_dict_get(queueDict, "uiSettings");
    preset_modified = ghb_dict_get_bool(uiDict, "preset_modified");
    name = ghb_dict_get_string(uiDict, "PresetFullName");
    type = ghb_dict_get_int(uiDict, "Type") == 1 ? "Custom " : "";
    modified = preset_modified ? "Modified " : "";
    ghb_log("%s%sPreset: %s", modified, type, name);

    ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_RUNNING);
    start_new_log(ud, uiDict);
    GhbValue *job_dict = ghb_dict_get(queueDict, "Job");
    int unique_id = ghb_add_job(ghb_queue_handle(), job_dict);
    ghb_dict_set_int(uiDict, "job_unique_id", unique_id);
    time_t now = time(NULL);
    ghb_dict_set_int(uiDict, "job_start_time", now);
    ghb_start_queue();

    // Show queue progress bar
    int index = ghb_find_queue_job(ud->queue, unique_id, NULL);
    ghb_queue_progress_set_visible(ud, index, 1);
}

static void
prune_logs(signal_user_data_t *ud)
{
    gchar *dest_dir;
    gint days;

    // Only prune logs stored in the default config dir location
    days = ghb_settings_combo_int(ud->prefs, "LogLongevity");
    if (days > 365)
        return;

    dest_dir = ghb_get_user_config_dir("EncodeLogs");
    if (g_file_test(dest_dir, G_FILE_TEST_IS_DIR))
    {
        const gchar *file;
        gint duration = days * 24 * 60 * 60;

        GDir *gdir = g_dir_open(dest_dir, 0, NULL);
        time_t now;

        now = time(NULL);
        file = g_dir_read_name(gdir);
        while (file)
        {
            gchar *path;
            struct stat stbuf;

            path = g_strdup_printf("%s/%s", dest_dir, file);
            g_stat(path, &stbuf);
            if (now - stbuf.st_mtime > duration)
            {
                g_unlink(path);
            }
            g_free(path);
            file = g_dir_read_name(gdir);
        }
        g_dir_close(gdir);
    }
    g_free(dest_dir);
    ghb_preview_cleanup(ud);
}

static gint
queue_pending_count(GhbValue *queue)
{
    gint nn, ii, count;
    GhbValue *queueDict, *uiDict;
    gint status;

    nn = 0;
    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {

        queueDict = ghb_array_get(queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_PENDING)
        {
            nn++;
        }
    }
    return nn;
}

void
ghb_update_pending(signal_user_data_t *ud)
{
    GtkLabel *label;
    gint pending;
    gchar *str = NULL;

    pending = queue_pending_count(ud->queue);
    if (pending == 1)
    {
        str = g_strdup_printf(_("%d encode pending"), pending);
    }
    else if (pending > 1)
    {
        str = g_strdup_printf(_("%d encodes pending"), pending);
    }
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "pending_status"));
    gtk_label_set_text(label, str);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_status_label"));
    gtk_label_set_text(label, str);
    g_free(str);

    update_queue_labels(ud);
}

void
ghb_start_next_job(signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *queueDict, *uiDict;
    gint status;
    GtkWidget *progress;

    ghb_log_func();
    progress = GHB_WIDGET(ud->builder, "progressbar");
    gtk_widget_show(progress);

    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {

        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_PENDING)
        {
            inhibit_suspend(ud);
            submit_job(ud, queueDict);
            ghb_update_pending(ud);
            return;
        }
    }
    // Nothing pending
    uninhibit_suspend(ud);
    ghb_notify_done(ud);
    ghb_update_pending(ud);
    gtk_widget_hide(progress);
    ghb_dict_set_bool(ud->globals, "SkipDiskFreeCheck", FALSE);
}

static gchar*
working_status_string(signal_user_data_t *ud, ghb_instance_status_t *status)
{
    gchar *task_str, *job_str, *status_str;
    gint qcount;
    gint index;

    qcount = ghb_array_len(ud->queue);
    index = ghb_find_queue_job(ud->queue, status->unique_id, NULL);
    if (qcount > 1)
    {
        job_str = g_strdup_printf(_("job %d of %d, "), index+1, qcount);
    }
    else
    {
        job_str = g_strdup("");
    }
    if (status->pass_count > 1)
    {
        if (status->pass_id == HB_PASS_SUBTITLE)
        {
            task_str = g_strdup_printf(_("pass %d (subtitle scan) of %d, "),
                status->pass, status->pass_count);
        }
        else
        {
            task_str = g_strdup_printf(_("pass %d of %d, "),
                status->pass, status->pass_count);
        }
    }
    else
    {
        task_str = g_strdup("");
    }
    if(status->seconds > -1)
    {
        if (status->rate_cur > 0.0)
        {
            status_str= g_strdup_printf(
                _("Encoding: %s%s%.2f %%"
                " (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)"),
                job_str, task_str,
                100.0 * status->progress,
                status->rate_cur, status->rate_avg, status->hours,
                status->minutes, status->seconds );
        }
        else
        {
            status_str= g_strdup_printf(
                _("Encoding: %s%s%.2f %%"
                " (ETA %02dh%02dm%02ds)"),
                job_str, task_str,
                100.0 * status->progress,
                status->hours, status->minutes, status->seconds );
        }
    }
    else
    {
        status_str= g_strdup_printf(
            _("Encoding: %s%s%.2f %%"),
            job_str, task_str,
            100.0 * status->progress );
    }
    g_free(task_str);
    g_free(job_str);
    return status_str;
}

static gchar*
searching_status_string(signal_user_data_t *ud, ghb_instance_status_t *status)
{
    gchar *task_str, *job_str, *status_str;
    gint qcount;
    gint index;

    qcount = ghb_array_len(ud->queue);
    index = ghb_find_queue_job(ud->queue, status->unique_id, NULL);
    if (qcount > 1)
    {
        job_str = g_strdup_printf(_("job %d of %d, "), index+1, qcount);
    }
    else
    {
        job_str = g_strdup("");
    }
    task_str = g_strdup_printf(_("Searching for start time, "));
    if(status->seconds > -1)
    {
        status_str= g_strdup_printf(
            _("Encoding: %s%s%.2f %%"
            " (ETA %02dh%02dm%02ds)"),
            job_str, task_str,
            100.0 * status->progress,
            status->hours, status->minutes, status->seconds );
    }
    else
    {
        status_str= g_strdup_printf(
            _("Encoding: %s%s%.2f %%"),
            job_str, task_str,
            100.0 * status->progress );
    }
    g_free(task_str);
    g_free(job_str);
    return status_str;
}

static void
ghb_backend_events(signal_user_data_t *ud)
{
    ghb_status_t     status;
    gchar          * status_str;
    GtkProgressBar * progress;
    GtkLabel       * work_status;
    GhbValue       * queueDict = NULL;
    gint             index = -1;
    static gint      prev_scan_state = -1;
    static gint      prev_queue_state = -1;
    static gint      event_sequence = 0;

    event_sequence++;
    ghb_track_status();
    ghb_get_status(&status);
    if (prev_scan_state != status.scan.state ||
        prev_queue_state != status.queue.state)
    {
        ghb_queue_buttons_grey(ud);
        prev_scan_state = status.scan.state;
        prev_queue_state = status.queue.state;
    }
    progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
    work_status = GTK_LABEL(GHB_WIDGET (ud->builder, "work_status"));
    if (status.scan.state == GHB_STATE_IDLE &&
        status.queue.state == GHB_STATE_IDLE)
    {
        static gboolean prev_dvdnav;
        gboolean dvdnav = ghb_dict_get_bool(ud->prefs, "use_dvdnav");
        if (dvdnav != prev_dvdnav)
        {
            hb_dvd_set_dvdnav(dvdnav);
            prev_dvdnav = dvdnav;
        }
    }
    // First handle the status of title scans
    // Then handle the status of the queue
    if (status.scan.state & GHB_STATE_SCANNING)
    {
        GtkProgressBar *scan_prog;
        GtkLabel *label;

        scan_prog = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "scan_prog"));
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "source_scan_label"));

        if (status.scan.title_cur == 0)
        {
            status_str = g_strdup (_("Scanning..."));
        }
        else
        {
            if (status.scan.preview_cur == 0)
                status_str = g_strdup_printf(_("Scanning title %d of %d..."),
                              status.scan.title_cur, status.scan.title_count );
            else
                status_str = g_strdup_printf(
                    _("Scanning title %d of %d preview %d..."),
                    status.scan.title_cur, status.scan.title_count,
                    status.scan.preview_cur);

        }
        gtk_label_set_text(label, status_str);
        g_free(status_str);
        if (status.scan.title_count > 0)
        {
            gtk_progress_bar_set_fraction(scan_prog, status.scan.progress);
        }
    }
    else if (status.scan.state & GHB_STATE_SCANDONE)
    {
        GtkWidget *widget;

        widget = GHB_WIDGET(ud->builder, "sourcetoolbutton");
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-source");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Open Source"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Choose Video Source"));

        hide_scan_progress(ud);

        int title_id, titleindex;
        const hb_title_t *title;
        title_id = ghb_longest_title();
        title = ghb_lookup_title(title_id, &titleindex);
        ghb_update_ui_combo_box(ud, "title", NULL, FALSE);
        load_all_titles(ud, titleindex);

        ghb_clear_scan_state(GHB_STATE_SCANDONE);
        // Are there really any titles.
        if (title == NULL)
        {
            ghb_ui_update(ud, "title", ghb_string_value("none"));
        }
        else
        {
            ghb_ui_update(ud, "title", ghb_int_value(title->index));
        }

        if (ghb_queue_edit_settings != NULL)
        {
            // Switch to the correct title in the list
            ghb_ui_update(ud, "title",
                ghb_dict_get_value(ghb_queue_edit_settings, "title"));

            // The above should cause the current title index to update
            title_id = ghb_dict_get_int(ud->settings, "title");
            title = ghb_lookup_title(title_id, &titleindex);
            ghb_array_replace(ud->settings_array, titleindex,
                              ghb_queue_edit_settings);
            ud->settings = ghb_queue_edit_settings;
            ghb_load_settings(ud);
            ghb_queue_edit_settings = NULL;
        }
    }

    if (status.queue.unique_id != 0)
    {
        index = ghb_find_queue_job(ud->queue, status.queue.unique_id,
                                   &queueDict);
        if ((status.queue.state & GHB_STATE_WORKING) &&
            (event_sequence % 50 == 0)) // check every 10 seconds
        {
            ghb_low_disk_check(ud);
        }
    }

    if (status.queue.state & GHB_STATE_SCANNING)
    {
        // This needs to be in scanning and working since scanning
        // happens fast enough that it can be missed
        gtk_label_set_text(work_status, _("Scanning ..."));
        gtk_progress_bar_set_fraction(progress, status.queue.progress);
        ghb_queue_update_live_stats(ud, index, &status.queue);
        ghb_queue_progress_set_fraction(ud, index, status.queue.progress);
    }
    else if (status.queue.state & GHB_STATE_SCANDONE)
    {
        ghb_clear_queue_state(GHB_STATE_SCANDONE);
    }
    else if (status.queue.state & GHB_STATE_PAUSED)
    {
        gtk_label_set_text (work_status, _("Paused"));
        ghb_queue_update_live_stats(ud, index, &status.queue);
    }
    else if (status.queue.state & GHB_STATE_SEARCHING)
    {
        gchar *status_str;

        status_str = searching_status_string(ud, &status.queue);
        gtk_label_set_text (work_status, status_str);
        gtk_progress_bar_set_fraction(progress, status.queue.progress);
        ghb_queue_progress_set_fraction(ud, index, status.queue.progress);
        g_free(status_str);
    }
    else if (status.queue.state & GHB_STATE_WORKING)
    {
        gchar *status_str;

        status_str = working_status_string(ud, &status.queue);
        gtk_label_set_text (work_status, status_str);
        gtk_progress_bar_set_fraction (progress, status.queue.progress);
        ghb_queue_update_live_stats(ud, index, &status.queue);
        ghb_queue_progress_set_fraction(ud, index, status.queue.progress);
        g_free(status_str);
    }
    else if (status.queue.state & GHB_STATE_WORKDONE)
    {
        gint qstatus;

        switch (status.queue.error)
        {
            case GHB_ERROR_NONE:
                gtk_label_set_text(work_status, _("Encode Done!"));
                qstatus = GHB_QUEUE_DONE;
                break;
            case GHB_ERROR_CANCELED:
                gtk_label_set_text(work_status, _("Encode Canceled."));
                qstatus = GHB_QUEUE_CANCELED;
                break;
            case GHB_ERROR_FAIL:
            default:
                gtk_label_set_text(work_status, _("Encode Failed."));
                qstatus = GHB_QUEUE_FAIL;
        }
        if (queueDict != NULL)
        {
            GhbValue *uiDict = ghb_dict_get(queueDict, "uiSettings");
            ghb_dict_set_int(uiDict, "job_status", qstatus);
            time_t now = time(NULL);
            ghb_dict_set_int(uiDict, "job_finish_time", now);
            ghb_dict_set_int(uiDict, "job_pause_time_ms", status.queue.paused);
        }
        ghb_queue_update_status_icon(ud, index);
        ghb_queue_update_live_stats(ud, index, &status.queue);
        gtk_progress_bar_set_fraction(progress, 1.0);
        ghb_queue_progress_set_visible(ud, index, FALSE);
        ghb_queue_progress_set_fraction(ud, index, 1.0);

        ghb_clear_queue_state(GHB_STATE_WORKDONE);
        if (ud->job_activity_log)
            g_io_channel_unref(ud->job_activity_log);
        ud->job_activity_log = NULL;
        if (ghb_dict_get_bool(ud->prefs, "RemoveFinishedJobs") &&
            status.queue.error == GHB_ERROR_NONE)
        {
            ghb_queue_remove_row(ud, index);
        }
        if (ud->cancel_encode != GHB_CANCEL_ALL &&
            ud->cancel_encode != GHB_CANCEL_FINISH)
        {
            ghb_start_next_job(ud);
        }
        else
        {
            uninhibit_suspend(ud);
            gtk_widget_hide(GTK_WIDGET(progress));
            ghb_dict_set_bool(ud->globals, "SkipDiskFreeCheck", FALSE);
        }
        ghb_save_queue(ud->queue);
        ud->cancel_encode = GHB_CANCEL_NONE;
    }
    else if (status.queue.state & GHB_STATE_MUXING)
    {
        gtk_label_set_text (work_status, _("Muxing: This may take a while..."));
    }

    if (status.live.state & GHB_STATE_WORKING)
    {
        GtkProgressBar *live_progress;
        live_progress = GTK_PROGRESS_BAR(
            GHB_WIDGET(ud->builder, "live_encode_progress"));
        status_str = working_status_string(ud, &status.live);
        gtk_progress_bar_set_text (live_progress, status_str);
        gtk_progress_bar_set_fraction (live_progress, status.live.progress);
        g_free(status_str);
    }
    if (status.live.state & GHB_STATE_WORKDONE)
    {
        switch( status.live.error )
        {
            case GHB_ERROR_NONE:
            {
                ghb_live_encode_done(ud, TRUE);
            } break;
            default:
            {
                ghb_live_encode_done(ud, FALSE);
            } break;
        }
        ghb_clear_live_state(GHB_STATE_WORKDONE);
    }
}

G_MODULE_EXPORT gboolean
ghb_timer_cb(gpointer data)
{
    signal_user_data_t *ud = (signal_user_data_t*)data;

    ghb_live_preview_progress(ud);
    ghb_backend_events(ud);
    if (update_preview)
    {
        g_debug("Updating preview");
        ghb_set_preview_image(ud);
        update_preview = FALSE;
    }

#if !defined(_NO_UPDATE_CHECK)
    if (!appcast_busy)
    {
        const gchar *updates;
        updates = ghb_dict_get_string(ud->prefs, "check_updates");
        gint64 duration = 0;
        if (strcmp(updates, "daily") == 0)
            duration = 60 * 60 * 24;
        else if (strcmp(updates, "weekly") == 0)
            duration = 60 * 60 * 24 * 7;
        else if (strcmp(updates, "monthly") == 0)
            duration = 60 * 60 * 24 * 7;

        if (duration != 0)
        {
            gint64 last;
            time_t tt;

            last = ghb_dict_get_int(ud->prefs, "last_update_check");
            time(&tt);
            if (last + duration < tt)
            {
                ghb_dict_set_int(ud->prefs,
                                        "last_update_check", tt);
                ghb_pref_save(ud->prefs, "last_update_check");
                GHB_THREAD_NEW("Update Check", G_THREAD_FUNC(ghb_check_update), ud);
            }
        }
    }
#endif
    return TRUE;
}

static void
append_activity(GtkTextBuffer * tb, const char * text, gsize length)
{
    if (text == NULL || length <= 0)
    {
        return;
    }
    GtkTextIter     iter;

    gtk_text_buffer_get_end_iter(tb, &iter);
    gtk_text_buffer_insert(tb, &iter, text, -1);
}

G_MODULE_EXPORT gboolean
ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data)
{
    gchar *text = NULL;
    gsize length, outlength;
    GError *gerror = NULL;
    GIOStatus status;

    signal_user_data_t *ud = (signal_user_data_t*)data;

    status = g_io_channel_read_line (source, &text, &length, NULL, &gerror);
    // Trim nils from end of text, they cause g_io_channel_write_chars to
    // fail with an assertion that aborts
    while (length > 0 && text[length-1] == 0)
        length--;
    if (text != NULL && length > 0)
    {
        gchar *utf8_text;

        // Assume logging is in current locale
        utf8_text = g_locale_to_utf8(text, -1, NULL, &length, NULL);
        if (utf8_text != NULL)
        {
            // Write to activity windows
            append_activity(ud->activity_buffer, utf8_text, length);
            append_activity(ud->queue_activity_buffer, utf8_text, length);

            // Write log files
#if defined(_WIN32)
            gsize one = 1;
            utf8_text[length-1] = '\r';
#endif
            if (ud->activity_log != NULL)
            {
                g_io_channel_write_chars (ud->activity_log, utf8_text,
                                        length, &outlength, NULL);
#if defined(_WIN32)
                g_io_channel_write_chars (ud->activity_log, "\n",
                                        one, &one, NULL);
#endif
                g_io_channel_flush(ud->activity_log, NULL);
            }
            if (ud->job_activity_log)
            {
                g_io_channel_write_chars (ud->job_activity_log, utf8_text,
                                        length, &outlength, NULL);
#if defined(_WIN32)
                g_io_channel_write_chars (ud->activity_log, "\n",
                                        one, &outlength, NULL);
#endif
                g_io_channel_flush(ud->job_activity_log, NULL);
            }
            g_free(utf8_text);
        }
    }
    if (text != NULL)
        g_free(text);

    if (status != G_IO_STATUS_NORMAL)
    {
        // This should never happen, but if it does I would get into an
        // infinite loop.  Returning false removes this callback.
        g_warning("Error while reading activity from pipe");
        if (gerror != NULL)
        {
            g_warning("%s", gerror->message);
            g_error_free (gerror);
        }
        return FALSE;
    }
    if (gerror != NULL)
        g_error_free (gerror);
    return TRUE;
}

G_MODULE_EXPORT void
show_activity_action_cb(GSimpleAction *action, GVariant *value,
                        signal_user_data_t *ud)
{
    GtkWidget * activity_window;
    gboolean    state = g_variant_get_boolean(value);

    g_simple_action_set_state(action, value);
    activity_window = GHB_WIDGET(ud->builder, "activity_window");
    gtk_widget_set_visible(activity_window, state);
}

G_MODULE_EXPORT gboolean
activity_window_delete_cb(
    GtkWidget *xwidget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    gtk_widget_set_visible(xwidget, FALSE);
    GtkWidget *widget = GHB_WIDGET (ud->builder, "show_activity");
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), FALSE);
    return TRUE;
}

void
ghb_log(gchar *log, ...)
{
    va_list args;
    time_t _now;
    struct tm *now;
    gchar fmt[362];

    _now = time(NULL);
    now = localtime( &_now );
    snprintf(fmt, 362, "[%02d:%02d:%02d] gtkgui: %s\n",
            now->tm_hour, now->tm_min, now->tm_sec, log);
    va_start(args, log);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void
ghb_browse_uri(signal_user_data_t *ud, const gchar *uri)
{
#if defined(_WIN32)
    ShellExecute(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
#else
    GtkWindow * parent;
    gboolean    result;

    parent = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    result = gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, NULL);
    if (result) return;
    char *argv[] =
        {"xdg-open",NULL,NULL,NULL};
    argv[1] = (gchar*)uri;
    result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                NULL, NULL, NULL);
    if (result) return;

    argv[0] = "gnome-open";
    result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                NULL, NULL, NULL);
    if (result) return;

    argv[0] = "kfmclient";
    argv[1] = "exec";
    argv[2] = (gchar*)uri;
    result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                NULL, NULL, NULL);
    if (result) return;

    argv[0] = "firefox";
    argv[1] = (gchar*)uri;
    argv[2] = NULL;
    result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                NULL, NULL, NULL);
#endif
}

G_MODULE_EXPORT void
about_action_cb(GSimpleAction *action, GVariant *param, signal_user_data_t *ud)
{
    GtkWidget *widget = GHB_WIDGET (ud->builder, "hb_about");
    gchar *ver;

    ver = g_strdup_printf("%s (%s)", HB_PROJECT_VERSION, HB_PROJECT_HOST_ARCH);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(widget), ver);
    g_free(ver);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(widget),
                                HB_PROJECT_URL_WEBSITE);
    gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(widget),
                                        HB_PROJECT_URL_WEBSITE);
    gtk_dialog_run(GTK_DIALOG(widget));
    gtk_widget_hide(widget);
}

#define HB_DOCS "https://handbrake.fr/docs/"

G_MODULE_EXPORT void
guide_action_cb(GSimpleAction *action, GVariant *param, signal_user_data_t *ud)
{
    ghb_browse_uri(ud, HB_DOCS);
}

static void
update_queue_labels(signal_user_data_t *ud)
{
    GtkToolButton *button;
    gint           pending;
    const gchar   *show_hide;
    gchar         *str;

    button  = GTK_TOOL_BUTTON(GHB_WIDGET(ud->builder, "show_queue"));
    pending = queue_pending_count(ud->queue);

    show_hide = _("Queue");
    if (pending > 0)
    {
        str = g_strdup_printf("%s (%d)", show_hide, pending);
    }
    else
    {
        str = g_strdup_printf("%s", show_hide);
    }
    gtk_tool_button_set_label(button, str);
    g_free(str);
}

static void
presets_window_set_visible(signal_user_data_t *ud, gboolean visible)
{
    GtkWidget     * presets_window;
    GtkWidget     * hb_window;

    hb_window = GHB_WIDGET(ud->builder, "hb_window");
    if (!gtk_widget_is_visible(hb_window))
    {
        return;
    }

    int w, h;
    w = ghb_dict_get_int(ud->prefs, "presets_window_width");
    h = ghb_dict_get_int(ud->prefs, "presets_window_height");

    presets_window = GHB_WIDGET(ud->builder, "presets_window");
    if (w > 200 && h > 200)
    {
        gtk_window_resize(GTK_WINDOW(presets_window), w, h);
    }
    gtk_widget_set_visible(presets_window, visible);

#if !GTK_CHECK_VERSION(4, 4, 0)
    // TODO: Is this possible in GTK4?
    int             x, y;

    if (visible)
    {
        gtk_window_get_position(GTK_WINDOW(hb_window), &x, &y);
        x -= w + 10;
        if (x < 0)
        {
            gtk_window_move(GTK_WINDOW(hb_window), w + 10, y);
            x = 0;
        }
        gtk_window_move(GTK_WINDOW(presets_window), x, y);
    }
#endif
}

G_MODULE_EXPORT void
show_presets_action_cb(GSimpleAction *action, GVariant *value,
                       signal_user_data_t *ud)
{
    gboolean state = g_variant_get_boolean(value);

    g_simple_action_set_state(action, value);
    presets_window_set_visible(ud, state);
}

void
ghb_hbfd(signal_user_data_t *ud, gboolean hbfd)
{
    GtkWidget *widget;
    ghb_log_func();
    widget = GHB_WIDGET(ud->builder, "queue_pause");
    gtk_widget_set_visible(widget, !hbfd);
    widget = GHB_WIDGET(ud->builder, "queue_add");
    gtk_widget_set_visible(widget, !hbfd);
    widget = GHB_WIDGET(ud->builder, "show_queue");
    gtk_widget_set_visible(widget, !hbfd);
    widget = GHB_WIDGET(ud->builder, "show_activity");
    gtk_widget_set_visible(widget, !hbfd);

    widget = GHB_WIDGET(ud->builder, "SettingsStackSwitcher");
    gtk_widget_set_visible(widget, !hbfd);
    widget = GHB_WIDGET(ud->builder, "SettingsStack");
    gtk_widget_set_visible(widget, !hbfd);
    widget = GHB_WIDGET (ud->builder, "hb_window");
    gtk_window_resize(GTK_WINDOW(widget), 16, 16);

}

G_MODULE_EXPORT void
hbfd_action_cb(GSimpleAction *action, GVariant *value, signal_user_data_t *ud)
{
    gboolean state = g_variant_get_boolean(value);

    g_simple_action_set_state(action, value);
    ghb_dict_set(ud->prefs, "hbfd", ghb_boolean_value(state));
    ghb_hbfd(ud, state);
    ghb_pref_save(ud->prefs, "hbfd");
}

G_MODULE_EXPORT void
activity_font_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{

    ghb_widget_to_setting(ud->prefs, widget);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);

    int size = ghb_dict_get_int(ud->prefs, "ActivityFontSize");

    const gchar *css_template =
        "                                   \n\
        #activity_view                      \n\
        {                                   \n\
            font-family: monospace;         \n\
            font-size: %dpt;                \n\
            font-weight: 300;               \n\
        }                                   \n\
        ";
    char           * css      = g_strdup_printf(css_template, size);
    GtkCssProvider * provider = gtk_css_provider_new();

    ghb_css_provider_load_from_data(provider, css, -1);
    GtkWidget * win = GHB_WIDGET(ud->builder, "hb_window");
#if GTK_CHECK_VERSION(4, 4, 0)
    GdkDisplay *dd = gtk_widget_get_display(win);
    gtk_style_context_add_provider_for_display(dd,
                                GTK_STYLE_PROVIDER(provider),
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#else
    GdkScreen *ss = gtk_window_get_screen(GTK_WINDOW(win));
    gtk_style_context_add_provider_for_screen(ss,
                                GTK_STYLE_PROVIDER(provider),
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif
    g_object_unref(provider);
    g_free(css);
}

// Changes the setting for the current session
// and also saves it for future sessions
G_MODULE_EXPORT void
when_complete_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue * value = ghb_widget_value(widget);
    ud->when_complete = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    ghb_ui_update(ud, "MainWhenComplete", value);
    ghb_ui_update(ud, "QueueWhenComplete", value);
    ghb_value_free(&value);

    ghb_widget_to_setting (ud->prefs, widget);

    ghb_check_dependency(ud, widget, NULL);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);
    ghb_prefs_store();
}

// Only changes the setting for the current session
G_MODULE_EXPORT void
temp_when_complete_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue * value = ghb_widget_value(widget);
    ud->when_complete = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    ghb_ui_update(ud, "MainWhenComplete", value);
    ghb_ui_update(ud, "QueueWhenComplete", value);
    ghb_value_free(&value);
}

G_MODULE_EXPORT void
pref_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting (ud->prefs, widget);

    ghb_check_dependency(ud, widget, NULL);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);
}

G_MODULE_EXPORT void
log_level_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    pref_changed_cb(widget, ud);
    int level = ghb_dict_get_int(ud->prefs, "LoggingLevel");
    ghb_log_level_set(level);
}

G_MODULE_EXPORT void
use_m4v_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting (ud->prefs, widget);
    ghb_check_dependency(ud, widget, NULL);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);
    ghb_update_destination_extension(ud);
}

G_MODULE_EXPORT void
tmp_dir_enable_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    pref_changed_cb(widget, ud);
    prefs_require_restart = TRUE;
}

G_MODULE_EXPORT void
temp_dir_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    char * orig_tmp_dir = NULL;
    const char * tmp_dir;

    tmp_dir = ghb_dict_get_string(ud->prefs, "CustomTmpDir");
    if (tmp_dir != NULL)
    {
        orig_tmp_dir = g_strdup(tmp_dir);
    }
    ghb_widget_to_setting (ud->prefs, widget);
    ghb_check_dependency(ud, widget, NULL);

    tmp_dir = ghb_dict_get_string(ud->prefs, "CustomTmpDir");
    if (tmp_dir == NULL)
    {
        tmp_dir = "";
    }
    if (orig_tmp_dir == NULL ||
        strcmp(orig_tmp_dir, tmp_dir))
    {
        ghb_pref_set(ud->prefs, "CustomTmpDir");
        prefs_require_restart = TRUE;
    }
}

G_MODULE_EXPORT void
vqual_granularity_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting (ud->prefs, widget);
    ghb_check_dependency(ud, widget, NULL);

    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);

    float val, vqmin, vqmax, step, page;
    int inverted, digits;

    ghb_vquality_range(ud, &vqmin, &vqmax, &step, &page, &digits, &inverted);
    val = ghb_dict_get_double(ud->settings, "VideoQualitySlider");
    ghb_scale_configure(ud, "VideoQualitySlider", val, vqmin, vqmax,
                        step, page, digits, inverted);
}

G_MODULE_EXPORT void
tweaks_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting (ud->prefs, widget);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);
}

G_MODULE_EXPORT void
show_preview_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_log_func();
    ghb_widget_to_setting (ud->prefs, widget);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);
    mini_preview_update(TRUE, ud);
}

G_MODULE_EXPORT void
hbfd_feature_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting (ud->prefs, widget);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_set(ud->prefs, name);

    gboolean hbfd = ghb_dict_get_bool(ud->prefs, "hbfd_feature");
    GMenu *view_menu = G_MENU(gtk_builder_get_object(ud->builder, "view-menu"));
    GMenuModel *hbfd_menu = G_MENU_MODEL(gtk_builder_get_object(ud->builder, "hbfd-section"));
    if (hbfd)
    {
        const GhbValue *val;
        val = ghb_dict_get_value(ud->prefs, "hbfd");
        ghb_ui_settings_update(ud, ud->prefs, "hbfd", val);
        GMenuItem *hbfd_item = g_menu_item_new_from_model(hbfd_menu, 0);
        g_menu_prepend_item(view_menu, hbfd_item);
    }
    else
    {
        g_menu_remove(view_menu, 0);
    }
}

gboolean
ghb_file_menu_add_dvd(signal_user_data_t *ud)
{
    GList *link, *drives;

    ghb_log_func();
    GMenu *dvd_menu = G_MENU(gtk_builder_get_object(ud->builder, "dvd-list"));

    // Clear previous dvd items from list
    g_menu_remove_all(dvd_menu);

    link = drives = dvd_device_list();
    if (drives != NULL)
    {
        GMenuModel *dvd_template = G_MENU_MODEL(gtk_builder_get_object(ud->builder, "dvd"));

        while (link != NULL)
        {
            gchar *action = g_strdup_printf("app.dvd-open('%s')", get_dvd_device_name(link->data));
            gchar *name = get_dvd_volume_name(link->data);

            GMenuItem *item = g_menu_item_new_from_model(dvd_template, 0);
            g_menu_item_set_label(item, name);
            g_menu_item_set_detailed_action(item, action);
            g_menu_append_item(dvd_menu, item);

            g_free(name);
            g_free(action);
            free_drive(link->data);
            link = link->next;
        }

        g_list_free(drives);
    }

    return FALSE;
}

gboolean ghb_is_cd(GDrive *gd);

static GList*
dvd_device_list (void)
{
    GList *dvd_devices = NULL;

#if defined(_WIN32)
    gint ii, drives;
    gchar drive[5];

    strcpy(drive, "A:" G_DIR_SEPARATOR_S);
    drives = GetLogicalDrives();
    for (ii = 0; ii < 26; ii++)
    {
        if (drives & 0x01)
        {
            guint dtype;

            drive[0] = 'A' + ii;
            dtype = GetDriveType(drive);
            if (dtype == DRIVE_CDROM)
            {
                dvd_devices = g_list_append(dvd_devices,
                        (gpointer)g_strdup(drive));
            }
        }
        drives >>= 1;
    }
#else
    GVolumeMonitor *gvm;
    GList *drives, *link;

    gvm = g_volume_monitor_get ();
    drives = g_volume_monitor_get_connected_drives (gvm);
    link = drives;
    while (link != NULL)
    {
        GDrive *gd;

        gd = (GDrive*)link->data;
        if (ghb_is_cd(gd))
        {
            dvd_devices = g_list_append(dvd_devices, gd);
        }
        else
            g_object_unref (gd);
        link = link->next;
    }
    g_list_free(drives);
#endif

    return dvd_devices;
}

#if defined(__linux__) && defined(_HAVE_GUDEV)
static GUdevClient *udev_ctx = NULL;
#endif

gboolean
ghb_is_cd(GDrive *gd)
{
#if defined(__linux__) && defined(_HAVE_GUDEV)
    gchar *device;
    GUdevDevice *udd;

    if (udev_ctx == NULL)
        return FALSE;

    device = g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (device == NULL)
        return FALSE;

    udd = g_udev_client_query_by_device_file(udev_ctx, device);
    if (udd == NULL)
    {
        g_message("udev: Failed to lookup device %s", device);
        g_free(device);
        return FALSE;
    }
    g_free(device);

    gint val;
    val = g_udev_device_get_property_as_int(udd, "ID_CDROM_DVD");
    g_object_unref(udd);
    if (val == 1)
        return TRUE;

    return FALSE;
#else
    return FALSE;
#endif
}

void
ghb_udev_init (void)
{
#if defined(__linux__) && defined(_HAVE_GUDEV)
    udev_ctx = g_udev_client_new(NULL);
#endif
}

#if defined(_WIN32)
static void
handle_media_change(const gchar *device, gboolean insert, signal_user_data_t *ud)
{
    guint dtype;
    static gint ins_count = 0;
    static gint rem_count = 0;

    // The media change event in windows bounces around a bit
    // so I debounce it here
    // DVD insertion detected.  Scan it.
    dtype = GetDriveType(device);
    if (dtype != DRIVE_CDROM)
        return;
    if (insert)
    {
        rem_count = 0;
        ins_count++;
        if (ins_count == 2)
        {
            GHB_THREAD_NEW("Cache Volume Names",
                    (GThreadFunc)ghb_cache_volnames, ud);
            if (ghb_dict_get_bool(ud->prefs, "AutoScan") &&
                ud->current_dvd_device != NULL &&
                strcmp(device, ud->current_dvd_device) == 0)
            {
                show_scan_progress(ud);
                gint preview_count;
                preview_count = ghb_dict_get_int(ud->prefs, "preview_count");
                ghb_dict_set_string(ud->globals, "scan_source", device);
                start_scan(ud, device, 0, preview_count);
            }
        }
    }
    else
    {
        ins_count = 0;
        rem_count++;
        if (rem_count == 2)
        {
            GHB_THREAD_NEW("Cache Volume Names",
                    (GThreadFunc)ghb_cache_volnames, ud);
            if (ud->current_dvd_device != NULL &&
                strcmp(device, ud->current_dvd_device) == 0)
            {
                ghb_hb_cleanup(TRUE);
                prune_logs(ud);
                ghb_dict_set_string(ud->globals, "scan_source", "/dev/null");
                start_scan(ud, "/dev/null", 0, 1);
            }
        }
    }
}

static gchar
FindDriveFromMask(ULONG unitmask)
{
    gchar cc;
    for (cc = 0; cc < 26; cc++)
    {
        if (unitmask & 0x01)
            return 'A' + cc;
        unitmask >>= 1;
    }
    return 0;
}

void
wm_drive_changed(MSG *msg, signal_user_data_t *ud)
{
    PDEV_BROADCAST_HDR bch = (PDEV_BROADCAST_HDR)msg->lParam;
    gchar drive[4];

    g_strlcpy(drive, "A:" G_DIR_SEPARATOR_S, 4);
    switch (msg->wParam)
    {
        case DBT_DEVICEARRIVAL:
        {
            if (bch->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME bcv = (PDEV_BROADCAST_VOLUME)bch;

                if (bcv->dbcv_flags & DBTF_MEDIA)
                {
                    drive[0] = FindDriveFromMask(bcv->dbcv_unitmask);
                    handle_media_change(drive, TRUE, ud);
                }
            }
        } break;

        case DBT_DEVICEREMOVECOMPLETE:
        {
            if (bch->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME bcv = (PDEV_BROADCAST_VOLUME)bch;

                if (bcv->dbcv_flags & DBTF_MEDIA)
                {
                    drive[0] = FindDriveFromMask(bcv->dbcv_unitmask);
                    handle_media_change(drive, FALSE, ud);
                }
            }
        } break;
        default: ;
    }
}

#else

G_MODULE_EXPORT void
drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, signal_user_data_t *ud)
{
    gchar *device;
    gint state;

    ghb_log_func();
    GHB_THREAD_NEW("Cache Volume Names", (GThreadFunc)ghb_cache_volnames, ud);

    state = ghb_get_scan_state();
    device = g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (ud->current_dvd_device == NULL ||
        strcmp(device, ud->current_dvd_device) != 0 ||
        state != GHB_STATE_IDLE )
    {
        return;
    }
    if (g_drive_has_media(gd))
    {
        if (ghb_dict_get_bool(ud->prefs, "AutoScan"))
        {
            show_scan_progress(ud);
            gint preview_count;
            preview_count = ghb_dict_get_int(ud->prefs, "preview_count");
            ghb_dict_set_string(ud->globals, "scan_source", device);
            start_scan(ud, device, 0, preview_count);
        }
    }
    else
    {
        ghb_hb_cleanup(TRUE);
        prune_logs(ud);
        ghb_dict_set_string(ud->globals, "scan_source", "/dev/null");
        start_scan(ud, "/dev/null", 0, 1);
    }
}
#endif

G_MODULE_EXPORT void
easter_egg_multi_cb(
    GtkGesture         * gest,
    gint                 n_press,
    gdouble              x,
    gdouble              y,
    signal_user_data_t * ud)
{
    if (n_press == 3)
    {
        GtkWidget *widget;
        widget = GHB_WIDGET(ud->builder, "allow_tweaks");
        gtk_widget_set_visible(widget, !gtk_widget_get_visible(widget));
        widget = GHB_WIDGET(ud->builder, "hbfd_feature");
        gtk_widget_set_visible(widget, !gtk_widget_get_visible(widget));
    }
}

G_MODULE_EXPORT gchar*
format_deblock_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    if (val < 5.0)
    {
        return g_strdup_printf(_("Off"));
    }
    else
    {
        return g_strdup_printf("%d", (gint)val);
    }
}

static void
process_appcast(signal_user_data_t *ud)
{
    gchar *description = NULL, *build = NULL, *version = NULL, *msg;
#if !defined(_WIN32) && !defined(_NO_UPDATE_CHECK)
    GtkWidget *window;
    static GtkWidget *html = NULL;
#endif
    GtkWidget *dialog, *label;
    gint    response, ibuild = 0, skip;

    if (ud->appcast == NULL || ud->appcast_len < 15 ||
        strncmp(&(ud->appcast[9]), "200 OK", 6))
    {
        goto done;
    }
    ghb_appcast_parse(ud->appcast, &description, &build, &version);
    if (build)
        ibuild = g_strtod(build, NULL);
    skip = ghb_dict_get_int(ud->prefs, "update_skip_version");
    if (description == NULL || build == NULL || version == NULL
        || ibuild <= hb_get_build(NULL) || skip == ibuild)
    {
        goto done;
    }
    msg = g_strdup_printf(_("HandBrake %s/%s is now available (you have %s/%d)."),
            version, build, hb_get_version(NULL), hb_get_build(NULL));
    label = GHB_WIDGET(ud->builder, "update_message");
    gtk_label_set_text(GTK_LABEL(label), msg);

#if !defined(_WIN32) && !defined(_NO_UPDATE_CHECK)
    if (html == NULL)
    {
        html = webkit_web_view_new();
        window = GHB_WIDGET(ud->builder, "update_scroll");
        gtk_container_add(GTK_CONTAINER(window), html);
        // Show it
        gtk_widget_set_size_request(html, 420, 240);
        gtk_widget_show(html);
    }
    webkit_web_view_open(WEBKIT_WEB_VIEW(html), description);
#endif
    dialog = GHB_WIDGET(ud->builder, "update_dialog");
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_OK)
    {
        // Skip
        ghb_dict_set_int(ud->prefs, "update_skip_version", ibuild);
        ghb_pref_save(ud->prefs, "update_skip_version");
    }
    g_free(msg);

done:
    if (description) g_free(description);
    if (build) g_free(build);
    if (version) g_free(version);
    g_free(ud->appcast);
    ud->appcast_len = 0;
    ud->appcast = NULL;
    appcast_busy = FALSE;
}

static void
net_close (GIOChannel *ioc)
{
    gint fd;

    ghb_log_func();
    if (ioc == NULL) return;
    fd = g_io_channel_unix_get_fd(ioc);
    close(fd);
    g_io_channel_unref(ioc);
}

G_MODULE_EXPORT gboolean
ghb_net_recv_cb(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
    gchar buf[2048];
    gsize len;
    GError *gerror = NULL;
    GIOStatus status;

    ghb_log_func();
    signal_user_data_t *ud = (signal_user_data_t*)data;

    status = g_io_channel_read_chars (ioc, buf, 2048, &len, &gerror);
    if ((status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_EOF) &&
        len > 0)
    {
        gint new_len = ud->appcast_len + len;
        ud->appcast = g_realloc(ud->appcast, new_len + 1);
        memcpy(&(ud->appcast[ud->appcast_len]), buf, len);
        ud->appcast_len = new_len;
    }
    if (status == G_IO_STATUS_EOF)
    {
        if ( ud->appcast != NULL )
        {
            ud->appcast[ud->appcast_len] = 0;
        }
        net_close(ioc);
        process_appcast(ud);
        return FALSE;
    }
    return TRUE;
}

static GIOChannel*
net_open(signal_user_data_t *ud, gchar *address, gint port)
{
    GIOChannel *ioc;
    gint fd;

    struct sockaddr_in   sock;
    struct hostent     * host;

    ghb_log_func();
    if( !( host = gethostbyname( address ) ) )
    {
        g_warning( "gethostbyname failed (%s)", address );
        appcast_busy = FALSE;
        return NULL;
    }

    memset( &sock, 0, sizeof( struct sockaddr_in ) );
    sock.sin_family = host->h_addrtype;
    sock.sin_port   = htons( port );
    memcpy( &sock.sin_addr, host->h_addr, host->h_length );

    fd = socket(host->h_addrtype, SOCK_STREAM, 0);
    if( fd < 0 )
    {
        g_debug( "socket failed" );
        appcast_busy = FALSE;
        return NULL;
    }

    if(connect(fd, (struct sockaddr*)&sock, sizeof(struct sockaddr_in )) < 0 )
    {
        g_debug( "connect failed" );
        appcast_busy = FALSE;
        return NULL;
    }
    ioc = g_io_channel_unix_new(fd);
    g_io_channel_set_encoding (ioc, NULL, NULL);
    g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch (ioc, G_IO_IN, ghb_net_recv_cb, (gpointer)ud );

    return ioc;
}

gpointer ghb_check_update (signal_user_data_t *ud)
{
    gchar *query;
    gsize len;
    GIOChannel *ioc;
    GError *gerror = NULL;
    GRegex *regex;
    GMatchInfo *mi;
    gchar *host, *appcast;

    ghb_log_func();
    appcast_busy = TRUE;
    regex = g_regex_new("^http://(.+)/(.+)$", 0, 0, NULL);
    if (!g_regex_match(regex, HB_PROJECT_URL_APPCAST, 0, &mi))
    {
        return NULL;
    }

    host = g_match_info_fetch(mi, 1);
    appcast = g_match_info_fetch(mi, 2);

    if (host == NULL || appcast == NULL)
        return NULL;

    query = g_strdup_printf("GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n",
                            appcast, host);

    ioc = net_open(ud, host, 80);
    if (ioc == NULL)
        goto free_resources;

    g_io_channel_write_chars(ioc, query, strlen(query), &len, &gerror);
    g_io_channel_flush(ioc, &gerror);

free_resources:
    g_free(query);
    g_free(host);
    g_free(appcast);
    g_match_info_free(mi);
    g_regex_unref(regex);
    return NULL;
}

void
ghb_notify_done(signal_user_data_t *ud)
{
    if (ud->when_complete == 0)
        return;

    GNotification * notification;
    GIcon         * icon;

    notification = g_notification_new(_("Encode Complete"));
    g_notification_set_body(notification,
        _("Put down that cocktail, Your HandBrake queue is done!"));
    icon = g_themed_icon_new("fr.handbrake.ghb");
    g_notification_set_icon(notification, icon);

    g_application_send_notification(G_APPLICATION(ud->app), "cocktail", notification);
    g_object_unref(G_OBJECT(notification));
    g_object_unref(G_OBJECT(icon));

    switch (ud->when_complete)    {
	    case 2:
            ghb_countdown_dialog(GTK_MESSAGE_INFO,
                                _("Your encode is complete."),
                                _("Quitting Handbrake"),
                                _("Cancel"), (GSourceFunc)quit_cb, ud, 60);
            break;
        case 3:
            if (can_suspend_logind())
            {
                ghb_countdown_dialog(GTK_MESSAGE_INFO,
                    _("Your encode is complete."),
                    _("Putting computer to sleep"),
                    _("Cancel"), (GSourceFunc)suspend_cb, ud, 60);
            }
            break;
	    case 4:
            if (can_shutdown_logind())
            {
                ghb_countdown_dialog(GTK_MESSAGE_INFO,
                    _("Your encode is complete."),
                    _("Shutting down the computer"),
                    _("Cancel"), (GSourceFunc)shutdown_cb, ud, 60);
            }

        default:
            break;    }
}

G_MODULE_EXPORT gboolean
window_map_cb(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEventAny *event,
#endif
    signal_user_data_t *ud)
{
    return FALSE;
}

G_MODULE_EXPORT void
hb_win_sz_alloc_cb(
    GtkWidget *widget,
#if GTK_CHECK_VERSION(4, 4, 0)
    int width,
    int height,
    int baseline,
#else
    GdkRectangle *rect,
#endif
    signal_user_data_t *ud)
{
    if (gtk_widget_get_visible(widget))
    {
        gint w, h, ww, wh;
        w = ghb_dict_get_int(ud->prefs, "window_width");
        h = ghb_dict_get_int(ud->prefs, "window_height");

        gtk_window_get_size(GTK_WINDOW(widget), &ww, &wh);
        if ( w != ww || h != wh )
        {
            ghb_dict_set_int(ud->prefs, "window_width", ww);
            ghb_dict_set_int(ud->prefs, "window_height", wh);
            ghb_pref_set(ud->prefs, "window_width");
            ghb_pref_set(ud->prefs, "window_height");
            ghb_prefs_store();
        }
    }
}

static void container_empty_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(widget);
}

void ghb_container_empty(GtkContainer *c)
{
    gtk_container_foreach(c, container_empty_cb, NULL);
}

static void
lang_combo_search(
    GtkComboBox * combo,
    guint         keyval,
    signal_user_data_t *ud)
{
    if (combo == NULL)
    {
        return;
    }
    GtkTreeModel * model = GTK_TREE_MODEL(gtk_combo_box_get_model(combo));
    GtkTreeIter    iter, prev_iter;
    gchar        * lang;
    gunichar       key_char;
    gunichar       first_char;
    int            pos = 0, count = 2048;

    key_char = g_unichar_toupper(gdk_keyval_to_unicode(keyval));
    if (gtk_combo_box_get_active_iter(combo, &iter))
    {
        while (pos <= count)
        {
            pos++;
            prev_iter = iter;
            if (!gtk_tree_model_iter_next(model, &iter))
            {
                GtkTreePath * path = gtk_tree_model_get_path(model, &prev_iter);
                gint        * ind  = gtk_tree_path_get_indices(path);;

                count = ind[0];
                gtk_tree_path_free(path);
                gtk_tree_model_get_iter_first(model, &iter);
            }
            gtk_tree_model_get(model, &iter, 0, &lang, -1);
            first_char = g_unichar_toupper(g_utf8_get_char(lang));
            g_free(lang);
            if (first_char == key_char)
            {
                gtk_combo_box_set_active_iter(combo, &iter);
                break;
            }
        }
    }
}

G_MODULE_EXPORT
void on_presets_list_press_cb (GtkGesture *gest, gint n_press, gdouble x,
                               gdouble y, signal_user_data_t *ud)
{
    if (n_press == 1)
    {
        GtkMenu *context_menu = GTK_MENU(GHB_WIDGET(ud->builder, "presets_window_submenu"));
        gtk_menu_popup_at_pointer(context_menu, NULL);
    }
}


GtkFileFilter *ghb_add_file_filter(GtkFileChooser *chooser,
                                   signal_user_data_t *ud,
                                   const char *name, const char *id)
{
    g_autoptr(GtkFileFilter) filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, id));
    gtk_file_filter_set_name(filter, name);
    gtk_file_chooser_add_filter(chooser, filter);
    return filter;
}

static void
add_video_file_filters (GtkFileChooser *chooser, signal_user_data_t *ud)
{
    ghb_add_file_filter(chooser, ud, _("All Files"), "SourceFilterAll");
    ghb_add_file_filter(chooser, ud, _("Video"), "SourceFilterVideo");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/mp4"), "SourceFilterMP4");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/mp2t"), "SourceFilterTS");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/mpeg"), "SourceFilterMPG");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/x-matroska"), "SourceFilterMKV");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/webm"), "SourceFilterWebM");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/ogg"), "SourceFilterOGG");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/x-msvideo"), "SourceFilterAVI");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/x-flv"), "SourceFilterFLV");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/quicktime"), "SourceFilterMOV");
    ghb_add_file_filter(chooser, ud, g_content_type_get_description("video/x-ms-wmv"), "SourceFilterWMV");
    ghb_add_file_filter(chooser, ud, "EVO", "SourceFilterEVO");
    ghb_add_file_filter(chooser, ud, "VOB", "SourceFilterVOB");
}
#if GTK_CHECK_VERSION(4, 4, 0)
G_MODULE_EXPORT gboolean
combo_search_key_press_cb(
    GtkEventControllerKey * keycon,
    guint                   keyval,
    guint                   keycode,
    GdkModifierType         state,
    signal_user_data_t    * ud)
{
    GtkComboBox  * combo;

    combo = GTK_COMBO_BOX(gtk_event_controller_get_widget(
                            GTK_EVENT_CONTROLLER(keycon)));
    lang_combo_search(combo, keyval, ud);
    return FALSE;
}
#else
G_MODULE_EXPORT gboolean
combo_search_key_press_cb(
    GtkWidget *widget,
    GdkEvent *event,
    signal_user_data_t *ud)
{
    guint          keyval;

    ghb_event_get_keyval(event, &keyval);
    lang_combo_search(GTK_COMBO_BOX(widget), keyval, ud);
    return FALSE;
}
#endif
