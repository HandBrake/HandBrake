/* Copyright (C) 2023 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "power-manager.h"
#include "queuehandler.h"
#include "callbacks.h"

static const char *battery_widgets[] = {
    "pause_encoding_label",
    "PauseEncodingOnBatteryPower",
    "PauseEncodingOnLowBattery",
    NULL,
};

static GDBusProxy *upower_proxy;
static GDBusProxy *battery_proxy;
static char **upower_devices;
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
show_power_widgets (const char *widgets[], signal_user_data_t *ud)
{
    GtkWidget *widget;
    int i = 0;

    if (ud->builder == NULL)
        return;

    while (widgets[i] != NULL)
    {
        widget = GHB_WIDGET(ud->builder, widgets[i]);
        gtk_widget_set_visible(widget, TRUE);
        i++;
    }
}

static void
upower_status_cb (GDBusProxy *proxy, GVariant *changed_properties,
                  GStrv invalidated_properties, signal_user_data_t *ud)
{
    int on_battery = -1;
    const char *prop_name;
    int queue_state;
    GVariant *var;
    GVariantIter iter;

    if (!ghb_dict_get_bool(ud->prefs, "PauseEncodingOnBatteryPower"))
        return;

    g_variant_iter_init(&iter, changed_properties);
    while (g_variant_iter_next(&iter, "{&sv}", &prop_name, &var))
    {
        if (g_strcmp0("OnBattery", prop_name) == 0)
            on_battery = !!g_variant_get_boolean(var);

        g_variant_unref(var);
    }

    if (on_battery == -1)
        return;

    queue_state = ghb_get_queue_state();

    if (on_battery && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        power_state = GHB_POWER_PAUSED_ON_BATTERY;
        ghb_log("Charger disconnected: pausing encode");
        ghb_pause_queue();
    }
    else if (!on_battery && (power_state == GHB_POWER_PAUSED_ON_BATTERY))
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Charger connected: resuming encode");
        }
        power_state = GHB_POWER_OK;
    }
}

static void
battery_status_cb (GDBusProxy *proxy, GVariant *changed_properties,
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
        }
        power_state = GHB_POWER_OK;
    }
    prev_battery_level = battery_level;
}

static GDBusProxy *
device_get_battery_proxy (const char *path)
{
    GDBusProxy *proxy;
    GVariant *var;
    uint32_t type;
    gboolean power_supply, is_present;
    GError *error = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          NULL, "org.freedesktop.UPower",
                                          path, "org.freedesktop.UPower.Device",
                                          NULL, &error);
    if (proxy != NULL)
    {
        var = g_dbus_proxy_get_cached_property(proxy, "Type");
        type = g_variant_get_uint32(var);
        g_variant_unref(var);
        var = g_dbus_proxy_get_cached_property(proxy, "PowerSupply");
        power_supply = g_variant_get_boolean(var);
        g_variant_unref(var);
        var = g_dbus_proxy_get_cached_property(proxy, "IsPresent");
        is_present = g_variant_get_boolean(var);
        g_variant_unref(var);

        if (type == 2 && power_supply && is_present)
            return proxy;
        else
            g_object_unref(proxy);
    }
    else
    {
        g_debug("Could not get proxy for device: %s", error->message);
        g_error_free(error);
    }
    return NULL;
}

static gpointer
devices_thread (signal_user_data_t *ud)
{
    GDBusProxy *proxy;

    for (guint i = 0; i < g_strv_length(upower_devices); i++)
    {
        proxy = device_get_battery_proxy(upower_devices[i]);
        if (proxy != NULL)
        {
            g_debug("Battery: %s", g_dbus_proxy_get_object_path(proxy));
            battery_proxy = proxy;
            g_signal_connect(battery_proxy, "g-properties-changed",
                             G_CALLBACK(battery_status_cb), ud);
            show_power_widgets(battery_widgets, ud);
            break;
        }
    }
    g_strfreev(upower_devices);
    g_thread_exit(0);
    return NULL;
}

static void
upower_devices_cb (GDBusProxy *proxy, GAsyncResult *result,
                   signal_user_data_t *ud)
{
    GVariant *var, *array;
    GError *error = NULL;

    var = g_dbus_proxy_call_finish(proxy, result, &error);
    if (error)
    {
        g_debug("Could not get device list: %s", error->message);
        g_error_free(error);
    }

    array = g_variant_get_child_value(var, 0);
    upower_devices = g_variant_dup_objv(array, NULL);
    g_thread_new("devices_thread", (GThreadFunc)devices_thread, ud);
    g_variant_unref(array);
    g_variant_unref(var);
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
        g_dbus_proxy_call(proxy, "EnumerateDevices", NULL,
                          G_DBUS_CALL_FLAGS_NONE, 10000, NULL,
                          (GAsyncReadyCallback) upower_devices_cb, ud);
        upower_proxy = proxy;
    }
    else
    {
        g_debug("Could not create DBus proxy: %s", error->message);
        g_error_free(error);
    }
}

static void
upower_proxy_new_async (signal_user_data_t *ud)
{
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
                             G_DBUS_PROXY_FLAGS_NONE, NULL,
                             "org.freedesktop.UPower",
                             "/org/freedesktop/UPower",
                             "org.freedesktop.UPower", NULL,
                             (GAsyncReadyCallback) upower_proxy_new_cb, ud);
}

#if GLIB_CHECK_VERSION(2, 70, 0)
static void
power_save_cb (GPowerProfileMonitor *monitor, GParamSpec *pspec,
               signal_user_data_t *ud)
{
    gboolean power_save;

    int queue_state = ghb_get_queue_state();

    g_object_get(monitor, "power-saver-enabled", &power_save, NULL);

    if (power_save && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        power_state = GHB_POWER_PAUSED_POWER_SAVE;
        ghb_log("Power saver enabled: pausing encode");
        ghb_pause_queue();
    }
    else if (!power_save && (power_state == GHB_POWER_PAUSED_POWER_SAVE))
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Power saver disabled: resuming encode");
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
        show_power_widgets(power_save_widgets, ud);
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
        g_signal_handlers_disconnect_by_func(upower_proxy,
                                             upower_status_cb, ud);
        g_object_unref(upower_proxy);
    }
    if (battery_proxy != NULL)
    {
        g_signal_handlers_disconnect_by_func(battery_proxy,
                                             battery_status_cb, ud);
        g_object_unref(battery_proxy);
    }
#if GLIB_CHECK_VERSION(2, 70, 0)
    if (power_monitor != NULL)
    {
        g_signal_handlers_disconnect_by_func(power_monitor,
                                             power_save_cb, ud);
        g_object_unref(power_monitor);
    }
#endif
}
