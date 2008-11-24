/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#if !defined(_GHB_PREVIEW_H_)
#define _GHB_PREVIEW_H_

void ghb_preview_init(signal_user_data_t *ud);
void ghb_set_preview_image(signal_user_data_t *ud);
void ghb_live_preview_progress(signal_user_data_t *ud);
void ghb_live_encode_done(signal_user_data_t *ud, gboolean success);
void ghb_preview_cleanup(signal_user_data_t *ud);
void ghb_live_reset(signal_user_data_t *ud);

#endif // _GHB_PREVIEW_H_
