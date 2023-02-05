/* Copyright (C) 2023 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "power-manager.h"
#include "queuehandler.h"
#include "callbacks.h"

static GDBusProxy *upower_proxy;
static gboolean on_battery_pause;

#if GLIB_CHECK_VERSION(2, 70, 0)
static GPowerProfileMonitor *power_monitor;
static gboolean power_save_pause;
#endif

static void
upower_status_cb (GDBusProxy *proxy, GVariant *changed_properties,
                   GStrv invalidated_properties, signal_user_data_t *ud)
{
    gboolean on_battery = FALSE;
    const char *prop_name;
    GVariant *var;
    GVariantIter *iter;

    g_variant_get(changed_properties, "a{sv}", &iter);
    while (g_variant_iter_next(iter, "{&sv}", &prop_name, &var))
    {
        if (g_strcmp0("OnBattery", prop_name) == 0)
        {
            on_battery = g_variant_get_boolean(var);
            break;
        }
    }
    g_variant_unref(var);
    g_variant_iter_free(iter);


    if (!ghb_dict_get_bool(ud->prefs, "PauseOnBattery"))
        return;

    int queue_state = ghb_get_queue_state();
    g_debug("Battery status changed: %s",
            on_battery ? "On battery" : "Charging");

    if (on_battery && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        on_battery_pause = TRUE;
        ghb_log("Charger disconnected: pausing encode");
        ghb_pause_queue();
    }
    else if (!on_battery && on_battery_pause)
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Charger connected: resuming encode");
        }
        on_battery_pause = FALSE;
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
    }
    else
    {
        g_debug("Could not create DBus proxy: %s", error->message);
        g_error_free(error);
    }

    upower_proxy = proxy;
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
static void power_save_cb (GPowerProfileMonitor *monitor,
                           GParamSpec           *pspec,
                           signal_user_data_t   *ud)
{
    gboolean power_save;

    int queue_state = ghb_get_queue_state();

    g_object_get(monitor, "power-saver-enabled", &power_save, NULL);
    g_debug("%s power save mode", power_save ? "Entered" : "Exited");

    if (power_save && (queue_state & GHB_STATE_WORKING)
                   && !(queue_state & GHB_STATE_PAUSED))
    {
        power_save_pause = TRUE;
        ghb_log("Power save enabled: pausing encode");
        ghb_pause_queue();
    }
    else if (!power_save && power_save_pause)
    {
        if (queue_state & GHB_STATE_PAUSED)
        {
            ghb_resume_queue();
            ghb_log("Power save disabled: resuming encode");
        }
        power_save_pause = FALSE;
    }
}

static GPowerProfileMonitor *
power_monitor_new (signal_user_data_t *ud)
{
    GPowerProfileMonitor *monitor;

    monitor = g_power_profile_monitor_dup_default();

    if (monitor != NULL)
        g_signal_connect(monitor, "notify::power-saver-enabled",
                         G_CALLBACK(power_save_cb), ud);
    else
        g_debug("Could not get power profile monitor");

    return monitor;
}
#endif

void
ghb_power_manager_init (signal_user_data_t *ud)
{
    upower_proxy_new_async(ud);
#if GLIB_CHECK_VERSION(2, 70, 0)
    power_monitor = power_monitor_new(ud);
#endif
}

void
ghb_power_manager_dispose (signal_user_data_t *ud)
{
    if (upower_proxy != NULL)
    {
        g_signal_handlers_disconnect_by_func(upower_proxy,
                                             upower_status_cb, ud);
        g_object_unref(upower_proxy);
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