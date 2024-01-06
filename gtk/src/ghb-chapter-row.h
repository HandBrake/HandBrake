/* Copyright (C) 2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define GHB_TYPE_CHAPTER_ROW (ghb_chapter_row_get_type())

G_DECLARE_FINAL_TYPE(GhbChapterRow, ghb_chapter_row, GHB, CHAPTER_ROW, GtkListBoxRow)

GtkWidget *ghb_chapter_row_new(int idx, gint64 start, gint64 duration,
                               const char *chapter_name);

const char *ghb_chapter_row_get_name(GhbChapterRow *self);
void ghb_chapter_row_set_name(GhbChapterRow *self, const char *name);
void ghb_chapter_row_grab_focus(GhbChapterRow *self);

G_END_DECLS
