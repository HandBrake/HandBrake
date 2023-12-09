/*
 * preview.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * preview.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * preview.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
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
