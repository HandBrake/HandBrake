#ifndef HB_HB_H
#define HB_HB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/* hb_init()
   Initializes a libhb session (launches his own thread, detects CPUs,
   etc) */
#define HB_DEBUG_NONE 0
#define HB_DEBUG_ALL  1
void          hb_register( hb_work_object_t * );
hb_handle_t * hb_init_real( int verbose, int update_check );
hb_handle_t * hb_init_dl ( int verbose, int update_check ); // hb_init for use with dylib 

#define hb_init(v,u) \
hb_init_real( v, u ); \
hb_register( &hb_sync ); \
hb_register( &hb_decmpeg2 ); \
hb_register( &hb_decsub ); \
hb_register( &hb_render ); \
hb_register( &hb_encavcodec ); \
hb_register( &hb_encxvid ); \
hb_register( &hb_encx264 ); \
hb_register( &hb_deca52 ); \
hb_register( &hb_decavcodec ); \
hb_register( &hb_declpcm ); \
hb_register( &hb_encfaac ); \
hb_register( &hb_enclame ); \
hb_register( &hb_encvorbis ); \

#define hb_init_express(v,u) \
hb_init_real( v, u ); \
hb_register( &hb_sync ); \
hb_register( &hb_decmpeg2 ); \
hb_register( &hb_decsub ); \
hb_register( &hb_render ); \
hb_register( &hb_encavcodec ); \
hb_register( &hb_encx264 ); \
hb_register( &hb_deca52 ); \
hb_register( &hb_decavcodec ); \
hb_register( &hb_declpcm ); \
hb_register( &hb_encfaac ); \

/* hb_get_version() */
char        * hb_get_version( hb_handle_t * );
int           hb_get_build( hb_handle_t * );

/* hb_check_update()
   Checks for an update on the website. If there is, returns the build
   number and points 'version' to a version description. Returns a
   negative value otherwise. */
int           hb_check_update( hb_handle_t * h, char ** version );

/* hb_set_cpu_count()
   Force libhb to act as if you had X CPU(s).
   Default is to use the detected count (see also hb_get_cpu_count() in
   ports.h) */
void          hb_set_cpu_count( hb_handle_t *, int );

char *        hb_dvd_name( char * path );

/* hb_scan()
   Scan the specified path. Can be a DVD device, a VIDEO_TS folder or
   a VOB file. If title_index is 0, scan all titles. */
void          hb_scan( hb_handle_t *, const char * path,
                       int title_index );

/* hb_get_titles()
   Returns the list of valid titles detected by the latest scan. */
hb_list_t   * hb_get_titles( hb_handle_t * );

void          hb_get_preview( hb_handle_t *, hb_title_t *, int,
                              uint8_t * );
void          hb_set_size( hb_job_t *, int ratio, int pixels );

/* Handling jobs */
int           hb_count( hb_handle_t * );
hb_job_t *    hb_job( hb_handle_t *, int );
void          hb_add( hb_handle_t *, hb_job_t * );
void          hb_rem( hb_handle_t *, hb_job_t * );

void          hb_start( hb_handle_t * );
void          hb_pause( hb_handle_t * );
void          hb_resume( hb_handle_t * );
void          hb_stop( hb_handle_t * );

/* hb_get_state()
   Should be regularly called by the UI (like 5 or 10 times a second).
   Look at test/test.c to see how to use it. */
void hb_get_state( hb_handle_t *, hb_state_t * );

/* hb_close()
   Aborts all current jobs if any, frees memory. */
void          hb_close( hb_handle_t ** );

#ifdef __cplusplus
}
#endif

#endif
