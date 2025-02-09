/* Copyright (C) 2023-2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"
#include "settings.h"

G_BEGIN_DECLS

typedef enum {
    GHB_POWER_OK,
    GHB_POWER_PAUSED_POWER_SAVE,
    GHB_POWER_PAUSED_ON_BATTERY,
    GHB_POWER_PAUSED_LOW_BATTERY,
} GhbPowerState;

void ghb_power_manager_init (signal_user_data_t *ud);
void ghb_power_manager_reset (void);
void ghb_power_manager_dispose (signal_user_data_t *ud);

G_END_DECLS

