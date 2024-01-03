/* notifications.c
 *
 * Copyright (C) HandBrake Team 2024
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "notifications.h"
#include <glib/gi18n.h>

static int n_succeeded = 0;
static int n_failed = 0;

static char *
item_complete_name (const char *filename)
{
    if (filename == NULL)
        return NULL;

    g_autofree char *name = g_filename_display_basename(filename);

    if (g_utf8_strlen(name, -1) > 64)
    {
        g_autofree char *shortname = g_utf8_substring(name, 0, 64);
        return g_strdup_printf("%s…", shortname);
    }
    else
    {
        return g_steal_pointer(&name);
    }
}

static char *
queue_complete_message (int n_succeeded, int n_failed)
{
    char *msg;

    if (n_failed > 0)
    {
        msg = g_strdup_printf((const char *)ngettext("%d item complete, %d failed",
                                                     "%d items complete, %d failed",
                                                     n_succeeded), n_succeeded, n_failed);
    }
    else
    {
       msg = g_strdup_printf((const char *)ngettext("%d item has finished encoding",
                                                    "%d items have finished encoding",
                                                    n_succeeded), n_succeeded);
    }
    return msg;
}

static void
send_notification (const char *title, const char *body, const char *category)
{
    g_autoptr(GNotification) notification = g_notification_new(title);
    g_autoptr(GIcon) icon = g_themed_icon_new("fr.handbrake.ghb");
    if (body != NULL)
    {
        g_notification_set_body(notification, body);
    }
    g_notification_set_icon(notification, icon);
    g_application_send_notification(g_application_get_default(), category, notification);
}

static void
notify_done (gboolean final, gboolean success, gint idx, signal_user_data_t *ud)
{
    if (!final)
    {
        if (success) n_succeeded += 1;
        else n_failed += 1;
    }

    GhbValue *queueDict, *jobDict, *destDict;
    g_autofree char *title = NULL;
    g_autofree char *body = NULL;
    gboolean notify_item, notify_queue;

    notify_item = ghb_dict_get_bool(ud->prefs, "NotifyOnEncodeDone");
    notify_queue = ghb_dict_get_bool(ud->prefs, "NotifyOnQueueDone");

    if (notify_queue && final)
    {
        const char *msg = ghb_dict_get_string(ud->prefs, "CustomNotificationMessage");
        if (msg && msg[0])
        {
            title = g_strdup(msg);
        }
        else
        {
            title = g_strdup(_("Put down that cocktail…"));
        }
        body = queue_complete_message(n_succeeded, n_failed);
    }
    else if (notify_item && !final)
    {
        queueDict = ghb_array_get(ud->queue, idx);
        jobDict = ghb_dict_get(queueDict, "Job");
        destDict = ghb_dict_get(jobDict, "Destination");
        body = item_complete_name(ghb_dict_get_string(destDict, "File"));
        title = g_strdup(success ? _("Encode Complete") : _("Encode Failed"));
    }
    else
    {
        return;
    }
    send_notification(title, body, "hb-done");
}

static void
notify_paused (GhbNotification type, int value, signal_user_data_t *ud)
{
    g_autofree char *body = NULL;
    int gigabyte, decimal;

    if (!ud->when_complete && !ghb_dict_get_bool(ud->prefs, "NotifyOnEncodeDone"))
        return;

    switch (type)
    {
        case GHB_NOTIFY_PAUSED_LOW_BATTERY:
            body = g_strdup(_("Battery level is low."));
            break;
        case GHB_NOTIFY_PAUSED_ON_BATTERY:
            body = g_strdup(_("Charger has been disconnected."));
            break;
        case GHB_NOTIFY_PAUSED_POWER_SAVE:
            body = g_strdup(_("Power Saver mode has been activated."));
            break;
        case GHB_NOTIFY_PAUSED_LOW_DISK_SPACE:
            gigabyte = value / 1000;
            decimal = (value % 1000) / 100;
            if (gigabyte != 0)
            {
                body = g_strdup_printf(_("%d.%d GB free space remaining."),
                                       gigabyte, decimal);
            }
            else
            {
                body = g_strdup_printf(("%d MB free space remaining."), value);
            }
            break;
        default:
            body = NULL;
    }

    send_notification(_("Encoding Paused"), body, "hb-paused");
}

/*
 * Sends a notification of the specified type. If a notification of the same or
 * a related type is already displayed, it will be replaced.
 * The value parameter will be used to determine the content of the message,
 * depending on the type. In some cases it will be displayed directly, in others
 * it is used as the index of the item to look up.
 */
void
ghb_send_notification (GhbNotification type, int value, signal_user_data_t *ud)
{
    switch (type)
    {
    case GHB_NOTIFY_ITEM_DONE:
        notify_done (FALSE, TRUE, value, ud);
        break;
    case GHB_NOTIFY_ITEM_FAILED:
        notify_done (FALSE, FALSE, value, ud);
        break;
    case GHB_NOTIFY_QUEUE_DONE:
        notify_done (TRUE, TRUE, value, ud);
        // Reset the counters
        n_succeeded = 0;
        n_failed = 0;
        break;
    case GHB_NOTIFY_PAUSED_LOW_DISK_SPACE:
    case GHB_NOTIFY_PAUSED_POWER_SAVE:
    case GHB_NOTIFY_PAUSED_LOW_BATTERY:
    case GHB_NOTIFY_PAUSED_ON_BATTERY:
        notify_paused (type, value, ud);
        break;
    default:
        g_debug("Notification not implemented");
        break;
    }
}

/*
 * Withdraws all notifications of the specified type. If there is a notification
 * of a different type which would otherwise replace this type, it will also be
 * withdrawn.
 */
void
ghb_withdraw_notification (GhbNotification type)
{
    switch (type)
    {
    case GHB_NOTIFY_ITEM_FAILED:
    case GHB_NOTIFY_ITEM_DONE:
    case GHB_NOTIFY_QUEUE_DONE:
        g_application_withdraw_notification(g_application_get_default(), "hb-done");
        break;
    case GHB_NOTIFY_PAUSED_LOW_DISK_SPACE:
    case GHB_NOTIFY_PAUSED_POWER_SAVE:
    case GHB_NOTIFY_PAUSED_LOW_BATTERY:
    case GHB_NOTIFY_PAUSED_ON_BATTERY:
        g_application_withdraw_notification(g_application_get_default(), "hb-paused");
        break;
    default:
        g_debug("Notification not implemented");
        break;
    }
}
