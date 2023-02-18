/* Copyright (C) 2023 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "power-manager.h"
#include "queuehandler.h"
#include "callbacks.h"

static const char *battery_widgets[] = {
    "pause_encoding_label",
    "PauseEncodingOnBatteryPower",
    NULL,
};
static const char *power_save_widgets[] = {
    "pause_encoding_label",
    "PauseEncodingOnPowerSave",
    NULL,
};

static GDBusProxy *upower_proxy;
static GhbPowerState power_state;

#if GLIB_CHECK_VERSION(2, 70, 0)
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
        power_state |= GHB_POWER_PAUSED_ON_BATTERY;
        ghb_log("Charger disconnected: pausing encode");
        ghb_pause_queue();
    }
    else if (!on_battery && (power_state & GHB_POWER_PAUSED_ON_BATTERY))
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
        show_power_widgets(battery_widgets, ud);
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
    else if (!power_save && (power_state & GHB_POWER_PAUSED_POWER_SAVE))
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