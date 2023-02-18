/* Copyright (C) 2023 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "ghbcompat.h"
#include "settings.h"

typedef enum {
    GHB_POWER_OK = 0,
    GHB_POWER_PAUSED_POWER_SAVE = 1,
    GHB_POWER_PAUSED_ON_BATTERY = 2,
    GHB_POWER_PAUSED_LOW_BATTERY = 4,
} GhbPowerState;

void ghb_power_manager_init (signal_user_data_t *ud);
void ghb_power_manager_dispose (signal_user_data_t *ud);

#endif // POWER_MANAGEMENT_H