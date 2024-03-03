/* hb-backend.h
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

#pragma once

#include "common.h"
#include "handbrake/handbrake.h"
#include "handbrake/lang.h"
#include "settings.h"
#include "values.h"

G_BEGIN_DECLS

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
    gint    unique_id;
    gint    pass_id;
    gint    pass;
    gint    pass_count;
    gdouble progress;
    gdouble rate_cur;
    gdouble rate_avg;
    gint64  eta_seconds;
    gint    hours;
    gint    minutes;
    gint    seconds;
    gint64  paused;
    gint    error;
} ghb_instance_status_t;

typedef struct
{
    ghb_instance_status_t scan;
    ghb_instance_status_t queue;
    ghb_instance_status_t live;
} ghb_status_t;

#define MOD_ROUND(v,m) ((m==1)?v:(m * ((v + (m>>1)) / m)))
#define MOD_DOWN(v,m)  (m * (v / m))
#define MOD_UP(v,m)    (m * ((v + m - 1) / m))

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
    float *min,
    float *max,
    float *step,
    float *page,
    gint *digits,
    int *direction);
float ghb_vquality_default(signal_user_data_t *ud);

void ghb_combo_init(signal_user_data_t *ud);
void ghb_backend_init(gint debug);
void ghb_log_level_set(int level);
void ghb_backend_close(void);
int  ghb_add_job(hb_handle_t *h, GhbValue *js);
void ghb_remove_job(gint unique_id);
void ghb_start_queue(void);
void ghb_stop_queue(void);
void ghb_pause_queue(void);
void ghb_resume_queue(void);
void ghb_pause_resume_queue(void);

void ghb_start_live_encode(void);
void ghb_stop_live_encode(void);

void ghb_set_scan_state(gint state);
void ghb_set_queue_state(gint state);
void ghb_set_live_state(gint state);

void ghb_clear_scan_state(gint state);
void ghb_clear_queue_state(gint state);
void ghb_clear_live_state(gint state);

void ghb_set_state(gint state);
gint ghb_get_scan_state(void);
gint ghb_get_queue_state(void);
void ghb_get_status(ghb_status_t *status);
void ghb_track_status(void);
void ghb_backend_scan(const char *path, int titleindex, int preview_count, uint64_t min_duration);
void ghb_backend_scan_stop(void);
void ghb_backend_queue_scan(const gchar *path, gint titleindex);
hb_list_t * ghb_get_title_list(void);
void ghb_par_init(signal_user_data_t *ud);
void ghb_apply_crop(GhbValue *settings, const hb_geometry_crop_t * geo, const hb_title_t * title);
void ghb_set_scale(signal_user_data_t *ud, gint mode);
void ghb_set_scale_settings(signal_user_data_t * ud,
                            GhbValue *settings, gint mode);

void ghb_set_scale_busy(gboolean busy);
void ghb_picture_settings_deps(signal_user_data_t *ud);
gint64 ghb_get_chapter_duration(const hb_title_t *title, gint chap);
gint64 ghb_get_chapter_start(const hb_title_t *title, gint chap);
gint64 ghb_chapter_range_get_duration(const hb_title_t *title,
                                      gint sc, gint ec);
gint ghb_get_best_mix(uint64_t layout, gint acodec, gint mix);
gboolean ghb_audio_is_passthru(gint acodec);
gboolean ghb_audio_can_passthru(gint acodec);
gint ghb_get_default_acodec(void);
void ghb_grey_combo_options(signal_user_data_t *ud);
void ghb_update_ui_combo_box(
    signal_user_data_t *ud, const gchar *name,
    const void* user_data, gboolean all);
const gchar* ghb_get_source_audio_lang(const hb_title_t *title, gint track);
gint ghb_find_audio_track(const hb_title_t *title, const gchar *lang, int start);
void ghb_add_all_subtitles(signal_user_data_t *ud, gint titleindex);
gint ghb_find_subtitle_track(const hb_title_t * title, const gchar * lang, int start);
gint ghb_pick_subtitle_track(signal_user_data_t *ud);
gint ghb_longest_title(void);
GdkPixbuf* ghb_get_preview_image(gint index, signal_user_data_t *ud);
gchar* ghb_dvd_volname(const gchar *device);
gint ghb_subtitle_track_source(GhbValue *settings, gint track);
const gchar* ghb_subtitle_track_lang(GhbValue *settings, gint track);

gboolean ghb_validate_vquality(GhbValue *settings);
gboolean ghb_validate_audio(GhbValue *settings, GtkWindow *parent);
gboolean ghb_validate_subtitles(GhbValue *settings, GtkWindow *parent);
gboolean ghb_validate_video(GhbValue *settings, GtkWindow *parent);
void ghb_set_custom_filter_tooltip(signal_user_data_t *ud,
                                   const char *name, const char * desc,
                                   int filter_id);
gboolean ghb_validate_filters(GhbValue *settings, GtkWindow *parent);
void ghb_hb_cleanup(gboolean partial);
gint ghb_lookup_combo_int(const gchar *name, const GhbValue *gval);
gdouble ghb_lookup_combo_double(const gchar *name, const GhbValue *gval);
gchar* ghb_lookup_combo_option(const gchar *name, const GhbValue *gval);
const char* ghb_lookup_filter_name(int filter_id, const char *short_name, int preset);
const char* ghb_get_tmp_dir(void);
gint ghb_find_closest_audio_samplerate(gint rate);

void ghb_init_lang_list_model(GtkTreeView *tv);
void ghb_init_lang_list(GtkTreeView *tv, signal_user_data_t *ud);

void ghb_init_combo_box(GtkComboBox *combo);
void ghb_audio_encoder_opts_add_autopass(GtkComboBox *combo);
void ghb_audio_encoder_opts_set(GtkComboBox *combo);
void ghb_audio_bitrate_opts_set(GtkComboBox *combo);
void ghb_audio_encoder_opts_set_with_mask(GtkComboBox *combo, int mask, int neg_mask);
void ghb_audio_bitrate_opts_filter(GtkComboBox *combo, gint first_rate, gint last_rate);
void ghb_mix_opts_set(GtkComboBox *combo);
void ghb_mix_opts_filter(GtkComboBox *combo, gint acodec);
void ghb_audio_samplerate_opts_set(GtkComboBox *combo);
void ghb_audio_samplerate_opts_filter(GtkComboBox *combo, gint acodec);

int ghb_lookup_lang(const GhbValue *glang);
const iso639_lang_t* ghb_iso639_lookup_by_int(int idx);

// libhb lookup helpers
const hb_title_t* ghb_lookup_title(int title_id, int *index);
int ghb_lookup_title_index(int title_id);
const hb_title_t* ghb_lookup_title(int title_id, int *index);
GhbValue* ghb_get_title_dict(int title_id);
int ghb_lookup_queue_title_index(int title_id);
const hb_title_t* ghb_lookup_queue_title(int title_id, int *index);
GhbValue* ghb_get_title_dict(int title_id);
const hb_container_t* ghb_lookup_container_by_name(const gchar *name);
const hb_encoder_t* ghb_lookup_audio_encoder(const char *name);
int ghb_lookup_audio_encoder_codec(const char *name);
int ghb_settings_audio_encoder_codec(const GhbValue *settings, const char *name);
const hb_encoder_t* ghb_settings_audio_encoder(
    const GhbValue *settings, const char *name);
const hb_encoder_t* ghb_lookup_video_encoder(const char *name);
int ghb_lookup_video_encoder_codec(const char *name);
int ghb_settings_video_encoder_codec(const GhbValue *settings, const char *name);
const hb_encoder_t* ghb_settings_video_encoder(
    const GhbValue *settings, const char *name);
const hb_mixdown_t* ghb_lookup_mixdown(const char *name);
int ghb_lookup_mixdown_mix(const char *name);
int ghb_settings_mixdown_mix(const GhbValue *settings, const char *name);
const hb_mixdown_t* ghb_settings_mixdown(
    const GhbValue *settings, const char *name);
const hb_rate_t* ghb_lookup_video_framerate(const char *name);
int ghb_lookup_video_framerate_rate(const char *name);
int ghb_settings_video_framerate_rate(const GhbValue *settings, const char *name);
const hb_rate_t* ghb_settings_video_framerate(
    const GhbValue *settings, const char *name);
const hb_rate_t* ghb_lookup_audio_samplerate(const char *name);
int ghb_lookup_audio_samplerate_rate(const char *name);
int ghb_settings_audio_samplerate_rate(
    const GhbValue *settings, const char *name);
int ghb_settings_audio_bitrate_rate(
    const GhbValue *settings, const char *name);
const hb_rate_t* ghb_settings_audio_samplerate(
    const GhbValue *settings, const char *name);
const char* ghb_audio_samplerate_get_short_name(int rate);
const hb_rate_t* ghb_lookup_audio_bitrate(const char *name);
int ghb_lookup_audio_bitrate_rate(const char *name);
const hb_rate_t* ghb_settings_audio_bitrate(
    const GhbValue *settings, const char *name);
const char* ghb_audio_bitrate_get_short_name(int rate);
hb_audio_config_t* ghb_get_audio_info(const hb_title_t *title, gint track);
hb_subtitle_t* ghb_get_subtitle_info(const hb_title_t *title, gint track);
char * ghb_get_display_aspect_string(double disp_width, double disp_height);

hb_handle_t* ghb_scan_handle(void);
hb_handle_t* ghb_queue_handle(void);
hb_handle_t* ghb_live_handle(void);
gchar* ghb_create_title_label(const hb_title_t *title);
gchar* ghb_create_source_label(const hb_title_t * title);
gchar* ghb_create_volume_label(const hb_title_t * title);

const gchar * ghb_lookup_resolution_limit(int width, int height);
int ghb_lookup_resolution_limit_dimensions(const gchar * opt,
                                           int * width, int * height);
const gchar * ghb_get_filter_name(hb_filter_object_t *filter);

G_END_DECLS
