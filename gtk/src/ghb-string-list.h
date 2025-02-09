/* Copyright (C) 2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"

G_BEGIN_DECLS

#define GHB_TYPE_STRING_LIST (ghb_string_list_get_type())

G_DECLARE_FINAL_TYPE(GhbStringList, ghb_string_list, GHB, STRING_LIST, GtkBox)

GtkWidget *ghb_string_list_new(gboolean editable);
void ghb_string_list_remove_item_at_index(GhbStringList *self, int idx);
void ghb_string_list_append(GhbStringList *self, const char *name);
void ghb_string_list_update_item_at_index(GhbStringList *self, int idx, const char *name);
char **ghb_string_list_get_items(GhbStringList *self);
void ghb_string_list_set_items(GhbStringList *self, const char **names);
void ghb_string_list_clear(GhbStringList *self);
int ghb_string_list_get_selected_index(GhbStringList *self);
char *ghb_string_list_get_selected_string(GhbStringList *self);

G_END_DECLS
