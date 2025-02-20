/* hbtypes.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_TYPES_H
#define HANDBRAKE_TYPES_H

typedef struct hb_handle_s hb_handle_t;
typedef struct hb_list_s hb_list_t;
typedef struct hb_buffer_list_s hb_buffer_list_t;
typedef struct hb_rate_s hb_rate_t;
typedef struct hb_dither_s hb_dither_t;
typedef struct hb_mixdown_s hb_mixdown_t;
typedef struct hb_encoder_s hb_encoder_t;
typedef struct hb_container_s hb_container_t;
typedef struct hb_rational_s hb_rational_t;
typedef struct hb_geometry_s hb_geometry_t;
typedef struct hb_geometry_crop_s hb_geometry_crop_t;
typedef struct hb_geometry_settings_s hb_geometry_settings_t;
typedef struct hb_image_s hb_image_t;
typedef struct hb_job_s  hb_job_t;
typedef struct hb_title_set_s hb_title_set_t;
typedef struct hb_title_s hb_title_t;
typedef struct hb_chapter_s hb_chapter_t;
typedef struct hb_audio_s hb_audio_t;
typedef struct hb_audio_config_s hb_audio_config_t;
typedef struct hb_subtitle_s hb_subtitle_t;
typedef struct hb_subtitle_config_s hb_subtitle_config_t;
typedef struct hb_attachment_s hb_attachment_t;
typedef struct hb_metadata_s hb_metadata_t;
typedef struct hb_coverart_s hb_coverart_t;
typedef struct hb_state_s hb_state_t;
typedef struct hb_data_s hb_data_t;
typedef struct hb_work_private_s hb_work_private_t;
typedef struct hb_work_object_s  hb_work_object_t;
typedef struct hb_filter_private_s hb_filter_private_t;
typedef struct hb_filter_object_s  hb_filter_object_t;
typedef struct hb_motion_metric_private_s  hb_motion_metric_private_t;
typedef struct hb_motion_metric_object_s  hb_motion_metric_object_t;
typedef struct hb_blend_private_s  hb_blend_private_t;
typedef struct hb_blend_object_s  hb_blend_object_t;
typedef struct hb_buffer_settings_s hb_buffer_settings_t;
typedef struct hb_image_format_s hb_image_format_t;
typedef struct hb_fifo_s hb_fifo_t;
typedef struct hb_lock_s hb_lock_t;
typedef struct hb_mastering_display_metadata_s hb_mastering_display_metadata_t;
typedef struct hb_content_light_metadata_s hb_content_light_metadata_t;
typedef struct hb_ambient_viewing_environment_metadata_s hb_ambient_viewing_environment_metadata_t;
typedef struct hb_dovi_conf_s hb_dovi_conf_t;

#endif // HANDBRAKE_TYPES_H
