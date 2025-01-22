/* Copyright (C) 2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"

G_BEGIN_DECLS

#define GHB_TYPE_FILE_BUTTON (ghb_file_button_get_type())

G_DECLARE_FINAL_TYPE(GhbFileButton, ghb_file_button, GHB, FILE_BUTTON, GtkButton)

GhbFileButton *ghb_file_button_new(const char *title, GtkFileChooserAction action);
void ghb_file_button_set_action(GhbFileButton *self, GtkFileChooserAction action);
void ghb_file_button_set_file(GhbFileButton *self, GFile *file);
void ghb_file_button_set_filename(GhbFileButton *self, const char *filename);
void ghb_file_button_set_title(GhbFileButton *self, const char *title);
void ghb_file_button_set_accept_label(GhbFileButton *self, const char *title);

GtkFileChooserAction ghb_file_button_get_action(GhbFileButton *self);
const GFile *ghb_file_button_get_file(GhbFileButton *self);
char *ghb_file_button_get_filename(GhbFileButton *self);
const char *ghb_file_button_get_title(GhbFileButton *self);
const char *ghb_file_button_get_accept_label(GhbFileButton *self);

G_END_DECLS
