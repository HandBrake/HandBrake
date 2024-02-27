/* preview.h
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

#if !defined(_GHB_PREVIEW_H_)
#define _GHB_PREVIEW_H_

#define GHB_PREVIEW_MAX 60

void ghb_preview_init(signal_user_data_t *ud);
void ghb_rescale_preview_image(signal_user_data_t *ud);
void ghb_reset_preview_image(signal_user_data_t *ud);
void ghb_set_preview_image(signal_user_data_t *ud);
void ghb_live_preview_progress(signal_user_data_t *ud);
void ghb_live_encode_done(signal_user_data_t *ud, gboolean success);
void ghb_preview_cleanup(signal_user_data_t *ud);
void ghb_live_reset(signal_user_data_t *ud);
void ghb_par_scale(signal_user_data_t *ud, gint *width, gint *height, gint par_n, gint par_d);
void ghb_preview_dispose (signal_user_data_t *ud);

#endif // _GHB_PREVIEW_H_
