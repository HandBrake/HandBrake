/* Copyright (C) 2023-2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"
#include "settings.h"

G_BEGIN_DECLS

typedef enum {
    GHB_NOTIFY_ITEM_DONE,
    GHB_NOTIFY_ITEM_FAILED,
    GHB_NOTIFY_QUEUE_DONE,
    GHB_NOTIFY_PAUSED_LOW_DISK_SPACE,
    GHB_NOTIFY_PAUSED_LOW_BATTERY,
    GHB_NOTIFY_PAUSED_ON_BATTERY,
    GHB_NOTIFY_PAUSED_POWER_SAVE,
} GhbNotification;

void ghb_send_notification(GhbNotification type, gint value,
                            signal_user_data_t *ud);
void ghb_withdraw_notification(GhbNotification type);

G_END_DECLS

