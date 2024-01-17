/* Copyright (C) 2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"

G_BEGIN_DECLS

#define GHB_TYPE_CHAPTER_ROW (ghb_chapter_row_get_type())

G_DECLARE_FINAL_TYPE(GhbChapterRow, ghb_chapter_row, GHB, CHAPTER_ROW, GtkListBoxRow)

GtkWidget *ghb_chapter_row_new(int idx, int64_t start, int64_t duration,
                               const char *chapter_name);

const char *ghb_chapter_row_get_name(GhbChapterRow *self);
int ghb_chapter_row_get_index(GhbChapterRow *self);
int64_t ghb_chapter_row_get_start(GhbChapterRow *self);
int64_t ghb_chapter_row_get_duration(GhbChapterRow *self);

void ghb_chapter_row_set_name(GhbChapterRow *self, const char *name);
void ghb_chapter_row_set_index(GhbChapterRow *self, int idx);
void ghb_chapter_row_set_start(GhbChapterRow *self, int64_t duration);
void ghb_chapter_row_set_duration(GhbChapterRow *self, int64_t duration);

void ghb_chapter_row_grab_focus(GhbChapterRow *self);

G_END_DECLS
