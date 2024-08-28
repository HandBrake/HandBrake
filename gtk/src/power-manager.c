/* notifications.c
 *
 * Copyright (C) 2023-2024 HandBrake Team
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "power-manager.h"

#include "application.h"
#include "callbacks.h"
#include "notifications.h"
#include "queuehandler.h"

#define UPOWER_PATH "org.freedesktop.UPower"
#define UPOWER_OBJECT "/org/freedesktop/UPower"
#define DEVICE_OBJECT "/org/freedesktop/UPower/devices/DisplayDevice"
#define UPOWER_INTERFACE "org.freedesktop.UPower"
#define DEVICE_INTERFACE "org.freedesktop.UPower.Device"

static const char *battery_widgets[] = {
    "pause_encoding_label",
    "PauseEncodingOnBatteryPower",
    "PauseEncodingOnLowBattery",
    NULL,
};

static GDBusProxy *upower_proxy;
static GDBusProxy *battery_proxy;
static GhbPowerState power_state;

/* We want to ensure that the encode is only paused when the battery
 * level first drops from normal to low, so the user can resume encoding
 * without it being paused again. So this variable tracks the previous
 * battery level, and if it was low already, we don't do anything. */
static int prev_battery_level;

#if GLIB_CHECK_VERSION(2, 70, 0)
static const char *power_save_widgets[] = {
    "pause_encoding_label",
    "PauseEncodingOnPowerSave",
    NULL,
};
static GPowerProfileMonitor *power_monitor;
#endif

static void
show_power_widgets (const char *widgets[])
{
    GtkWidget *widget;

    for (int i = 0; widgets[i] != NULL; i++)
    {
        widget = ghb_builder_widget(widgets[i]);
        if (widget)
            gtk_widget_set_visible(widget, TRUE);
    }
}

static void
battery_level_cb (GDBusProxy *proxy, GVariant *changed_properties,
                  GStrv invalidated_properties, signal_user_data_t *ud)
{
    int battery_level = -1;
    const char *prop_name;
    int queue_state;
    GVariant *var;
    GVariantIter iter;
    int low_battery_level = 0;

    if (!ghb_dict_get_bool(ud->prefs, "PauseEncodingOnLowBattery"))
        return;

    low_battery_level = ghb_dict_get_int(ud->prefs, "LowBatteryLevel");

    g_variant_iter_init(&iter, changed_properties);
    while (g_variant_iter_next(&iter, "{&sv}", &prop_name, &var))
    {
        if (g_strcmp0("Percentage", prop_name) == 0)
            battery_level = (int) g_variant_get_double(var);

        g_variant_unref(var);
    }
    if (battery_level < 0)
        return;

    queue_state = ghb_get_queue_state();

    if (battery_level <= low_battery_level
        && prev_battery_level > low_battery_level
        && (queue_state & GHB_STATE_WORKING)
        && !(queue_state & GHB_STATE_PAUSED))
    {
        power_state = GHB_POWER_PAUSED_LOW_BATTERY;
        ghb_log("Battery level %d%%: pausing encode", battery_level);
        ghb_send_notification(GHB_NOTIFY_PAUSED_LOW_BATTERY, 0, ud);
        ghb_pause_queue();
    }
    else if (battery_level > low_battery_level
             && prev_battery_level <= low_battery_level
             && (power_state == GHB_POWER_PAUSED_LOW_BATTERY))
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Battery level %d%%: resuming encode", battery_level);
            ghb_withdraw_notification(GHB_NOTIFY_PAUSED_LOW_BATTERY);
        }
        power_state = GHB_POWER_OK;
    }
    prev_battery_level = battery_level;
}

static void
battery_proxy_new_cb (GObject *source, GAsyncResult *result,
                      signal_user_data_t *ud)
{
    GDBusProxy *proxy;
    GVariant *is_present;
    GError *error = NULL;

    proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
    if (proxy != NULL)
    {
        is_present = g_dbus_proxy_get_cached_property(proxy, "IsPresent");
        if (g_variant_get_boolean(is_present))
        {
            g_signal_connect(proxy, "g-properties-changed",
                         G_CALLBACK(battery_level_cb), ud);
            show_power_widgets(battery_widgets);
            battery_proxy = proxy;
        }
        else
        {
            g_debug("No battery present. Disconnecting UPower proxy.");
            g_clear_object(&proxy);
            g_clear_object(&upower_proxy);
        }
        g_variant_unref(is_present);
    }
    else
    {
        g_debug("Could not get DisplayDevice proxy: %s", error->message);
        g_error_free(error);
        g_clear_object(&upower_proxy);
    }
}

static void
battery_proxy_new_async (signal_user_data_t *ud)
{
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,
                             UPOWER_PATH, DEVICE_OBJECT, DEVICE_INTERFACE, NULL,
                             (GAsyncReadyCallback) battery_proxy_new_cb, ud);
}

static void
upower_status_cb (GDBusProxy *proxy, GVariant *changed_properties,
                  GStrv invalidated_properties, signal_user_data_t *ud)
{
    gboolean on_battery;
    int queue_state;

    if (!ghb_dict_get_bool(ud->prefs, "PauseEncodingOnBatteryPower") ||
        !g_variant_lookup(changed_properties, "OnBattery", "b", &on_battery))
        return;

    queue_state = ghb_get_queue_state();

    if (on_battery && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        power_state = GHB_POWER_PAUSED_ON_BATTERY;
        ghb_log("Charger disconnected: pausing encode");
        ghb_send_notification(GHB_NOTIFY_PAUSED_ON_BATTERY, 0, ud);
        ghb_pause_queue();
    }
    else if (!on_battery && (power_state == GHB_POWER_PAUSED_ON_BATTERY))
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Charger connected: resuming encode");
            ghb_withdraw_notification(GHB_NOTIFY_PAUSED_ON_BATTERY);
        }
        power_state = GHB_POWER_OK;
    }
}

static void
upower_proxy_new_cb (GObject *source, GAsyncResult *result,
                     signal_user_data_t *ud)
{
    GDBusProxy *proxy;
    GError *error = NULL;

    proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
    if (proxy != NULL)
    {
        g_signal_connect(proxy, "g-properties-changed",
                         G_CALLBACK(upower_status_cb), ud);
        upower_proxy = proxy;
        battery_proxy_new_async(ud);
    }
    else
    {
        g_debug("Could not create UPower proxy: %s", error->message);
        g_error_free(error);
    }
}

static void
upower_proxy_new_async (signal_user_data_t *ud)
{
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,
                             UPOWER_PATH, UPOWER_OBJECT, UPOWER_INTERFACE, NULL,
                             (GAsyncReadyCallback) upower_proxy_new_cb, ud);
}

#if GLIB_CHECK_VERSION(2, 70, 0)
static void
power_save_cb (GPowerProfileMonitor *monitor, GParamSpec *pspec,
               signal_user_data_t *ud)
{
    gboolean power_save;

    if (!ghb_dict_get_bool(ud->prefs, "PauseEncodingOnPowerSave"))
        return;

    int queue_state = ghb_get_queue_state();

    g_object_get(monitor, "power-saver-enabled", &power_save, NULL);

    if (power_save && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        power_state = GHB_POWER_PAUSED_POWER_SAVE;
        ghb_log("Power saver enabled: pausing encode");
        ghb_pause_queue();
        ghb_send_notification(GHB_NOTIFY_PAUSED_POWER_SAVE, 0, ud);
    }
    else if (!power_save && (power_state == GHB_POWER_PAUSED_POWER_SAVE))
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Power saver disabled: resuming encode");
            ghb_withdraw_notification(GHB_NOTIFY_PAUSED_POWER_SAVE);
        }
        power_state = GHB_POWER_OK;
    }
}

static GPowerProfileMonitor *
power_monitor_new (signal_user_data_t *ud)
{
    GPowerProfileMonitor *monitor;

    monitor = g_power_profile_monitor_dup_default();

    if (monitor != NULL)
    {
        g_signal_connect(monitor, "notify::power-saver-enabled",
                         G_CALLBACK(power_save_cb), ud);
        show_power_widgets(power_save_widgets);
    }
    else
    {
        g_debug("Could not get power profile monitor");
    }
    return monitor;
}
#endif

/* Initializes the D-Bus connections to monitor power state. */
void
ghb_power_manager_init (signal_user_data_t *ud)
{
    static size_t init = 0;

    if (g_once_init_enter(&init))
    {
        upower_proxy_new_async(ud);
#if GLIB_CHECK_VERSION(2, 70, 0)
        power_monitor = power_monitor_new(ud);
#endif
        g_once_init_leave(&init, 1);
    }
}

/* Resets the status when the start/pause button is clicked, in order to
 * avoid phantom resumes. */
void
ghb_power_manager_reset (void)
{
    power_state = GHB_POWER_OK;
}

/* Disposes of the signals and other objects before shutdown. */
void
ghb_power_manager_dispose (signal_user_data_t *ud)
{
    if (upower_proxy != NULL)
    {
        g_debug("Disconnecting UPower proxy");
        g_signal_handlers_disconnect_by_func(upower_proxy,
                                             upower_status_cb, ud);
        g_clear_object(&upower_proxy);
    }
    if (battery_proxy != NULL)
    {
        g_debug("Disconnecting battery level proxy");
        g_signal_handlers_disconnect_by_func(battery_proxy,
                                             battery_level_cb, ud);
        g_clear_object(&battery_proxy);
    }
#if GLIB_CHECK_VERSION(2, 70, 0)
    if (power_monitor != NULL)
    {
        g_debug("Disconnecting power monitor\n");
        g_signal_handlers_disconnect_by_func(power_monitor,
                                             power_save_cb, ud);
        g_clear_object(&power_monitor);
    }
#endif
}
