/* Copyright (C) 2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "settings.h"

enum {
    GHB_QUEUE_STATUS_READY,
    GHB_QUEUE_STATUS_IN_PROGRESS,
    GHB_QUEUE_STATUS_DONE,
};

G_BEGIN_DECLS

#define GHB_TYPE_QUEUE_ROW (ghb_queue_row_get_type())

G_DECLARE_FINAL_TYPE(GhbQueueRow, ghb_queue_row, GHB, QUEUE_ROW, GtkListBoxRow)

GtkWidget *ghb_queue_row_new(const char *dest, int status, signal_user_data_t *ud);

const char *ghb_queue_row_get_destination(GhbQueueRow *self);
void ghb_queue_row_set_destination(GhbQueueRow *self, const char *dest);
double ghb_queue_row_get_progress(GhbQueueRow *self);
void ghb_queue_row_set_progress(GhbQueueRow *self, double fraction);
void ghb_queue_row_set_progress_bar_visible(GhbQueueRow *self, gboolean visible);
int ghb_queue_row_get_status(GhbQueueRow *self);
void ghb_queue_row_set_status(GhbQueueRow *self, int status);

G_END_DECLS
