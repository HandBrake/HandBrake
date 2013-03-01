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
#include "hb.h"

enum
{
    GHB_ERROR_NONE,
    GHB_ERROR_CANCELED,
    GHB_ERROR_FAIL,
};

typedef struct
{
    gint state;

    // SCANNING
    gint title_count;
    gint title_cur;
    gint preview_count;
    gint preview_cur;

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
} ghb_instance_status_t;

typedef struct
{
    ghb_instance_status_t scan;
    ghb_instance_status_t queue;
} ghb_status_t;

#define GHB_PIC_KEEP_WIDTH          0x01
#define GHB_PIC_KEEP_HEIGHT         0x02
#define GHB_PIC_KEEP_DISPLAY_WIDTH  0x04
#define GHB_PIC_KEEP_DISPLAY_HEIGHT 0x08
#define GHB_PIC_KEEP_DAR            0x10
#define GHB_PIC_KEEP_PAR            0x20
#define GHB_PIC_USE_MAX             0x40

#define GHB_AUDIO_SAMPLERATE 1
#define GHB_AUDIO_BITRATE 2
#define GHB_FRAMERATE 3

const gchar* ghb_version(void);
void ghb_vquality_range(
    signal_user_data_t *ud, 
    gdouble *min, 
    gdouble *max,
    gdouble *step,
    gdouble *page,
    gint *digits,
    gboolean *inverted);
//const gchar* ghb_get_rate_string(gint rate, gint type);
void ghb_combo_init(signal_user_data_t *ud);
void ghb_backend_init(gint debug);
void ghb_backend_close(void);
void ghb_add_job(GValue *js, gint unique_id);
void ghb_remove_job(gint unique_id);
void ghb_start_queue(void);
void ghb_stop_queue(void);
void ghb_pause_queue(void);

void ghb_add_live_job(GValue *js, gint unique_id);
void ghb_start_live_encode();
void ghb_stop_live_encode();

void ghb_clear_scan_state(gint state);
void ghb_clear_queue_state(gint state);

void ghb_set_state(gint state);
gint ghb_get_scan_state();
gint ghb_get_queue_state();
void ghb_get_status(ghb_status_t *status);
void ghb_track_status(void);
void ghb_backend_scan(const gchar *path, gint titleindex, gint preview_count, guint64 min_duration);
void ghb_backend_scan_stop();
void ghb_backend_queue_scan(const gchar *path, gint titleindex);
hb_title_t* ghb_get_title_info(gint titleindex);
void ghb_par_init(signal_user_data_t *ud);
void ghb_set_scale(signal_user_data_t *ud, gint mode);
void ghb_set_scale_settings(GValue *settings, gint mode);
GValue* ghb_get_chapters(gint titleindex);
void ghb_get_chapter_duration(gint ti, gint ii, gint *hh, gint *mm, gint *ss);
void ghb_part_duration(gint tt, gint sc, gint ec, gint *hh, gint *mm, gint *ss);
gint ghb_get_best_mix(hb_audio_config_t *aconfig, gint acodec, gint mix);
gboolean ghb_ac3_in_audio_list(const GValue *audio_list);
gboolean ghb_audio_is_passthru(gint acodec);
gboolean ghb_audio_can_passthru(gint acodec);
gint ghb_get_default_acodec(void);
hb_audio_config_t* ghb_get_scan_audio_info(gint titleindex, gint audioindex);
void ghb_set_bitrate_opts(
    GtkBuilder *builder, gint first_rate, gint last_rate, gint extra_rate);
void ghb_grey_combo_options(signal_user_data_t *ud);
void ghb_update_ui_combo_box(
    signal_user_data_t *ud, const gchar *name, gint user_data, gboolean all);
const gchar* ghb_get_source_audio_lang(gint titleindex, gint track);
gint ghb_find_audio_track(
    gint titleindex, const gchar *lang, gint acodec, 
    gint fallback_acodec, GHashTable *track_indices);
const gchar* ghb_audio_track_description(gint track, int titleindex);
void ghb_add_all_subtitles(signal_user_data_t *ud, gint titleindex);
gint ghb_find_pref_subtitle_track(const gchar *lang);
gint ghb_find_subtitle_track(
    gint titleindex, const gchar *lang, gboolean burn, 
    gboolean force, gint source, GHashTable *track_indices);
gint ghb_pick_subtitle_track(signal_user_data_t *ud);
gint ghb_find_cc_track(gint titleindex);
gint ghb_longest_title(void);
gchar* ghb_build_advanced_opts_string(GValue *settings);
GdkPixbuf* ghb_get_preview_image(
    gint titleindex, gint index, signal_user_data_t *ud,
    gint *width, gint *height);
gchar* ghb_dvd_volname(const gchar *device);
gint ghb_get_title_number(gint titleindex);
int ghb_get_title_count();
gint ghb_subtitle_track_source(GValue *settings, gint track);
const char* ghb_subtitle_track_source_name(GValue *settings, gint track);
const gchar* ghb_subtitle_track_lang(GValue *settings, gint track);

gboolean ghb_validate_vquality(GValue *settings);
gboolean ghb_validate_audio(GValue *settings);
gboolean ghb_validate_subtitles(GValue *settings);
gboolean ghb_validate_video(GValue *settings);
gboolean ghb_validate_filters(GValue *settings);
gboolean ghb_validate_filter_string(const gchar *str, gint max_fields);
void ghb_hb_cleanup(gboolean partial);
gint ghb_lookup_combo_int(const gchar *name, const GValue *gval);
gdouble ghb_lookup_combo_double(const gchar *name, const GValue *gval);
const gchar* ghb_lookup_combo_option(const gchar *name, const GValue *gval);
const gchar* ghb_lookup_combo_string(const gchar *name, const GValue *gval);
gchar* ghb_get_tmp_dir();
gint ghb_find_closest_audio_rate(gint rate);
GValue* ghb_lookup_acodec_value(gint val);

#endif // _HBBACKEND_H_
