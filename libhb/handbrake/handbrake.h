/* handbrake.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_HANDBRAKE_H
#define HANDBRAKE_HANDBRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "handbrake/common.h"
#include "handbrake/project.h"
#include "handbrake/compat.h"
#include "handbrake/hb_json.h"
#include "handbrake/preset.h"
#include "handbrake/param.h"
#include "handbrake/colormap.h"

/* hb_init()
   Initializes a libhb session (launches his own thread, detects CPUs,
   etc) */
#define HB_DEBUG_NONE 0
#define HB_DEBUG_ALL  1
#define HB_PREVIEW_FORMAT_YUV 0
#define HB_PREVIEW_FORMAT_JPG 1
void          hb_register( hb_work_object_t * );
void          hb_register_logger( void (*log_cb)(const char* message) );
hb_handle_t * hb_init( int verbose );
void          hb_log_level_set(hb_handle_t *h, int level);

/* hb_get_version() */
const char  * hb_get_full_description(void);
const char  * hb_get_version( hb_handle_t * );
int           hb_get_build( hb_handle_t * );

char *        hb_dvd_name( char * path );
void          hb_dvd_set_dvdnav( int enable );

/* hb_scan()
   Scan the specified paths. Can be a DVD device, a VIDEO_TS folder or
   a VOB file. If title_index is 0, scan all titles. */
void          hb_scan( hb_handle_t * h, hb_list_t * paths, int title_index,
                      int preview_count, int store_previews, uint64_t min_duration, uint64_t max_duration,
                      int crop_threshold_frames, int crop_threshold_pixels,
                      hb_list_t * exclude_extensions, int hw_decode, int keep_duplicate_titles);

void          hb_scan_stop( hb_handle_t * );
void          hb_force_rescan( hb_handle_t * );
uint64_t      hb_first_duration( hb_handle_t * );

/* hb_get_titles()
   Returns the list of valid titles detected by the latest scan. */
hb_list_t   * hb_get_titles( hb_handle_t * );

/* hb_get_title_set()
   Returns the title set which contains a list of valid titles detected
   by the latest scan and title set data. */
hb_title_set_t   * hb_get_title_set( hb_handle_t * );

#ifdef __LIBHB__
/* hb_detect_comb()
   Analyze a frame for interlacing artifacts, returns true if they're found.
   Taken from Thomas Oestreich's 32detect filter in the Transcode project.  */
int hb_detect_comb( hb_buffer_t * buf, int color_equal, int color_diff, int threshold, int prog_equal, int prog_diff, int prog_threshold );

// JJJ: title->job?
int           hb_save_preview( hb_handle_t * h, int title, int preview,
                               hb_buffer_t *buf, int format );
hb_buffer_t * hb_read_preview( hb_handle_t * h, hb_title_t *title,
                               int preview, int format );
#endif // __LIBHB__

hb_image_t  * hb_get_preview(hb_handle_t * h, hb_dict_t * job_dict,
                             int picture, int rescale, int pix_fmt);
hb_image_t  * hb_get_preview3(hb_handle_t * h, int picture,
                              hb_dict_t * job_dict);
void          hb_rotate_geometry( hb_geometry_crop_t * geo,
                                  hb_geometry_crop_t * result,
                                  int angle, int hflip);
void          hb_set_anamorphic_size2(hb_geometry_t          * src_geo,
                                      hb_geometry_settings_t * geo,
                                      hb_geometry_t          * result);
void          hb_add_filter_dict( hb_job_t * job, hb_filter_object_t * filter,
                                  const hb_dict_t * settings_in );
void          hb_add_filter( hb_job_t * job, hb_filter_object_t * filter,
                             const char * settings );
void          hb_add_filter2( hb_value_array_t * list, hb_dict_t * filter );

/* Handling jobs */
int           hb_count( hb_handle_t * );
hb_job_t    * hb_job( hb_handle_t *, int );
int           hb_add( hb_handle_t *, hb_job_t * );
void          hb_rem( hb_handle_t *, hb_job_t * );

hb_title_t  * hb_find_title_by_index( hb_handle_t *h, int title_index );
hb_job_t    * hb_job_init_by_index( hb_handle_t *h, int title_index );
hb_job_t    * hb_job_init( hb_title_t * title );
void          hb_job_close( hb_job_t ** job );

void          hb_start( hb_handle_t * );
void          hb_pause( hb_handle_t * );
void          hb_resume( hb_handle_t * );
void          hb_stop( hb_handle_t * );

void          hb_system_sleep_allow(hb_handle_t*);
void          hb_system_sleep_prevent(hb_handle_t*);

/* Persistent data between jobs. */
typedef struct hb_interjob_s
{
    int     sequence_id;     /* job->sequence_id                   */
    int     frame_count;     /* number of frames counted by sync   */
    int     out_frame_count; /* number of frames counted by render */
    int64_t total_time;      /* measured length in 90kHz ticks     */
    hb_rational_t vrate;     /* measured output vrate              */

    hb_subtitle_t *select_subtitle; /* foreign language scan subtitle */

    void *context;
    int   context_size;
} hb_interjob_t;

hb_interjob_t * hb_interjob_get( hb_handle_t * );

/* hb_get_state()
   Should be regularly called by the UI (like 5 or 10 times a second).
   Look at test/test.c to see how to use it. */
void hb_get_state( hb_handle_t *, hb_state_t * );
void hb_get_state2( hb_handle_t *, hb_state_t * );

/* hb_close()
   Aborts all current jobs if any, frees memory. */
void          hb_close( hb_handle_t ** );

/* hb_global_init()
   Performs process initialization. */
int           hb_global_init(void);
int           hb_global_init_no_hardware(void);
/* hb_global_close()
   Performs final cleanup for the process. */
void          hb_global_close(void);

/* hb_get_instance_id()
   Return the unique instance id of an libhb instance created by hb_init. */
int hb_get_instance_id( hb_handle_t * h );

int is_hardware_disabled(void);

#ifdef __cplusplus
}
#endif

#endif // HANDBRAKE_HANDBRAKE_H
