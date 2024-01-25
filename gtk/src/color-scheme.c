/* color-scheme.c
 *
 * Copyright (C) 2022-2024 HandBrake Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define G_LOG_DOMAIN "ghb"

#include "color-scheme.h"

#define APP_DARK  APP_PREFERS_DARK /* 1 */
#define APP_FORCE APP_FORCES_LIGHT /* 2 */
#define DBUS_TIMEOUT 1000 /* Timeout for D-Bus to respond in ms */

#if GLIB_CHECK_VERSION(2, 72, 0)
#define SETTING_CHANGED_SIGNAL "g-signal::SettingChanged"
#else
#define SETTING_CHANGED_SIGNAL "g-signal"
#endif

static AppColorScheme app_scheme = APP_FORCES_LIGHT;
static GDBusProxy *desktop_portal;
static GtkSettings *settings;
static gboolean init_done = FALSE;

static void
set_gtk_theme (gboolean dark)
{
    g_object_set(settings, "gtk-application-prefer-dark-theme", dark, NULL);
}

/*
 * Returns the currently selected app colour scheme.
 */
AppColorScheme
color_scheme_get_app_scheme (void)
{
    return app_scheme;
}

/*
 * Returns TRUE if the app is currently using the dark theme.
 */
gboolean
color_scheme_is_dark_theme (void)
{
    gboolean is_dark;

    g_object_get (settings, "gtk-application-prefer-dark-theme", &is_dark, NULL);
    return is_dark;
}

/*
 * Switches the color state of the app, no matter which color
 * is currently in use. Afterwards, the app color scheme is
 * changed to APP_FORCES_LIGHT or APP_FORCES_DARK, so changes
 * to the desktop color scheme no longer have any effect.
 */
gboolean
color_scheme_toggle (void)
{
    gboolean set_dark = !color_scheme_is_dark_theme();
    g_object_set(settings, "gtk-application-prefer-dark-theme",
                 set_dark, NULL);

    /* Change app_scheme to APP_FORCES_[DARK|LIGHT] */
    app_scheme = (AppColorScheme) (set_dark | APP_FORCE);

    return set_dark;
}

static void
setting_changed (GDBusProxy *proxy, char *sender_name, char *signal_name,
                  GVariant *parameters, gpointer user_data)
{
    const char *namespace_;
    const char *key;
    g_autoptr (GVariant) var = NULL;
    guint32 portal_value;
    gboolean set_dark;

    if (g_strcmp0(signal_name, "SettingChanged"))
        return;

    g_variant_get(parameters, "(&s&sv)", &namespace_, &key, &var);

    if (!g_strcmp0(namespace_, "org.freedesktop.appearance") &&
          !g_strcmp0(key, "color-scheme"))
    {
        g_variant_get(var, "u", &portal_value);
        set_dark = (portal_value == DESKTOP_PREFERS_DARK) ? TRUE : FALSE;
        if (!(app_scheme & APP_FORCE))
            set_gtk_theme(set_dark);
    }
}

static GDBusProxy *
portal_init (void)
{
    GDBusProxy *portal;
    GError *error = NULL;

    portal = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                            G_DBUS_PROXY_FLAGS_NONE, NULL,
                                            "org.freedesktop.portal.Desktop",
                                            "/org/freedesktop/portal/desktop",
                                            "org.freedesktop.portal.Settings",
                                            NULL, &error);
    if (!portal)
        g_debug ("Could not access portal: %s", error->message);

    return portal;
}

static void
set_theme (void)
{
    DesktopColorScheme desktop_scheme;
    gboolean set_dark;

    desktop_scheme = color_scheme_get_desktop_scheme();
    if ((app_scheme & APP_FORCE) || desktop_scheme == DESKTOP_NO_PREFERENCE)
        set_dark = (app_scheme & APP_DARK);
    else
        set_dark = (desktop_scheme == DESKTOP_PREFERS_DARK);

    set_gtk_theme(set_dark);
}

static void
init_color_scheme_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
    GDBusProxy *portal;
    GError *error = NULL;

    portal = g_dbus_proxy_new_for_bus_finish(res, &error);

    if (portal)
    {
        g_signal_connect(portal, SETTING_CHANGED_SIGNAL,
                         G_CALLBACK(setting_changed), NULL);
    }
    else
    {
        g_debug("Could not access portal: %s", error->message);
    }
    init_done = TRUE;
}

static void
init_color_scheme_async (void)
{
    static size_t init = 0;

    if (g_once_init_enter(&init))
    {
        settings = gtk_settings_get_default();
        g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                  G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  "org.freedesktop.portal.Desktop",
                                  "/org/freedesktop/portal/desktop",
                                  "org.freedesktop.portal.Settings",
                                  NULL,
                                  (GAsyncReadyCallback)init_color_scheme_cb,
                                  NULL);
        g_once_init_leave(&init, 1);
    }
    set_theme();
}

static gboolean
init_color_scheme (void)
{
    static GMutex init_mutex;

    g_mutex_lock(&init_mutex);
    {
        if (!init_done)
        {
            settings = gtk_settings_get_default();
            desktop_portal = portal_init();
            if (desktop_portal)
            {
                g_signal_connect(desktop_portal, SETTING_CHANGED_SIGNAL,
                                 G_CALLBACK(setting_changed), NULL);
            }
        }
    }
    init_done = TRUE;
    g_mutex_unlock(&init_mutex);
    return TRUE;
}

/*
 * Returns the current desktop color scheme, given by
 * the org.freedesktop.portal.Settings portal. If the portal
 * is not available, DESKTOP_NO_PREFERENCE is returned.
 */
DesktopColorScheme
color_scheme_get_desktop_scheme (void)
{
    g_autoptr (GVariant) outer = NULL;
    g_autoptr (GVariant) inner = NULL;
    g_autoptr (GVariant) result = NULL;
    g_autoptr (GError) error = NULL;
    DesktopColorScheme scheme;

    if (!init_done && !init_color_scheme())
        return DESKTOP_NO_PREFERENCE;

    if (!desktop_portal)
        return DESKTOP_NO_PREFERENCE;

    GVariant *call = g_variant_new("(ss)", "org.freedesktop.appearance",
                                   "color-scheme");
    result = g_dbus_proxy_call_sync(desktop_portal, "Read", call,
                                    G_DBUS_CALL_FLAGS_NONE, DBUS_TIMEOUT,
                                    NULL, &error);
    if (!result)
    {
        g_debug("%s", error->message);
        return DESKTOP_NO_PREFERENCE;
    }

    g_variant_get(result, "(v)", &outer);
    g_variant_get(outer, "v", &inner);
    scheme = (DesktopColorScheme) g_variant_get_uint32(inner);

    if (scheme > DESKTOP_PREFERS_LIGHT)
        scheme = DESKTOP_NO_PREFERENCE;
    return scheme;
}

/*
 * Sets the color scheme to one of the four options.
 * Can be called once GTK has been initialised.
 * Returns FALSE if the color scheme could not be set.
 * If the return value is not needed, use color_scheme_set_async instead.
 */
gboolean
color_scheme_set (AppColorScheme scheme)
{
    gboolean set_dark;
    DesktopColorScheme desktop_scheme;

    if (!init_done && !init_color_scheme())
        return FALSE;

    app_scheme = scheme;
    desktop_scheme = color_scheme_get_desktop_scheme();

    if ((app_scheme & APP_FORCE) || desktop_scheme == DESKTOP_NO_PREFERENCE)
        set_dark = (app_scheme & APP_DARK);
    else
        set_dark = (desktop_scheme == DESKTOP_PREFERS_DARK);

    set_gtk_theme(set_dark);
    return TRUE;
}

/*
 * Sets the color scheme to one of the four options.
 * Can be called once GTK has been initialised.
 * Runs asynchronously, so preferred over color_scheme_set.
 */
void
color_scheme_set_async (AppColorScheme scheme)
{
    app_scheme = scheme;
    if (init_done)
        set_theme();
    else
        init_color_scheme_async();
}
