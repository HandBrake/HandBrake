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
	GHB_ERROR_NONE,
	GHB_ERROR_CANCELED,
	GHB_ERROR_FAIL,
};

typedef struct ghb_status_s
{
	gint state;
	gint queue_state;

	// SCANNING
	gint title_count;
	gint title_cur;

	// WORKING
	gint unique_id;
	gint job_cur;
	gint job_count;
	gdouble progress;
	gdouble rate_cur;
	gdouble rate_avg;
	gint hours;
	gint minutes;
	gint seconds;
	gint error;
} ghb_status_t;

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
	gint64 duration;
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

const gchar* ghb_version(void);
void ghb_vquality_range(signal_user_data_t *ud, gint *min, gint *max);
//const gchar* ghb_get_rate_string(gint rate, gint type);
void ghb_backend_init(GtkBuilder *builder, gint debug, gint update);
void ghb_backend_close(void);
void ghb_add_job(GValue *js, gint unique_id);
void ghb_remove_job(gint unique_id);
void ghb_start_queue(void);
void ghb_stop_queue(void);
void ghb_pause_queue(void);

gint ghb_get_state(void);
void ghb_clear_state(gint state);
void ghb_clear_queue_state(gint state);

void ghb_set_state(gint state);
gint ghb_get_queue_state();
void ghb_get_status(ghb_status_t *status);
void ghb_track_status(void);
void ghb_backend_scan(const gchar *path, gint titleindex);
void ghb_backend_queue_scan(const gchar *path, gint titleindex);
gboolean ghb_get_title_info(ghb_title_info_t *tinfo, gint titleindex);
void ghb_set_scale(signal_user_data_t *ud, gint mode);
GValue* ghb_get_chapters(gint titleindex);
gint ghb_get_best_mix(gint titleindex, gint track, gint acodec, gint mix);
gboolean ghb_ac3_in_audio_list(const GValue *audio_list);
gboolean ghb_audio_is_passthru(gint acodec);
gint ghb_get_default_acodec(void);
gboolean ghb_get_audio_info(
	ghb_audio_info_t *ainfo, gint titleindex, gint audioindex);
gboolean ghb_set_passthru_rate_opts(GtkBuilder *builder, gint bitrate);
gboolean ghb_set_default_rate_opts(GtkBuilder *builder);
void ghb_grey_combo_options(GtkBuilder *builder);
void ghb_update_ui_combo_box(
	GtkBuilder *builder, const gchar *name, gint user_data, gboolean all);
gint ghb_find_audio_track(gint titleindex, const gchar *lang, gint index);
gint ghb_longest_title(void);
gchar* ghb_build_x264opts_string(GValue *settings);
GdkPixbuf* ghb_get_preview_image(
	gint titleindex, gint index, GValue *settings, gboolean borders);
gint ghb_calculate_target_bitrate(GValue *settings, gint titleindex);
gchar* ghb_dvd_volname(const gchar *device);
gint ghb_get_title_number(gint titleindex);

gint ghb_guess_bitrate(GValue *settings);
gboolean ghb_validate_container(signal_user_data_t *ud);
gboolean ghb_validate_vquality(GValue *settings);
gboolean ghb_validate_audio(signal_user_data_t *ud);
gboolean ghb_validate_video(signal_user_data_t *ud);
gboolean ghb_validate_filters(signal_user_data_t *ud);
gboolean ghb_validate_filter_string(const gchar *str, gint max_fields);
void ghb_hb_cleanup(gboolean partial);
gint ghb_lookup_acodec(const GValue *acodec);
const gchar* ghb_lookup_acodec_option(const GValue *acodec);
gint ghb_lookup_mix(const GValue *mix);
const gchar* ghb_lookup_mix_option(const GValue *mix);
const gchar* ghb_lookup_container_option(const GValue *container);
const gchar* ghb_lookup_vcodec_option(const GValue *vcodec);
#if 0
gint ghb_lookup_bitrate(const gchar *bitrate);
gint ghb_lookup_rate(const gchar *rate);
gdouble ghb_lookup_drc(const gchar *drc);
#endif

#endif // _HBBACKEND_H_
