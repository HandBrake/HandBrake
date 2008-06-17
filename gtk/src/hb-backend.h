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
 
#if !defined(_HBBACKEND_H_)
#define _HBBACKEND_H_

#include "settings.h"

enum
{
	GHB_EVENT_NONE,
	GHB_EVENT_SCAN_DONE,
	GHB_EVENT_WORKING,
	GHB_EVENT_PAUSED,
	GHB_EVENT_WORK_DONE,
	GHB_EVENT_WORK_CANCELED
};

#define GHB_SCALE_KEEP_NONE 0
#define GHB_SCALE_KEEP_WIDTH 1
#define GHB_SCALE_KEEP_HEIGHT 2

typedef struct
{
	gint width;
	gint height;
	gint crop[4];
	gint num_chapters;
	gint rate;
	gint rate_base;
	gint aspect_n;
	gint aspect_d;
	gint hours;
	gint minutes;
	gint seconds;
} ghb_title_info_t;

typedef struct
{
	gint codec;
	gint bitrate;
	gint samplerate;
} ghb_audio_info_t;

#define GHB_AUDIO_SAMPLERATE 1
#define GHB_AUDIO_BITRATE 2
#define GHB_FRAMERATE 3

const gchar* ghb_version();
//const gchar* ghb_get_rate_string(gint rate, gint type);
void ghb_backend_init(GtkBuilder *builder, gint debug, gint update);
void ghb_add_job(job_settings_t *js, gint unique_id);
void ghb_remove_job(gint unique_id);
void ghb_start_queue();
void ghb_stop_queue();
void ghb_pause_queue();

gint ghb_backend_events(signal_user_data_t *ud, gint *unique_id);
void ghb_backend_scan(const gchar *path, gint titleindex);
gboolean ghb_get_title_info(ghb_title_info_t *tinfo, gint titleindex);
void ghb_set_scale(signal_user_data_t *ud, gint mode);
gchar ** ghb_get_chapters(gint titleindex);
gint ghb_get_best_mix(gint titleindex, gint track, gint acodec, gint mix);
gboolean ghb_audio_is_passthru(gint acodec);
gint ghb_get_default_acodec();
gboolean ghb_get_audio_info(ghb_audio_info_t *ainfo, gint titleindex, gint audioindex);
gboolean ghb_set_passthru_rate_opts(GtkBuilder *builder, gint bitrate);
gboolean ghb_set_default_rate_opts(GtkBuilder *builder);
void ghb_grey_combo_options(GtkBuilder *builder);
void ghb_update_ui_combo_box(GtkBuilder *builder, const gchar *name, gint user_data, gboolean all);
gint ghb_find_audio_track(gint titleindex, const gchar *lang, gint acodec);
gint ghb_longest_title();
gchar* ghb_build_x264opts_string(GHashTable *settings);
GdkPixbuf* ghb_get_preview_image(gint titleindex, gint index, GHashTable *settings, gboolean borders);
gint ghb_calculate_target_bitrate(GHashTable *settings, gint titleindex);
gchar* ghb_dvd_volname(const gchar *device);

gint ghb_guess_bitrate(GHashTable *settings);
gboolean ghb_validate_vquality(GHashTable *settings);
gboolean ghb_validate_audio(signal_user_data_t *ud);
gboolean ghb_validate_video(signal_user_data_t *ud);
#endif // _HBBACKEND_H_
