/* queuehandler.h
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#if !defined(_QUEUEHANDLER_H_)
#define _QUEUEHANDLER_H_

#include <gtk/gtk.h>
#include "settings.h"
#include "ghb-queue-row.h"
#include "hb-backend.h"

void     ghb_queue_buttons_grey(signal_user_data_t *ud);
gboolean ghb_reload_queue(signal_user_data_t *ud);
void     ghb_queue_remove_row(signal_user_data_t *ud, int row);
gint     ghb_find_queue_job(GhbValue *queue, gint unique_id, GhbValue **job);
void     ghb_low_disk_check(signal_user_data_t *ud);
void     ghb_queue_progress_set_visible(signal_user_data_t *ud, int index,
                                        gboolean visible);
void     ghb_queue_progress_set_fraction(signal_user_data_t *ud, int index,
                                         gdouble frac);
void     ghb_queue_update_status(signal_user_data_t *ud, int index, int status);
void     ghb_queue_update_status_icon(signal_user_data_t *ud, int index);
void     ghb_queue_select_log(signal_user_data_t * ud);
void     ghb_queue_update_live_stats(signal_user_data_t * ud, int index,
                                     ghb_instance_status_t * status);
void     ghb_queue_drag_n_drop_init(signal_user_data_t * ud);
void     ghb_add_to_queue_list(signal_user_data_t *ud, GhbValue *queueDict);
void     ghb_queue_selection_init(signal_user_data_t * ud);
void     ghb_queue_row_remove(GhbQueueRow *row);

#endif // _QUEUEHANDLER_H_
