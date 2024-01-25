/* Copyright (C) 2023-2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "ghbcompat.h"
#include "settings.h"

typedef enum {
    GHB_POWER_OK,
    GHB_POWER_PAUSED_POWER_SAVE,
    GHB_POWER_PAUSED_ON_BATTERY,
    GHB_POWER_PAUSED_LOW_BATTERY,
} GhbPowerState;

void ghb_power_manager_init (signal_user_data_t *ud);
void ghb_power_manager_reset (void);
void ghb_power_manager_dispose (signal_user_data_t *ud);

#endif // POWER_MANAGEMENT_H
