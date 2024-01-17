/* Copyright (C) 2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"

G_BEGIN_DECLS

#define GHB_TYPE_BUTTON (ghb_button_get_type())

G_DECLARE_FINAL_TYPE (GhbButton, ghb_button, GHB, BUTTON, GtkButton)

GhbButton *ghb_button_new (void);
void ghb_button_set_icon_name (GhbButton *self, const char *icon_name);
void ghb_button_set_label (GhbButton *self, const char *label);
void ghb_button_set_indicator_label (GhbButton *self, const char *label);
void ghb_button_set_orientation (GhbButton *self, GtkOrientation orientation);

const char *ghb_button_get_icon_name (GhbButton *self);
const char *ghb_button_get_label (GhbButton *self);
const char *ghb_button_get_indicator_label (GhbButton *self);
GtkOrientation ghb_button_get_orientation (GhbButton *self);

G_END_DECLS
