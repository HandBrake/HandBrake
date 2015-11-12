/* ports.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_PORTS_H
#define HB_PORTS_H

#if ARCH_X86_64 || ARCH_X86_32
#define ARCH_X86
#endif

#if defined(_WIN32)
#define DIR_SEP_STR "\\"
#define DIR_SEP_CHAR '\\'
#define IS_DIR_SEP(c) (c == '\\' || c == '/')
#else
#define DIR_SEP_STR "/"
#define DIR_SEP_CHAR '/'
#define IS_DIR_SEP(c) (c == '/')
#endif


/************************************************************************
 * CPU info utilities
 ***********************************************************************/
enum hb_cpu_platform
{
    // list of microarchitecture codenames
    HB_CPU_PLATFORM_UNSPECIFIED = 0,
    HB_CPU_PLATFORM_INTEL_BNL,
    HB_CPU_PLATFORM_INTEL_SNB,
    HB_CPU_PLATFORM_INTEL_IVB,
    HB_CPU_PLATFORM_INTEL_SLM,
    HB_CPU_PLATFORM_INTEL_HSW,
    HB_CPU_PLATFORM_INTEL_BDW,
    HB_CPU_PLATFORM_INTEL_CHT,
    HB_CPU_PLATFORM_INTEL_SKL,
};
int         hb_get_cpu_count();
int         hb_get_cpu_platform();
const char* hb_get_cpu_name();
const char* hb_get_cpu_platform_name();

/************************************************************************
 * Utils
 ***********************************************************************/
// provide time in ms
uint64_t hb_get_date();
// provide time in us
uint64_t hb_get_time_us();

void     hb_snooze( int delay );
int      hb_platform_init();

#ifdef SYS_MINGW
typedef struct
{
    _WDIR *wdir;
    struct dirent entry;
} HB_DIR;
#else
typedef DIR HB_DIR;
#endif

#ifdef SYS_MINGW
typedef struct _stat64 hb_stat_t;
#else
typedef struct stat hb_stat_t;
#endif

HB_DIR* hb_opendir(const char *path);
int hb_closedir(HB_DIR *dir);
void hb_rewinddir(HB_DIR *dir);
struct dirent * hb_readdir(HB_DIR *dir);
int hb_mkdir(char * name);
int hb_stat(const char *path, hb_stat_t *sb);
FILE * hb_fopen(const char *path, const char *mode);
char * hb_strr_dir_sep(const char *path);

#ifdef __LIBHB__

// Convert utf8 string to current code page.
char * hb_utf8_to_cp(const char *src);

/* Everything from now is only used internally and hidden to the UI */

/************************************************************************
 * DVD utils
 ***********************************************************************/
int hb_dvd_region(char *device, int *region_mask);

/************************************************************************
 * File utils
 ***********************************************************************/
void hb_get_temporary_directory( char path[512] );
void hb_get_tempory_filename( hb_handle_t *, char name[1024],
                              char * fmt, ... );

#if defined( SYS_DARWIN )
int osx_get_user_config_directory( char path[512] );
#endif
void hb_get_user_config_directory( char path[512] );
void hb_get_user_config_filename( char name[1024], char *fmt, ... );
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
#elif defined( SYS_LINUX ) || defined( SYS_FREEBSD ) || defined ( SYS_SunOS ) || defined ( __FreeBSD_kernel__ )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#elif defined( SYS_CYGWIN )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 1
#elif defined( SYS_MINGW )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#endif

typedef void (thread_func_t)(void *);
hb_thread_t * hb_thread_init( const char * name, thread_func_t *function,
                              void * arg, int priority );
void          hb_thread_close( hb_thread_t ** );
int           hb_thread_has_exited( hb_thread_t * );

void          hb_yield(void);

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
void        hb_cond_timedwait( hb_cond_t * c, hb_lock_t * lock, int msec );
void        hb_cond_signal( hb_cond_t * );
void        hb_cond_broadcast( hb_cond_t * c );
void        hb_cond_close( hb_cond_t ** );

/************************************************************************
 * Network
 ***********************************************************************/
typedef struct hb_net_s hb_net_t;

hb_net_t * hb_net_open( char * address, int port );
int        hb_net_send( hb_net_t *, char * );
int        hb_net_recv( hb_net_t *, char *, int );
void       hb_net_close( hb_net_t ** );

/************************************************************************
* OS Sleep Allow / Prevent
***********************************************************************/
void* hb_system_sleep_opaque_init();
void  hb_system_sleep_opaque_close(void **opaque);
void  hb_system_sleep_private_enable(void *opaque);
void  hb_system_sleep_private_disable(void *opaque);

/************************************************************************
* Loadable Libraries
***********************************************************************/
void * hb_dlopen(const char *name);
void * hb_dlsym(void *h, const char *name);
int    hb_dlclose(void *h);

#if defined( SYS_MINGW )
#define HB_SO_EXT  ".dll"
#elif defined( SYS_DARWIN )
#define HB_SO_EXT  ".dylib"
#else
#define HB_SO_EXT  ".so"
#endif

#endif /* __LIBHB__ */

#endif

