/* ports.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifdef SYS_MINGW
#define _WIN32_WINNT 0x600
#endif

#ifdef USE_PTHREAD
#ifdef SYS_LINUX
#define _GNU_SOURCE
#include <sched.h>
#endif
#include <pthread.h>
#endif

#ifdef SYS_BEOS
#include <kernel/OS.h>
#endif

#if defined(SYS_DARWIN) || defined(SYS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef SYS_OPENBSD
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#ifdef SYS_MINGW
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <dlfcn.h>
#endif

#ifdef SYS_CYGWIN
#include <windows.h>
#endif

#ifdef SYS_MINGW
#include <pthread.h>
#include <windows.h>
#include <wchar.h>
#include <mbctype.h>
#include <locale.h>
#include <shlobj.h>
#endif

#ifdef SYS_SunOS
#include <sys/processor.h>
#endif

#include <time.h>
#include <sys/time.h>
#include <ctype.h>

#if defined( SYS_LINUX )
#include <linux/cdrom.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#elif defined( SYS_OPENBSD )
#include <sys/dvdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

#ifdef __APPLE__
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

#include <stddef.h>
#include <unistd.h>

#include "hb.h"
#include "libavutil/cpu.h"

/************************************************************************
 * hb_get_date()
 ************************************************************************
 * Returns the current date in milliseconds.
 * On Win32, we implement a gettimeofday emulation here because
 * libdvdread and libmp4v2 use it without checking.
 ************************************************************************/
/*
#ifdef SYS_CYGWIN
struct timezone
{
};

int gettimeofday( struct timeval * tv, struct timezone * tz )
{
    int tick;
    tick        = GetTickCount();
    tv->tv_sec  = tick / 1000;
    tv->tv_usec = ( tick % 1000 ) * 1000;
    return 0;
}
#endif
*/

// Convert utf8 string to current code page.
// The internal string representation in hb is utf8. But some
// libraries (libmkv, and mp4v2) expect filenames in the current
// code page.  So we must convert.
char * hb_utf8_to_cp(const char *src)
{
    char *dst = NULL;

#if defined( SYS_MINGW )
    int num_chars = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (num_chars <= 0)
        return NULL;
    wchar_t * tmp = calloc(num_chars, sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, src, -1, tmp, num_chars);
    int len = WideCharToMultiByte(GetACP(), 0, tmp, num_chars, NULL, 0, NULL, NULL);
    if (len <= 0)
        return NULL;
    dst = calloc(len, sizeof(char));
    WideCharToMultiByte(GetACP(), 0, tmp, num_chars, dst, len, NULL, NULL);
    free(tmp);
#else
    // Other platforms don't have code pages
    dst = strdup(src);
#endif

    return dst;
}

int hb_dvd_region(char *device, int *region_mask)
{
#if defined( DVD_LU_SEND_RPC_STATE ) && defined( DVD_AUTH )
    struct stat  st;
    dvd_authinfo ai;
    int          fd, ret;

    fd = open( device, O_RDONLY );
    if ( fd < 0 )
        return -1;
    if ( fstat( fd, &st ) < 0 )
	{
        close( fd );
        return -1;
	}
    if ( !( S_ISBLK( st.st_mode ) || S_ISCHR( st.st_mode ) ) )
	{
        close( fd );
        return -1;
	}

    ai.type = DVD_LU_SEND_RPC_STATE;
    ret = ioctl(fd, DVD_AUTH, &ai);
    close( fd );
    if ( ret < 0 )
        return ret;

    *region_mask = ai.lrpcs.region_mask;
    return 0;
#else
    return -1;
#endif
}

uint64_t hb_get_date()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( (uint64_t) tv.tv_sec * 1000 + (uint64_t) tv.tv_usec / 1000 );
}

uint64_t hb_get_time_us()
{
#ifdef SYS_MINGW
    static LARGE_INTEGER frequency;
    LARGE_INTEGER cur_time;

    if (frequency.QuadPart == 0)
    {
          QueryPerformanceFrequency(&frequency);
    }

    QueryPerformanceCounter(&cur_time);

    return (uint64_t)(1000000 * cur_time.QuadPart / frequency.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec);
#endif
}

/************************************************************************
 * hb_snooze()
 ************************************************************************
 * Waits <delay> milliseconds.
 ************************************************************************/
void hb_snooze( int delay )
{
    if( delay < 1 )
    {
        return;
    }
#if defined( SYS_BEOS )
    snooze( 1000 * delay );
#elif defined( SYS_DARWIN ) || defined( SYS_LINUX ) || defined( SYS_FREEBSD) || defined( SYS_SunOS )
    usleep( 1000 * delay );
#elif defined( SYS_CYGWIN ) || defined( SYS_MINGW )
    Sleep( delay );
#endif
}

/************************************************************************
 * Get information about the CPU (number of cores, name, platform name)
 ************************************************************************/
static void init_cpu_info();
static int  init_cpu_count();
struct
{
    enum hb_cpu_platform platform;
    const char *name;
    union
    {
        char buf[48];
        uint32_t buf4[12];
    };
    int count;
} hb_cpu_info;

int hb_get_cpu_count()
{
    return hb_cpu_info.count;
}

int hb_get_cpu_platform()
{
    return hb_cpu_info.platform;
}

const char* hb_get_cpu_name()
{
    return hb_cpu_info.name;
}

const char* hb_get_cpu_platform_name()
{
    switch (hb_cpu_info.platform)
    {
        // Intel 64 and IA-32 Architectures Software Developer's Manual, Vol. 3C
        // Table 35-1: CPUID Signature Values of DisplayFamily_DisplayModel
        case HB_CPU_PLATFORM_INTEL_BNL:
            return "Intel microarchitecture Bonnell";
        case HB_CPU_PLATFORM_INTEL_SNB:
            return "Intel microarchitecture Sandy Bridge";
        case HB_CPU_PLATFORM_INTEL_IVB:
            return "Intel microarchitecture Ivy Bridge";
        case HB_CPU_PLATFORM_INTEL_SLM:
            return "Intel microarchitecture Silvermont";
        case HB_CPU_PLATFORM_INTEL_HSW:
            return "Intel microarchitecture Haswell";
        case HB_CPU_PLATFORM_INTEL_BDW:
            return "Intel microarchitecture Broadwell";
        case HB_CPU_PLATFORM_INTEL_SKL:
            return "Intel microarchitecture Skylake";
        case HB_CPU_PLATFORM_INTEL_CHT:
            return "Intel microarchitecture Airmont";

        default:
            return NULL;
    }
}

#if ARCH_X86_64
#    define REG_b "rbx"
#    define REG_S "rsi"
#elif ARCH_X86_32
#    define REG_b "ebx"
#    define REG_S "esi"
#endif // ARCH_X86_32

#if ARCH_X86_64 || ARCH_X86_32
#define cpuid(index, eax, ebx, ecx, edx)                        \
    __asm__ volatile (                                          \
        "mov    %%"REG_b", %%"REG_S" \n\t"                      \
        "cpuid                       \n\t"                      \
        "xchg   %%"REG_b", %%"REG_S                             \
        : "=a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)    \
        : "0" (index))
#endif // ARCH_X86_64 || ARCH_X86_32

static void init_cpu_info()
{
    hb_cpu_info.name     = NULL;
    hb_cpu_info.count    = init_cpu_count();
    hb_cpu_info.platform = HB_CPU_PLATFORM_UNSPECIFIED;

    if (av_get_cpu_flags() & AV_CPU_FLAG_SSE)
    {
#if ARCH_X86_64 || ARCH_X86_32
        int eax, ebx, ecx, edx, family, model;

        cpuid(1, &eax, &ebx, &ecx, &edx);
        family = ((eax >> 8) & 0xf) + ((eax >> 20) & 0xff);
        model  = ((eax >> 4) & 0xf) + ((eax >> 12) & 0xf0);

        // Intel 64 and IA-32 Architectures Software Developer's Manual, Vol. 3C
        // Table 35-1: CPUID Signature Values of DisplayFamily_DisplayModel
        switch (family)
        {
            case 0x06:
            {
                switch (model)
                {
                    case 0x1C:
                    case 0x26:
                    case 0x27:
                    case 0x35:
                    case 0x36:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_BNL;
                        break;
                    case 0x2A:
                    case 0x2D:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_SNB;
                        break;
                    case 0x3A:
                    case 0x3E:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_IVB;
                        break;
                    case 0x37:
                    case 0x4A:
                    case 0x4D:
                    case 0x5A:
                    case 0x5D:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_SLM;
                        break;
                    case 0x3C:
                    case 0x3F:
                    case 0x45:
                    case 0x46:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_HSW;
                        break;
                    case 0x3D:
                    case 0x4F:
                    case 0x56:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_BDW;
                        break;
                    case 0x4C:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_CHT;
                        break;
                    case 0x4E:
                    case 0x5E:
                        hb_cpu_info.platform = HB_CPU_PLATFORM_INTEL_SKL;
                        break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        // Intel 64 and IA-32 Architectures Software Developer's Manual, Vol. 2A
        // Figure 3-8: Determination of Support for the Processor Brand String
        // Table 3-17: Information Returned by CPUID Instruction
        cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
        if ((eax & 0x80000004) < 0x80000004)
        {
            cpuid(0x80000002,
                  &hb_cpu_info.buf4[ 0],
                  &hb_cpu_info.buf4[ 1],
                  &hb_cpu_info.buf4[ 2],
                  &hb_cpu_info.buf4[ 3]);
            cpuid(0x80000003,
                  &hb_cpu_info.buf4[ 4],
                  &hb_cpu_info.buf4[ 5],
                  &hb_cpu_info.buf4[ 6],
                  &hb_cpu_info.buf4[ 7]);
            cpuid(0x80000004,
                  &hb_cpu_info.buf4[ 8],
                  &hb_cpu_info.buf4[ 9],
                  &hb_cpu_info.buf4[10],
                  &hb_cpu_info.buf4[11]);

            hb_cpu_info.name    = hb_cpu_info.buf;
            hb_cpu_info.buf[47] = '\0'; // just in case

            while (isspace(*hb_cpu_info.name))
            {
                // skip leading whitespace to prettify
                hb_cpu_info.name++;
            }
        }
#endif // ARCH_X86_64 || ARCH_X86_32
    }
}

/*
 * Whenever possible, returns the number of CPUs on the current computer.
 * Returns 1 otherwise.
 */
static int init_cpu_count()
{
    int cpu_count = 1;

#if defined(SYS_CYGWIN) || defined(SYS_MINGW)
    SYSTEM_INFO cpuinfo;
    GetSystemInfo( &cpuinfo );
    cpu_count = cpuinfo.dwNumberOfProcessors;

#elif defined(SYS_LINUX)
    unsigned int bit;
    cpu_set_t p_aff;
    memset( &p_aff, 0, sizeof(p_aff) );
    sched_getaffinity( 0, sizeof(p_aff), &p_aff );
    for( cpu_count = 0, bit = 0; bit < sizeof(p_aff); bit++ )
         cpu_count += (((uint8_t *)&p_aff)[bit / 8] >> (bit % 8)) & 1;

#elif defined(SYS_BEOS)
    system_info info;
    get_system_info( &info );
    cpu_count = info.cpu_count;

#elif defined(SYS_DARWIN) || defined(SYS_FREEBSD) || defined(SYS_OPENBSD)
    size_t length = sizeof( cpu_count );
#ifdef SYS_OPENBSD
    int mib[2] = { CTL_HW, HW_NCPU };
    if( sysctl(mib, 2, &cpu_count, &length, NULL, 0) )
#else
    if( sysctlbyname("hw.ncpu", &cpu_count, &length, NULL, 0) )
#endif
    {
        cpu_count = 1;
    }

#elif defined( SYS_SunOS )
    {
        processorid_t cpumax;
        int i,j=0;

        cpumax = sysconf(_SC_CPUID_MAX);

        for(i = 0; i <= cpumax; i++ )
        {
            if(p_online(i, P_STATUS) != -1)
            {
                j++;
            }
        }
        cpu_count=j;
    }
#endif

    cpu_count = MAX( 1, cpu_count );
    cpu_count = MIN( cpu_count, 64 );

    return cpu_count;
}

int hb_platform_init()
{
    int result = 0;

#if defined(SYS_MINGW) && defined(PTW32_STATIC_LIB)
    result = !pthread_win32_process_attach_np();
    if (result)
    {
        hb_error("pthread_win32_process_attach_np() failed!");
        return -1;
    }
#endif

#if defined(_WIN32) || defined(__MINGW32__)
    /*
     * win32 _IOLBF (line-buffering) is the same as _IOFBF (full-buffering).
     * force it to unbuffered otherwise informative output is not easily parsed.
     */
    result = setvbuf(stdout, NULL, _IONBF, 0);
    if (result)
    {
        hb_error("setvbuf(stdout, NULL, _IONBF, 0) failed!");
        return -1;
    }
    result = setvbuf(stderr, NULL, _IONBF, 0);
    if (result)
    {
        hb_error("setvbuf(stderr, NULL, _IONBF, 0) failed!");
        return -1;
    }
#endif

    init_cpu_info();

    return result;
}

/************************************************************************
 * Get app data config directory
 ***********************************************************************/
void hb_get_user_config_directory( char path[512] )
{
    /* Create the base */
#if defined( SYS_CYGWIN ) || defined( SYS_MINGW )
#ifndef CSIDL_FLAG_DONT_UNEXPAND
    /*
     * XXX: some old MinGW toolchains don't have SHGetKnownFolderPath.
     *
     * SHGetFolderPath is deprecated, but this should be no problem in practice.
     *
     * Note: explicitly call the Unicode/WCHAR function SHGetFolderPathW.
     */
    WCHAR wide_path[MAX_PATH];

    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wide_path) == S_OK &&
        WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, path, 512, NULL, NULL) != 0)
    {
        path[511] = 0;
        return;
    }
#else
    WCHAR *wide_path;

    if (SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &wide_path) == S_OK &&
        WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, path, 512, NULL, NULL) != 0)
    {
        CoTaskMemFree(wide_path);
        path[511] = 0;
        return;
    }
    else if (wide_path != NULL)
    {
        CoTaskMemFree(wide_path);
    }
#endif // !defined CSIDL_FLAG_DONT_UNEXPAND
#elif defined( SYS_LINUX )
    char *p;

    if ((p = getenv("XDG_CONFIG_HOME")) != NULL)
    {
        strncpy(path, p, 511);
        path[511] = 0;
        return;
    }
    else if ((p = getenv("HOME")) != NULL)
    {
        strncpy(path, p, 511);
        path[511] = 0;
        int len = strlen(path);
        strncpy(path + len, "/.config", 511 - len - 1);
        path[511] = 0;
        return;
    }
#elif defined( __APPLE__ )
    if (osx_get_user_config_directory(path) == 0)
    {
        return;
    }
#endif

    hb_error("Failed to lookup user config directory!");
    path[0] = 0;
}

/************************************************************************
 * Get a user config filename for HB
 ***********************************************************************/
void hb_get_user_config_filename( char name[1024], char *fmt, ... )
{
    va_list args;

    hb_get_user_config_directory( name );
#if defined( SYS_CYGWIN ) || defined( SYS_MINGW )
    strcat( name, "\\" );
#else
    strcat( name, "/" );
#endif

    va_start( args, fmt );
    vsnprintf( &name[strlen(name)], 1024 - strlen(name), fmt, args );
    va_end( args );
}

/************************************************************************
 * Get a temporary directory for HB
 ***********************************************************************/
void hb_get_temporary_directory( char path[512] )
{
    char base[512];
    char *p;

    /* Create the base */
#if defined( SYS_CYGWIN ) || defined( SYS_MINGW )
    int i_size = GetTempPath( 512, base );
    if( i_size <= 0 || i_size >= 512 )
    {
        if( getcwd( base, 512 ) == NULL )
            strcpy( base, "c:" ); /* Bad fallback but ... */
    }

    /* c:/path/ works like a charm under cygwin(win32?) so use it */
    while( ( p = strchr( base, '\\' ) ) )
        *p = '/';
#else
    if( (p = getenv( "TMPDIR" ) ) != NULL ||
        (p = getenv( "TEMP" ) ) != NULL )
        strcpy( base, p );
    else
        strcpy( base, "/tmp" );
#endif
    /* I prefer to remove evntual last '/' (for cygwin) */
    if( base[strlen(base)-1] == '/' )
        base[strlen(base)-1] = '\0';

    snprintf(path, 512, "%s/hb.%d", base, (int)getpid());
}

/************************************************************************
 * Get a tempory filename for HB
 ***********************************************************************/
void hb_get_tempory_filename( hb_handle_t * h, char name[1024],
                              char *fmt, ... )
{
    va_list args;

    hb_get_temporary_directory( name );
    strcat( name, "/" );

    va_start( args, fmt );
    vsnprintf( &name[strlen(name)], 1024 - strlen(name), fmt, args );
    va_end( args );
}

/************************************************************************
 * hb_stat
 ************************************************************************
 * Wrapper to the real stat, needed to handle utf8 filenames on
 * windows.
 ***********************************************************************/
int hb_stat(const char *path, hb_stat_t *sb)
{
#ifdef SYS_MINGW
    wchar_t path_utf16[MAX_PATH];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, path_utf16, MAX_PATH))
        return -1;
    return _wstat64( path_utf16, sb );
#else
    return stat(path, sb);
#endif
}

/************************************************************************
 * hb_fopen
 ************************************************************************
 * Wrapper to the real fopen, needed to handle utf8 filenames on
 * windows.
 ***********************************************************************/
FILE * hb_fopen(const char *path, const char *mode)
{
#ifdef SYS_MINGW
    FILE *f;
    wchar_t path_utf16[MAX_PATH];
    wchar_t mode_utf16[16];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, path_utf16, MAX_PATH))
        return NULL;
    if (!MultiByteToWideChar(CP_UTF8, 0, mode, -1, mode_utf16, 16))
        return NULL;
    errno_t ret = _wfopen_s(&f, path_utf16, mode_utf16);
    if (ret)
        return NULL;
    return f;
#else
    return fopen(path, mode);
#endif
}

HB_DIR* hb_opendir(const char *path)
{
#ifdef SYS_MINGW
    HB_DIR *dir;
    wchar_t path_utf16[MAX_PATH];

    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, path_utf16, MAX_PATH))
        return NULL;
    dir = malloc(sizeof(HB_DIR));
    if (dir == NULL)
        return NULL;
    dir->wdir = _wopendir(path_utf16);
    if (dir->wdir == NULL)
    {
        free(dir);
        return NULL;
    }
    return dir;
#else
    return opendir(path);
#endif
}

int hb_closedir(HB_DIR *dir)
{
#ifdef SYS_MINGW
    int ret;

    ret = _wclosedir(dir->wdir);
    free(dir);
    return ret;
#else
    return closedir(dir);
#endif
}

struct dirent * hb_readdir(HB_DIR *dir)
{
#ifdef SYS_MINGW
    struct _wdirent *entry;
    entry = _wreaddir(dir->wdir);
    if (entry == NULL)
        return NULL;

    int len = WideCharToMultiByte(CP_UTF8, 0, entry->d_name, -1,
                                  dir->entry.d_name, sizeof(dir->entry.d_name),
                                  NULL, NULL );
    dir->entry.d_ino = entry->d_ino;
    dir->entry.d_reclen = entry->d_reclen;
    dir->entry.d_namlen = len - 1;
    return &dir->entry;
#else
    return readdir(dir);
#endif
}

void hb_rewinddir(HB_DIR *dir)
{
#ifdef SYS_MINGW
    _wrewinddir(dir->wdir);
#else
    return rewinddir(dir);
#endif
}

char * hb_strr_dir_sep(const char *path)
{
#ifdef SYS_MINGW
    char *sep = strrchr(path, '/');
    if (sep == NULL)
        sep = strrchr(path, '\\');
    return sep;
#else
    return strrchr(path, '/');
#endif
}

/************************************************************************
 * hb_mkdir
 ************************************************************************
 * Wrapper to the real mkdir, needed only because it doesn't take a
 * second argument on Win32. Grrr.
 ***********************************************************************/
int hb_mkdir(char * path)
{
#ifdef SYS_MINGW
    wchar_t path_utf16[MAX_PATH];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, path_utf16, MAX_PATH))
        return -1;
    return _wmkdir(path_utf16);
#else
    return mkdir(path, 0755);
#endif
}

/************************************************************************
 * Portable thread implementation
 ***********************************************************************/
struct hb_thread_s
{
    char          * name;
    int             priority;
    thread_func_t * function;
    void          * arg;

    hb_lock_t     * lock;
    int             exited;

#if defined( SYS_BEOS )
    thread_id       thread;
#elif USE_PTHREAD
    pthread_t       thread;
//#elif defined( SYS_CYGWIN )
//    HANDLE          thread;
#endif
};

/* Get a unique identifier to thread and represent as 64-bit unsigned.
 * If unsupported, the value 0 is be returned.
 * Caller should use result only for display/log purposes.
 */
static uint64_t hb_thread_to_integer( const hb_thread_t* t )
{
#if defined( USE_PTHREAD )
    #if defined( SYS_CYGWIN )
        return (uint64_t)t->thread;
    #elif defined( _WIN32 ) || defined( __MINGW32__ )
    #if defined(PTW32_STATIC_LIB)
        return (uint64_t)(ptrdiff_t)t->thread.p;
    #else
        return (uint64_t)t->thread;
    #endif
    #elif defined( SYS_DARWIN )
        return (unsigned long)t->thread;
    #else
        return (uint64_t)t->thread;
    #endif
#else
    return 0;
#endif
}

/************************************************************************
 * hb_thread_func()
 ************************************************************************
 * We use it as the root routine for any thread, for two reasons:
 *  + To set the thread priority on OS X (pthread_setschedparam() could
 *    be called from hb_thread_init(), but it's nicer to do it as we
 *    are sure it is done before the real routine starts)
 *  + Get informed when the thread exits, so we know whether
 *    hb_thread_close() will block or not.
 ***********************************************************************/
static void attribute_align_thread hb_thread_func( void * _t )
{
    hb_thread_t * t = (hb_thread_t *) _t;

#if defined( SYS_DARWIN ) || defined( SYS_FREEBSD )
    /* Set the thread priority */
    struct sched_param param;
    memset( &param, 0, sizeof( struct sched_param ) );
    param.sched_priority = t->priority;
    pthread_setschedparam( pthread_self(), SCHED_OTHER, &param );
#endif

#if defined( SYS_BEOS )
    signal( SIGINT, SIG_IGN );
#endif

    /* Start the actual routine */
    t->function( t->arg );

    /* Inform that the thread can be joined now */
    hb_deep_log( 2, "thread %"PRIx64" exited (\"%s\")", hb_thread_to_integer( t ), t->name );
    hb_lock( t->lock );
    t->exited = 1;
    hb_unlock( t->lock );
}

/************************************************************************
 * hb_thread_init()
 ************************************************************************
 * name:     user-friendly name
 * function: the thread routine
 * arg:      argument of the routine
 * priority: HB_LOW_PRIORITY or HB_NORMAL_PRIORITY
 ***********************************************************************/
hb_thread_t * hb_thread_init( const char * name, void (* function)(void *),
                              void * arg, int priority )
{
    hb_thread_t * t = calloc( sizeof( hb_thread_t ), 1 );

    t->name     = strdup( name );
    t->function = function;
    t->arg      = arg;
    t->priority = priority;

    t->lock     = hb_lock_init();

    /* Create and start the thread */
#if defined( SYS_BEOS )
    t->thread = spawn_thread( (thread_func) hb_thread_func,
                              name, priority, t );
    resume_thread( t->thread );

#elif USE_PTHREAD
    pthread_create( &t->thread, NULL,
                    (void * (*)( void * )) hb_thread_func, t );

//#elif defined( SYS_CYGWIN )
//    t->thread = CreateThread( NULL, 0,
//        (LPTHREAD_START_ROUTINE) hb_thread_func, t, 0, NULL );
//
//    /* Maybe use THREAD_PRIORITY_LOWEST instead */
//    if( priority == HB_LOW_PRIORITY )
//        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
#endif

    hb_deep_log( 2, "thread %"PRIx64" started (\"%s\")", hb_thread_to_integer( t ), t->name );
    return t;
}

/************************************************************************
 * hb_thread_close()
 ************************************************************************
 * Joins the thread and frees memory.
 ***********************************************************************/
void hb_thread_close( hb_thread_t ** _t )
{
    hb_thread_t * t = *_t;

    /* Join the thread */
#if defined( SYS_BEOS )
    long exit_value;
    wait_for_thread( t->thread, &exit_value );

#elif USE_PTHREAD
    pthread_join( t->thread, NULL );

//#elif defined( SYS_CYGWIN )
//    WaitForSingleObject( t->thread, INFINITE );
#endif

    hb_deep_log( 2, "thread %"PRIx64" joined (\"%s\")", hb_thread_to_integer( t ), t->name );

    hb_lock_close( &t->lock );
    free( t->name );
    free( t );
    *_t = NULL;
}

/************************************************************************
 * hb_thread_has_exited()
 ************************************************************************
 * Returns 1 if the thread can be joined right away, 0 otherwise.
 ***********************************************************************/
int hb_thread_has_exited( hb_thread_t * t )
{
    int exited;

    hb_lock( t->lock );
    exited = t->exited;
    hb_unlock( t->lock );

    return exited;
}

/************************************************************************
 * Portable mutex implementation
 ***********************************************************************/
struct hb_lock_s
{
#if defined( SYS_BEOS )
    sem_id          sem;
#elif USE_PTHREAD
    pthread_mutex_t mutex;
//#elif defined( SYS_CYGWIN )
//    HANDLE          mutex;
#endif
};

/************************************************************************
 * hb_lock_init()
 * hb_lock_close()
 * hb_lock()
 * hb_unlock()
 ************************************************************************
 * Basic wrappers to OS-specific semaphore or mutex functions.
 ***********************************************************************/
hb_lock_t * hb_lock_init()
{
    hb_lock_t * l = calloc( sizeof( hb_lock_t ), 1 );

#if defined( SYS_BEOS )
    l->sem = create_sem( 1, "sem" );
#elif USE_PTHREAD
    pthread_mutexattr_t mta;

    pthread_mutexattr_init(&mta);

#if defined( SYS_CYGWIN ) || defined( SYS_FREEBSD )
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
#endif

    pthread_mutex_init( &l->mutex, &mta );
//#elif defined( SYS_CYGWIN )
//    l->mutex = CreateMutex( 0, FALSE, 0 );
#endif

    return l;
}

void hb_lock_close( hb_lock_t ** _l )
{
    hb_lock_t * l = *_l;

    if (l == NULL)
    {
        return;
    }
#if defined( SYS_BEOS )
    delete_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_destroy( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    CloseHandle( l->mutex );
#endif
    free( l );

    *_l = NULL;
}

void hb_lock( hb_lock_t * l )
{
#if defined( SYS_BEOS )
    acquire_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_lock( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    WaitForSingleObject( l->mutex, INFINITE );
#endif
}

void hb_unlock( hb_lock_t * l )
{
#if defined( SYS_BEOS )
    release_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_unlock( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    ReleaseMutex( l->mutex );
#endif
}

/************************************************************************
 * Portable condition variable implementation
 ***********************************************************************/
struct hb_cond_s
{
#if defined( SYS_BEOS )
    int                 thread;
#elif USE_PTHREAD
    pthread_cond_t      cond;
//#elif defined( SYS_CYGWIN )
//    HANDLE              event;
#endif
};

/************************************************************************
 * hb_cond_init()
 * hb_cond_close()
 * hb_cond_wait()
 * hb_cond_signal()
 ************************************************************************
 * Win9x is not supported by this implementation (SignalObjectAndWait()
 * only available on Windows 2000/XP).
 ***********************************************************************/
hb_cond_t * hb_cond_init()
{
    hb_cond_t * c = calloc( sizeof( hb_cond_t ), 1 );

    if( c == NULL )
        return NULL;

#if defined( SYS_BEOS )
    c->thread = -1;
#elif USE_PTHREAD
    pthread_cond_init( &c->cond, NULL );
//#elif defined( SYS_CYGWIN )
//    c->event = CreateEvent( NULL, FALSE, FALSE, NULL );
#endif

    return c;
}

void hb_cond_close( hb_cond_t ** _c )
{
    hb_cond_t * c = *_c;

    if (c == NULL)
    {
        return;
    }
#if defined( SYS_BEOS )
#elif USE_PTHREAD
    pthread_cond_destroy( &c->cond );
//#elif defined( SYS_CYGWIN )
//    CloseHandle( c->event );
#endif
    free( c );

    *_c = NULL;
}

void hb_cond_wait( hb_cond_t * c, hb_lock_t * lock )
{
#if defined( SYS_BEOS )
    c->thread = find_thread( NULL );
    release_sem( lock->sem );
    suspend_thread( c->thread );
    acquire_sem( lock->sem );
    c->thread = -1;
#elif USE_PTHREAD
    pthread_cond_wait( &c->cond, &lock->mutex );
//#elif defined( SYS_CYGWIN )
//    SignalObjectAndWait( lock->mutex, c->event, INFINITE, FALSE );
//    WaitForSingleObject( lock->mutex, INFINITE );
#endif
}

void hb_clock_gettime( struct timespec *tp )
{
    struct timeval tv;

    gettimeofday( &tv, NULL );
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
}

void hb_yield(void)
{
    sched_yield();
}

void hb_cond_timedwait( hb_cond_t * c, hb_lock_t * lock, int msec )
{
#if defined( SYS_BEOS )
    c->thread = find_thread( NULL );
    release_sem( lock->sem );
    suspend_thread( c->thread );
    acquire_sem( lock->sem );
    c->thread = -1;
#elif USE_PTHREAD
    struct timespec ts;
    hb_clock_gettime(&ts);
    ts.tv_nsec += (msec % 1000) * 1000000;
    ts.tv_sec += msec / 1000 + (ts.tv_nsec / 1000000000);
    ts.tv_nsec %= 1000000000;
    pthread_cond_timedwait( &c->cond, &lock->mutex, &ts );
#endif
}

void hb_cond_signal( hb_cond_t * c )
{
#if defined( SYS_BEOS )
    while( c->thread != -1 )
    {
        thread_info info;
        get_thread_info( c->thread, &info );
        if( info.state == B_THREAD_SUSPENDED )
        {
            resume_thread( c->thread );
            break;
        }
        /* Looks like we have been called between hb_cond_wait's
           release_sem() and suspend_thread() lines. Wait until the
           thread is actually suspended before we resume it */
        snooze( 5000 );
    }
#elif USE_PTHREAD
    pthread_cond_signal( &c->cond );
//#elif defined( SYS_CYGWIN )
//    PulseEvent( c->event );
#endif
}

void hb_cond_broadcast( hb_cond_t * c )
{
#if USE_PTHREAD
    pthread_cond_broadcast( &c->cond );
#endif
}

/************************************************************************
 * Network
 ***********************************************************************/

struct hb_net_s
{
    int socket;
};

hb_net_t * hb_net_open( char * address, int port )
{
    hb_net_t * n = calloc( sizeof( hb_net_t ), 1 );

    struct sockaddr_in   sock;
    struct hostent     * host;

#ifdef SYS_MINGW
    WSADATA wsaData;
    int iResult, winsock_init = 0;

    // Initialize Winsock
    if (!winsock_init)
    {
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
        {
            hb_log("WSAStartup failed: %d", iResult);
            free(n);
            return NULL;
        }
        winsock_init = 1;
    }
#endif

    /* TODO: find out why this doesn't work on Win32 */
    if( !( host = gethostbyname( address ) ) )
    {
        hb_log( "gethostbyname failed (%s)", address );
        free( n );
        return NULL;
    }

    memset( &sock, 0, sizeof( struct sockaddr_in ) );
    sock.sin_family = host->h_addrtype;
    sock.sin_port   = htons( port );
    memcpy( &sock.sin_addr, host->h_addr, host->h_length );

    if( ( n->socket = socket( host->h_addrtype, SOCK_STREAM, 0 ) ) < 0 )
    {
        hb_log( "socket failed" );
        free( n );
        return NULL;
    }

    if( connect( n->socket, (struct sockaddr *) &sock,
                 sizeof( struct sockaddr_in ) ) < 0 )
    {
        hb_log( "connect failed" );
        free( n );
        return NULL;
    }

    return n;
}

int hb_net_send( hb_net_t * n, char * buffer )
{
    return send( n->socket, buffer, strlen( buffer ), 0 );
}

int hb_net_recv( hb_net_t * n, char * buffer, int size )
{
    return recv( n->socket, buffer, size - 1, 0 );
}

void hb_net_close( hb_net_t ** _n )
{
    hb_net_t * n = (hb_net_t *) *_n;
    close( n->socket );
    free( n );
    *_n = NULL;
}

/************************************************************************
* OS Sleep Allow / Prevent
***********************************************************************/

#ifdef __APPLE__
// 128 chars limit for IOPMAssertionCreateWithName
static CFStringRef reasonForActivity =
    CFSTR("HandBrake is currently scanning and/or encoding");
#endif

void* hb_system_sleep_opaque_init()
{
    void *opaque = NULL;
#ifdef __APPLE__
    opaque = calloc(sizeof(IOPMAssertionID), 1);
    if (opaque == NULL)
    {
        hb_error("hb_system_sleep: failed to allocate opaque");
        return NULL;
    }

    IOPMAssertionID *assertionID = (IOPMAssertionID*)opaque;
    *assertionID = -1;
#endif
    return opaque;
}

void hb_system_sleep_opaque_close(void **opaque)
{
    if (*opaque != NULL)
    {
        hb_system_sleep_private_enable(*opaque);
    }
#ifdef __APPLE__
    if (*opaque != NULL)
    {
        IOPMAssertionID *assertionID = (IOPMAssertionID*)*opaque;
        free(assertionID);
    }
#endif
    *opaque = NULL;
}

void hb_system_sleep_private_enable(void *opaque)
{
#ifdef __APPLE__
    if (opaque == NULL)
    {
        hb_error("hb_system_sleep: opaque is NULL");
    }

    IOPMAssertionID *assertionID = (IOPMAssertionID*)opaque;
    if (*assertionID == -1)
    {
        // nothing to do
        return;
    }

    IOReturn success = IOPMAssertionRelease(*assertionID);
    if (success == kIOReturnSuccess)
    {
        hb_deep_log(3,
                    "hb_system_sleep: assertion %d released, sleep allowed",
                    *assertionID);
        *assertionID = -1;
    }
    else
    {
        hb_log("hb_system_sleep: failed to allow system sleep");
    }
#endif
}

void hb_system_sleep_private_disable(void *opaque)
{
#ifdef __APPLE__
    if (opaque == NULL)
    {
        hb_error("hb_system_sleep: opaque is NULL");
    }
    
    IOPMAssertionID *assertionID = (IOPMAssertionID*)opaque;
    if (*assertionID != -1)
    {
        // nothing to do
        return;
    }

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleSystemSleep,
                                                   kIOPMAssertionLevelOn,
                                                   reasonForActivity,
                                                   assertionID);
    if (success == kIOReturnSuccess)
    {
        hb_deep_log(3,
                    "hb_system_sleep: assertion %d created, sleep prevented",
                    *assertionID);
    }
    else
    {
        hb_log("hb_system_sleep: failed to prevent system sleep");
    }
#endif
}

void * hb_dlopen(const char *name)
{
#ifdef SYS_MINGW
    HMODULE h = LoadLibraryA(name);
#else
    void *h = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
#endif

    return h;
}

void * hb_dlsym(void *h, const char *name)
{
#ifdef SYS_MINGW
    FARPROC p = GetProcAddress(h, name);
#else
    void *p = dlsym(h, name);
#endif
    return p;
}

int hb_dlclose(void *h)
{
#ifdef SYS_MINGW
    return FreeLibrary(h);
#else
    return dlclose(h);
#endif
}

