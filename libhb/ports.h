/* $Id: ports.h,v 1.7 2005/10/15 18:05:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_PORTS_H
#define HB_PORTS_H

/************************************************************************
 * Utils
 ***********************************************************************/
uint64_t hb_get_date();
void     hb_snooze( int delay );
int      hb_get_cpu_count();

#ifdef __LIBHB__

/* Everything from now is only used internally and hidden to the UI */

/************************************************************************
 * Files utils
 ***********************************************************************/
void hb_get_tempory_directory( hb_handle_t * h, char path[512] );
void hb_get_tempory_filename( hb_handle_t *, char name[1024],
                              char * fmt, ... );
void hb_mkdir( char * name );

/************************************************************************
 * Threads
 ***********************************************************************/
typedef struct hb_thread_s hb_thread_t;

#if defined( SYS_BEOS )
#  define HB_LOW_PRIORITY    5
#  define HB_NORMAL_PRIORITY 10
#elif defined( SYS_DARWIN )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 31
#elif defined( SYS_LINUX ) || defined( SYS_FREEBSD ) || defined ( SYS_SunOS )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#elif defined( SYS_CYGWIN )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 1
#endif

hb_thread_t * hb_thread_init( char * name, void (* function)(void *),
                              void * arg, int priority );
void          hb_thread_close( hb_thread_t ** );
int           hb_thread_has_exited( hb_thread_t * );

/************************************************************************
 * Mutexes
 ***********************************************************************/

hb_lock_t * hb_lock_init();
void        hb_lock_close( hb_lock_t ** );
void        hb_lock( hb_lock_t * );
void        hb_unlock( hb_lock_t * );

/************************************************************************
 * Condition variables
 ***********************************************************************/
typedef struct hb_cond_s hb_cond_t;

hb_cond_t * hb_cond_init();
void        hb_cond_wait( hb_cond_t *, hb_lock_t * );
void        hb_cond_signal( hb_cond_t * );
void        hb_cond_close( hb_cond_t ** );

/************************************************************************
 * Network
 ***********************************************************************/
typedef struct hb_net_s hb_net_t;

hb_net_t * hb_net_open( char * address, int port );
int        hb_net_send( hb_net_t *, char * );
int        hb_net_recv( hb_net_t *, char *, int );
void       hb_net_close( hb_net_t ** );

#endif /* __LIBHB__ */

#endif

