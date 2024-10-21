/* stream.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/lang.h"
#include "handbrake/extradata.h"
#include "libbluray/bluray.h"

#define min(a, b) a < b ? a : b
#define HB_MAX_PROBE_SIZE (1*1024*1024)
#define HB_MAX_PROBES     3

/*
 * This table defines how ISO MPEG stream type codes map to HandBrake
 * codecs. It is indexed by the 8 bit stream type and contains the codec
 * worker object id and a parameter for that worker proc (ignored except
 * for the ffmpeg-based codecs in which case it is the ffmpeg codec id).
 *
 * Entries with a worker proc id of 0 or a kind of 'U' indicate that HB
 * doesn't handle the stream type.
 * N - Not used
 * U - Unknown (to be determined by further processing)
 * A - Audio
 * V - Video
 * S - Subtitle
 * P - PCR
 */
typedef enum { U, N, A, V, P, S } kind_t;
typedef struct {
    kind_t kind; /* not handled / unknown / audio / video */
    int codec;          /* HB worker object id of codec */
    int codec_param;    /* param for codec (usually ffmpeg codec id) */
    const char* name;   /* description of type */
} stream2codec_t;

#define st(id, kind, codec, codec_param, name) \
 [id] = { kind, codec, codec_param, name }

static const stream2codec_t st2codec[256] = {
    st(0x00, U, 0,                0,                      NULL),
    st(0x01, V, WORK_DECAVCODECV, AV_CODEC_ID_MPEG1VIDEO, "MPEG1"),
    st(0x02, V, WORK_DECAVCODECV, AV_CODEC_ID_MPEG2VIDEO, "MPEG2"),
    st(0x03, A, HB_ACODEC_MP2,    AV_CODEC_ID_MP2,        "MPEG1"),
    st(0x04, A, HB_ACODEC_MP2,    AV_CODEC_ID_MP2,        "MPEG2"),
    st(0x05, N, 0,                0,                      "ISO 13818-1 private section"),
    st(0x06, U, 0,                0,                      "ISO 13818-1 PES private data"),
    st(0x07, N, 0,                0,                      "ISO 13522 MHEG"),
    st(0x08, N, 0,                0,                      "ISO 13818-1 DSM-CC"),
    st(0x09, N, 0,                0,                      "ISO 13818-1 auxiliary"),
    st(0x0a, N, 0,                0,                      "ISO 13818-6 encap"),
    st(0x0b, N, 0,                0,                      "ISO 13818-6 DSM-CC U-N msgs"),
    st(0x0c, N, 0,                0,                      "ISO 13818-6 Stream descriptors"),
    st(0x0d, N, 0,                0,                      "ISO 13818-6 Sections"),
    st(0x0e, N, 0,                0,                      "ISO 13818-1 auxiliary"),
    st(0x0f, A, HB_ACODEC_FFAAC,  AV_CODEC_ID_AAC,        "AAC"),
    st(0x10, V, WORK_DECAVCODECV, AV_CODEC_ID_MPEG4,      "MPEG4"),
    st(0x11, A, HB_ACODEC_FFMPEG, AV_CODEC_ID_AAC_LATM,   "LATM AAC"),
    st(0x12, U, 0,                0,                      "MPEG4 generic"),

    st(0x14, N, 0,                0,                      "ISO 13818-6 DSM-CC download"),

    st(0x1b, V, WORK_DECAVCODECV, AV_CODEC_ID_H264,       "H.264"),

    st(0x80, U, HB_ACODEC_FFMPEG, AV_CODEC_ID_PCM_BLURAY, "Digicipher II Video"),
    st(0x81, A, HB_ACODEC_AC3,    AV_CODEC_ID_AC3,        "AC3"),
    st(0x82, A, HB_ACODEC_DCA,    AV_CODEC_ID_DTS,        "DTS"),
    // 0x83 can be LPCM or BD TrueHD.  Set to 'unknown' till we know more.
    st(0x83, U, HB_ACODEC_LPCM,   0,                      "LPCM"),
    // BD E-AC3 Primary audio
    st(0x84, U, 0,                0,                      "SDDS"),
    st(0x85, U, 0,                0,                      "ATSC Program ID"),
    // 0x86 can be BD DTS-HD/DTS. Set to 'unknown' till we know more.
    st(0x86, U, HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,        "DTS-HD MA"),
    st(0x87, A, HB_ACODEC_FFEAC3, AV_CODEC_ID_EAC3,       "E-AC3"),

    st(0x8a, A, HB_ACODEC_DCA,    AV_CODEC_ID_DTS,        "DTS"),

    st(0x90, S, WORK_DECAVSUB,    AV_CODEC_ID_HDMV_PGS_SUBTITLE, "PGS Subtitle"),
    // 0x91 can be AC3 or BD Interactive Graphics Stream.
    st(0x91, U, 0,                0,                      "AC3/IGS"),
    st(0x92, N, 0,                0,                      "Subtitle"),

    st(0x94, U, 0,                0,                      "SDDS"),
    st(0xa0, V, 0,                0,                      "MSCODEC"),
    // BD E-AC3 Secondary audio
    st(0xa1, U, 0,                0,                      "E-AC3"),
    // BD DTS-HD Secondary audio
    st(0xa2, U, HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,        "DTS-HD LBR"),

    st(0xea, V, WORK_DECAVCODECV, AV_CODEC_ID_VC1,        "VC-1"),
};
#undef st

typedef enum {
    hb_stream_type_unknown = 0,
    transport,
    program,
    ffmpeg
} hb_stream_type_t;

#define MAX_PS_PROBE_SIZE (32*1024*1024)
#define kMaxNumberPMTStreams 32

typedef struct
{
    uint8_t has_stream_id_ext;
    uint8_t stream_id;
    uint8_t stream_id_ext;
    uint8_t bd_substream_id;
    int64_t pts;
    int64_t dts;
    int64_t scr;
    int     header_len;
    int     packet_len;
    int     stuffing_len;
} hb_pes_info_t;

typedef struct {
    hb_buffer_t     * buf;
    hb_pes_info_t     pes_info;
    int8_t            pes_info_valid;
    int               packet_len;
    int               packet_offset;
    int8_t            skipbad;
    int8_t            continuity;
    uint8_t           pkt_summary[8];
    int               pid;
    uint8_t           is_pcr;
    int               pes_list;
    int               start;
} hb_ts_stream_t;

typedef struct {
    int      map_idx;
    int      stream_id;
    uint8_t  stream_id_ext;
    uint8_t  stream_type;
    kind_t   stream_kind;
    int      lang_code;
    uint32_t format_id;
#define TS_FORMAT_ID_AC3 (('A' << 24) | ('C' << 16) | ('-' << 8) | '3')
    int      codec;         // HB worker object id of codec
    int      codec_param;   // param for codec (usually ffmpeg codec id)
    char     codec_name[80];
    int      next;          // next pointer for list
                            // hb_ts_stream_t points to a list of
                            // hb_pes_stream_t
    hb_buffer_t * probe_buf;
    int           probe_next_size;
    int           probe_count;
    uint8_t *     extradata;
    int           extradata_size;
} hb_pes_stream_t;

struct hb_stream_s
{
    hb_handle_t * h;

    int     scan;
    int     frames;             /* video frames so far */
    int     errors;             /* total errors so far */
    int     last_error_frame;   /* frame # at last error message */
    int     last_error_count;   /* # errors at last error message */
    int     packetsize;         /* Transport Stream packet size */

    int     need_keyframe;      // non-zero if want to start at a keyframe

    int      chapter;           /* Chapter that we are currently in */
    int64_t  chapter_end;       /* HB time that the current chapter ends */


    struct
    {
        int     discontinuity;
        uint8_t found_pcr;      // non-zero if we've found at least one pcr
        int64_t pcr;            // most recent input pcr
        int64_t last_timestamp; // used for discontinuity detection when
                                // there are no PCRs

        uint8_t *packet;        // buffer for one TS packet
        hb_ts_stream_t *list;
        int count;
        int alloc;
    } ts;

    struct
    {
        uint8_t found_scr;      // non-zero if we've found at least one scr
        int64_t scr;            // most recent input scr
        hb_pes_stream_t *list;
        int count;
        int alloc;
    } pes;

    /*
     * Stuff before this point is dynamic state updated as we read the
     * stream. Stuff after this point is stream description state that
     * we learn during the initial scan but cache so it can be
     * reused during the conversion read.
     */
    uint8_t has_IDRs;           // # IDRs found during duration scan
    uint8_t ts_flags;           // stream characteristics:
#define         TS_HAS_PCR  (1 << 0)    // at least one PCR seen
#define         TS_HAS_RAP  (1 << 1)    // Random Access Point bit seen
#define         TS_HAS_RSEI (1 << 2)    // "Restart point" SEI seen

    char    *path;
    FILE    *file_handle;
    hb_stream_type_t hb_stream_type;
    hb_title_t *title;

    AVFormatContext *ffmpeg_ic;
    AVPacket *ffmpeg_pkt;
    uint8_t ffmpeg_video_id;

    uint32_t reg_desc;          // 4 byte registration code that identifies
                                // stream semantics

    struct
    {
        unsigned short program_number;
        unsigned short program_map_PID;
    } pat_info[kMaxNumberPMTStreams];
    int     ts_number_pat_entries;

    struct
    {
        int reading;
        unsigned char *tablebuf;
        unsigned int tablepos;
        unsigned char current_continuity_counter;

        unsigned int PCR_PID;
    } pmt_info;
};

typedef struct {
    uint8_t *buf;
    uint32_t val;
    int pos;
    int size;
} bitbuf_t;


/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle);
static off_t align_to_next_packet(hb_stream_t *stream);
static int64_t pes_timestamp( const uint8_t *pes );

static int hb_ts_stream_init(hb_stream_t *stream);
static hb_buffer_t * hb_ts_stream_decode(hb_stream_t *stream);
static void hb_init_audio_list(hb_stream_t *stream, hb_title_t *title);
static void hb_init_subtitle_list(hb_stream_t *stream, hb_title_t *title);
static int hb_ts_stream_find_pids(hb_stream_t *stream);

static void hb_ps_stream_init(hb_stream_t *stream);
static hb_buffer_t * hb_ps_stream_decode(hb_stream_t *stream);
static void hb_ps_stream_find_streams(hb_stream_t *stream);
static int hb_ps_read_packet( hb_stream_t * stream, hb_buffer_t *b );
static int update_ps_streams( hb_stream_t * stream, int stream_id, int stream_id_ext, int stream_type, int in_kind );
static int update_ts_streams( hb_stream_t * stream, int pid, int stream_id_ext, int stream_type, int in_kind, int *pes_idx );
static void update_pes_kind( hb_stream_t * stream, int idx );

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title, int scan );
static void ffmpeg_close( hb_stream_t *d );
static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream, hb_title_t *title );
hb_buffer_t *hb_ffmpeg_read( hb_stream_t *stream );
static int ffmpeg_seek( hb_stream_t *stream, float frac );
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts );
static inline unsigned int bits_get(bitbuf_t *bb, int bits);
static inline void bits_init(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear);
static inline unsigned int bits_peek(bitbuf_t *bb, int bits);
static void pes_add_audio_to_title(hb_stream_t *s, int i, hb_title_t *t, int sort);
static int hb_parse_ps( hb_stream_t *stream, uint8_t *buf, int len, hb_pes_info_t *pes_info );
static void hb_ts_resolve_pid_types(hb_stream_t *stream);
static void hb_ps_resolve_stream_types(hb_stream_t *stream);
void hb_ts_stream_reset(hb_stream_t *stream);
void hb_ps_stream_reset(hb_stream_t *stream);

/*
 * logging routines.
 * these frontend hb_log because transport streams can have a lot of errors
 * so we want to rate limit messages. this routine limits the number of
 * messages to at most one per minute of video. other errors that occur
 * during the minute are counted & the count is output with the next
 * error msg we print.
 */
static void ts_warn_helper( hb_stream_t *stream, char *log, va_list args )
{
    // limit error printing to at most one per minute of video (at 30fps)
    ++stream->errors;
    if ( stream->frames - stream->last_error_frame >= 30*60 )
    {
        char msg[256];

        vsnprintf( msg, sizeof(msg), log, args );

        if ( stream->errors - stream->last_error_count < 10 )
        {
            hb_log( "stream: error near frame %d: %s", stream->frames, msg );
        }
        else
        {
            int Edelta = stream->errors - stream->last_error_count;
            double Epcnt = (double)Edelta * 100. /
                            (stream->frames - stream->last_error_frame);
            hb_log( "stream: %d new errors (%.0f%%) up to frame %d: %s",
                    Edelta, Epcnt, stream->frames, msg );
        }
        stream->last_error_frame = stream->frames;
        stream->last_error_count = stream->errors;
    }
}

static void ts_warn( hb_stream_t*, char*, ... ) HB_WPRINTF(2,3);
static void ts_err( hb_stream_t*, int, char*, ... ) HB_WPRINTF(3,4);

static void ts_warn( hb_stream_t *stream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );
}

static int get_id(hb_pes_stream_t *pes)
{
    return ( pes->stream_id_ext << 16 ) + pes->stream_id;
}

static int index_of_id(hb_stream_t *stream, int id)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == get_id( &stream->pes.list[i] ) )
            return i;
    }

    return -1;
}

static int index_of_pid(hb_stream_t *stream, int pid)
{
    int i;

    for ( i = 0; i < stream->ts.count; ++i )
    {
        if ( pid == stream->ts.list[i].pid )
        {
            return i;
        }
    }

    return -1;
}

static int index_of_ps_stream(hb_stream_t *stream, int id, int sid)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == stream->pes.list[i].stream_id &&
             sid == stream->pes.list[i].stream_id_ext )
        {
            return i;
        }
    }
    // If there is no match on the stream_id_ext, try matching
    // on only the stream_id.
    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == stream->pes.list[i].stream_id &&
             0 == stream->pes.list[i].stream_id_ext )
        {
            return i;
        }
    }

    return -1;
}

static kind_t ts_stream_kind( hb_stream_t * stream, int idx )
{
    if ( stream->ts.list[idx].pes_list != -1 )
    {
        // Returns kind for the first pes substream in the pes list
        // All substreams in a TS stream are the same kind.
        return stream->pes.list[stream->ts.list[idx].pes_list].stream_kind;
    }
    else
    {
        return U;
    }
}

static kind_t ts_stream_type( hb_stream_t * stream, int idx )
{
    if ( stream->ts.list[idx].pes_list != -1 )
    {
        // Returns stream type for the first pes substream in the pes list
        // All substreams in a TS stream are the same stream type.
        return stream->pes.list[stream->ts.list[idx].pes_list].stream_type;
    }
    else
    {
        return 0x00;
    }
}

static int pes_index_of_video(hb_stream_t *stream)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
        if ( V == stream->pes.list[i].stream_kind )
            return i;

    return -1;
}

static int ts_index_of_video(hb_stream_t *stream)
{
    int i;

    for ( i = 0; i < stream->ts.count; ++i )
        if ( V == ts_stream_kind( stream, i ) )
            return i;

    return -1;
}

static void ts_err( hb_stream_t *stream, int curstream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );

    if (curstream >= 0)
    {
        stream->ts.list[curstream].skipbad = 1;
        stream->ts.list[curstream].continuity = -1;
    }
}

static int check_ps_sync(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack header in the
    // first four bytes.
    return (buf[0] == 0x00) && (buf[1] == 0x00) &&
           (buf[2] == 0x01) && (buf[3] == 0xba);
}

static int check_ps_sc(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack followed by a
    // some other start code. If we've already verified the pack, this skip
    // it and checks for a start code prefix.
    int pos;
    int mark = buf[4] >> 4;
    if ( mark == 0x02 )
    {
        // Check other marker bits to make it less likely
        // that we are being spoofed.
        if( ( buf[4] & 0xf1 ) != 0x21 ||
            ( buf[6] & 0x01 ) != 0x01 ||
            ( buf[8] & 0x01 ) != 0x01 ||
            ( buf[9] & 0x80 ) != 0x80 ||
            ( buf[11] & 0x01 ) != 0x01 )
        {
            return 0;
        }
        // mpeg-1 pack header
        pos = 12;   // skip over the PACK
    }
    else
    {
        // Check other marker bits to make it less likely
        // that we are being spoofed.
        if( ( buf[4] & 0xC4 ) != 0x44 ||
            ( buf[6] & 0x04 ) != 0x04 ||
            ( buf[8] & 0x04 ) != 0x04 ||
            ( buf[9] & 0x01 ) != 0x01 ||
            ( buf[12] & 0x03 ) != 0x03 )
        {
            return 0;
        }
        // mpeg-2 pack header
        pos = 14 + ( buf[13] & 0x7 );   // skip over the PACK
    }
    return (buf[pos+0] == 0x00) && (buf[pos+1] == 0x00) && (buf[pos+2] == 0x01);
}

static int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte & a legal adaptation ctrl
    return (buf[0] == 0x47) && (((buf[3] & 0x30) >> 4) > 0);
}

static int have_ts_sync(const uint8_t *buf, int psize, int count)
{
    int ii;
    for ( ii = 0; ii < count; ii++ )
    {
        if ( !check_ts_sync(&buf[ii*psize]) )
            return 0;
    }
    return 1;
}

static int hb_stream_check_for_ts(const uint8_t *buf)
{
    // transport streams should have a sync byte every 188 bytes.
    // search the first 8KB of buf looking for at least 8 consecutive
    // correctly located sync patterns.
    int offset = 0;
    int count = 16;

    for ( offset = 0; offset < 8*1024-count*188; ++offset )
    {
        if ( have_ts_sync( &buf[offset], 188, count) )
            return 188 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 192, count) )
            return 192 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 204, count) )
            return 204 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 208, count) )
            return 208 | (offset << 8);
    }
    return 0;
}

static int hb_stream_check_for_ps(hb_stream_t *stream)
{
    uint8_t buf[2048*4];
    uint8_t sc_buf[4];
    int pos = 0;

    fseek(stream->file_handle, 0, SEEK_SET);

    // program streams should start with a PACK then some other mpeg start
    // code (usually a SYS but that might be missing if we only have a clip).
    while (pos < 512 * 1024)
    {
        int offset;

        if ( fread(buf, 1, sizeof(buf), stream->file_handle) != sizeof(buf) )
            return 0;

        for ( offset = 0; offset < 8*1024-27; ++offset )
        {
            if ( check_ps_sync( &buf[offset] ) && check_ps_sc( &buf[offset] ) )
            {
                int pes_offset, prev, data_len;
                uint8_t sid;
                uint8_t *b = buf+offset;

                // Skip the pack header
                int mark = buf[4] >> 4;
                if ( mark == 0x02 )
                {
                    // mpeg-1 pack header
                    pes_offset = 12;
                }
                else
                {
                    // mpeg-2 pack header
                    pes_offset = 14 + ( buf[13] & 0x7 );
                }

                b +=  pes_offset;
                // Get the next stream id
                sid = b[3];
                data_len = (b[4] << 8) + b[5];
                if ( data_len && sid > 0xba && sid < 0xf9 )
                {
                    prev = ftell( stream->file_handle );
                    pos = prev - ( sizeof(buf) - offset );
                    pos += pes_offset + 6 + data_len;
                    fseek( stream->file_handle, pos, SEEK_SET );
                    if ( fread(sc_buf, 1, 4, stream->file_handle) != 4 )
                        return 0;
                    if (sc_buf[0] == 0x00 && sc_buf[1] == 0x00 &&
                        sc_buf[2] == 0x01)
                    {
                        return 1;
                    }
                    fseek( stream->file_handle, prev, SEEK_SET );
                }
            }
        }
        fseek( stream->file_handle, -27, SEEK_CUR );
        pos = ftell( stream->file_handle );
    }
    return 0;
}

static int hb_stream_get_type(hb_stream_t *stream)
{
    uint8_t buf[2048*4];

    if ( fread(buf, 1, sizeof(buf), stream->file_handle) == sizeof(buf) )
    {
        int psize;
        if ( ( psize = hb_stream_check_for_ts(buf) ) != 0 )
        {
            int offset = psize >> 8;
            psize &= 0xff;
            hb_log("file is MPEG Transport Stream with %d byte packets"
                   " offset %d bytes", psize, offset);
            stream->packetsize = psize;
            stream->hb_stream_type = transport;
            if (hb_ts_stream_init(stream) == 0)
                return 1;
        }
        else if ( hb_stream_check_for_ps(stream) != 0 )
        {
            hb_log("file is MPEG Program Stream");
            stream->hb_stream_type = program;
            hb_ps_stream_init(stream);
            // We default to mpeg codec for ps streams if no
            // video found in program stream map
            return 1;
        }
    }
    return 0;
}

static void hb_stream_delete_dynamic( hb_stream_t *d )
{
    if( d->file_handle )
    {
        fclose( d->file_handle );
        d->file_handle = NULL;
    }

    int i=0;

    if ( d->ts.packet )
    {
        free( d->ts.packet );
        d->ts.packet = NULL;
    }
    if ( d->ts.list )
    {
        for (i = 0; i < d->ts.count; i++)
        {
            if (d->ts.list[i].buf)
            {
                hb_buffer_close(&(d->ts.list[i].buf));
                d->ts.list[i].buf = NULL;
            }
        }
    }
    if ( d->pes.list )
    {
        for (i = 0; i < d->pes.count; i++)
        {
            if (d->pes.list[i].extradata)
            {
                free(d->pes.list[i].extradata);
            }
        }
    }
}

static void hb_stream_delete( hb_stream_t *d )
{
    hb_stream_delete_dynamic( d );
    free( d->ts.list );
    free( d->pes.list );
    free( d->path );
    free( d );
}

static int audio_inactive( hb_stream_t *stream, int id, int stream_id_ext )
{
    if ( id < 0 )
    {
        // PID declared inactive by hb_stream_title_scan
        return 1;
    }
    if ( id == stream->pmt_info.PCR_PID )
    {
        // PCR PID is always active
        return 0;
    }

    int i;
    for ( i = 0; i < hb_list_count( stream->title->list_audio ); ++i )
    {
        hb_audio_t *audio = hb_list_item( stream->title->list_audio, i );
        if ( audio->id == ((stream_id_ext << 16) | id) )
        {
            return 0;
        }
    }
    return 1;
}

/* when the file was first opened we made entries for all the audio elementary
 * streams we found in it. Streams that were later found during the preview scan
 * now have an audio codec, type, rate, etc., associated with them. At the end
 * of the scan we delete all the audio entries that weren't found by the scan
 * or don't have a format we support. This routine deletes audio entry 'indx'
 * by setting its PID to an invalid value so no packet will match it. (We can't
 * move any of the entries since the index of the entry is used as the id
 * of the media stream for HB. */
static void hb_stream_delete_ts_entry(hb_stream_t *stream, int indx)
{
    if ( stream->ts.list[indx].pid > 0 )
    {
        stream->ts.list[indx].pid = -stream->ts.list[indx].pid;
    }
}

static int hb_stream_try_delete_ts_entry(hb_stream_t *stream, int indx)
{
    int ii;

    if ( stream->ts.list[indx].pid < 0 )
        return 1;

    for ( ii = stream->ts.list[indx].pes_list; ii != -1;
          ii = stream->pes.list[ii].next )
    {
        if ( stream->pes.list[ii].stream_id >= 0 )
            return 0;
    }
    stream->ts.list[indx].pid = -stream->ts.list[indx].pid;
    return 1;
}

static void hb_stream_delete_ps_entry(hb_stream_t *stream, int indx)
{
    if ( stream->pes.list[indx].stream_id > 0 )
    {
        stream->pes.list[indx].stream_id = -stream->pes.list[indx].stream_id;
    }
}

static void prune_streams(hb_stream_t *d)
{
    if ( d->hb_stream_type == transport )
    {
        int ii, jj;
        for ( ii = 0; ii < d->ts.count; ii++)
        {
            // If probing didn't find audio or video, and the pid
            // is not the PCR, remove the track
            if ( ts_stream_kind ( d, ii ) == U &&
                 !d->ts.list[ii].is_pcr )
            {
                hb_stream_delete_ts_entry(d, ii);
                continue;
            }

            if ( ts_stream_kind ( d, ii ) == A )
            {
                for ( jj = d->ts.list[ii].pes_list; jj != -1;
                      jj = d->pes.list[jj].next )
                {
                    if ( audio_inactive( d, d->pes.list[jj].stream_id,
                                         d->pes.list[jj].stream_id_ext ) )
                    {
                        hb_stream_delete_ps_entry(d, jj);
                    }
                }
                if ( !d->ts.list[ii].is_pcr &&
                     hb_stream_try_delete_ts_entry(d, ii) )
                {
                    continue;
                }
            }
        }
        // reset to beginning of file and reset some stream
        // state information
        hb_stream_seek( d, 0. );
    }
    else if ( d->hb_stream_type == program )
    {
        int ii;
        for ( ii = 0; ii < d->pes.count; ii++)
        {
            // If probing didn't find audio or video, remove the track
            if ( d->pes.list[ii].stream_kind == U )
            {
                hb_stream_delete_ps_entry(d, ii);
            }

            if ( d->pes.list[ii].stream_kind == A &&
                 audio_inactive( d, d->pes.list[ii].stream_id,
                                 d->pes.list[ii].stream_id_ext ) )
            {
                // this PID isn't wanted (we don't have a codec for it
                // or scan didn't find audio parameters)
                hb_stream_delete_ps_entry(d, ii);
                continue;
            }
        }
        // reset to beginning of file and reset some stream
        // state information
        hb_stream_seek( d, 0. );
    }
}

/***********************************************************************
 * hb_stream_open
 ***********************************************************************
 *
 **********************************************************************/
hb_stream_t *
hb_stream_open(hb_handle_t *h, const char *path, hb_title_t *title, int scan)
{
    if (title == NULL)
    {
        hb_log("hb_stream_open: title is null");
        return NULL;
    }

    FILE *f = hb_fopen(path, "rb");
    if ( f == NULL )
    {
        hb_log( "hb_stream_open: open %s failed", path );
        return NULL;
    }

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        fclose( f );
        hb_log( "hb_stream_open: can't allocate space for %s stream state", path );
        return NULL;
    }

    if (!(title->flags & HBTF_NO_IDR))
    {
        d->has_IDRs = 1;
    }

    /*
     * If it's something we can deal with (MPEG2 PS or TS) return a stream
     * reference structure & null otherwise.
     */
    d->h = h;
    d->file_handle = f;
    d->title = title;
    d->scan = scan;
    d->path = strdup( path );
    if (d->path != NULL )
    {
        if (hb_stream_get_type( d ) != 0)
        {
            if( !scan )
            {
                prune_streams( d );
            }
            // reset to beginning of file and reset some stream
            // state information
            hb_stream_seek( d, 0. );
            return d;
        }
        fclose( d->file_handle );
        d->file_handle = NULL;
        if ( ffmpeg_open( d, title, scan ) )
        {
            return d;
        }
    }
    if ( d->file_handle )
    {
        fclose( d->file_handle );
    }
    if (d->path)
    {
        free( d->path );
    }
    hb_log( "hb_stream_open: open %s failed", path );
    free( d );
    return NULL;
}

static int new_pid( hb_stream_t * stream )
{
    int num = stream->ts.alloc;

    if ( stream->ts.count == stream->ts.alloc )
    {
        num = stream->ts.alloc ? stream->ts.alloc * 2 : 32;
        stream->ts.list = realloc( stream->ts.list,
                                   sizeof( hb_ts_stream_t ) * num );
    }
    int ii;
    for ( ii = stream->ts.alloc; ii < num; ii++ )
    {
        memset(&stream->ts.list[ii], 0, sizeof( hb_ts_stream_t ));
        stream->ts.list[ii].continuity = -1;
        stream->ts.list[ii].pid = -1;
        stream->ts.list[ii].pes_list = -1;
    }
    stream->ts.alloc = num;
    num = stream->ts.count;
    stream->ts.count++;

    return num;
}

static int new_pes( hb_stream_t * stream )
{
    int num = stream->pes.alloc;

    if ( stream->pes.count == stream->pes.alloc )
    {
        num = stream->pes.alloc ? stream->pes.alloc * 2 : 32;
        stream->pes.list = realloc( stream->pes.list,
                                    sizeof( hb_pes_stream_t ) * num );
    }
    int ii;
    for ( ii = stream->pes.alloc; ii < num; ii++ )
    {
        memset(&stream->pes.list[ii], 0, sizeof( hb_pes_stream_t ));
        stream->pes.list[ii].stream_id = -1;
        stream->pes.list[ii].next = -1;
    }
    stream->pes.alloc = num;
    num = stream->pes.count;
    stream->pes.count++;

    return num;
}

hb_stream_t * hb_bd_stream_open( hb_handle_t *h, hb_title_t *title )
{
    int ii;

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        hb_error( "hb_bd_stream_open: can't allocate space for stream state" );
        return NULL;
    }

    d->h = h;
    d->file_handle = NULL;
    d->title = title;
    d->path = NULL;
    d->ts.packet = NULL;

    int pid = title->video_id;
    int stream_type = title->video_stream_type;
    update_ts_streams( d, pid, 0, stream_type, V, NULL );

    hb_audio_t * audio;
    for ( ii = 0; ( audio = hb_list_item( title->list_audio, ii ) ); ++ii )
    {
        int stream_id_ext = audio->config.in.substream_type;
        pid = audio->id & 0xFFFF;
        stream_type = audio->config.in.stream_type;

        update_ts_streams( d, pid, stream_id_ext, stream_type, A, NULL );
    }

    hb_subtitle_t * subtitle;
    for ( ii = 0; ( subtitle = hb_list_item( title->list_subtitle, ii ) ); ++ii )
    {
        // If the subtitle track is CC embedded in the video stream, then
        // it does not have an independent pid.  In this case, we assigned
        // the subtitle->id to HB_SUBTITLE_EMBEDDED_CC_TAG.
        if (subtitle->id != HB_SUBTITLE_EMBEDDED_CC_TAG)
        {
            pid = subtitle->id & 0xFFFF;
            stream_type = subtitle->stream_type;

            update_ts_streams( d, pid, 0, stream_type, S, NULL );
        }
    }

    // We don't need to wait for a PCR when scanning. In fact, it
    // trips us up on the first preview of every title since we would
    // have to read quite a lot of data before finding the PCR.
    if ( title->flags & HBTF_SCAN_COMPLETE )
    {
        /* BD has PCRs, but the BD index always points to a packet
         * after a PCR packet, so we will not see the initial PCR
         * after any seek.  So don't set the flag that causes us
         * to drop packets till we see a PCR. */
        //d->ts_flags = TS_HAS_RAP | TS_HAS_PCR;

        // BD PCR PID is specified to always be 0x1001
        update_ts_streams( d, 0x1001, 0, -1, P, NULL );
    }

    d->packetsize = 192;
    d->hb_stream_type = transport;

    for ( ii = 0; ii < d->ts.count; ii++ )
    {
        d->ts.list[ii].buf = hb_buffer_init(d->packetsize);
        d->ts.list[ii].buf->size = 0;
    }

    return d;
}

/***********************************************************************
 * hb_stream_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_stream_close( hb_stream_t ** _d )
{
    hb_stream_t *stream = * _d;

    if (stream == NULL)
    {
        return;
    }

    if ( stream->hb_stream_type == ffmpeg )
    {
        ffmpeg_close( stream );
        hb_stream_delete( stream );
        *_d = NULL;
        return;
    }

    if ( stream->frames )
    {
        hb_log( "stream: %d good frames, %d errors (%.0f%%)", stream->frames,
                stream->errors, (double)stream->errors * 100. /
                (double)stream->frames );
    }

    hb_stream_delete( stream );
    *_d = NULL;
}

/***********************************************************************
 * hb_ps_stream_title_scan
 ***********************************************************************
 *
 **********************************************************************/
hb_title_t * hb_stream_title_scan(hb_stream_t *stream, hb_title_t * title)
{
    if ( stream->hb_stream_type == ffmpeg )
        return ffmpeg_title_scan( stream, title );

    // 'Barebones Title'
    title->type = HB_STREAM_TYPE;

    // Copy part of the stream path to the title name
    char * name = stream->path;
    char * sep  = hb_strr_dir_sep(stream->path);
    if (sep)
        name = sep + 1;
    title->name = strdup(name);
    char *dot_term = strrchr(title->name, '.');
    if (dot_term)
        *dot_term = '\0';

    // Figure out how many audio streams we really have:
    // - For transport streams, for each PID listed in the PMT (whether
    //   or not it was an audio stream type) read the bitstream until we
    //   find an packet from that PID containing a PES header and see if
    //   the elementary stream is an audio type.
    // - For program streams read the first 4MB and take every unique
    //   audio stream we find.
    hb_init_audio_list(stream, title);
    hb_init_subtitle_list(stream, title);

    // set the video id, codec & muxer
    int idx = pes_index_of_video( stream );
    if ( idx < 0 )
    {
        hb_title_close( &title );
        return NULL;
    }

    title->video_id = get_id( &stream->pes.list[idx] );
    title->video_codec = stream->pes.list[idx].codec;
    title->video_codec_param = stream->pes.list[idx].codec_param;
    title->video_timebase.num = 1;
    title->video_timebase.den = 90000;

    if (stream->hb_stream_type == transport)
    {
        title->demuxer = HB_TS_DEMUXER;

        // make sure we're grabbing the PCR PID
        update_ts_streams( stream, stream->pmt_info.PCR_PID, 0, -1, P, NULL );
    }
    else
    {
        title->demuxer = HB_PS_DEMUXER;
    }

    // IDRs will be search for in hb_stream_duration
    stream->has_IDRs = 0;
    hb_stream_duration(stream, title);

    // One Chapter
    hb_chapter_t * chapter;
    chapter = calloc( sizeof( hb_chapter_t ), 1 );
    hb_chapter_set_title( chapter, "Chapter 1" );
    chapter->index = 1;
    chapter->duration = title->duration;
    chapter->hours = title->hours;
    chapter->minutes = title->minutes;
    chapter->seconds = title->seconds;
    hb_list_add( title->list_chapter, chapter );

    if ( stream->has_IDRs < 1 )
    {
        hb_log( "stream doesn't seem to have video IDR frames" );
        title->flags |= HBTF_NO_IDR;
    }

    if ( stream->hb_stream_type == transport &&
         ( stream->ts_flags & TS_HAS_PCR ) == 0 )
    {
        hb_log( "transport stream missing PCRs - using video DTS instead" );
    }

    // Height, width, rate and aspect ratio information is filled in
    // when the previews are built
    return title;
}

/*
 * read the next transport stream packet from 'stream'. Return NULL if
 * we hit eof & a pointer to the sync byte otherwise.
 */
static const uint8_t *next_packet( hb_stream_t *stream )
{
    uint8_t *buf = stream->ts.packet + stream->packetsize - 188;

    while ( 1 )
    {
        if ( fread(stream->ts.packet, 1, stream->packetsize, stream->file_handle) !=
             stream->packetsize )
        {
            int err;
            if ((err = ferror(stream->file_handle)) != 0)
            {
                hb_error("next_packet: error (%d)", err);
                hb_set_work_error(stream->h, HB_ERROR_READ);
            }
            return NULL;
        }
        if (buf[0] == 0x47)
        {
            return buf;
        }
        // lost sync - back up to where we started then try to re-establish.
        off_t pos = ftello(stream->file_handle) - stream->packetsize;
        off_t pos2 = align_to_next_packet(stream);
        if ( pos2 == 0 )
        {
            hb_log( "next_packet: eof while re-establishing sync @ %"PRId64, pos );
            return NULL;
        }
        ts_warn( stream, "next_packet: sync lost @ %"PRId64", regained after %"PRId64" bytes",
                 pos, pos2 );
    }
}

/*
 * skip to the start of the next PACK header in program stream src_stream.
 */
static void skip_to_next_pack( hb_stream_t *src_stream )
{
    // scan forward until we find the start of the next pack
    uint32_t strt_code = -1;
    int c;

    flockfile( src_stream->file_handle );
    while ( ( c = getc_unlocked( src_stream->file_handle ) ) != EOF )
    {
        strt_code = ( strt_code << 8 ) | c;
        if ( strt_code == 0x000001ba )
            // we found the start of the next pack
            break;
    }
    funlockfile( src_stream->file_handle );

    // if we didn't terminate on an eof back up so the next read
    // starts on the pack boundary.
    if ( c != EOF )
    {
        fseeko( src_stream->file_handle, -4, SEEK_CUR );
    }
}

static void CreateDecodedNAL( uint8_t **dst, int *dst_len,
                              const uint8_t *src, int src_len )
{
    const uint8_t *end = &src[src_len];
    uint8_t *d = malloc( src_len );

    *dst = d;

    if( d )
    {
        while( src < end )
        {
            if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
                src[2] == 0x01 )
            {
                // Next start code found
                break;
            }
            if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
                src[2] == 0x03 )
            {
                *d++ = 0x00;
                *d++ = 0x00;

                src += 3;
                continue;
            }
            *d++ = *src++;
        }
    }
    *dst_len = d - *dst;
}

static int isRecoveryPoint( const uint8_t *buf, int len )
{
    uint8_t *nal;
    int nal_len;
    int ii, type, size;
    int recovery_frames = 0;

    CreateDecodedNAL( &nal, &nal_len, buf, len );

    for ( ii = 0; ii+1 < nal_len; )
    {
        type = 0;
        while ( ii+1 < nal_len )
        {
            type += nal[ii++];
            if ( nal[ii-1] != 0xff )
                break;
        }
        size = 0;
        while ( ii+1 < nal_len )
        {
            size += nal[ii++];
            if ( nal[ii-1] != 0xff )
                break;
        }

        if( type == 6 )
        {
            recovery_frames = 1;
            break;
        }
        ii += size;
    }

    free( nal );
    return recovery_frames;
}

static int isIframe( hb_stream_t *stream, const uint8_t *buf, int len )
{
    // For mpeg2: look for a gop start or i-frame picture start
    // for h.264: look for idr nal type or a slice header for an i-frame
    // for vc1:   look for a Sequence header
    int ii;
    uint32_t strid = 0;


    int vid = pes_index_of_video( stream );
    hb_pes_stream_t *pes = &stream->pes.list[vid];
    if ( pes->stream_type <= 2 ||
         pes->codec_param == AV_CODEC_ID_MPEG1VIDEO ||
         pes->codec_param == AV_CODEC_ID_MPEG2VIDEO )
    {
        // This section of the code handles MPEG-1 and MPEG-2 video streams
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code
                uint8_t id = strid;
                switch ( id )
                {
                    case 0xB8: // group_start_code (GOP header)
                    case 0xB3: // sequence_header code
                        return 1;

                    case 0x00: // picture_start_code
                        // picture_header, let's see if it's an I-frame
                        if (ii < len - 3)
                        {
                            // check if picture_coding_type == 1
                            if ((buf[ii+2] & (0x7 << 3)) == (1 << 3))
                            {
                                // found an I-frame picture
                                return 1;
                            }
                        }
                        break;
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( pes->stream_type == 0x1b || pes->codec_param == AV_CODEC_ID_H264 )
    {
        // we have an h.264 stream
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code - remove the ref_idc from the nal type
                uint8_t nal_type = strid & 0x1f;
                if ( nal_type == 0x01 )
                {
                    // Found slice and no recovery point
                    return 0;
                }
                if ( nal_type == 0x05 )
                {
                    // h.264 IDR picture start
                    return 1;
                }
                else if ( nal_type == 0x06 )
                {
                    int off = ii + 1;
                    int recovery_frames = isRecoveryPoint( buf+off, len-off );
                    if ( recovery_frames )
                    {
                        return recovery_frames;
                    }
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( pes->stream_type == 0xea || pes->codec_param == AV_CODEC_ID_VC1 )
    {
        // we have an vc1 stream
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( strid == 0x10f )
            {
                // the ffmpeg vc1 decoder requires a seq hdr code in the first
                // frame.
                return 1;
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( pes->stream_type == 0x10 || pes->codec_param == AV_CODEC_ID_MPEG4 )
    {
        // we have an mpeg4 stream
        for (ii = 0; ii < len-1; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( strid == 0x1b6 )
            {
                if ((buf[ii+1] & 0xC0) == 0)
                    return 1;
            }
        }
        // didn't find an I-frame
        return 0;
    }

    // we don't understand the stream type so just say "yes" otherwise
    // we'll discard all the video.
    return 1;
}

static int ts_isIframe( hb_stream_t *stream, const uint8_t *buf, int adapt_len )
{
    return isIframe( stream, buf + 13 + adapt_len, 188 - ( 13 + adapt_len ) );
}

/*
 * scan the next MB of 'stream' to find the next start packet for
 * the Packetized Elementary Stream associated with TS PID 'pid'.
 */
static const uint8_t *hb_ts_stream_getPEStype(hb_stream_t *stream, uint32_t pid, int *out_adapt_len)
{
    int npack = 300000; // max packets to read

    while (--npack >= 0)
    {
        const uint8_t *buf = next_packet( stream );
        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_getPEStype: EOF while searching for PID 0x%x", pid);
            return 0;
        }

        // while we're reading the stream, check if it has valid PCRs
        // and/or random access points.
        uint32_t pack_pid = ( (buf[1] & 0x1f) << 8 ) | buf[2];
        if ( pack_pid == stream->pmt_info.PCR_PID )
        {
            if ( ( buf[5] & 0x10 ) &&
                 ( ( ( buf[3] & 0x30 ) == 0x20 ) ||
                   ( ( buf[3] & 0x30 ) == 0x30 && buf[4] > 6 ) ) )
            {
                stream->ts_flags |= TS_HAS_PCR;
            }
        }
        if ( buf[5] & 0x40 )
        {
            stream->ts_flags |= TS_HAS_RAP;
        }

        /*
         * The PES header is only in TS packets with 'start' set so we check
         * that first then check for the right PID.
         */
        if ((buf[1] & 0x40) == 0 || pack_pid != pid )
        {
            // not a start packet or not the pid we want
            continue;
        }

        int adapt_len = 0;
        /* skip over the TS hdr to return a pointer to the PES hdr */
        switch (buf[3] & 0x30)
        {
            case 0x00: // illegal
            case 0x20: // fill packet
                continue;

            case 0x30: // adaptation
                adapt_len = buf[4] + 1;
                if (adapt_len > 184)
                {
                    hb_log("hb_ts_stream_getPEStype: invalid adaptation field length %d for PID 0x%x", buf[4], pid);
                    continue;
                }
                break;
        }
        /* PES hdr has to begin with an mpeg start code */
        if (buf[adapt_len+4] == 0x00 && buf[adapt_len+5] == 0x00 && buf[adapt_len+6] == 0x01)
        {
            *out_adapt_len = adapt_len;
            return buf;
        }
    }

    /* didn't find it */
    return 0;
}

static hb_buffer_t * hb_ps_stream_getVideo(
    hb_stream_t *stream,
    hb_pes_info_t *pi)
{
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
    hb_pes_info_t pes_info;
    // how many blocks we read while searching for a video PES header
    int blksleft = 2048;

    while (--blksleft >= 0)
    {
        buf->size = 0;
        int len = hb_ps_read_packet( stream, buf );
        if ( len == 0 )
        {
            // EOF
            break;
        }
        if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
            continue;

        int idx;
        if ( pes_info.stream_id == 0xbd )
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.bd_substream_id );
        }
        else
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.stream_id_ext );
        }
        if ( idx >= 0 && stream->pes.list[idx].stream_kind == V )
        {
            if ( pes_info.pts != AV_NOPTS_VALUE )
            {
                *pi = pes_info;
                return buf;
            }
        }
    }
    hb_buffer_close( &buf );
    return NULL;
}

/***********************************************************************
 * hb_stream_duration
 ***********************************************************************
 *
 * Finding stream duration is difficult.  One issue is that the video file
 * may have chunks from several different program fragments (main feature,
 * commercials, station id, trailers, etc.) all with their own base pts
 * value.  We can't find the piece boundaries without reading the entire
 * file but if we compute a rate based on time stamps from two different
 * pieces the result will be meaningless.  The second issue is that the
 * data rate of compressed video normally varies by 5-10x over the length
 * of the video. This says that we want to compute the rate over relatively
 * long segments to get a representative average but long segments increase
 * the likelihood that we'll cross a piece boundary.
 *
 * What we do is take time stamp samples at several places in the file
 * (currently 16) then compute the average rate (i.e., ticks of video per
 * byte of the file) for all pairs of samples (N^2 rates computed for N
 * samples). Some of those rates will be absurd because the samples came
 * from different segments. Some will be way low or high because the
 * samples came from a low or high motion part of the segment. But given
 * that we're comparing *all* pairs the majority of the computed rates
 * should be near the overall average.  So we median filter the computed
 * rates to pick the most representative value.
 *
 **********************************************************************/
struct pts_pos {
    uint64_t pos;   /* file position of this PTS sample */
    uint64_t pts;   /* PTS from video stream */
};

#define NDURSAMPLES 128

// get one (position, timestamp) sample from a transport or program
// stream.
static struct pts_pos hb_sample_pts(hb_stream_t *stream, uint64_t fpos)
{
    struct pts_pos pp = { 0, 0 };

    if ( stream->hb_stream_type == transport )
    {
        const uint8_t *buf;
        int adapt_len;
        fseeko( stream->file_handle, fpos, SEEK_SET );
        align_to_next_packet( stream );
        int pid = stream->ts.list[ts_index_of_video(stream)].pid;
        buf = hb_ts_stream_getPEStype( stream, pid, &adapt_len );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %"PRIu64, fpos);
            return pp;
        }
        const uint8_t *pes = buf + 4 + adapt_len;
        if ( ( pes[7] >> 7 ) != 1 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %"PRIu64, fpos);
            return pp;
        }
        pp.pts = ((((uint64_t)pes[ 9] >> 1 ) & 7) << 30) |
                 (  (uint64_t)pes[10] << 22)             |
                 ( ((uint64_t)pes[11] >> 1 )      << 15) |
                 (  (uint64_t)pes[12] << 7 )             |
                 (  (uint64_t)pes[13] >> 1 );

        if ( ts_isIframe( stream, buf, adapt_len ) )
        {
            if (  stream->has_IDRs < 255 )
            {
                ++stream->has_IDRs;
            }
        }
        pp.pos = ftello(stream->file_handle);
        if ( !stream->has_IDRs )
        {
            // Scan a little more to see if we will stumble upon one
            int ii;
            for ( ii = 0; ii < 10; ii++ )
            {
                buf = hb_ts_stream_getPEStype( stream, pid, &adapt_len );
                if ( buf == NULL )
                    break;
                if ( ts_isIframe( stream, buf, adapt_len ) )
                {
                    ++stream->has_IDRs;
                    break;
                }
            }
        }
    }
    else
    {
        hb_buffer_t *buf;
        hb_pes_info_t pes_info;

        // round address down to nearest dvd sector start
        fpos &=~ ( HB_DVD_READ_BUFFER_SIZE - 1 );
        fseeko( stream->file_handle, fpos, SEEK_SET );
        if ( stream->hb_stream_type == program )
        {
            skip_to_next_pack( stream );
        }
        buf = hb_ps_stream_getVideo( stream, &pes_info );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %"PRIu64, fpos);
            return pp;
        }
        if ( pes_info.pts < 0 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %"PRIu64, fpos);
            hb_buffer_close( &buf );
            return pp;
        }
        if ( isIframe( stream, buf->data, buf->size ) )
        {
            if (  stream->has_IDRs < 255 )
            {
                ++stream->has_IDRs;
            }
        }
        hb_buffer_close( &buf );
        if ( !stream->has_IDRs )
        {
            // Scan a little more to see if we will stumble upon one
            int ii;
            for ( ii = 0; ii < 10; ii++ )
            {
                buf = hb_ps_stream_getVideo( stream, &pes_info );
                if ( buf == NULL )
                    break;
                if ( isIframe( stream, buf->data, buf->size ) )
                {
                    ++stream->has_IDRs;
                    hb_buffer_close( &buf );
                    break;
                }
                hb_buffer_close( &buf );
            }
        }

        pp.pts = pes_info.pts;
        pp.pos = ftello(stream->file_handle);
    }
    return pp;
}

static int dur_compare( const void *a, const void *b )
{
    const double *aval = a, *bval = b;
    return ( *aval < *bval ? -1 : ( *aval == *bval ? 0 : 1 ) );
}

// given an array of (position, time) samples, compute a max-likelihood
// estimate of the average rate by computing the rate between all pairs
// of samples then taking the median of those rates.
static double compute_stream_rate( struct pts_pos *pp, int n )
{
    int i, j;
    double rates[NDURSAMPLES * NDURSAMPLES / 8];
    double *rp = rates;

    // the following nested loops compute the rates between all pairs.
    *rp = 0;
    for ( i = 0; i < n-1; ++i )
    {
        // Bias the median filter by not including pairs that are "far"
        // from one another. This is to handle cases where the file is
        // made of roughly equal size pieces where a symmetric choice of
        // pairs results in having the same number of intra-piece &
        // inter-piece rate estimates. This would mean that the median
        // could easily fall in the inter-piece part of the data which
        // would give a bogus estimate. The 'ns' index creates an
        // asymmetry that favors locality.
        int ns = i + ( n >> 3 );
        if ( ns > n )
            ns = n;
        for ( j = i+1; j < ns; ++j )
        {
            if ( (uint64_t)(pp[j].pts - pp[i].pts) > 90000LL*3600*6 )
                break;
            if ( pp[j].pts != pp[i].pts && pp[j].pos > pp[i].pos )
            {
                *rp = ((double)( pp[j].pts - pp[i].pts )) /
                      ((double)( pp[j].pos - pp[i].pos ));
                ++rp;
            }
        }
    }
    // now compute and return the median of all the (n*n/2) rates we computed
    // above.
    int nrates = rp - rates;
    qsort( rates, nrates, sizeof (rates[0] ), dur_compare );
    return rates[nrates >> 1];
}

static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle)
{
    struct pts_pos ptspos[NDURSAMPLES];
    struct pts_pos *pp = ptspos;
    int i;

    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    uint64_t fincr = fsize / NDURSAMPLES;
    uint64_t fpos = fincr / 2;
    for ( i = NDURSAMPLES; --i >= 0; fpos += fincr )
    {
        *pp++ = hb_sample_pts(stream, fpos);
    }
    uint64_t dur = compute_stream_rate( ptspos, pp - ptspos ) * (double)fsize;
    inTitle->duration = dur;
    dur /= 90000;
    inTitle->hours    = dur / 3600;
    inTitle->minutes  = ( dur % 3600 ) / 60;
    inTitle->seconds  = dur % 60;

    rewind(stream->file_handle);
}

/***********************************************************************
 * hb_stream_read
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_stream_read( hb_stream_t * src_stream )
{
    if ( src_stream->hb_stream_type == ffmpeg )
    {
        return hb_ffmpeg_read( src_stream );
    }
    if ( src_stream->hb_stream_type == program )
    {
        return hb_ps_stream_decode( src_stream );
    }
    return hb_ts_stream_decode( src_stream );
}

int64_t ffmpeg_initial_timestamp( hb_stream_t * stream )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    if (ic->start_time != AV_NOPTS_VALUE)
        return ic->start_time;
    else
        return 0;
}

int hb_stream_seek_chapter( hb_stream_t * stream, int chapter_num )
{
    if ( !stream || !stream->title ||
         chapter_num > hb_list_count( stream->title->list_chapter ) )
    {
        return 0;
    }

    if ( stream->hb_stream_type != ffmpeg )
    {
        // currently meaningless for transport and program streams
        return 1;
    }

    // TODO: add chapter start time to hb_chapter_t
    // The first chapter does not necessarily start at time 0.
    int64_t        sum_dur = 0;
    hb_chapter_t * chapter = NULL;
    int            ii;
    for (ii = 0; ii < chapter_num - 1; ii++)
    {
        chapter = hb_list_item(stream->title->list_chapter, ii);
        sum_dur += chapter->duration;
    }
    stream->chapter     = chapter_num - 1;
    stream->chapter_end = sum_dur;

    if (chapter != NULL && chapter_num > 1)
    {
        int64_t pos = ((sum_dur * AV_TIME_BASE) / 90000) +
                      ffmpeg_initial_timestamp(stream);

        if (pos > 0)
        {
            hb_deep_log(2,
                        "Seeking to chapter %d: starts %"PRId64", ends %"PRId64
                        ", AV pos %"PRId64,
                        chapter_num, sum_dur, sum_dur + chapter->duration, pos);

            AVStream *st = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id];
            // timebase must be adjusted to match timebase of stream we are
            // using for seeking.
            pos = av_rescale(pos, st->time_base.den,
                             AV_TIME_BASE * (int64_t)st->time_base.num);
            avformat_seek_file(stream->ffmpeg_ic, stream->ffmpeg_video_id, 0,
                               pos, pos, AVSEEK_FLAG_BACKWARD);
        }
    }
    return 1;
}

/***********************************************************************
 * hb_stream_chapter
 ***********************************************************************
 * Return the number of the chapter that we are currently in. We store
 * the chapter number starting from 0, so + 1 for the real chapter num.
 **********************************************************************/
int hb_stream_chapter( hb_stream_t * src_stream )
{
    return( src_stream->chapter );
}

/***********************************************************************
 * hb_stream_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_seek( hb_stream_t * stream, float f )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek( stream, f );
    }
    off_t stream_size, cur_pos, new_pos;
    double pos_ratio = f;
    cur_pos = ftello( stream->file_handle );
    fseeko( stream->file_handle, 0, SEEK_END );
    stream_size = ftello( stream->file_handle );
    new_pos = (off_t) ((double) (stream_size) * pos_ratio);
    new_pos &=~ (HB_DVD_READ_BUFFER_SIZE - 1);

    int r = fseeko( stream->file_handle, new_pos, SEEK_SET );
    if (r == -1)
    {
        fseeko( stream->file_handle, cur_pos, SEEK_SET );
        return 0;
    }

    if ( stream->hb_stream_type == transport )
    {
        // We need to drop the current decoder output and move
        // forwards to the next transport stream packet.
        hb_ts_stream_reset(stream);
        align_to_next_packet(stream);
        if ( !stream->has_IDRs )
        {
            // the stream has no IDRs so don't look for one.
            stream->need_keyframe = 0;
        }
    }
    else if ( stream->hb_stream_type == program )
    {
        hb_ps_stream_reset(stream);
        skip_to_next_pack( stream );
        if ( !stream->has_IDRs )
        {
            // the stream has no IDRs so don't look for one.
            stream->need_keyframe = 0;
        }
    }

    return 1;
}

int hb_stream_seek_ts( hb_stream_t * stream, int64_t ts )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek_ts( stream, ts );
    }
    return -1;
}

static char* strncpyupper( char *dst, const char *src, int len )
{
    int ii;

    for ( ii = 0; ii < len-1 && src[ii]; ii++ )
    {
        dst[ii] = islower(src[ii]) ? toupper(src[ii]) : src[ii];
    }
    dst[ii] = '\0';
    return dst;
}

static const char *stream_type_name2(hb_stream_t *stream, hb_pes_stream_t *pes)
{
    static char codec_name_caps[80];

    if ( stream->reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // Names for streams we know about.
        switch ( pes->stream_type )
        {
            case 0x80:
                return "BD LPCM";

            case 0x83:
                return "TrueHD";

            case 0x84:
                return "E-AC3";

            case 0x85:
                return "DTS-HD HRA";

            case 0x86:
                return "DTS-HD MA";

            default:
                break;
        }
    }
    if ( pes->codec_name[0] != 0 )
    {
        return pes->codec_name;
    }
    if ( st2codec[pes->stream_type].name )
    {
        return st2codec[pes->stream_type].name;
    }
    if ( pes->codec & HB_ACODEC_FF_MASK )
    {
        const AVCodec *codec = avcodec_find_decoder( pes->codec_param );
        if ( codec && codec->name && codec->name[0] )
        {
            strncpyupper( codec_name_caps, codec->name, 80 );
            return codec_name_caps;
        }
    }
    return "Unknown";
}

static void set_audio_description(hb_audio_t *audio, iso639_lang_t *lang)
{
    snprintf( audio->config.lang.simple,
              sizeof( audio->config.lang.simple ), "%s",
              strlen( lang->native_name ) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2,
              sizeof( audio->config.lang.iso639_2 ), "%s", lang->iso639_2 );
}

// Sort specifies the index in the audio list where you would
// like sorted items to begin.
static void pes_add_subtitle_to_title(
    hb_stream_t *stream,
    int         idx,
    hb_title_t  *title,
    int         sort)
{
    hb_pes_stream_t *pes = &stream->pes.list[idx];

    // Sort by id when adding to the list
    // This assures that they are always displayed in the same order
    int id = get_id( pes );
    int i;
    hb_subtitle_t *tmp = NULL;

    int count = hb_list_count( title->list_subtitle );

    // Don't add the same audio twice.  Search for audio.
    for ( i = 0; i < count; i++ )
    {
        tmp = hb_list_item( title->list_subtitle, i );
        if ( id == tmp->id )
            return;
    }

    hb_subtitle_t *subtitle = calloc( sizeof( hb_subtitle_t ), 1 );
    iso639_lang_t * lang;

    subtitle->track        = idx;
    subtitle->id           = id;
    subtitle->timebase.num = 1;
    subtitle->timebase.den = 90000;

    switch (pes->codec)
    {
        case WORK_DECAVSUB:
        {
            switch (pes->codec_param)
            {
                case AV_CODEC_ID_DVB_SUBTITLE:
                    subtitle->source = DVBSUB;
                    subtitle->format = PICTURESUB;
                    subtitle->config.dest = RENDERSUB;
                    if (pes->extradata != NULL)
                    {
                        hb_set_extradata(&subtitle->extradata,
                                         pes->extradata,
                                         pes->extradata_size);
                    }
                    break;
                case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
                    subtitle->source = PGSSUB;
                    subtitle->format = PICTURESUB;
                    subtitle->config.dest = RENDERSUB;
                    break;
                case AV_CODEC_ID_DVD_SUBTITLE:
                    subtitle->source = VOBSUB;
                    subtitle->format = PICTURESUB;
                    subtitle->config.dest = RENDERSUB;
                    break;
                default:
                    // Unrecognized, don't add to list
                    hb_log("unrecognized subtitle!");
                    free( subtitle );
                    return;
            }
        } break;
        default:
            // Unrecognized, don't add to list
            hb_log("unrecognized subtitle!");
            free( subtitle );
            return;
    }

    lang = lang_for_code( pes->lang_code );
    snprintf(subtitle->lang, sizeof( subtitle->lang ), "%s (%s)",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name,
             hb_subsource_name(subtitle->source));
    snprintf(subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
             lang->iso639_2);
    subtitle->reg_desc       = stream->reg_desc;
    subtitle->stream_type    = pes->stream_type;
    subtitle->substream_type = pes->stream_id_ext;
    subtitle->codec          = pes->codec;
    subtitle->codec_param    = pes->codec_param;

    // Create a default palette since vob files do not include the
    // vobsub palette.
    if ( subtitle->source == VOBSUB )
    {
        subtitle->palette[0] = 0x108080;
        subtitle->palette[1] = 0x108080;
        subtitle->palette[2] = 0x108080;
        subtitle->palette[3] = 0xbff000;

        subtitle->palette[4] = 0xbff000;
        subtitle->palette[5] = 0x108080;
        subtitle->palette[6] = 0x108080;
        subtitle->palette[7] = 0x108080;

        subtitle->palette[8] = 0xbff000;
        subtitle->palette[9] = 0x108080;
        subtitle->palette[10] = 0x108080;
        subtitle->palette[11] = 0x108080;

        subtitle->palette[12] = 0x108080;
        subtitle->palette[13] = 0xbff000;
        subtitle->palette[14] = 0x108080;
        subtitle->palette[15] = 0x108080;
    }

    hb_log("stream id 0x%x (type 0x%x substream 0x%x) subtitle 0x%x",
           pes->stream_id, pes->stream_type, pes->stream_id_ext, subtitle->id);

    // Search for the sort position
    if ( sort >= 0 )
    {
        sort = sort < count ? sort : count;
        for ( i = sort; i < count; i++ )
        {
            tmp = hb_list_item( title->list_subtitle, i );
            int sid = tmp->id & 0xffff;
            int ssid = tmp->id >> 16;
            if ( pes->stream_id < sid )
                break;
            else if ( pes->stream_id <= sid &&
                      pes->stream_id_ext <= ssid )
            {
                break;
            }
        }
        hb_list_insert( title->list_subtitle, i, subtitle );
    }
    else
    {
        hb_list_add( title->list_subtitle, subtitle );
    }
}

// Fix up title.list_audio indexes since audio can be inserted
// out of order in pes_add_audio_to_title
static void pes_set_audio_indices(hb_title_t * title)
{
    int ii, jj, count;

    count = hb_list_count( title->list_audio );

    for ( ii = 0; ii < count; ii++ )
    {
        hb_audio_t *audio_ii;

        audio_ii = hb_list_item( title->list_audio, ii );
        audio_ii->config.index = ii;
        for ( jj = 0; jj < count; jj++ )
        {
            hb_audio_t *audio_jj;

            audio_jj = hb_list_item( title->list_audio, jj );
            // if same PID or stream_id and not same audio track, link
            if ((audio_ii->id & 0xffff) == (audio_jj->id & 0xffff) && ii != jj)
            {
                if (audio_ii->config.list_linked_index == NULL)
                {
                    audio_ii->config.list_linked_index = hb_list_init();
                }
                hb_list_add_dup(audio_ii->config.list_linked_index,
                                &jj, sizeof(jj));
            }
        }
    }
}

// Sort specifies the index in the audio list where you would
// like sorted items to begin.
static void pes_add_audio_to_title(
    hb_stream_t *stream,
    int         track,
    hb_title_t  *title,
    int         sort)
{
    hb_pes_stream_t *pes = &stream->pes.list[track];

    // Sort by id when adding to the list
    // This assures that they are always displayed in the same order
    int id = get_id( pes );
    int i;
    hb_audio_t *tmp = NULL;

    int count = hb_list_count( title->list_audio );

    // Don't add the same audio twice.  Search for audio.
    for ( i = 0; i < count; i++ )
    {
        tmp = hb_list_item( title->list_audio, i );
        if ( id == tmp->id )
            return;
    }

    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = id;
    audio->config.in.reg_desc = stream->reg_desc;
    audio->config.in.stream_type = pes->stream_type;
    audio->config.in.substream_type = pes->stream_id_ext;

    audio->config.in.codec = pes->codec;
    audio->config.in.codec_param = pes->codec_param;
    audio->config.in.timebase.num = 1;
    audio->config.in.timebase.den = 90000;

    set_audio_description(audio, lang_for_code(pes->lang_code));

    hb_log("stream id 0x%x (type 0x%x substream 0x%x) audio 0x%x",
           pes->stream_id, pes->stream_type, pes->stream_id_ext, audio->id);

    audio->config.in.track = track;

    // Search for the sort position
    if ( sort >= 0 )
    {
        sort = sort < count ? sort : count;
        for ( i = sort; i < count; i++ )
        {
            tmp = hb_list_item( title->list_audio, i );
            int sid = tmp->id & 0xffff;
            int ssid = tmp->id >> 16;
            if ( pes->stream_id < sid )
                break;
            else if ( pes->stream_id <= sid &&
                      pes->stream_id_ext <= ssid )
            {
                break;
            }
        }
        hb_list_insert( title->list_audio, i, audio );
    }
    else
    {
        hb_list_add( title->list_audio, audio );
    }
}

static void hb_init_subtitle_list(hb_stream_t *stream, hb_title_t *title)
{
    int ii;
    int map_idx;
    int largest = -1;

    // First add all that were found in a map.
    for ( map_idx = 0; 1; map_idx++ )
    {
        for ( ii = 0; ii < stream->pes.count; ii++ )
        {
            if ( stream->pes.list[ii].stream_kind == S )
            {
                if ( stream->pes.list[ii].map_idx == map_idx )
                {
                    pes_add_subtitle_to_title( stream, ii, title, -1 );
                }
                if ( stream->pes.list[ii].map_idx > largest )
                    largest = stream->pes.list[ii].map_idx;
            }
        }
        if ( map_idx > largest )
            break;
    }

    int count = hb_list_count( title->list_subtitle );
    // Now add the reset.  Sort them by stream id.
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == S )
        {
            pes_add_subtitle_to_title( stream, ii, title, count );
        }
    }
}

static void hb_init_audio_list(hb_stream_t *stream, hb_title_t *title)
{
    int ii;
    int map_idx;
    int largest = -1;

    // First add all that were found in a map.
    for ( map_idx = 0; 1; map_idx++ )
    {
        for ( ii = 0; ii < stream->pes.count; ii++ )
        {
            if ( stream->pes.list[ii].stream_kind == A )
            {
                if ( stream->pes.list[ii].map_idx == map_idx )
                {
                    pes_add_audio_to_title( stream, ii, title, -1 );
                }
                if ( stream->pes.list[ii].map_idx > largest )
                    largest = stream->pes.list[ii].map_idx;
            }
        }
        if ( map_idx > largest )
            break;
    }

    int count = hb_list_count( title->list_audio );
    // Now add the reset.  Sort them by stream id.
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == A )
        {
            pes_add_audio_to_title( stream, ii, title, count );
        }
    }
    pes_set_audio_indices(title);
}

/***********************************************************************
 * hb_ts_stream_init
 ***********************************************************************
 *
 **********************************************************************/

static int hb_ts_stream_init(hb_stream_t *stream)
{
    int i;

    if ( stream->ts.list )
    {
        for (i=0; i < stream->ts.alloc; i++)
        {
            stream->ts.list[i].continuity = -1;
            stream->ts.list[i].pid = -1;
            stream->ts.list[i].pes_list = -1;
        }
    }
    stream->ts.count = 0;

    if ( stream->pes.list )
    {
        for (i=0; i < stream->pes.alloc; i++)
        {
            stream->pes.list[i].stream_id = -1;
            stream->pes.list[i].next = -1;
        }
    }
    stream->pes.count = 0;

    stream->ts.packet = malloc( stream->packetsize );

    // Find the audio and video pids in the stream
    if (hb_ts_stream_find_pids(stream) < 0)
    {
        return -1;
    }

    // hb_ts_resolve_pid_types reads some data, so the TS buffers
    // are needed here.
    for (i = 0; i < stream->ts.count; i++)
    {
        // demuxing buffer for TS to PS conversion
        stream->ts.list[i].buf = hb_buffer_init(stream->packetsize);
        stream->ts.list[i].buf->size = 0;
    }
    hb_ts_resolve_pid_types(stream);

    if( stream->scan )
    {
        hb_log("Found the following PIDS");
        hb_log("    Video PIDS : ");
        for (i=0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == V )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
        }
        hb_log("    Audio PIDS : ");
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == A )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
        }
        hb_log("    Subtitle PIDS : ");
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == S )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
        }
        hb_log("    Other PIDS : ");
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == N ||
                 ts_stream_kind( stream, i ) == P )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
            if ( ts_stream_kind( stream, i ) == N )
                hb_stream_delete_ts_entry(stream, i);
        }
    }
    else
    {
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == N )
                hb_stream_delete_ts_entry(stream, i);
        }
    }
    return 0;
}

static void hb_ps_stream_init(hb_stream_t *stream)
{
    int i;

    if ( stream->pes.list )
    {
        for (i=0; i < stream->pes.alloc; i++)
        {
            stream->pes.list[i].stream_id = -1;
            stream->pes.list[i].next = -1;
        }
    }
    stream->pes.count = 0;

    // Find the audio and video pids in the stream
    hb_ps_stream_find_streams(stream);
    hb_ps_resolve_stream_types(stream);

    if( stream->scan )
    {
        hb_log("Found the following streams");
        hb_log("    Video Streams : ");
        for (i=0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == V )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type);
            }
        }
        hb_log("    Audio Streams : ");
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == A )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type );
            }
        }
        hb_log("    Subtitle Streams : ");
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == S )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type );
            }
        }
        hb_log("    Other Streams : ");
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == N )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type );
                hb_stream_delete_ps_entry(stream, i);
            }
        }
    }
    else
    {
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == N )
                hb_stream_delete_ps_entry(stream, i);
        }
    }
}

#define MAX_HOLE 208*80

static off_t align_to_next_packet(hb_stream_t *stream)
{
    uint8_t buf[MAX_HOLE];
    off_t pos = 0;
    off_t start = ftello(stream->file_handle);
    off_t orig;

    if ( start >= stream->packetsize ) {
        start -= stream->packetsize;
        fseeko(stream->file_handle, start, SEEK_SET);
    }
    orig = start;

    while (1)
    {
        if (fread(buf, sizeof(buf), 1, stream->file_handle) == 1)
        {
            const uint8_t *bp = buf;
            int i;

            for ( i = sizeof(buf) - 8 * stream->packetsize; --i >= 0; ++bp )
            {
                if ( have_ts_sync( bp, stream->packetsize, 8 ) )
                {
                    break;
                }
            }
            if ( i >= 0 )
            {
                pos = ( bp - buf ) - stream->packetsize + 188;
                break;
            }
            fseeko(stream->file_handle, -8 * stream->packetsize, SEEK_CUR);
            start = ftello(stream->file_handle);
        }
        else
        {
            int err;
            if ((err = ferror(stream->file_handle)) != 0)
            {
                hb_error("align_to_next_packet: error (%d)", err);
                hb_set_work_error(stream->h, HB_ERROR_READ);
            }
            return 0;
        }
    }
    fseeko(stream->file_handle, start+pos, SEEK_SET);
    return start - orig + pos;
}

static const unsigned int bitmask[] = {
    0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
    0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
    0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
    0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};

static inline void bits_init(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear)
{
    bb->pos = 0;
    bb->buf = buf;
    bb->size = bufsize;
    bb->val = ((uint32_t)bb->buf[0] << 24) | (bb->buf[1] << 16) |
              (bb->buf[2] << 8) | bb->buf[3];
    if (clear)
        memset(bb->buf, 0, bufsize);
    bb->size = bufsize;
}

static inline void bits_clone( bitbuf_t *dst, bitbuf_t *src, int bufsize )
{
    *dst = *src;
    dst->size = (dst->pos >> 3) + bufsize;
}

static inline int bits_bytes_left(bitbuf_t *bb)
{
    return bb->size - (bb->pos >> 3);
}

static inline unsigned int bits_peek(bitbuf_t *bb, int bits)
{
    unsigned int val;
    int left = 32 - (bb->pos & 31);

    if (bits < left)
    {
        val = (bb->val >> (left - bits)) & bitmask[bits];
    }
    else
    {
        val = (bb->val & bitmask[left]) << (bits - left);
        int bpos = bb->pos + left;
        bits -= left;


        if (bits > 0)
        {
            int pos = bpos >> 3;
            int bval =  (bb->buf[pos]     << 24) |
                        (bb->buf[pos + 1] << 16) |
                        (bb->buf[pos + 2] <<  8) |
                         bb->buf[pos + 3];
            val |= (bval >> (32 - bits)) & bitmask[bits];
        }
    }

    return val;
}

static inline unsigned int bits_get(bitbuf_t *bb, int bits)
{
    unsigned int val;
    int left = 32 - (bb->pos & 31);

    if (bits < left)
    {
        val = (bb->val >> (left - bits)) & bitmask[bits];
        bb->pos += bits;
    }
    else
    {
        val = (bb->val & bitmask[left]) << (bits - left);
        bb->pos += left;
        bits -= left;

        int pos = bb->pos >> 3;
        bb->val = ((uint32_t)bb->buf[pos] << 24) | (bb->buf[pos + 1] << 16) | (bb->buf[pos + 2] << 8) | bb->buf[pos + 3];

        if (bits > 0)
        {
            val |= (bb->val >> (32 - bits)) & bitmask[bits];
            bb->pos += bits;
        }
    }

    return val;
}

static inline int bits_skip(bitbuf_t *bb, int bits)
{
    if (bits <= 0)
        return 0;
    while (bits > 32)
    {
        bits_get(bb, 32);
        bits -= 32;
    }
    bits_get(bb, bits);
    return 0;
}

// extract what useful information we can from the elementary stream
// descriptor list at 'dp' and add it to the stream at 'esindx'.
// Descriptors with info we don't currently use are ignored.
// The descriptor list & descriptor item formats are defined in
// ISO 13818-1 (2000E) section 2.6 (pg. 62).
static void decode_element_descriptors(
    hb_stream_t   *stream,
    int           pes_idx,
    bitbuf_t   *bb)
{
    int ii;

    while( bits_bytes_left( bb ) > 2 )
    {
        uint8_t tag = bits_get(bb, 8);
        uint8_t len = bits_get(bb, 8);
        switch ( tag )
        {
            case 5:    // Registration descriptor
                stream->pes.list[pes_idx].format_id = bits_get(bb, 32);
                bits_skip(bb, 8 * (len - 4));
                break;

            case 10:    // ISO_639_language descriptor
            {
                char code[3];
                for (ii = 0; ii < 3; ii++)
                {
                    code[ii] = bits_get(bb, 8);
                }
                stream->pes.list[pes_idx].lang_code =
                                    lang_to_code(lang_for_code2(code));
                bits_skip(bb, 8 * (len - 3));
            } break;

            case 0x56:  // DVB Teletext descriptor
            {
                // We don't currently process teletext from
                // TS or PS streams.  Set stream 'kind' to N
                stream->pes.list[pes_idx].stream_type = 0x00;
                stream->pes.list[pes_idx].stream_kind = N;
                strncpy(stream->pes.list[pes_idx].codec_name,
                        "DVB Teletext", 80);
                bits_skip(bb, 8 * len);
            } break;

            case 0x59:  // DVB Subtitleing descriptor
            {
                int lang_count = len / 8;

                stream->pes.list[pes_idx].stream_type = 0x00;
                stream->pes.list[pes_idx].stream_kind = S;
                stream->pes.list[pes_idx].codec = WORK_DECAVSUB;
                stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DVB_SUBTITLE;
                strncpy(stream->pes.list[pes_idx].codec_name,
                        "DVB Subtitling", 80);
                if (lang_count > 0)
                {
                    uint8_t * extradata = malloc(5);
                    char      code[3];

                    stream->pes.list[pes_idx].extradata = extradata;
                    stream->pes.list[pes_idx].extradata_size = 5;
                    code[0] = bits_get(bb, 8);
                    code[1] = bits_get(bb, 8);
                    code[2] = bits_get(bb, 8);

                    extradata[4] = bits_get(bb, 8);
                    *extradata++ = bits_get(bb, 8);
                    *extradata++ = bits_get(bb, 8);
                    *extradata++ = bits_get(bb, 8);
                    *extradata++ = bits_get(bb, 8);

                    stream->pes.list[pes_idx].lang_code =
                                        lang_to_code(lang_for_code2(code));
                    bits_skip(bb, 8 * (len - 8));
                }
                else
                {
                    bits_skip(bb, 8 * len);
                }
            } break;

            case 0x6a:  // DVB AC-3 descriptor
            {
                stream->pes.list[pes_idx].stream_type = 0x81;
                update_pes_kind( stream, pes_idx );
                bits_skip(bb, 8 * len);
            } break;

            case 0x7a:  // DVB EAC-3 descriptor
            {
                stream->pes.list[pes_idx].stream_type = 0x87;
                update_pes_kind( stream, pes_idx );
                bits_skip(bb, 8 * len);
            } break;

            default:
                bits_skip(bb, 8 * len);
                break;
        }
    }
}

int decode_program_map(hb_stream_t* stream)
{
    bitbuf_t bb;
    bits_init(&bb, stream->pmt_info.tablebuf, stream->pmt_info.tablepos, 0);

    bits_get(&bb, 8);  // table_id
    bits_get(&bb, 4);
    unsigned int section_length = bits_get(&bb, 12);

    bits_get(&bb, 16); // program number
    bits_get(&bb, 2);
    bits_get(&bb, 5);  // version_number
    bits_get(&bb, 1);
    bits_get(&bb, 8);  // section_number
    bits_get(&bb, 8);  // last_section_number
    bits_get(&bb, 3);
    stream->pmt_info.PCR_PID = bits_get(&bb, 13);
    bits_get(&bb, 4);
    int program_info_length = bits_get(&bb, 12);
    int i;
    for (i = 0; i < program_info_length - 2; )
    {
        uint8_t tag, len;
        tag = bits_get(&bb, 8);
        len = bits_get(&bb, 8);
        i += 2;
        if ( i + len > program_info_length )
        {
            break;
        }
        if (tag == 0x05 && len >= 4)
        {
            // registration descriptor
            stream->reg_desc = bits_get(&bb, 32);
            i += 4;
            len -= 4;
        }
        int j;
        for ( j = 0; j < len; j++ )
        {
            bits_get(&bb, 8);
        }
        i += len;
    }
    for ( ; i < program_info_length; i++ )
    {
        bits_get(&bb, 8);
    }

    int cur_pos =  9 /* data after the section length field*/ + program_info_length;
    int done_reading_stream_types = 0;
    int ii = 0;
    while (!done_reading_stream_types)
    {
        unsigned char stream_type = bits_get(&bb, 8);
        bits_get(&bb, 3);
        unsigned int elementary_PID = bits_get(&bb, 13);
        bits_get(&bb, 4);
        unsigned int info_len = bits_get(&bb, 12);
        // Defined audio stream types are 0x81 for AC-3/A52 audio
        // and 0x03 for mpeg audio. But content producers seem to
        // use other values (0x04 and 0x06 have both been observed)
        // so at this point we say everything that isn't a video
        // pid is audio then at the end of hb_stream_title_scan
        // we'll figure out which are really audio by looking at
        // the PES headers.
        int pes_idx;
        update_ts_streams( stream, elementary_PID, 0,
                           stream_type, -1, &pes_idx );
        if ( pes_idx >= 0 )
            stream->pes.list[pes_idx].map_idx = ii;
        if (info_len > 0)
        {
            bitbuf_t bb_desc;
            bits_clone( &bb_desc, &bb, info_len );
            if ( pes_idx >= 0 )
                decode_element_descriptors( stream, pes_idx, &bb_desc );
            bits_skip(&bb, 8 * info_len);
        }

        cur_pos += 5 /* stream header */ + info_len;

        if (cur_pos >= section_length - 4 /* stop before the CRC */)
            done_reading_stream_types = 1;
        ii++;
    }

    return 1;
}

static int build_program_map(const uint8_t *buf, hb_stream_t *stream)
{
    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    // Get pointer length - only valid in packets with a start flag
    int pointer_len = 0;

    if (start)
    {
        pointer_len = buf[4 + adapt_len] + 1;
        stream->pmt_info.tablepos = 0;
    }
    // Get Continuity Counter
    int continuity_counter = buf[3] & 0x0f;
    if (!start && (stream->pmt_info.current_continuity_counter + 1 != continuity_counter))
    {
        hb_log("build_program_map - Continuity Counter %d out of sequence - expected %d", continuity_counter, stream->pmt_info.current_continuity_counter+1);
        return 0;
    }
    stream->pmt_info.current_continuity_counter = continuity_counter;
    stream->pmt_info.reading |= start;

    // Add the payload for this packet to the current buffer
    int amount_to_copy = 184 - adapt_len - pointer_len;
    if (stream->pmt_info.reading && (amount_to_copy > 0))
    {
            stream->pmt_info.tablebuf = realloc(stream->pmt_info.tablebuf, stream->pmt_info.tablepos + amount_to_copy);

            memcpy(stream->pmt_info.tablebuf + stream->pmt_info.tablepos, buf + 4 + adapt_len + pointer_len, amount_to_copy);
            stream->pmt_info.tablepos += amount_to_copy;
    }
    if (stream->pmt_info.tablepos > 3)
    {
        // We have enough to check the section length
        int length;
        length = ((stream->pmt_info.tablebuf[1] << 8) +
                  stream->pmt_info.tablebuf[2]) & 0xFFF;
        if (stream->pmt_info.tablepos > length + 1)
        {
            // We just finished a bunch of packets - parse the program map details
            int decode_ok = 0;
            if (stream->pmt_info.tablebuf[0] == 0x02)
                decode_ok = decode_program_map(stream);
            free(stream->pmt_info.tablebuf);
            stream->pmt_info.tablebuf = NULL;
            stream->pmt_info.tablepos = 0;
            stream->pmt_info.reading = 0;
            if (decode_ok)
                return decode_ok;
        }

    }

    return 0;
}

static int decode_PAT(const uint8_t *buf, hb_stream_t *stream)
{
    unsigned char tablebuf[1024];
    unsigned int tablepos = 0;

    int reading = 0;


    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get pointer length
    int pointer_len = buf[4 + adapt_len] + 1;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    if (start)
            reading = 1;

    // Add the payload for this packet to the current buffer
    if (reading && (184 - adapt_len) > 0)
    {
            if (tablepos + 184 - adapt_len - pointer_len > 1024)
            {
                    hb_log("decode_PAT - Bad program section length (> 1024)");
                    return 0;
            }
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + pointer_len, 184 - adapt_len - pointer_len);
            tablepos += 184 - adapt_len - pointer_len;
    }

    if (start && reading)
    {
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + 1, pointer_len - 1);


            unsigned int pos = 0;
            //while (pos < tablepos)
            {
                    bitbuf_t bb;
                    bits_init(&bb, tablebuf + pos, tablepos - pos, 0);

                    unsigned char section_id    = bits_get(&bb, 8);
                    bits_get(&bb, 4);
                    unsigned int section_len    = bits_get(&bb, 12);
                    bits_get(&bb, 16); // transport_id
                    bits_get(&bb, 2);
                    bits_get(&bb, 5);  // version_num
                    bits_get(&bb, 1);  // current_next
                    bits_get(&bb, 8);  // section_num
                    bits_get(&bb, 8);  // last_section

                    switch (section_id)
                    {
                      case 0x00:
                        {
                          // Program Association Section
                          section_len -= 5;    // Already read transport stream ID, version num, section num, and last section num
                          section_len -= 4;   // Ignore the CRC
                          int curr_pos = 0;
                          stream->ts_number_pat_entries = 0;
                          while ((curr_pos < section_len) && (stream->ts_number_pat_entries < kMaxNumberPMTStreams))
                          {
                            unsigned int pkt_program_num = bits_get(&bb, 16);
                            stream->pat_info[stream->ts_number_pat_entries].program_number = pkt_program_num;

                            bits_get(&bb, 3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              bits_get(&bb, 13); // pkt_network_id
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = bits_get(&bb, 13);
                                stream->pat_info[stream->ts_number_pat_entries].program_map_PID = pkt_program_map_PID;
                            }
                            curr_pos += 4;
                            stream->ts_number_pat_entries++;
                          }
                        }
                        break;
                      case 0xC7:
                            {
                                    break;
                            }
                      case 0xC8:
                            {
                                    break;
                            }
                    }

//                    pos += 3 + section_len;
            }

//            tablepos = 0;
    }
    return 1;
}

// convert a PES PTS or DTS to an int64
static int64_t parse_pes_timestamp( bitbuf_t *bb )
{
    int64_t ts;

    ts =  ( (uint64_t)   bits_get(bb,  3) << 30 ) +
                         bits_skip(bb, 1)         +
                       ( bits_get(bb, 15) << 15 ) +
                         bits_skip(bb, 1)         +
                         bits_get(bb, 15);
    bits_skip(bb, 1);
    return ts;
}

static int parse_pes_header(
    hb_stream_t   *stream,
    bitbuf_t      *bb,
    hb_pes_info_t *pes_info )
{
    if ( bits_bytes_left(bb) < 6 )
    {
        return 0;
    }

    bits_skip(bb, 8 * 4);
    pes_info->packet_len   = bits_get(bb, 16);
    pes_info->stuffing_len = 0;

    /*
     * This would normally be an error.  But the decoders can generally
     * recover well from missing data.  So let the packet pass.
    if ( bits_bytes_left(bb) < pes_info->packet_len )
    {
        return 0;
    }
    */

    int mark = bits_peek(bb, 2);
    if ( mark == 0x02 )
    {
        // mpeg2 pes
        if ( bits_bytes_left(bb) < 3 )
        {
            return 0;
        }

        /*
        bits_skip(bb, 2);
        bits_get(bb, 2);    // scrambling
        bits_get(bb, 1);    // priority
        bits_get(bb, 1);    // alignment
        bits_get(bb, 1);    // copyright
        bits_get(bb, 1);    // original
        */
        bits_get(bb, 8);    // skip all of the above

        int has_pts        = bits_get(bb, 2);
        int has_escr       = bits_get(bb, 1);
        int has_esrate     = bits_get(bb, 1);
        int has_dsm        = bits_get(bb, 1);
        int has_copy_info  = bits_get(bb, 1);
        int has_crc        = bits_get(bb, 1);
        int has_ext        = bits_get(bb, 1);
        int hdr_len = pes_info->header_len = bits_get(bb, 8);
        pes_info->header_len += bb->pos >> 3;

        bitbuf_t bb_hdr;
        bits_clone(&bb_hdr, bb, hdr_len);

        if ( bits_bytes_left(&bb_hdr) < hdr_len )
        {
            return 0;
        }

        int expect = (!!has_pts) * 5 + (has_pts & 0x01) * 5 + has_escr * 6 +
                     has_esrate * 3 + has_dsm + has_copy_info + has_crc * 2 +
                     has_ext;

        if ( bits_bytes_left(&bb_hdr) < expect )
        {
            return 0;
        }

        if( has_pts )
        {
            if ( bits_bytes_left(&bb_hdr) < 5 )
            {
                return 0;
            }
            bits_skip(&bb_hdr, 4);
            pes_info->pts = parse_pes_timestamp( &bb_hdr );
            if ( has_pts & 1 )
            {
                if ( bits_bytes_left(&bb_hdr) < 5 )
                {
                    return 0;
                }
                bits_skip(&bb_hdr, 4);
                pes_info->dts = parse_pes_timestamp( &bb_hdr );
            }
            else
            {
                pes_info->dts = pes_info->pts;
            }
        }
        // A user encountered a stream that has garbage DTS timestamps.
        // DTS should never be > PTS.  Such broken timestamps leads to
        // HandBrake computing negative buffer start times.
        if (pes_info->dts > pes_info->pts)
        {
            pes_info->dts = pes_info->pts;
        }

        if ( has_escr )
            bits_skip(&bb_hdr, 8 * 6);
        if ( has_esrate )
            bits_skip(&bb_hdr, 8 * 3);
        if ( has_dsm )
            bits_skip(&bb_hdr, 8);
        if ( has_copy_info )
            bits_skip(&bb_hdr, 8);
        if ( has_crc )
            bits_skip(&bb_hdr, 8 * 2);

        if ( has_ext )
        {
            int has_private = bits_get(&bb_hdr, 1);
            int has_pack = bits_get(&bb_hdr, 1);
            int has_counter = bits_get(&bb_hdr, 1);
            int has_pstd = bits_get(&bb_hdr, 1);
            bits_skip(&bb_hdr, 3);   // reserved bits
            int has_ext2 = bits_get(&bb_hdr, 1);

            expect = (has_private) * 16 + has_pack + has_counter * 2 +
                     has_pstd * 2 + has_ext2 * 2;

            if ( bits_bytes_left(&bb_hdr) < expect )
            {
                return 0;
            }

            if ( has_private )
            {
                bits_skip(&bb_hdr, 8 * 16);
                expect -= 2;
            }
            if ( has_pack )
            {
                int len = bits_get(&bb_hdr, 8);
                expect -= 1;
                if ( bits_bytes_left(&bb_hdr) < len + expect )
                {
                    return 0;
                }
                bits_skip(&bb_hdr, 8 * len);
            }
            if ( has_counter )
                bits_skip(&bb_hdr, 8 * 2);
            if ( has_pstd )
                bits_skip(&bb_hdr, 8 * 2);

            if ( has_ext2 )
            {
                bits_skip(&bb_hdr, 1);   // marker
                bits_get(&bb_hdr, 7);    // extension length
                pes_info->has_stream_id_ext = !bits_get(&bb_hdr, 1);
                if ( pes_info->has_stream_id_ext )
                    pes_info->stream_id_ext = bits_get(&bb_hdr, 7);
            }
        }
        // eat header stuffing
        bits_skip(bb, 8 * hdr_len);
    }
    else
    {
        // mpeg1 pes

        // Skip stuffing
        while ( bits_peek(bb, 1) && bits_bytes_left(bb) )
            bits_get(bb, 8);

        if ( !bits_bytes_left(bb) )
            return 0;

        // Skip std buffer info
        int mark = bits_get(bb, 2);
        if ( mark == 0x01 )
        {
            if ( bits_bytes_left(bb) < 2 )
                return 0;
            bits_skip(bb, 8 * 2);
        }

        int has_pts = bits_get(bb, 2);
        if( has_pts == 0x02 )
        {
            pes_info->pts = parse_pes_timestamp( bb );
            pes_info->dts = pes_info->pts;
        }
        else if( has_pts == 0x03 )
        {
            pes_info->pts = parse_pes_timestamp( bb );
            bits_skip(bb, 4);
            pes_info->dts = parse_pes_timestamp( bb );
        }
        else
        {
            bits_skip(bb, 8); // 0x0f flag
        }
        if ( bits_bytes_left(bb) < 0 )
            return 0;
        pes_info->header_len = bb->pos >> 3;
    }
    if ( pes_info->stream_id == 0xbd && stream->hb_stream_type == program )
    {
        if ( bits_bytes_left(bb) < 4 )
        {
            return 0;
        }
        int ssid = bits_peek(bb, 8);
        if( ( ssid >= 0xa0 && ssid <= 0xaf ) ||
            ( ssid >= 0x20 && ssid <= 0x2f ) )
        {
            // DVD LPCM or DVD SPU (subtitles)
            pes_info->bd_substream_id = bits_get(bb, 8);
            pes_info->header_len += 1;
        }
        else if ( ssid >= 0xb0 && ssid <= 0xbf )
        {
            // HD-DVD TrueHD has a 4 byte header
            pes_info->bd_substream_id = bits_get(bb, 8);
            bits_skip(bb, 8 * 4);
            pes_info->header_len += 5;
        }
        else if( ( ssid >= 0x80 && ssid <= 0x9f ) ||
                 ( ssid >= 0xc0 && ssid <= 0xcf ) )
        {
            // AC3, E-AC3, DTS, and DTS-HD has 3 byte header
            pes_info->bd_substream_id = bits_get(bb, 8);
            bits_skip(bb, 8 * 3);
            pes_info->header_len += 4;
        }
    }
    if ( pes_info->stream_id == 0xbd && stream->hb_stream_type == transport )
    {
        if ( bits_bytes_left(bb) < 2 )
        {
            return 0;
        }
        int ssid = bits_peek(bb, 8);
        if ( ssid == 0x20 )
        {
            // DVB (subtitles)
            bits_skip(bb, 8);
            pes_info->bd_substream_id = bits_get(bb, 8);
            pes_info->header_len += 2;
            pes_info->stuffing_len = 1;
        }
    }
    return 1;
}

static int parse_pack_header(
    hb_stream_t   *stream,
    bitbuf_t      *bb,
    hb_pes_info_t *pes_info )
{
    if ( bits_bytes_left(bb) < 12)
    {
        return 0;
    }

    bits_skip(bb, 8 * 4);
    int mark = bits_get(bb, 2);

    if ( mark == 0x00 )
    {
        // mpeg1 pack
        bits_skip(bb, 2); // marker
    }
    pes_info->scr = parse_pes_timestamp( bb );

    if ( mark == 0x00 )
    {
        bits_skip(bb, 24);
        pes_info->header_len = (bb->pos >> 3);
    }
    else
    {
        bits_skip(bb, 39);
        int stuffing = bits_get(bb, 3);
        pes_info->header_len = stuffing;
        pes_info->header_len += (bb->pos >> 3);
    }
    return 1;
}

// Returns the length of the header
static int hb_parse_ps(
    hb_stream_t   *stream,
    uint8_t       *buf,
    int           len,
    hb_pes_info_t *pes_info )
{
    memset( pes_info, 0, sizeof( hb_pes_info_t ) );
    pes_info->pts = AV_NOPTS_VALUE;
    pes_info->dts = AV_NOPTS_VALUE;

    bitbuf_t bb, cc;
    bits_init(&bb, buf, len, 0);
    bits_clone(&cc, &bb, len);

    if ( bits_bytes_left(&bb) < 4 )
        return 0;

    // Validate start code
    if ( bits_get(&bb, 8 * 3) != 0x000001 )
    {
        return 0;
    }

    pes_info->stream_id = bits_get(&bb, 8);
    if ( pes_info->stream_id == 0xb9 )
    {
        // Program stream end code
        return 1;
    }
    else if ( pes_info->stream_id == 0xba )
    {
        return parse_pack_header( stream, &cc, pes_info );
    }
    else if ( pes_info->stream_id >= 0xbd &&
              pes_info->stream_id != 0xbe &&
              pes_info->stream_id != 0xbf &&
              pes_info->stream_id != 0xf0 &&
              pes_info->stream_id != 0xf1 &&
              pes_info->stream_id != 0xf2 &&
              pes_info->stream_id != 0xf8 &&
              pes_info->stream_id != 0xff )
    {
        return parse_pes_header( stream, &cc, pes_info );
    }
    else
    {
        if ( bits_bytes_left(&bb) < 2 )
        {
            return 0;
        }
        pes_info->packet_len = bits_get(&bb, 16);
        if ( pes_info->stream_id == 0xbe )
        {
            // Skip all stuffing
            pes_info->header_len = 6 + pes_info->packet_len;
        }
        else
        {
            pes_info->header_len = bb.pos >> 3;
        }
        return 1;
    }
}

static int hb_ps_read_packet( hb_stream_t * stream, hb_buffer_t *b )
{
    // Appends to buffer if size != 0
    unsigned int start_code = -1;
    int pos = b->size;
    int stream_id = -1;
    int c;

#define cp (b->data)
    flockfile( stream->file_handle );
    while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
    {
        start_code = ( start_code << 8 ) | c;
        if ( ( start_code >> 8 )== 0x000001 )
            // we found the start of the next start
            break;
    }
    if ( c == EOF )
        goto done;

    if ( pos + 4 > b->alloc )
    {
        // need to expand the buffer
        hb_buffer_realloc( b, b->alloc * 2 );
    }
    cp[pos++] = ( start_code >> 24 ) & 0xff;
    cp[pos++] = ( start_code >> 16 ) & 0xff;
    cp[pos++] = ( start_code >>  8 ) & 0xff;
    cp[pos++] = ( start_code )       & 0xff;
    stream_id = start_code & 0xff;

    if ( stream_id == 0xba )
    {
        int start = pos - 4;
        // Read pack header
        if ( pos + 21 >= b->alloc )
        {
            // need to expand the buffer
            hb_buffer_realloc( b, b->alloc * 2 );
        }

        // There are at least 8 bytes.  More if this is mpeg2 pack.
        if (fread( cp+pos, 1, 8, stream->file_handle ) < 8)
            goto done;

        int mark = cp[pos] >> 4;
        pos += 8;

        if ( mark != 0x02 )
        {
            // mpeg-2 pack,
            if (fread( cp+pos, 1, 2, stream->file_handle ) == 2)
            {
                int len = cp[start+13] & 0x7;
                pos += 2;
                if (len > 0 &&
                    fread( cp+pos, 1, len, stream->file_handle ) == len)
                    pos += len;
                else
                    goto done;
            }
        }
    }
    // Non-video streams can emulate start codes, so we need
    // to inspect PES packets and skip over their data
    // sections to avoid mis-detection of the next pack or pes start code
    else if ( stream_id >= 0xbb )
    {
        int len = 0;
        c = getc_unlocked( stream->file_handle );
        if ( c == EOF )
            goto done;
        len = c << 8;
        c = getc_unlocked( stream->file_handle );
        if ( c == EOF )
            goto done;
        len |= c;
        if ( pos + len + 2 > b->alloc )
        {
            if ( b->alloc * 2 > pos + len + 2 )
                hb_buffer_realloc( b, b->alloc * 2 );
            else
                hb_buffer_realloc( b, b->alloc * 2 + len + 2 );
        }
        cp[pos++] = len >> 8;
        cp[pos++] = len & 0xff;
        if ( len )
        {
            // Length is non-zero, read the packet all at once
            len = fread( cp+pos, 1, len, stream->file_handle );
            pos += len;
        }
        else
        {
            // Length is zero, read bytes till we find a start code.
            // Only video PES packets are allowed to have zero length.
            start_code = -1;
            while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
            {
                start_code = ( start_code << 8 ) | c;
                if ( pos  >= b->alloc )
                {
                    // need to expand the buffer
                    hb_buffer_realloc( b, b->alloc * 2 );
                }
                cp[pos++] = c;
                if ( ( start_code >> 8   ) == 0x000001 &&
                     ( start_code & 0xff ) >= 0xb9 )
                {
                    // we found the start of the next start
                    break;
                }
            }
            if ( c == EOF )
                goto done;
            pos -= 4;
            fseeko( stream->file_handle, -4, SEEK_CUR );
        }
    }
    else
    {
        // Unknown, find next start code
        start_code = -1;
        while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
        {
            start_code = ( start_code << 8 ) | c;
            if ( pos  >= b->alloc )
            {
                // need to expand the buffer
                hb_buffer_realloc( b, b->alloc * 2 );
            }
            cp[pos++] = c;
            if ( ( start_code >> 8 ) == 0x000001 &&
                 ( start_code & 0xff ) >= 0xb9 )
                // we found the start of the next start
                break;
        }
        if ( c == EOF )
            goto done;
        pos -= 4;
        fseeko( stream->file_handle, -4, SEEK_CUR );
    }

done:
    // Parse packet for information we might need
    funlockfile( stream->file_handle );

    int err;
    if ((err = ferror(stream->file_handle)) != 0)
    {
        hb_error("hb_ps_read_packet: error (%d)", err);
        hb_set_work_error(stream->h, HB_ERROR_READ);
    }

    int len = pos - b->size;
    b->size = pos;
#undef cp
    return len;
}

static hb_buffer_t * hb_ps_stream_decode( hb_stream_t *stream )
{
    hb_pes_info_t pes_info;
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);

    while (1)
    {
        buf->size = 0;
        int len = hb_ps_read_packet( stream, buf );
        if ( len == 0 )
        {
            // End of file
            hb_buffer_close( &buf );
            return buf;
        }
        if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
        {
            ++stream->errors;
            continue;
        }
        // pack header
        if ( pes_info.stream_id == 0xba )
        {
            stream->pes.found_scr = 1;
            stream->ts_flags |= TS_HAS_PCR;
            stream->pes.scr = pes_info.scr;
            continue;
        }

        // If we don't have a SCR yet but the stream has SCRs just loop
        // so we don't process anything until we have a clock reference.
        if ( !stream->pes.found_scr && ( stream->ts_flags & TS_HAS_PCR ) )
        {
            continue;
        }

        // system header
        if ( pes_info.stream_id == 0xbb )
            continue;

        int idx;
        if ( pes_info.stream_id == 0xbd )
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.bd_substream_id );
        }
        else
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.stream_id_ext );
        }

        // Is this a stream carrying data that we care about?
        if ( idx < 0 )
            continue;

        switch (stream->pes.list[idx].stream_kind)
        {
            case A:
                buf->s.type = AUDIO_BUF;
                break;

            case V:
                buf->s.type = VIDEO_BUF;
                break;

            default:
                buf->s.type = OTHER_BUF;
                break;
        }

        if ( stream->need_keyframe )
        {
            // we're looking for the first video frame because we're
            // doing random access during 'scan'
            if ( buf->s.type != VIDEO_BUF ||
                 !isIframe( stream, buf->data, buf->size ) )
            {
                // not the video stream or didn't find an I frame
                // but we'll only wait 600 video frames for an I frame.
                if ( buf->s.type != VIDEO_BUF || ++stream->need_keyframe < 600 )
                {
                    continue;
                }
            }
            stream->need_keyframe = 0;
        }
        if ( buf->s.type == VIDEO_BUF )
            ++stream->frames;

        buf->s.id = get_id( &stream->pes.list[idx] );
        buf->s.pcr = stream->pes.scr;
        buf->s.start = pes_info.pts;
        buf->s.renderOffset = pes_info.dts;
        memmove( buf->data, buf->data + pes_info.header_len,
                 buf->size - pes_info.header_len );
        buf->size -= pes_info.header_len;
        if ( buf->size == 0 )
            continue;
        stream->pes.scr = AV_NOPTS_VALUE;
        return buf;
    }
}

static int update_ps_streams( hb_stream_t * stream, int stream_id, int stream_id_ext, int stream_type, int in_kind )
{
    int ii;
    int same_stream = -1;
    kind_t kind = in_kind == -1 ? st2codec[stream_type].kind : in_kind;

    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_id == stream_id )
            same_stream = ii;

        if ( stream->pes.list[ii].stream_id == stream_id &&
             stream->pes.list[ii].stream_id_ext == 0 &&
             stream->pes.list[ii].stream_kind == U )
        {
            // This is an unknown stream type that hasn't been
            // given a stream_id_ext.  So match only to stream_id
            //
            // is the stream_id_ext being updated?
            if ( stream_id_ext != 0 )
                break;

            // If stream is already in the list and the new 'kind' is
            // PCR, Unknown, or same as before, just return the index
            // to the entry found.
            if ( kind == P || kind == U || kind == stream->pes.list[ii].stream_kind )
                return ii;
            // Update stream_type and kind
            break;
        }
        if ( stream_id == stream->pes.list[ii].stream_id &&
             stream_id_ext == stream->pes.list[ii].stream_id_ext )
        {
            // If stream is already in the list and the new 'kind' is
            // PCR and the old 'kind' is unknown, set the new 'kind'
            if ( kind == P && stream->pes.list[ii].stream_kind == U )
                break;

            // If stream is already in the list and the new 'kind' is
            // PCR, Unknown, or same as before, just return the index
            // to the entry found.
            if ( kind == P || kind == U || kind == stream->pes.list[ii].stream_kind )
                return ii;
            // Replace unknown 'kind' with known 'kind'
            break;
        }
        // Resolve multiple videos
        if ( kind == V && stream->pes.list[ii].stream_kind == V )
        {
            if ( stream_id <= stream->pes.list[ii].stream_id &&
                 stream_id_ext <= stream->pes.list[ii].stream_id_ext )
            {
                // Assume primary video stream has the smallest stream id
                // and only use the primary. move the current item
                // to the end of the list.  we want to keep it for
                // debug and informational purposes.
                int jj = new_pes( stream );
                memcpy( &stream->pes.list[jj], &stream->pes.list[ii],
                        sizeof( hb_pes_stream_t ) );
                break;
            }
        }
    }

    if ( ii == stream->pes.count )
    {
        ii = new_pes( stream );
        if ( same_stream >= 0 )
        {
            memcpy( &stream->pes.list[ii], &stream->pes.list[same_stream],
                    sizeof( hb_pes_stream_t ) );
        }
        else
        {
            stream->pes.list[ii].map_idx = -1;
        }
    }

    stream->pes.list[ii].stream_id = stream_id;
    stream->pes.list[ii].stream_id_ext = stream_id_ext;
    stream->pes.list[ii].stream_type = stream_type;
    stream->pes.list[ii].stream_kind = kind;
    return ii;
}

static void update_pes_kind( hb_stream_t * stream, int idx )
{
    kind_t kind = st2codec[stream->pes.list[idx].stream_type].kind;
    if ( kind != U && kind != N )
    {
        stream->pes.list[idx].stream_kind = kind;
    }
}

static void ts_pes_list_add( hb_stream_t *stream, int ts_idx, int pes_idx )
{
    int ii = stream->ts.list[ts_idx].pes_list;
    if ( ii == -1 )
    {
        stream->ts.list[ts_idx].pes_list = pes_idx;
        return;
    }

    int idx;
    while ( ii != -1 )
    {
        if ( ii == pes_idx ) // Already in list
            return;
        idx = ii;
        ii = stream->pes.list[ii].next;
    }
    stream->pes.list[idx].next = pes_idx;
}

static int update_ts_streams( hb_stream_t * stream, int pid, int stream_id_ext, int stream_type, int in_kind, int *out_pes_idx )
{
    int ii;
    int pes_idx = update_ps_streams( stream, pid, stream_id_ext,
                                     stream_type, in_kind );

    if ( out_pes_idx )
        *out_pes_idx = pes_idx;

    if ( pes_idx < 0 )
        return -1;

    kind_t kind = stream->pes.list[pes_idx].stream_kind;
    for ( ii = 0; ii < stream->ts.count; ii++ )
    {
        if ( pid == stream->ts.list[ii].pid )
        {
            break;
        }
        // Resolve multiple videos
        if ( kind == V && ts_stream_kind( stream, ii ) == V &&
             pes_idx < stream->ts.list[ii].pes_list )
        {
            // We have a new candidate for the primary video.  Move
            // the current video to the end of the list. And put the
            // new video in this slot
            int jj = new_pid( stream );
            memcpy( &stream->ts.list[jj], &stream->ts.list[ii],
                    sizeof( hb_ts_stream_t ) );
            break;
        }
    }
    if ( ii == stream->ts.count )
        ii = new_pid( stream );

    stream->ts.list[ii].pid = pid;
    ts_pes_list_add( stream, ii, pes_idx );
    if ( in_kind == P )
        stream->ts.list[ii].is_pcr = 1;

    return ii;
}

static int decode_ps_map( hb_stream_t * stream, uint8_t *buf, int len )
{
    int retval = 1;
    bitbuf_t bb;
    bits_init(&bb, buf, len, 0);

    if ( bits_bytes_left(&bb) < 10 )
        return 0;

    // Skip stuff not needed
    bits_skip(&bb, 8 * 8);
    int info_len = bits_get(&bb, 16);
    if ( bits_bytes_left(&bb) < info_len )
        return 0;

    if ( info_len )
    {
        bitbuf_t cc;
        bits_clone( &cc, &bb, info_len );

        while ( bits_bytes_left(&cc) >= 2 )
        {
            uint8_t tag, len;

            tag = bits_get(&cc, 8);
            len = bits_get(&cc, 8);
            if ( bits_bytes_left(&cc) < len )
                return 0;

            if (tag == 0x05 && len >= 4)
            {
                // registration descriptor
                stream->reg_desc = bits_get(&cc, 32);
                bits_skip(&cc, 8 * (len - 4));
            }
            else
            {
                bits_skip(&cc, 8 * len);
            }
        }
        bits_skip(&bb, 8 * info_len);
    }

    int map_len = bits_get(&bb, 16);
    if ( bits_bytes_left(&bb) < map_len )
        return 0;

    // Process the map
    int ii = 0;
    while ( bits_bytes_left(&bb) >= 8 )
    {
        int pes_idx;
        int stream_type = bits_get(&bb, 8);
        int stream_id = bits_get(&bb, 8);
        info_len = bits_get(&bb, 16);
        if ( info_len > bits_bytes_left(&bb) )
            return 0;

        int substream_id = 0;
        switch ( stream_type )
        {
            case 0x81: // ac3
            case 0x82: // dts
            case 0x83: // lpcm
            case 0x87: // eac3
                // If the stream_id isn't one of the standard mpeg
                // stream ids, assume it is an private stream 1 substream id.
                // This is how most PS streams specify this type of audio.
                //
                // TiVo sets the stream id to 0xbd and does not
                // give a substream id.  This limits them to one audio
                // stream and differs from how everyone else specifies
                // this type of audio.
                if ( stream_id < 0xb9 )
                {
                    substream_id = stream_id;
                    stream_id = 0xbd;
                }
                break;
            default:
                break;
        }

        pes_idx = update_ps_streams( stream, stream_id, substream_id,
                                     stream_type, -1 );
        if ( pes_idx >= 0 )
            stream->pes.list[pes_idx].map_idx = ii;

        if ( info_len > 0 )
        {
            bitbuf_t bb_desc;
            bits_clone( &bb_desc, &bb, info_len );
            if ( pes_idx >= 0 )
                decode_element_descriptors( stream, pes_idx, &bb_desc );
            bits_skip(&bb, 8 * info_len);
        }
        ii++;
    }
    // skip CRC 32
    return retval;
}

static void hb_ps_stream_find_streams(hb_stream_t *stream)
{
    int ii, jj;
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);

    fseeko( stream->file_handle, 0, SEEK_SET );
    // Scan beginning of file, then if no program stream map is found
    // seek to 20% and scan again since there's occasionally no
    // audio at the beginning (particularly for vobs).
    for ( ii = 0; ii < 2; ii++ )
    {
        for ( jj = 0; jj < MAX_PS_PROBE_SIZE; jj += buf->size )
        {
            int stream_type;
            int len;

            hb_pes_info_t pes_info;
            buf->size = 0;
            len = hb_ps_read_packet( stream, buf );
            if ( len == 0 )
            {
                // Must have reached EOF
                break;
            }
            if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
            {
                hb_deep_log( 2, "hb_ps_stream_find_streams: Error parsing PS packet");
                continue;
            }
            if ( pes_info.stream_id == 0xba )
            {
                stream->ts_flags |= TS_HAS_PCR;
            }
            else if ( pes_info.stream_id == 0xbc )
            {
                // program stream map
                // Note that if there is a program map, any
                // extrapolation that is made below based on
                // stream id may be overridden by entry in the map.
                if ( decode_ps_map( stream, buf->data, buf->size ) )
                {
                    hb_log("Found program stream map");
                    // Normally, we could quit here since the program
                    // stream map *should* map all streams. But once
                    // again Tivo breaks things by not always creating
                    // complete maps.  So continue processing...
                }
                else
                {
                    hb_error("Error parsing program stream map");
                }
            }
            else if ( ( pes_info.stream_id & 0xe0 ) == 0xc0 )
            {
                // MPeg audio (c0 - df)
                stream_type = 0x04;
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
            else if ( pes_info.stream_id == 0xbd )
            {
                int ssid = pes_info.bd_substream_id;
                // Add a potential audio stream
                // Check dvd substream id
                if ( ssid >= 0x20 && ssid <= 0x37 )
                {
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, -1 );
                    stream->pes.list[idx].stream_kind = S;
                    stream->pes.list[idx].codec = WORK_DECAVSUB;
                    stream->pes.list[idx].codec_param = AV_CODEC_ID_DVD_SUBTITLE;
                    strncpy(stream->pes.list[idx].codec_name,
                            "DVD Subtitle", 80);
                    continue;
                }
                if ( ssid >= 0x80 && ssid <= 0x87 )
                {
                    stream_type = 0x81; // ac3
                }
                else if ( ( ssid >= 0x88 && ssid <= 0x8f ) ||
                          ( ssid >= 0x98 && ssid <= 0x9f ) )
                {
                    // Could be either dts or dts-hd
                    // will have to probe to resolve
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, U );
                    stream->pes.list[idx].codec = HB_ACODEC_DCA_HD;
                    stream->pes.list[idx].codec_param = AV_CODEC_ID_DTS;
                    continue;
                }
                else if ( ssid >= 0xa0 && ssid <= 0xaf )
                {
                    stream_type = 0x83; // lpcm
                    // This is flagged as an unknown stream type in
                    // st2codec because it can be either LPCM or
                    // BD TrueHD. In this case it is LPCM.
                    update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.bd_substream_id, stream_type, A );
                    continue;
                }
                else if ( ssid >= 0xb0 && ssid <= 0xbf )
                {
                    // HD-DVD TrueHD
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, A );
                    stream->pes.list[idx].codec       = HB_ACODEC_FFTRUEHD;
                    stream->pes.list[idx].codec_param = AV_CODEC_ID_TRUEHD;
                    continue;
                }
                else if ( ssid >= 0xc0 && ssid <= 0xcf )
                {
                    // HD-DVD uses this for both ac3 and eac3.
                    // Check ac3 bitstream_id to distinguish between them.
                    bitbuf_t bb;
                    bits_init(&bb, buf->data + pes_info.header_len,
                              buf->size - pes_info.header_len, 0);
                    int sync = bits_get(&bb, 16);
                    if ( sync == 0x0b77 )
                    {
                        bits_skip(&bb, 24);
                        int bsid = bits_get(&bb, 5);
                        if ( bsid <= 10 )
                        {
                            // ac3
                            stream_type = 0x81; // ac3
                        }
                        else
                        {
                            // eac3
                            stream_type = 0x87; // eac3
                        }
                    }
                    else
                    {
                        // Doesn't look like an ac3 stream.  Probe it.
                        stream_type = 0x00;
                    }
                }
                else
                {
                    // Unknown. Probe it.
                    stream_type = 0x00;
                }
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.bd_substream_id, stream_type, -1 );
            }
            else if ( ( pes_info.stream_id & 0xf0 ) == 0xe0 )
            {
                // Normally this is MPEG video, but MPEG-1 PS streams
                // (which do not have a program stream map)  may use
                // this for other types of video.
                //
                // Also, the hddvd folks decided to use 0xe2 and 0xe3 for
                // h.264 video :( and the folks decided not to put a
                // program stream map in the stream :'(
                //
                // So set this to an unknown stream type and probe.
                stream_type = 0x00;
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
            else if ( pes_info.stream_id == 0xfd )
            {
                if ( pes_info.stream_id_ext == 0x55 ||
                     pes_info.stream_id_ext == 0x56 )
                {
                    // hddvd uses this for vc-1.
                    stream_type = 0xea;
                }
                else
                {
                    // mark as unknown and probe.
                    stream_type = 0x00;
                }
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
        }
        hb_stream_seek( stream, 0.2 );
    }
    hb_buffer_close( &buf );
}

static int probe_dts_profile( hb_stream_t *stream, hb_pes_stream_t *pes )
{
    hb_work_info_t info;
    hb_work_object_t *w = hb_audio_decoder( stream->h, pes->codec );

    w->codec_param = pes->codec_param;
    int ret = w->bsinfo( w, pes->probe_buf, &info );
    if ( ret < 0 )
    {
        hb_log( "probe_dts_profile: no info type %d/0x%x for id 0x%x",
                pes->codec, pes->codec_param, pes->stream_id );

    }
    switch (info.profile)
    {
        case AV_PROFILE_DTS:
        case AV_PROFILE_DTS_ES:
        case AV_PROFILE_DTS_96_24:
        case AV_PROFILE_DTS_EXPRESS:
            pes->codec = HB_ACODEC_DCA;
            pes->stream_type = 0x82;
            pes->stream_kind = A;
            break;

        case AV_PROFILE_DTS_HD_HRA:
        case AV_PROFILE_DTS_HD_MA:
        case AV_PROFILE_DTS_HD_MA_X:
        case AV_PROFILE_DTS_HD_MA_X_IMAX:
            pes->stream_type = 0;
            pes->stream_kind = A;
            break;

        default:
            free(w);
            return 0;
    }
    const char *profile_name;
    const AVCodec *codec = avcodec_find_decoder( pes->codec_param );
    profile_name = av_get_profile_name( codec, info.profile );
    if ( profile_name )
    {
        strncpy(pes->codec_name, profile_name, 80);
        pes->codec_name[79] = 0;
    }
    free(w);
    return 1;
}

static int
do_deep_probe(hb_stream_t *stream, hb_pes_stream_t *pes)
{
    int       result = 0;
    const AVCodec *codec = avcodec_find_decoder(pes->codec_param);

    if (codec == NULL)
    {
        return -1;
    }

    AVCodecContext       * context = avcodec_alloc_context3(codec);
    AVCodecParserContext * parser  = av_parser_init(codec->id);

    if (context == NULL)
    {
        return -1;
    }
    if (parser == NULL)
    {
        return -1;
    }
    if (hb_avcodec_open(context, codec, NULL, 0))
    {
        return -1;
    }

    int pos = 0;
    while (pos < pes->probe_buf->size)
    {
        int       len, out_size;
        uint8_t * out;

        len = av_parser_parse2(parser, context, &out, &out_size,
                               pes->probe_buf->data + pos,
                               pes->probe_buf->size - pos, 0, 0, 0);
        pos += len;
        if (out_size == 0)
        {
            continue;
        }
        // Parser changes context->codec_id if it detects a different but
        // related stream type.  E.g. AV_CODEC_ID_MPEG2VIDEO gets changed
        // to AV_CODEC_ID_MPEG1VIDEO when the stream is MPEG-1
        switch (context->codec_id)
        {
            case AV_CODEC_ID_MPEG1VIDEO:
                pes->codec_param = context->codec_id;
                pes->stream_type = 0x01;
                pes->stream_kind = V;
                result = 1;
                break;

            case AV_CODEC_ID_MPEG2VIDEO:
                pes->codec_param = context->codec_id;
                pes->stream_type = 0x02;
                pes->stream_kind = V;
                result = 1;
                break;

            default:
                hb_error("do_deep_probe: unexpected codec_id (%d)",
                         context->codec_id);
                result = -1;
                break;
        }
    }
    av_parser_close(parser);
    hb_avcodec_free_context(&context);

    return result;
}

static int do_probe(hb_stream_t *stream, hb_pes_stream_t *pes, hb_buffer_t *buf)
{
    int result = 0;

    // Check upper limit of per stream data to probe
    if ( pes->probe_buf == NULL )
    {
        pes->probe_buf = hb_buffer_init( 0 );
        pes->probe_next_size = 0;
        pes->probe_count = 0;
    }

    if ( pes->probe_buf->size > HB_MAX_PROBE_SIZE )
    {
        // Max size reached before finding anything.  Try again from
        // another start PES
        pes->probe_count++;
        hb_buffer_close( &pes->probe_buf );
        if (pes->probe_count >= HB_MAX_PROBES)
        {
            return -1;
        }
        pes->probe_buf = hb_buffer_init( 0 );
        pes->probe_next_size = 0;
    }

    // Add this stream buffer to probe buffer and perform probe
    int size = pes->probe_buf->size + buf->size;

    hb_buffer_realloc(pes->probe_buf, size + AVPROBE_PADDING_SIZE );
    memcpy( pes->probe_buf->data + pes->probe_buf->size, buf->data, buf->size );
    pes->probe_buf->size = size;

    if ( pes->codec == HB_ACODEC_DCA_HD )
    {
        // We need to probe for the profile of DTS audio in this stream.
        return probe_dts_profile( stream, pes );
    }

    // Probing is slow, so we don't want to re-probe the probe
    // buffer for every packet we add to it.  Grow the buffer
    // by a factor of 2 before probing again.
    if ( pes->probe_buf->size < pes->probe_next_size )
        return 0;
    pes->probe_next_size = pes->probe_buf->size * 2;

    if (pes->codec_param != AV_CODEC_ID_NONE)
    {
        // Already did a format probe, but some stream types require a
        // deeper parser probe. E.g. MPEG-1/2, av_probe_input_format2
        // resolves to AV_CODEC_ID_MPEG2VIDEO for both MPEG-1 and MPEG-2
        result = do_deep_probe(stream, pes);
        if (result)
        {
            hb_buffer_close(&pes->probe_buf);
        }
        return result;
    }

    const AVInputFormat *fmt = NULL;
    int score = 0;
    AVProbeData pd = {0,};

    pd.buf = pes->probe_buf->data;
    pd.buf_size = pes->probe_buf->size;
    fmt = av_probe_input_format2( &pd, 1, &score );
    if ( fmt && score > AVPROBE_SCORE_MAX / 2 )
    {
        const AVCodec *codec = avcodec_find_decoder_by_name( fmt->name );
        if( !codec )
        {
            int i;
            static const struct
            {
                const char *name;
                enum AVCodecID id;
            }
            fmt_id_type[] =
            {
                { "g722"     , AV_CODEC_ID_ADPCM_G722 },
                { "mlp"      , AV_CODEC_ID_MLP        },
                { "truehd"   , AV_CODEC_ID_TRUEHD     },
                { "shn"      , AV_CODEC_ID_SHORTEN    },
                { "aac"      , AV_CODEC_ID_AAC        },
                { "ac3"      , AV_CODEC_ID_AC3        },
                { "dts"      , AV_CODEC_ID_DTS        },
                { "eac3"     , AV_CODEC_ID_EAC3       },
                { "h264"     , AV_CODEC_ID_H264       },
                { "m4v"      , AV_CODEC_ID_MPEG4      },
                { "mp2"      , AV_CODEC_ID_MP2        },
                { "mp3"      , AV_CODEC_ID_MP3        },
                { "mpegvideo", AV_CODEC_ID_MPEG2VIDEO },
                { "cavsvideo", AV_CODEC_ID_CAVS       },
                { "dnxhd"    , AV_CODEC_ID_DNXHD      },
                { "h261"     , AV_CODEC_ID_H261       },
                { "h263"     , AV_CODEC_ID_H263       },
                { "mjpeg"    , AV_CODEC_ID_MJPEG      },
                { "vc1"      , AV_CODEC_ID_VC1        },
                { 0 },
            };
            for( i = 0; fmt_id_type[i].name; i++ )
            {
                if( !strcmp(fmt->name, fmt_id_type[i].name ) )
                {
                    codec = avcodec_find_decoder( fmt_id_type[i].id );
                    break;
                }
            }
        }
        if( codec )
        {
            pes->codec_param = codec->id;
            if ( codec->type == AVMEDIA_TYPE_VIDEO )
            {
                switch ( codec->id )
                {
                    case AV_CODEC_ID_MPEG1VIDEO:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0x01;
                        pes->stream_kind = V;
                        break;

                    case AV_CODEC_ID_MPEG2VIDEO:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0x02;
                        break;

                    case AV_CODEC_ID_H264:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0x1b;
                        pes->stream_kind = V;
                        break;

                    case AV_CODEC_ID_VC1:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0xea;
                        pes->stream_kind = V;
                        break;

                    default:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_kind = V;
                        break;
                }
            }
            else if ( codec->type == AVMEDIA_TYPE_AUDIO )
            {
                pes->stream_kind = A;
                switch ( codec->id )
                {
                    case AV_CODEC_ID_AC3:
                        pes->codec = HB_ACODEC_AC3;
                        break;
                    default:
                        pes->codec = HB_ACODEC_FFMPEG;
                }
            }
            strncpy(pes->codec_name, codec->name, 79);
            pes->codec_name[79] = 0;
        }
        if (pes->codec_param != AV_CODEC_ID_MPEG2VIDEO)
        {
            hb_buffer_close( &pes->probe_buf );
            result = 1;
        }
    }
    return result;
}

static void hb_ts_resolve_pid_types(hb_stream_t *stream)
{
    int ii, probe = 0;

    for ( ii = 0; ii < stream->ts.count; ii++ )
    {
        int pid = stream->ts.list[ii].pid;
        int stype = ts_stream_type( stream, ii );
        int pes_idx;

        if ( stype == 0x80 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // LPCM audio in bluray have an stype of 0x80
            // 0x80 is used for other DigiCipher normally
            // To distinguish, Bluray streams have a reg_desc of HDMV
            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_FFMPEG;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_PCM_BLURAY;
            continue;
        }

        // The blu ray consortium apparently forgot to read the portion
        // of the MPEG spec that says one PID should map to one media
        // stream and multiplexed multiple types of audio into one PID
        // using the extended stream identifier of the PES header to
        // distinguish them. So we have to check if that's happening and
        // if so tell the runtime what esid we want.
        if ( stype == 0x83 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved TrueHD/AC-3 stream and the esid of
            // the AC-3 is 0x76
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_AC3,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_AC3;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_AC3;

            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_TRUEHD,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec       = HB_ACODEC_FFTRUEHD;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_TRUEHD;
            continue;
        }
        if ( ( stype == 0x84 || stype == 0xa1 ) &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // EAC3 audio in bluray has an stype of 0x84
            // which conflicts with SDDS
            // To distinguish, Bluray streams have a reg_desc of HDMV
            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec       = HB_ACODEC_FFEAC3;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_EAC3;
            continue;
        }
        if ( stype == 0xa2 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // Blueray DTS-HD LBR audio
            // This is no interleaved DTS core
            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA_HD;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DTS;
            continue;
        }
        if ( stype == 0x85 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // DTS-HD HRA audio in bluray has an stype of 0x85
            // which conflicts with ATSC Program ID
            // To distinguish, Bluray streams have a reg_desc of HDMV
            // This is an interleaved DTS-HD HRA/DTS stream and the
            // esid of the DTS is 0x71
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_DTS,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DTS;

            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA_HD;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DTS;
            continue;
        }
        if ( stype == 0x86 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved DTS-HD MA/DTS stream and the
            // esid of the DTS is 0x71
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_DTS,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DTS;

            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA_HD;
            stream->pes.list[pes_idx].codec_param = AV_CODEC_ID_DTS;
            continue;
        }

        // stype == 0 indicates a type not in st2codec table
        if ( stype != 0 &&
             ( ts_stream_kind( stream, ii ) == A ||
               ts_stream_kind( stream, ii ) == S ||
               ts_stream_kind( stream, ii ) == V ) )
        {
            // Assuming there are no substreams.
            // This should be true before probing.
            // This function is only called before
            // probing.
            pes_idx = stream->ts.list[ii].pes_list;
            stream->pes.list[pes_idx].codec = st2codec[stype].codec;
            stream->pes.list[pes_idx].codec_param = st2codec[stype].codec_param;
            continue;
        }

        if ( ts_stream_kind( stream, ii ) == U )
        {
            probe++;
        }
    }

    // Probe remaining unknown streams for stream types
    hb_stream_seek( stream, 0.0 );
    stream->need_keyframe = 0;

    hb_buffer_t *buf;

    if ( probe )
        hb_log("Probing %d unknown stream%s", probe, probe > 1 ? "s" : "" );

    while ( probe && ( buf = hb_ts_stream_decode( stream ) ) != NULL )
    {
        int idx, result;
        idx = index_of_id( stream, buf->s.id );

        if (idx < 0 || stream->pes.list[idx].stream_kind != U )
        {
            hb_buffer_close(&buf);
            continue;
        }

        hb_pes_stream_t *pes = &stream->pes.list[idx];

        result = do_probe(stream, pes, buf);
        if (result < 0)
        {
            hb_log("    Probe: Unsupported stream %s. stream id 0x%x-0x%x",
                    pes->codec_name, pes->stream_id, pes->stream_id_ext);
            pes->stream_kind = N;
            probe--;
        }
        else if (result && pes->stream_kind != U)
        {
            hb_log("    Probe: Found stream %s. stream id 0x%x-0x%x",
                    pes->codec_name, pes->stream_id, pes->stream_id_ext);
            probe--;
        }
        hb_buffer_close(&buf);
    }
    // Clean up any probe buffers and set all remaining unknown
    // streams to 'kind' N
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == U )
            stream->pes.list[ii].stream_kind = N;
        hb_buffer_close( &stream->pes.list[ii].probe_buf );
        stream->pes.list[ii].probe_next_size = 0;
    }
}

static void hb_ps_resolve_stream_types(hb_stream_t *stream)
{
    int ii, probe = 0;

    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        int stype = stream->pes.list[ii].stream_type;

        // stype == 0 indicates a type not in st2codec table
        if ( stype != 0 &&
             ( stream->pes.list[ii].stream_kind == A ||
               stream->pes.list[ii].stream_kind == S ||
               stream->pes.list[ii].stream_kind == V ) )
        {
            stream->pes.list[ii].codec = st2codec[stype].codec;
            stream->pes.list[ii].codec_param = st2codec[stype].codec_param;
            continue;
        }

        if ( stream->pes.list[ii].stream_kind == U )
        {
            probe++;
        }
    }

    // Probe remaining unknown streams for stream types
    hb_stream_seek( stream, 0.0 );
    stream->need_keyframe = 0;

    hb_buffer_t *buf;

    if ( probe )
        hb_log("Probing %d unknown stream%s", probe, probe > 1 ? "s" : "" );

    while ( probe && ( buf = hb_ps_stream_decode( stream ) ) != NULL )
    {
        int idx, result;
        idx = index_of_id( stream, buf->s.id );

        if (idx < 0 || stream->pes.list[idx].stream_kind != U )
        {
            hb_buffer_close(&buf);
            continue;
        }

        hb_pes_stream_t *pes = &stream->pes.list[idx];

        result = do_probe(stream, pes, buf);
        if (result < 0)
        {
            hb_log("    Probe: Unsupported stream %s. stream id 0x%x-0x%x",
                    pes->codec_name, pes->stream_id, pes->stream_id_ext);
            pes->stream_kind = N;
            probe--;
        }
        else if (result && pes->stream_kind != U)
        {
            hb_log("    Probe: Found stream %s. stream id 0x%x-0x%x",
                    pes->codec_name, pes->stream_id, pes->stream_id_ext);
            probe--;
        }
        hb_buffer_close(&buf);
    }
    // Clean up any probe buffers and set all remaining unknown
    // streams to 'kind' N
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == U )
            stream->pes.list[ii].stream_kind = N;
        hb_buffer_close( &stream->pes.list[ii].probe_buf );
        stream->pes.list[ii].probe_next_size = 0;
    }
}


static int hb_ts_stream_find_pids(hb_stream_t *stream)
{
    // To be different from every other broadcaster in the world, New Zealand TV
    // changes PMTs (and thus video & audio PIDs) when 'programs' change. Since
    // we may have the tail of the previous program at the beginning of this
    // file, take our PMT from the middle of the file.
    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    fseeko(stream->file_handle, fsize >> 1, SEEK_SET);
    align_to_next_packet(stream);

    // Read the Transport Stream Packets (188 bytes each) looking at first for PID 0 (the PAT PID), then decode that
    // to find the program map PID and then decode that to get the list of audio and video PIDs

    for (;;)
    {
        const uint8_t *buf = next_packet( stream );

        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_find_pids - end of file");
            break;
        }

        // Get pid
        int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;

        if ((pid == 0x0000) && (stream->ts_number_pat_entries == 0))
        {
            decode_PAT(buf, stream);
            continue;
        }

        int pat_index = 0;
        for (pat_index = 0; pat_index < stream->ts_number_pat_entries; pat_index++)
        {
            // There are some streams where the PAT table has multiple
            // entries as if their are multiple programs in the same
            // transport stream, and yet there's actually only one
            // program really in the stream. This seems to be true for
            // transport streams that originate in the HDHomeRun but have
            // been output by EyeTV's export utility. What I think is
            // happening is that the HDHomeRun is sending the entire
            // transport stream as broadcast, but the EyeTV is only
            // recording a single (selected) program number and not
            // rewriting the PAT info on export to match what's actually
            // on the stream. Until we have a way of handling multiple
            // programs per transport stream elegantly we'll match on the
            // first pat entry for which we find a matching program map PID.
            // The ideal solution would be to build a title choice popup
            // from the PAT program number details and then select from
            // their - but right now the API's not capable of that.
            if (stream->pat_info[pat_index].program_number != 0 &&
                pid == stream->pat_info[pat_index].program_map_PID)
            {
                if (build_program_map(buf, stream) > 0)
                {
                    break;
                }
            }
        }
        // Keep going  until we have a complete set of PIDs
        if ( ts_index_of_video( stream ) >= 0 )
          break;
    }
    if ( ts_index_of_video( stream ) < 0 )
        return -1;
    update_ts_streams( stream, stream->pmt_info.PCR_PID, 0, -1, P, NULL );
    return 0;
}


// convert a PES PTS or DTS to an int64
static int64_t pes_timestamp( const uint8_t *buf )
{
    int64_t ts;

    ts = ( (uint64_t)  ( buf[0] & 0x0e ) << 29 ) +
                       ( buf[1] <<  22 )         +
                     ( ( buf[2] >>   1 ) << 15 ) +
                       ( buf[3] <<   7 )         +
                       ( buf[4] >>   1 );
    return ts;
}

static int stream_kind_to_buf_type(int kind)
{
    switch (kind)
    {
        case A:
            return AUDIO_BUF;
        case V:
            return VIDEO_BUF;
        case S:
            return SUBTITLE_BUF;
        default:
            return OTHER_BUF;
    }
}

static hb_buffer_t * generate_output_data(hb_stream_t *stream, int curstream)
{
    hb_buffer_list_t list;
    hb_buffer_t *buf = NULL;

    hb_buffer_list_clear(&list);
    hb_ts_stream_t * ts_stream = &stream->ts.list[curstream];
    hb_buffer_t * b = ts_stream->buf;
    if (!ts_stream->pes_info_valid)
    {
        if (!hb_parse_ps(stream, b->data, b->size, &ts_stream->pes_info))
        {
            b->size = 0;
            ts_stream->packet_len = 0;
            ts_stream->packet_offset = 0;
            return NULL;
        }
        ts_stream->pes_info_valid = 1;
        ts_stream->packet_offset = ts_stream->pes_info.header_len;
    }

    uint8_t *tdat = b->data + ts_stream->packet_offset;
    int es_size = b->size - ts_stream->packet_offset;

    if (ts_stream->packet_len >= ts_stream->pes_info.packet_len + 6)
    {
        // Remove trailing stuffing bytes (DVB subtitles)
        es_size -= ts_stream->pes_info.stuffing_len;
    }
    if (es_size <= 0)
    {
        if (ts_stream->pes_info.packet_len > 0 &&
            ts_stream->packet_len >= ts_stream->pes_info.packet_len + 6)
        {
            ts_stream->pes_info_valid = 0;
            ts_stream->packet_len = 0;
        }
        b->size = 0;
        ts_stream->packet_offset = 0;
        return NULL;
    }

    int pes_idx;
    pes_idx = ts_stream->pes_list;
    hb_pes_stream_t *pes_stream = &stream->pes.list[pes_idx];
    if (stream->need_keyframe)
    {
        // we're looking for the first video frame because we're
        // doing random access during 'scan'
        int kind = pes_stream->stream_kind;
        if (kind != V || !isIframe(stream, tdat, es_size))
        {
            // not the video stream or didn't find an I frame
            // but we'll only wait 255 video frames for an I frame.
            if (kind != V || ++stream->need_keyframe < 512)
            {
                b->size = 0;
                ts_stream->pes_info_valid = 0;
                ts_stream->packet_len = 0;
                ts_stream->packet_offset = 0;
                return NULL;
            }
        }
        stream->need_keyframe = 0;
    }

    // Some TS streams carry multiple substreams.  E.g. DTS-HD contains
    // a core DTS substream.  We demux these as separate streams here.
    // Check all substreams to see if this packet matches
    for (pes_idx = ts_stream->pes_list; pes_idx != -1;
         pes_idx = stream->pes.list[pes_idx].next)
    {
        hb_pes_stream_t *pes_stream = &stream->pes.list[pes_idx];
        if (pes_stream->stream_id_ext != ts_stream->pes_info.stream_id_ext &&
            pes_stream->stream_id_ext != 0)
        {
            continue;
        }
        // The substreams match.
        // Note that when stream->pes.list[pes_idx].stream_id_ext == 0,
        // we want the whole TS stream including all substreams.
        // DTS-HD is an example of this.

        buf = hb_buffer_init(es_size);
        if (ts_stream->packet_len < ts_stream->pes_info.packet_len + 6)
        {
            buf->s.split = 1;
        }
        hb_buffer_list_append(&list, buf);

        buf->s.id = get_id(pes_stream);
        buf->s.type = stream_kind_to_buf_type(pes_stream->stream_kind);
        buf->s.new_chap = b->s.new_chap;
        b->s.new_chap = 0;

        // put the PTS & possible DTS into 'start' & 'renderOffset'
        // only put timestamps on the first output buffer for this PES packet.
        if (ts_stream->packet_offset > 0)
        {
            buf->s.discontinuity = stream->ts.discontinuity;
            stream->ts.discontinuity = 0;
            buf->s.pcr = stream->ts.pcr;
            stream->ts.pcr = AV_NOPTS_VALUE;
            buf->s.start = ts_stream->pes_info.pts;
            buf->s.renderOffset = ts_stream->pes_info.dts;
            buf->s.duration = (int64_t)AV_NOPTS_VALUE;
        }
        else
        {
            buf->s.pcr = AV_NOPTS_VALUE;
            buf->s.start = AV_NOPTS_VALUE;
            buf->s.renderOffset = AV_NOPTS_VALUE;
        }
        // copy the elementary stream data into the buffer
        memcpy(buf->data, tdat, es_size);
    }

    if (ts_stream->pes_info.packet_len > 0 &&
        ts_stream->packet_len >= ts_stream->pes_info.packet_len + 6)
    {
        ts_stream->pes_info_valid = 0;
        ts_stream->packet_len = 0;
    }
    b->size = 0;
    ts_stream->packet_offset = 0;
    return hb_buffer_list_clear(&list);
}

static void hb_ts_stream_append_pkt(hb_stream_t *stream, int idx,
                                    const uint8_t *buf, int len)
{
    if (stream->ts.list[idx].skipbad || len <= 0)
        return;

    if (stream->ts.list[idx].buf->size + len > stream->ts.list[idx].buf->alloc)
    {
        int size;

        size = MAX(stream->ts.list[idx].buf->alloc * 2,
                   stream->ts.list[idx].buf->size + len);
        hb_buffer_realloc(stream->ts.list[idx].buf, size);
    }
    memcpy(stream->ts.list[idx].buf->data + stream->ts.list[idx].buf->size,
           buf, len);
    stream->ts.list[idx].buf->size += len;
    stream->ts.list[idx].packet_len += len;
}

static hb_buffer_t * flush_ts_streams( hb_stream_t *stream )
{
    hb_buffer_list_t list;
    hb_buffer_t *buf;
    int ii;

    hb_buffer_list_clear(&list);
    for (ii = 0; ii < stream->ts.count; ii++)
    {
        buf = generate_output_data(stream, ii);
        hb_buffer_list_append(&list, buf);
    }
    return hb_buffer_list_clear(&list);
}

/***********************************************************************
 * hb_ts_stream_decode
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_ts_decode_pkt( hb_stream_t *stream, const uint8_t * pkt,
                                int chapter, int discontinuity )
{
    /*
     * stash the output buffer pointer in our stream so we don't have to
     * pass it & its original value to everything we call.
     */
    int video_index = ts_index_of_video(stream);
    int curstream;
    hb_buffer_t *buf = NULL;
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);

    if (chapter > 0)
    {
        stream->chapter = chapter;
    }
    if (discontinuity)
    {
        // If there is a discontinuity, flush all data
        buf = flush_ts_streams(stream);
        hb_buffer_list_append(&list, buf);

        stream->ts.discontinuity = 1;
    }

    /* This next section validates the packet */

    // Get pid and use it to find stream state.
    int pid = ((pkt[1] & 0x1F) << 8) | pkt[2];
    if ( ( curstream = index_of_pid( stream, pid ) ) < 0 )
    {
        // Not a stream we care about
        return hb_buffer_list_clear(&list);
    }


    // Get error
    int errorbit = (pkt[1] & 0x80) != 0;
    if (errorbit)
    {
        ts_err( stream, curstream,  "packet error bit set");
        return hb_buffer_list_clear(&list);
    }

    // Get adaption header info
    int adaption = (pkt[3] & 0x30) >> 4;
    int adapt_len = 0;
    if (adaption == 0)
    {
        ts_err( stream, curstream,  "adaptation code 0");
        return hb_buffer_list_clear(&list);
    }
    else if (adaption == 0x2)
        adapt_len = 184;
    else if (adaption == 0x3)
    {
        adapt_len = pkt[4] + 1;
        if (adapt_len > 184)
        {
            ts_err( stream, curstream,  "invalid adapt len %d", adapt_len);
            return hb_buffer_list_clear(&list);
        }
    }

    if (adapt_len > 0)
    {
        if (pkt[5] & 0x40)
        {
            // found a random access point
        }
        // if there's an adaptation header & PCR_flag is set
        // get the PCR (Program Clock Reference)
        //
        // JAS: I have a badly mastered BD that does adaptation field
        // stuffing incorrectly which results in invalid PCRs.  Test
        // for all 0xff to guard against this.
        if (adapt_len > 7 && (pkt[5] & 0x10) != 0 &&
            !(pkt[5] == 0xff && pkt[6] == 0xff && pkt[7] == 0xff &&
              pkt[8] == 0xff && pkt[9] == 0xff && pkt[10] == 0xff))
        {
            // When we get a new pcr, we flush all data that was
            // referenced to the last pcr.  This makes it easier
            // for reader to resolve pcr discontinuities.
            buf = flush_ts_streams(stream);
            hb_buffer_list_append(&list, buf);

            int64_t pcr;
            pcr = ((uint64_t)pkt[ 6] << (33 -  8) ) |
                  ((uint64_t)pkt[ 7] << (33 - 16) ) |
                  ((uint64_t)pkt[ 8] << (33 - 24) ) |
                  ((uint64_t)pkt[ 9] << (33 - 32) ) |
                  (          pkt[10] >> 7         );
            stream->ts.found_pcr = 1;
            stream->ts_flags |= TS_HAS_PCR;
            stream->ts.pcr = pcr;
        }
    }

    // If we don't have a PCR yet but the stream has PCRs just loop
    // so we don't process anything until we have a clock reference.
    // Unfortunately the HD Home Run appears to null out the PCR so if
    // we didn't detect a PCR during scan keep going and we'll use
    // the video stream DTS for the PCR.
    if (!stream->ts.found_pcr && (stream->ts_flags & TS_HAS_PCR))
    {
        return hb_buffer_list_clear(&list);
    }

    // Get continuity
    // Continuity only increments for adaption values of 0x3 or 0x01
    // and is not checked for start packets.
    hb_ts_stream_t * ts_stream = &stream->ts.list[curstream];
    ts_stream->start = ts_stream->start || ((pkt[1] & 0x40) != 0);

    if ( (adaption & 0x01) != 0 )
    {
        int continuity = (pkt[3] & 0xF);
        if ( continuity == ts_stream->continuity )
        {
            // Spliced transport streams can have duplicate
            // continuity counts at the splice boundary.
            // Test to see if the packet is really a duplicate
            // by comparing packet summaries to see if they
            // match.
            uint8_t summary[8];

            summary[0] = adaption;
            summary[1] = adapt_len;
            if (adapt_len + 4 + 6 + 9 <= 188)
            {
                memcpy(&summary[2], pkt+4+adapt_len+9, 6);
            }
            else
            {
                memset(&summary[2], 0, 6);
            }
            if ( memcmp( summary, ts_stream->pkt_summary, 8 ) == 0 )
            {
                // we got a duplicate packet (usually used to introduce
                // a PCR when one is needed). The only thing that can
                // change in the dup is the PCR which we grabbed above
                // so ignore the rest.
                return hb_buffer_list_clear(&list);
            }
        }
        if ( !ts_stream->start && (ts_stream->continuity != -1) &&
             !ts_stream->skipbad &&
             (continuity != ( (ts_stream->continuity + 1) & 0xf ) ) )
        {
            if (continuity == ts_stream->continuity)
            {
                // Duplicate packet as defined by ITU-T Rec. H.222
                // Drop the packet.
                return hb_buffer_list_clear(&list);
            }
            ts_warn( stream, "continuity error: got %d expected %d",
                    (int)continuity, (ts_stream->continuity + 1) & 0xf );
            ts_stream->continuity = continuity;
        }
        ts_stream->continuity = continuity;

        // Save a summary of this packet for later duplicate
        // testing.  The summary includes some header information
        // and payload bytes.  Should be enough to detect
        // non-duplicates.
        ts_stream->pkt_summary[0] = adaption;
        ts_stream->pkt_summary[1] = adapt_len;
        if (adapt_len + 4 + 6 + 9 <= 188)
        {
            memcpy(&ts_stream->pkt_summary[2],
                    pkt+4+adapt_len+9, 6);
        }
        else
        {
            memset(&ts_stream->pkt_summary[2], 0, 6);
        }
    }

    if (ts_stream_kind( stream, curstream ) == P)
    {
        // This is a stream that only contains PCRs.  No need to process
        // the remainder of the packet.
        //
        // I ran across a poorly mastered BD that does not properly pad
        // the adaptation field and causes parsing errors below if we
        // do not exit early here.
        return hb_buffer_list_clear(&list);
    }

    /* If we get here the packet is valid - process its data */
    if (ts_stream->start)
    {
        // Found the start of a new PES packet.
        // If we have previous packet data on this stream,
        // output the elementary stream data for that packet.
        if (ts_stream->buf->size > 0)
        {
            // we have to ship the old packet before updating the pcr
            // since the packet we've been accumulating is referenced
            // to the old pcr.
            buf = generate_output_data(stream, curstream);
            hb_buffer_list_append(&list, buf);
        }
        ts_stream->pes_info_valid = 0;
        ts_stream->packet_len = 0;

        // PES must begin with an mpeg start code
        const uint8_t *pes = pkt + adapt_len + 4;
        if (adapt_len + 4 + 3 > stream->packetsize)
        {
            return hb_buffer_list_clear(&list);
        }
        ts_stream->start = 0;
        if (pes[0] != 0x00 || pes[1] != 0x00 || pes[2] != 0x01)
        {
            ts_err( stream, curstream, "missing start code" );
            ts_stream->skipbad = 1;
            return hb_buffer_list_clear(&list);
        }

        // If we were skipping a bad packet, start fresh on this new PES packet
        ts_stream->skipbad = 0;

        if (curstream == video_index)
        {
            ++stream->frames;

            // if we don't have a pcr yet use the dts from this frame
            // to attempt to detect discontinuities
            if (!stream->ts.found_pcr)
            {
                // PES must begin with an mpeg start code & contain
                // a DTS or PTS.
                if (adapt_len + 4 + 19 >= stream->packetsize)
                {
                    return hb_buffer_list_clear(&list);
                }
                if (stream->ts.last_timestamp < 0 && (pes[7] >> 6) == 0)
                {
                    return hb_buffer_list_clear(&list);
                }
                if ((pes[7] >> 6) != 0)
                {
                    // if we have a dts use it otherwise use the pts
                    // We simulate a psuedo-PCR here by sampling a timestamp
                    // about every 600ms.
                    int64_t timestamp;
                    timestamp = pes_timestamp(pes + (pes[7] & 0x40 ? 14 : 9));
                    if (stream->ts.last_timestamp < 0 ||
                        timestamp - stream->ts.last_timestamp > 90 * 600 ||
                        stream->ts.last_timestamp - timestamp > 90 * 600)
                    {
                        stream->ts.pcr = timestamp;
                    }
                    stream->ts.last_timestamp = timestamp;
                }
            }
        }
    }

    // Add the payload for this packet to the current buffer
    hb_ts_stream_append_pkt(stream, curstream, pkt + 4 + adapt_len,
                            184 - adapt_len);
    if (stream->chapter > 0 &&
        stream->pes.list[ts_stream->pes_list].stream_kind == V)
    {
        ts_stream->buf->s.new_chap = stream->chapter;
        stream->chapter = 0;
    }

    if (!ts_stream->pes_info_valid && ts_stream->buf->size >= 19)
    {
        if (hb_parse_ps(stream, ts_stream->buf->data, ts_stream->buf->size,
                        &ts_stream->pes_info))
        {
            ts_stream->pes_info_valid = 1;
            ts_stream->packet_offset = ts_stream->pes_info.header_len;
        }
    }

    // see if we've hit the end of this PES packet
    if (ts_stream->pes_info_valid &&
        ts_stream->pes_info.packet_len > 0 &&
        ts_stream->packet_len >= ts_stream->pes_info.packet_len + 6)
    {
        buf = generate_output_data(stream, curstream);
        hb_buffer_list_append(&list, buf);
    }
    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * hb_ts_stream_decode( hb_stream_t *stream )
{
    hb_buffer_t * b;

    // spin until we get a packet of data from some stream or hit eof
    while ( 1 )
    {
        const uint8_t *buf = next_packet(stream);
        if ( buf == NULL )
        {
            // end of file - we didn't finish filling our ps write buffer
            // so just discard the remainder (the partial buffer is useless)
            hb_log("hb_ts_stream_decode - eof");
            b = flush_ts_streams(stream);
            return b;
        }

        b = hb_ts_decode_pkt( stream, buf, 0, 0 );
        if ( b )
        {
            return b;
        }
    }
    return NULL;
}

void hb_stream_set_need_keyframe(hb_stream_t *stream, int need_keyframe)
{
    if ( stream->hb_stream_type == transport ||
         stream->hb_stream_type == program )
    {
        // Only wait for a keyframe if the stream is known to have IDRs
        stream->need_keyframe = !!need_keyframe & !!stream->has_IDRs;
    }
    else
    {
        stream->need_keyframe = need_keyframe;
    }
}

void hb_ts_stream_reset(hb_stream_t *stream)
{
    int i;

    for (i=0; i < stream->ts.count; i++)
    {
        if ( stream->ts.list[i].buf )
            stream->ts.list[i].buf->size = 0;
        stream->ts.list[i].skipbad = 1;
        stream->ts.list[i].continuity = -1;
        stream->ts.list[i].pes_info_valid = 0;
    }

    stream->need_keyframe = 1;

    stream->ts.found_pcr = 0;
    stream->ts.pcr = AV_NOPTS_VALUE;
    stream->ts.last_timestamp = AV_NOPTS_VALUE;

    stream->frames = 0;
    stream->errors = 0;
    stream->last_error_frame = -10000;
    stream->last_error_count = 0;
}

void hb_ps_stream_reset(hb_stream_t *stream)
{
    stream->need_keyframe = 1;

    stream->pes.found_scr = 0;
    stream->pes.scr = AV_NOPTS_VALUE;

    stream->frames = 0;
    stream->errors = 0;
}

// ------------------------------------------------------------------
// Support for reading media files via the ffmpeg libraries.

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title, int scan )
{
    AVFormatContext *info_ic = NULL;

    av_log_set_level( AV_LOG_ERROR );

    // Increase probe buffer size
    // The default (5MB) is not big enough to successfully scan
    // some files with large PNGs
    AVDictionary * av_opts = NULL;
    av_dict_set( &av_opts, "probesize", "15000000", 0 );

    // FFMpeg has issues with seeking.  After av_find_stream_info, the
    // streams are left in an indeterminate position.  So a seek is
    // necessary to force things back to the beginning of the stream.
    // But then the seek fails for some stream types.  So the safest thing
    // to do seems to be to open 2 AVFormatContext.  One for probing info
    // and the other for reading.
    if ( avformat_open_input( &info_ic, stream->path, NULL, &av_opts ) < 0 )
    {
        av_dict_free( &av_opts );
        return 0;
    }
    // libav populates av_opts with the things it didn't recognize.
    AVDictionaryEntry *t = NULL;
    while ((t = av_dict_get(av_opts, "", t, AV_DICT_IGNORE_SUFFIX)) != NULL)
    {
            hb_log("ffmpeg_open: unknown option '%s'", t->key);
    }
    av_dict_free( &av_opts );

    if (title->color_prim     == HB_COLR_PRI_UNSET &&
        title->color_transfer == HB_COLR_TRA_UNSET &&
        title->color_matrix   == HB_COLR_MAT_UNSET)
    {
        // Read the video track color info
        // before it's overwritten with the stream info
        // in avformat_find_stream_info
        for (int i = 0; i < info_ic->nb_streams; ++i)
        {
            AVStream *st = info_ic->streams[i];

            if ( st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
                !(st->disposition & AV_DISPOSITION_ATTACHED_PIC) &&
                avcodec_find_decoder(st->codecpar->codec_id))
            {
                AVCodecParameters *codecpar = st->codecpar;
                title->color_prim     = codecpar->color_primaries;
                title->color_transfer = codecpar->color_trc;
                title->color_matrix   = codecpar->color_space;
                title->color_range    = codecpar->color_range;
                break;
            }
        }
    }

    if ( avformat_find_stream_info( info_ic, NULL ) < 0 )
        goto fail;

    title->opaque_priv = (void*)info_ic;
    stream->ffmpeg_ic = info_ic;
    stream->hb_stream_type = ffmpeg;
    stream->chapter_end = INT64_MAX;
    stream->ffmpeg_pkt = av_packet_alloc();

    if (stream->ffmpeg_pkt == NULL)
    {
        hb_error("stream: av_packet_alloc failed");
        goto fail;
    }

    if ( !scan )
    {
        // we're opening for read. scan passed out codec params that
        // indexed its stream so we need to remap them so they point
        // to this stream.
        stream->ffmpeg_video_id = title->video_id;
        av_log_set_level( AV_LOG_ERROR );
    }
    else
    {
        // we're opening for scan. let ffmpeg put some info into the
        // log about what we've got.
        stream->ffmpeg_video_id = title->video_id;
        av_log_set_level( AV_LOG_INFO );
        av_dump_format( info_ic, 0, stream->path, 0 );
        av_log_set_level( AV_LOG_ERROR );

        // accept this file if it has at least one video stream we can decode
        int i;
        for (i = 0; i < info_ic->nb_streams; ++i )
        {
            if (info_ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                break;
            }
        }
        if ( i >= info_ic->nb_streams )
            goto fail;
    }
    return 1;

  fail:
    avformat_close_input(&info_ic);
    av_packet_free(&stream->ffmpeg_pkt);
    return 0;
}

static void ffmpeg_close( hb_stream_t *d )
{
    avformat_close_input( &d->ffmpeg_ic );
    av_packet_free(&d->ffmpeg_pkt);
}

// Track names can be in multiple metadata entries, one per
// language that the track name is translated to.
//
// HandBrake only supports one track name which we write with the lang "und"
//
// Search for best candidate track name from the available options.
static const char * ffmpeg_track_name(AVStream * st, const char * lang)
{
    AVDictionaryEntry * t;
    char              * key;

    // Use key with no language extension
    // ffmpeg sets this for "und" entries or when source format
    // doesn't have a language field
    t = av_dict_get(st->metadata, "title", NULL, 0);
    if (t != NULL && t->value[0] != 0)
    {
        return t->value;
    }
    // Try explicit "und" extension
    t = av_dict_get(st->metadata, "title-und", NULL, 0);
    if (t != NULL && t->value[0] != 0)
    {
        return t->value;
    }
    // Try source track language
    key = hb_strdup_printf("title-%s", lang);
    t = av_dict_get(st->metadata, key, NULL, 0);
    free(key);
    if (t != NULL && t->value[0] != 0)
    {
        return t->value;
    }
    while ((t = av_dict_get(st->metadata, "title-", t, AV_DICT_IGNORE_SUFFIX)))
    {
        // Use first available
        if (t != NULL && t->value[0] != 0)
        {
            return t->value;
        }
    }
    return NULL;
}

static void add_ffmpeg_linked_audio(hb_audio_t * audio, int audio_index, hb_list_t * list_linked_index)
{
    if (audio->config.list_linked_index != NULL)
    {
	return;
    }
    audio->config.list_linked_index = hb_list_init();
    hb_list_add_dup(audio->config.list_linked_index, &audio_index, sizeof(audio_index));

    int ii;
    int linked_count = hb_list_count(list_linked_index);
    for (ii = 0; ii < linked_count; ii++)
    {
	int linked_index = *(int*)hb_list_item(list_linked_index, ii);
	if (linked_index != audio_index && linked_index != audio->config.index)
	{
            hb_list_add_dup(audio->config.list_linked_index, &linked_index, sizeof(audio_index));
	}
    }
}

static void find_ffmpeg_fallback_audio(hb_title_t *title)
{
    int count = hb_list_count(title->list_audio);
    int ii, jj, kk;

    for (ii = 0; ii < count; ii++)
    {
        hb_audio_t *audio_ii = hb_list_item(title->list_audio, ii);

        if (audio_ii->config.list_linked_index != NULL)
        {
            hb_list_t * list_linked_index = audio_ii->config.list_linked_index;
            hb_list_t * new_list_linked_index = hb_list_init();
            int         linked_count = hb_list_count(list_linked_index);

            for (jj = 0; jj < linked_count; jj++)
            {
                int linked_index = *(int*)hb_list_item(list_linked_index, jj);
                for (kk = 0; kk < count; kk++)
                {
                    hb_audio_t *audio_kk = hb_list_item(title->list_audio, kk);
                    if (linked_index == audio_kk->id && kk != ii)
                    {
                        hb_list_add_dup(new_list_linked_index, &kk, sizeof(kk));
                        add_ffmpeg_linked_audio(audio_kk, ii, list_linked_index);
                        break;
                    }
                }
            }
            int * item;
            while ((item = hb_list_item(list_linked_index, 0)))
            {
                hb_list_rem(list_linked_index, item);
                free(item);
            }
            hb_list_close(&audio_ii->config.list_linked_index);
            if (hb_list_count(new_list_linked_index) > 0)
            {
                audio_ii->config.list_linked_index = new_list_linked_index;
            }
            else
            {
                hb_list_close(&new_list_linked_index);
            }
        }
    }
}

static void add_ffmpeg_audio(hb_title_t *title, hb_stream_t *stream, int id)
{
    AVStream *st                = stream->ffmpeg_ic->streams[id];
    AVCodecParameters * codecpar = st->codecpar;
    AVDictionaryEntry * tag_lang = av_dict_get(st->metadata, "language", NULL, 0);
    iso639_lang_t     * lang = lang_for_code2(tag_lang != NULL ?
                                              tag_lang->value : "und");
    const char        * name = ffmpeg_track_name(st, lang->iso639_2);

    hb_audio_t        * audio = calloc(1, sizeof(*audio));
    int ii;
    for (ii = 0; ii < st->codecpar->nb_coded_side_data; ii++)
    {
        AVPacketSideData *sd = &st->codecpar->coded_side_data[ii];
        switch (sd->type)
        {
            case AV_PKT_DATA_FALLBACK_TRACK:
            {
                if (sd->size == sizeof(int))
                {
                    int fallback = *(int*)sd->data;
                    if (audio->config.list_linked_index == NULL)
                    {
                        audio->config.list_linked_index = hb_list_init();
                    }
                    // This is not actually the "right" index to store
                    // in list_linked_index. But we need the complete
                    // audio list before we can correct it.
                    //
                    // This gets corrected in find_ffmpeg_fallback_audio
                    // after all tracks have been scanned.
                    hb_list_add_dup(audio->config.list_linked_index,
                                    &fallback, sizeof(fallback));
                }
            } break;

            default:
                break;
        }
    }
    audio->id                      = id;
    audio->config.index            = hb_list_count(title->list_audio);
    audio->config.in.track         = id;
    audio->config.in.codec         = HB_ACODEC_FFMPEG;
    audio->config.in.codec_param   = codecpar->codec_id;
    // set the bitrate to 0; decavcodecaBSInfo will be called and fill the rest
    audio->config.in.bitrate       = 0;
    audio->config.in.encoder_delay = codecpar->initial_padding;

    // If we ever improve our pipeline to allow other time bases...
    // audio->config.in.timebase.num  = st->time_base.num;
    // audio->config.in.timebase.den  = st->time_base.den;
    audio->config.in.timebase.num  = 1;
    audio->config.in.timebase.den  = 90000;

    // set the input codec and extradata for Passthru
    switch (codecpar->codec_id)
    {
        case AV_CODEC_ID_AAC:
        {
            hb_set_extradata(&audio->priv.extradata, codecpar->extradata, codecpar->extradata_size);
            audio->config.in.codec = HB_ACODEC_FFAAC;
        } break;

        case AV_CODEC_ID_AC3:
            audio->config.in.codec = HB_ACODEC_AC3;
            break;

        case AV_CODEC_ID_EAC3:
            audio->config.in.codec = HB_ACODEC_FFEAC3;
            break;

        case AV_CODEC_ID_TRUEHD:
            audio->config.in.codec = HB_ACODEC_FFTRUEHD;
            break;

        case AV_CODEC_ID_DTS:
        {
            switch (codecpar->profile)
            {
                case AV_PROFILE_DTS:
                case AV_PROFILE_DTS_ES:
                case AV_PROFILE_DTS_96_24:
                case AV_PROFILE_DTS_EXPRESS:
                    audio->config.in.codec = HB_ACODEC_DCA;
                    break;

                case AV_PROFILE_DTS_HD_MA:
                case AV_PROFILE_DTS_HD_HRA:
                case AV_PROFILE_DTS_HD_MA_X:
                case AV_PROFILE_DTS_HD_MA_X_IMAX:
                    audio->config.in.codec = HB_ACODEC_DCA_HD;
                    break;

                default:
                    break;
            }
        } break;

        case AV_CODEC_ID_FLAC:
        {
            hb_set_extradata(&audio->priv.extradata, codecpar->extradata, codecpar->extradata_size);
            audio->config.in.codec = HB_ACODEC_FFFLAC;
        } break;

        case AV_CODEC_ID_MP2:
            audio->config.in.codec = HB_ACODEC_MP2;
            break;

        case AV_CODEC_ID_MP3:
            audio->config.in.codec = HB_ACODEC_MP3;
            break;

        case AV_CODEC_ID_OPUS:
        {
            hb_set_extradata(&audio->priv.extradata, codecpar->extradata, codecpar->extradata_size);
            audio->config.in.codec = HB_ACODEC_OPUS;
        } break;

        default:
            break;
    }

    if (st->disposition & AV_DISPOSITION_DEFAULT)
    {
        audio->config.lang.attributes |= HB_AUDIO_ATTR_DEFAULT;
    }

    set_audio_description(audio, lang);
    if (name != NULL)
    {
        audio->config.in.name = strdup(name);
    }
    hb_list_add(title->list_audio, audio);
}

/*
 * Format:
 *   MkvVobSubtitlePrivateData = ( Line )*
 *   Line = FieldName ':' ' ' FieldValue '\n'
 *   FieldName = [^:]+
 *   FieldValue = [^\n]+
 *
 * The line of interest is:
 *   PaletteLine = "palette" ':' ' ' RRGGBB ( ',' ' ' RRGGBB )*
 *
 * More information on the format at:
 *   http://www.matroska.org/technical/specs/subtitles/images.html
 */
static int ffmpeg_parse_vobsub_extradata_mkv( AVCodecParameters *codecpar,
                                              hb_subtitle_t *subtitle )
{
    // lines = (string) codecpar->extradata;
    char *lines = malloc( codecpar->extradata_size + 1 );
    if ( lines == NULL )
        return 1;
    memcpy( lines, codecpar->extradata, codecpar->extradata_size );
    lines[codecpar->extradata_size] = '\0';

    uint32_t rgb[16];
    int gotPalette = 0;
    int gotDimensions = 0;

    char *curLine, *curLine_parserData;
    for ( curLine = strtok_r( lines, "\n", &curLine_parserData );
          curLine;
          curLine = strtok_r( NULL, "\n", &curLine_parserData ) )
    {
        if (!gotPalette)
        {
            int numElementsRead = sscanf(curLine, "palette: "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x",
                &rgb[0],  &rgb[1],  &rgb[2],  &rgb[3],
                &rgb[4],  &rgb[5],  &rgb[6],  &rgb[7],
                &rgb[8],  &rgb[9],  &rgb[10], &rgb[11],
                &rgb[12], &rgb[13], &rgb[14], &rgb[15]);

            if (numElementsRead == 16) {
                gotPalette = 1;
            }
        }
        if (!gotDimensions)
        {
            int numElementsRead = sscanf(curLine, "size: %dx%d",
                &subtitle->width, &subtitle->height);

            if (numElementsRead == 2) {
                gotDimensions = 1;
            }
        }
        if (gotPalette && gotDimensions)
            break;
    }

    if (subtitle->width == 0 || subtitle->height == 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }

    free( lines );

    if ( gotPalette )
    {
        int i;
        for (i=0; i<16; i++)
            subtitle->palette[i] = hb_rgb2yuv(rgb[i]);
        subtitle->palette_set = 1;
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
 * Format: 8-bit {0,Y,Cb,Cr} x 16
 */
static int ffmpeg_parse_vobsub_extradata_mp4( AVCodecParameters *codecpar,
                                              hb_subtitle_t *subtitle )
{
    if ( codecpar->extradata_size != 4*16 )
        return 1;

    int i, j;
    for ( i=0, j=0; i<16; i++, j+=4 )
    {
        subtitle->palette[i] =
            codecpar->extradata[j+1] << 16 |   // Y
            codecpar->extradata[j+2] << 8  |   // Cb
            codecpar->extradata[j+3] << 0;     // Cr
        subtitle->palette_set = 1;
    }
    if (codecpar->width <= 0 || codecpar->height <= 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }
    else
    {
        subtitle->width = codecpar->width;
        subtitle->height = codecpar->height;
    }
    return 0;
}

/*
 * Parses the 'subtitle->palette' information from the specific VOB subtitle track's private data.
 * Returns 0 if successful or 1 if parsing failed or was incomplete.
 */
static int ffmpeg_parse_vobsub_extradata( AVCodecParameters *codecpar,
                                          hb_subtitle_t *subtitle )
{
    // XXX: Better if we actually chose the correct parser based on the input container
    return
        ffmpeg_parse_vobsub_extradata_mkv(codecpar, subtitle) &&
        ffmpeg_parse_vobsub_extradata_mp4(codecpar, subtitle);
}

static void add_ffmpeg_subtitle( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream          * st       = stream->ffmpeg_ic->streams[id];
    AVCodecParameters * codecpar = st->codecpar;
    AVDictionaryEntry * tag_lang = av_dict_get(st->metadata, "language", NULL, 0 );
    iso639_lang_t     * lang = lang_for_code2(tag_lang != NULL ?
                                              tag_lang->value : "und");
    const char        * name = ffmpeg_track_name(st, lang->iso639_2);

    hb_subtitle_t *subtitle = calloc( 1, sizeof(*subtitle) );

    subtitle->id = id;
    // If we ever improve our pipeline to allow other time bases...
    // subtitle->timebase.num = st->time_base.num;
    // subtitle->timebase.den = st->time_base.den;
    subtitle->timebase.num = 1;
    subtitle->timebase.den = 90000;

    switch ( codecpar->codec_id )
    {
        case AV_CODEC_ID_DVD_SUBTITLE:
            subtitle->format      = PICTURESUB;
            subtitle->source      = VOBSUB;
            subtitle->config.dest = RENDERSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = AV_CODEC_ID_DVD_SUBTITLE;
            if (ffmpeg_parse_vobsub_extradata(codecpar, subtitle))
            {
                hb_log( "add_ffmpeg_subtitle: malformed extradata for VOB subtitle track; "
                        "subtitle colors likely to be wrong" );
            }
            break;
        case AV_CODEC_ID_DVB_SUBTITLE:
            subtitle->format      = PICTURESUB;
            subtitle->source      = DVBSUB;
            subtitle->config.dest = RENDERSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = codecpar->codec_id;
            break;
        case AV_CODEC_ID_TEXT:
        case AV_CODEC_ID_SUBRIP:
            subtitle->format      = TEXTSUB;
            subtitle->source      = UTF8SUB;
            subtitle->config.dest = PASSTHRUSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = codecpar->codec_id;
            break;
        case AV_CODEC_ID_MOV_TEXT: // TX3G
            subtitle->format = TEXTSUB;
            subtitle->source = TX3GSUB;
            subtitle->config.dest = PASSTHRUSUB;
            subtitle->codec = WORK_DECTX3GSUB;
            break;
        case AV_CODEC_ID_ASS:
            subtitle->format      = TEXTSUB;
            subtitle->source      = SSASUB;
            subtitle->config.dest = PASSTHRUSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = codecpar->codec_id;
            break;
        case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
            subtitle->format      = PICTURESUB;
            subtitle->source      = PGSSUB;
            subtitle->config.dest = RENDERSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = codecpar->codec_id;
            break;
        case AV_CODEC_ID_EIA_608:
            subtitle->format      = TEXTSUB;
            subtitle->source      = CC608SUB;
            subtitle->config.dest = PASSTHRUSUB;
            subtitle->codec       = WORK_DECAVSUB;
            subtitle->codec_param = codecpar->codec_id;
            subtitle->attributes  = HB_SUBTITLE_ATTR_CC;
            break;
        default:
            hb_log( "add_ffmpeg_subtitle: unknown subtitle stream type: 0x%x",
                    (int) codecpar->codec_id );
            free(subtitle);
            return;
    }

    snprintf(subtitle->lang, sizeof( subtitle->lang ), "%s (%s)",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name,
             hb_subsource_name(subtitle->source));
    strncpy(subtitle->iso639_2, lang->iso639_2, 3);
    subtitle->iso639_2[3] = 0;
    if (name != NULL)
    {
        subtitle->name = strdup(name);
    }

    // Copy the extradata for the subtitle track
    if (codecpar->extradata != NULL)
    {
        hb_set_text_extradata(&subtitle->extradata, codecpar->extradata, codecpar->extradata_size);
    }

    if (st->disposition & AV_DISPOSITION_DEFAULT)
    {
        subtitle->config.default_track = 1;
        subtitle->attributes |= HB_SUBTITLE_ATTR_DEFAULT;
    }
    if (st->disposition & AV_DISPOSITION_FORCED)
    {
        subtitle->attributes |= HB_SUBTITLE_ATTR_FORCED;
    }

    subtitle->track = hb_list_count(title->list_subtitle);
    hb_list_add(title->list_subtitle, subtitle);
}

static char *get_ffmpeg_metadata_value( AVDictionary *m, char *key )
{
    AVDictionaryEntry *tag = NULL;

    while ( (tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)) )
    {
        if ( !strcmp( key, tag->key ) )
        {
            return tag->value;
        }
    }
    return NULL;
}

static void add_ffmpeg_attachment( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecParameters *codecpar = st->codecpar;

    enum attachtype type;
    const char *name = get_ffmpeg_metadata_value( st->metadata, "filename" );
    switch ( codecpar->codec_id )
    {
        case AV_CODEC_ID_TTF:
            // FFmpeg sets codec ID based on mime type of the attachment
            type = FONT_TTF_ATTACH;
            break;
        default:
        {
            int len = name ? strlen( name ) : 0;
            if( len >= 4 )
            {
                // Some attachments don't have the right mime type.
                // So also trigger on file name extension.
                if( !strcasecmp( name + len - 4, ".ttc" ) ||
                    !strcasecmp( name + len - 4, ".ttf" ) )
                {
                    type = FONT_TTF_ATTACH;
                    break;
                }
                else if( !strcasecmp( name + len - 4, ".otf" ) )
                {
                    type = FONT_OTF_ATTACH;
                    break;
                }
            }
            // Ignore unrecognized attachment type
            return;
        }
    }

    hb_attachment_t *attachment = calloc( 1, sizeof(*attachment) );

    // Copy the attachment name and data
    attachment->type = type;
    attachment->name = strdup( name );
    attachment->data = malloc( codecpar->extradata_size );
    memcpy( attachment->data, codecpar->extradata, codecpar->extradata_size );
    attachment->size = codecpar->extradata_size;

    hb_list_add(title->list_attachment, attachment);
}

static int ffmpeg_decmetadata( AVDictionary *m, hb_title_t *title )
{
    int result = 0;
    AVDictionaryEntry *tag = NULL;
    while ( (tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)) )
    {
        const char * hb_key;

        hb_key = hb_lookup_meta_key(tag->key);
        if (hb_key != NULL)
        {
            hb_update_meta_dict(title->metadata->dict, hb_key, tag->value);
        }
    }
    return result;
}

static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream, hb_title_t *title )
{
    AVFormatContext *ic = stream->ffmpeg_ic;

    // 'Barebones Title'
    title->type = HB_FF_STREAM_TYPE;

    // Copy part of the stream path to the title name
    char * name = stream->path;
    char * sep = hb_strr_dir_sep(stream->path);
    if (sep)
        name = sep + 1;
    title->name = strdup(name);
    char *dot_term = strrchr(title->name, '.');
    if (dot_term)
        *dot_term = '\0';

    if (ic->duration > 0)
    {
        uint64_t dur = ic->duration * 90000 / AV_TIME_BASE;
        title->duration = dur;
        dur /= 90000;
        title->hours    = dur / 3600;
        title->minutes  = ( dur % 3600 ) / 60;
        title->seconds  = dur % 60;
    }

    // set the title to decode the first video stream in the file
    title->demuxer = HB_NULL_DEMUXER;
    title->video_codec = 0;
    int i;
    for (i = 0; i < ic->nb_streams; ++i )
    {
        AVStream * st = ic->streams[i];

        if ( st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
           !(st->disposition & AV_DISPOSITION_ATTACHED_PIC) &&
             avcodec_find_decoder( st->codecpar->codec_id ) &&
             title->video_codec == 0 )
        {
            AVCodecParameters *codecpar = st->codecpar;
            // Check for unsupported pixel formats.
            // Exclude 'NONE' from check since we may not know this
            // information yet.
            if ( codecpar->format != AV_PIX_FMT_NONE &&
                 !sws_isSupportedInput( codecpar->format ) )
            {
                hb_log( "ffmpeg_title_scan: Unsupported pixel format (%d)",
                        codecpar->format );
                continue;
            }
            title->video_id = i;
            stream->ffmpeg_video_id = i;
            if ( st->sample_aspect_ratio.num &&
                 st->sample_aspect_ratio.den )
            {
                title->geometry.par.num = st->sample_aspect_ratio.num;
                title->geometry.par.den = st->sample_aspect_ratio.den;
            }

            int j;
            for (j = 0; j < codecpar->nb_coded_side_data; j++)
            {
                AVPacketSideData sd = codecpar->coded_side_data[j];
                switch (sd.type)
                {
                    case AV_PKT_DATA_DISPLAYMATRIX:
                    {
                        int rotation = av_display_rotation_get((int32_t *)sd.data);
                        switch (rotation) {
                            case 0:
                                title->rotation = HB_ROTATION_0;
                                break;
                            case 90:
                                title->rotation = HB_ROTATION_90;
                                break;
                            case 180:
                            case -180:
                                title->rotation = HB_ROTATION_180;
                                break;
                            case -90:
                            case 270:
                                title->rotation = HB_ROTATION_270;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    case AV_PKT_DATA_MASTERING_DISPLAY_METADATA:
                    {
                        AVMasteringDisplayMetadata *mastering = (AVMasteringDisplayMetadata *)sd.data;
                        title->mastering = hb_mastering_ff_to_hb(*mastering);
                        break;
                    }
                    case AV_PKT_DATA_CONTENT_LIGHT_LEVEL:
                    {
                        AVContentLightMetadata *coll = (AVContentLightMetadata *)sd.data;
                        title->coll.max_cll = coll->MaxCLL;
                        title->coll.max_fall = coll->MaxFALL;
                        break;
                    }
                    case AV_PKT_DATA_AMBIENT_VIEWING_ENVIRONMENT:
                    {
                        AVAmbientViewingEnvironment *ambient = (AVAmbientViewingEnvironment *)sd.data;
                        title->ambient = hb_ambient_ff_to_hb(*ambient);
                        break;
                    }
                    case AV_PKT_DATA_DOVI_CONF:
                    {
                        AVDOVIDecoderConfigurationRecord *dovi = (AVDOVIDecoderConfigurationRecord *)sd.data;
                        title->dovi = hb_dovi_ff_to_hb(*dovi);
                        break;
                    }
                    default:
                        break;
                }
            }

            title->video_codec        = WORK_DECAVCODECV;
            title->video_codec_param  = codecpar->codec_id;
            // If we ever improve our pipeline to allow other time bases...
            // title->video_timebase.num = st->time_base.num;
            // title->video_timebase.den = st->time_base.den;
            title->video_timebase.num = 1;
            title->video_timebase.den = 90000;

            // If neither the start_time neither the duration is set,
            // it's probably a raw video with no timestamps
            if (st->start_time == AV_NOPTS_VALUE && st->duration == AV_NOPTS_VALUE)
            {
                title->flags |= HBTF_RAW_VIDEO;
            }
        }
        else if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                 avcodec_find_decoder( st->codecpar->codec_id))
        {
            add_ffmpeg_audio( title, stream, i );
        }
        else if (st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            add_ffmpeg_subtitle( title, stream, i );
        }
        else if (st->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT)
        {
            add_ffmpeg_attachment( title, stream, i );
        }
    }
    find_ffmpeg_fallback_audio(title);

    title->container_name = strdup( ic->iformat->name );
    title->data_rate = ic->bit_rate;

    hb_deep_log( 2, "Found ffmpeg %d chapters, container=%s", ic->nb_chapters, ic->iformat->name );

    if( ic->nb_chapters != 0 )
    {
        AVChapter *m;
        uint64_t duration_sum = 0;
        for( i = 0; i < ic->nb_chapters; i++ )
            if( ( m = ic->chapters[i] ) != NULL )
            {
                AVDictionaryEntry * tag;
                hb_chapter_t      * chapter;
                int64_t             end;

                chapter = calloc(sizeof(hb_chapter_t), 1);
                chapter->index    = i + 1;

                /* AVChapter.end is not guaranteed to be set.
                 * Calculate chapter durations based on AVChapter.start.
                 */
                if (i + 1 < ic->nb_chapters)
                {
                    end = ic->chapters[i + 1]->start * 90000 *
                          m->time_base.num / m->time_base.den;
                }
                else
                {
                    end = ic->duration * 90000 / AV_TIME_BASE;
                }
                chapter->duration = end - duration_sum;
                duration_sum     += chapter->duration;

                int seconds      = ( chapter->duration + 45000 ) / 90000;
                chapter->hours   = ( seconds / 3600 );
                chapter->minutes = ( seconds % 3600 ) / 60;
                chapter->seconds = ( seconds % 60 );

                tag = av_dict_get( m->metadata, "title", NULL, 0 );
                /* Ignore generic chapter names set by MakeMKV
                 * ("Chapter 00" etc.).
                 * Our default chapter names are better. */
                if( tag && tag->value && tag->value[0] &&
                    ( strncmp( "Chapter ", tag->value, 8 ) ||
                      strlen( tag->value ) > 11 ) )
                {
                    hb_chapter_set_title( chapter, tag->value );
                }
                else
                {
                    char chapter_title[80];
                    snprintf( chapter_title, sizeof(chapter_title), "Chapter %d", chapter->index );
                    hb_chapter_set_title( chapter, chapter_title );
                }

                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%"PRIu64", (%02i:%02i:%02i)",
                             chapter->index, chapter->title, chapter->duration,
                             chapter->hours, chapter->minutes, chapter->seconds );

                hb_list_add( title->list_chapter, chapter );
            }
    }

    /*
     * Fill the metadata.
     */
    ffmpeg_decmetadata( ic->metadata, title );

    if( hb_list_count( title->list_chapter ) == 0 )
    {
        // Need at least one chapter
        hb_chapter_t * chapter;
        chapter = calloc( sizeof( hb_chapter_t ), 1 );
        chapter->index = 1;
        chapter->duration = title->duration;
        chapter->hours = title->hours;
        chapter->minutes = title->minutes;
        chapter->seconds = title->seconds;
        hb_list_add( title->list_chapter, chapter );
    }

    return title;
}

static int64_t av_to_hb_pts( int64_t pts, double conv_factor, int64_t offset )
{
    if ( pts == AV_NOPTS_VALUE )
        return AV_NOPTS_VALUE;
    return (int64_t)( (double)pts * conv_factor ) - offset;
}

static int ffmpeg_is_keyframe( hb_stream_t *stream )
{
    uint8_t *pkt;

    switch (stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codecpar->codec_id)
    {
        case AV_CODEC_ID_VC1:
            // XXX the VC1 codec doesn't mark key frames so to get previews
            // we do it ourselves here. The decoder gets messed up if it
            // doesn't get a SEQ header first so we consider that to be a key frame.
            pkt = stream->ffmpeg_pkt->data;
            if ( !pkt[0] && !pkt[1] && pkt[2] == 1 && pkt[3] == 0x0f )
                return 1;

            return 0;

        case AV_CODEC_ID_WMV3:
            // XXX the ffmpeg WMV3 codec doesn't mark key frames.
            // Only M$ could make I-frame detection this complicated: there
            // are two to four bits of unused junk ahead of the frame type
            // so we have to look at the sequence header to find out how much
            // to skip. Then there are three different ways of coding the type
            // depending on whether it's main or advanced profile then whether
            // there are bframes or not so we have to look at the sequence
            // header to get that.
            pkt = stream->ffmpeg_pkt->data;
            uint8_t *seqhdr = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codecpar->extradata;
            int pshift = 2;
            if ( ( seqhdr[3] & 0x02 ) == 0 )
                // no FINTERPFLAG
                ++pshift;
            if ( ( seqhdr[3] & 0x80 ) == 0 )
                // no RANGEREDUCTION
                ++pshift;
            if ( seqhdr[3] & 0x70 )
                // stream has b-frames
                return ( ( pkt[0] >> pshift ) & 0x3 ) == 0x01;

            return ( ( pkt[0] >> pshift ) & 0x2 ) == 0;

        default:
            break;
    }
    return ( stream->ffmpeg_pkt->flags & AV_PKT_FLAG_KEY );
}

hb_buffer_t * hb_ffmpeg_read( hb_stream_t *stream )
{
    int err;
    hb_buffer_t * buf;

  again:
    if ( ( err = av_read_frame( stream->ffmpeg_ic, stream->ffmpeg_pkt )) < 0 )
    {
        // av_read_frame can return EAGAIN.  In this case, it expects
        // to be called again to get more data.
        if ( err == AVERROR(EAGAIN) )
        {
            goto again;
        }
        // XXX the following conditional is to handle avi files that
        // use M$ 'packed b-frames' and occasionally have negative
        // sizes for the null frames these require.
        if ( err != AVERROR(ENOMEM) || stream->ffmpeg_pkt->size >= 0 )
        {
            // error or eof
            if (err != AVERROR_EOF)
            {
                char errstr[80];
                av_strerror(err, errstr, 80);
                hb_error("av_read_frame error (%d): %s", err, errstr);
                hb_set_work_error(stream->h, HB_ERROR_READ);
            }
            return NULL;
        }
    }
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id )
    {
        if ( stream->need_keyframe )
        {
            // we've just done a seek (generally for scan or live preview) and
            // want to start at a keyframe. Some ffmpeg codecs seek to a key
            // frame but most don't. So we spin until we either get a keyframe
            // or we've looked through 50 video frames without finding one.
            if ( ! ffmpeg_is_keyframe( stream ) && ++stream->need_keyframe < 50 )
            {
                av_packet_unref(stream->ffmpeg_pkt);
                goto again;
            }
            stream->need_keyframe = 0;
        }
        ++stream->frames;
    }
    AVStream *s = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index];
    if ( stream->ffmpeg_pkt->size <= 0 )
    {
        // M$ "invalid and inefficient" packed b-frames require 'null frames'
        // following them to preserve the timing (since the packing puts two
        // or more frames in what looks like one avi frame). The contents and
        // size of these null frames are ignored by the ff_h263_decode_frame
        // as long as they're < 20 bytes. Zero length buffers are also
        // use by theora to indicate duplicate frames.
        buf = hb_buffer_init( 0 );
    }
    else
    {
        // sometimes we get absurd sizes from ffmpeg
        if ( stream->ffmpeg_pkt->size >= (1 << 27) )
        {
            hb_log( "ffmpeg_read: pkt too big: %d bytes", stream->ffmpeg_pkt->size );
            av_packet_unref(stream->ffmpeg_pkt);
            return hb_ffmpeg_read( stream );
        }
        switch (s->codecpar->codec_type)
        {
            case AVMEDIA_TYPE_SUBTITLE:
                // Some ffmpeg subtitle decoders expect a null terminated
                // string, but the null is not included in the packet size.
                // WTF ffmpeg.
                buf = hb_buffer_init(stream->ffmpeg_pkt->size + 1);
                memcpy(buf->data, stream->ffmpeg_pkt->data,
                                  stream->ffmpeg_pkt->size);
                buf->data[stream->ffmpeg_pkt->size] = 0;
                buf->size = stream->ffmpeg_pkt->size;
                break;
            default:
                buf = hb_buffer_init(stream->ffmpeg_pkt->size);
                memcpy(buf->data, stream->ffmpeg_pkt->data,
                                  stream->ffmpeg_pkt->size);
                break;
        }

        const uint8_t *palette;
        size_t size;
        palette = av_packet_get_side_data(stream->ffmpeg_pkt,
                                          AV_PKT_DATA_PALETTE, &size);
        if (palette != NULL)
        {
            buf->palette = hb_buffer_init( size );
            memcpy( buf->palette->data, palette, size );
        }
    }
    if (stream->ffmpeg_pkt->flags & AV_PKT_FLAG_DISCARD)
    {
        buf->s.flags |= HB_FLAG_DISCARD;
    }
    buf->s.id = stream->ffmpeg_pkt->stream_index;

    // compute a conversion factor to go from the ffmpeg
    // timebase for the stream to HB's 90kHz timebase.
    double tsconv = (double)90000. * s->time_base.num / s->time_base.den;
    int64_t offset = 90000LL * ffmpeg_initial_timestamp(stream) / AV_TIME_BASE;

    buf->s.start = av_to_hb_pts(stream->ffmpeg_pkt->pts, tsconv, offset);
    buf->s.renderOffset = av_to_hb_pts(stream->ffmpeg_pkt->dts, tsconv, offset);
    if ( buf->s.renderOffset >= 0 && buf->s.start == AV_NOPTS_VALUE )
    {
        buf->s.start = buf->s.renderOffset;
    }
    else if ( buf->s.renderOffset == AV_NOPTS_VALUE && buf->s.start >= 0 )
    {
        buf->s.renderOffset = buf->s.start;
    }

    switch (s->codecpar->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
            buf->s.type = VIDEO_BUF;
            /*
             * libav avcodec_decode_video2() needs AVPacket flagged with AV_PKT_FLAG_KEY
             * for some codecs. For example, sequence of PNG in a mov container.
             */
            if (stream->ffmpeg_pkt->flags & AV_PKT_FLAG_KEY)
            {
                buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
                buf->s.frametype = HB_FRAME_I;
            }
            break;

        case AVMEDIA_TYPE_AUDIO:
            buf->s.type = AUDIO_BUF;
            break;

        case AVMEDIA_TYPE_SUBTITLE:
        {
            // Fill out stop and duration for subtitle packets
            int64_t pkt_duration = stream->ffmpeg_pkt->duration;
            if (pkt_duration != AV_NOPTS_VALUE)
            {
                buf->s.duration = av_to_hb_pts(pkt_duration, tsconv, 0);
                buf->s.stop = buf->s.start + buf->s.duration;
            }
            else
            {
                buf->s.duration = (int64_t)AV_NOPTS_VALUE;
            }
            buf->s.type = SUBTITLE_BUF;
        } break;

        default:
            buf->s.type = OTHER_BUF;
            break;
    }

    /*
     * Check to see whether this buffer is on a chapter
     * boundary, if so mark it as such in the buffer then advance
     * chapter_end to the end of the next chapter.
     * If there are no chapters, chapter_end is always initialized to INT64_MAX
     * (roughly 3 million years at our 90KHz clock rate) so the test
     * below handles both the chapters & no chapters case.
     */
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id &&
         buf->s.start >= stream->chapter_end )
    {
        hb_chapter_t *chapter = hb_list_item( stream->title->list_chapter,
                                              stream->chapter);
        if (chapter != NULL)
        {
            stream->chapter++;
            stream->chapter_end += chapter->duration;
            buf->s.new_chap = stream->chapter;
            hb_deep_log( 2, "ffmpeg_read starting chapter %i at %"PRId64,
                         stream->chapter, buf->s.start);
        } else {
            // Some titles run longer than the sum of their chapters
            // Don't increment to a nonexistent chapter number
            // Must have run out of chapters, stop looking.
            hb_deep_log( 2, "ffmpeg_read end of chapter %i at %"PRId64,
                         stream->chapter, buf->s.start);
            stream->chapter_end = INT64_MAX;
            buf->s.new_chap = 0;
        }
    } else {
        buf->s.new_chap = 0;
    }
    av_packet_unref(stream->ffmpeg_pkt);
    return buf;
}

static int ffmpeg_seek( hb_stream_t *stream, float frac )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    int res;
    if ( frac > 0. )
    {
        int64_t pos = (double)stream->ffmpeg_ic->duration * (double)frac +
                ffmpeg_initial_timestamp( stream );
        res = avformat_seek_file( ic, -1, 0, pos, pos, AVSEEK_FLAG_BACKWARD);
        if (res < 0)
        {
            hb_error("avformat_seek_file failed");
        }
    }
    else
    {
        int64_t pos = ffmpeg_initial_timestamp( stream );
        res = avformat_seek_file( ic, -1, 0, pos, pos, AVSEEK_FLAG_BACKWARD);
        if (res < 0)
        {
            hb_error("avformat_seek_file failed");
        }
    }
    stream->need_keyframe = 1;
    return 1;
}

// Assumes that we are always seeking forward
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    int64_t pos;
    int ret;

    // Find the initial chapter we have seeked into
    int count = hb_list_count(stream->title->list_chapter);
    if (count > 0)
    {
        int64_t        sum_dur = 0;
        hb_chapter_t * chapter;
        int            ii;
        for (ii = 0; ii < count; ii++)
        {
            chapter = hb_list_item( stream->title->list_chapter, ii );
            if (sum_dur + chapter->duration > ts)
            {
                break;
            }
            sum_dur += chapter->duration;
        }
        stream->chapter     = ii;
        stream->chapter_end = sum_dur;
    }
    else
    {
        stream->chapter     = 0;
        stream->chapter_end = INT64_MAX;
    }

    pos = ts * AV_TIME_BASE / 90000 + ffmpeg_initial_timestamp( stream );
    AVStream *st = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id];
    // timebase must be adjusted to match timebase of stream we are
    // using for seeking.
    pos = av_rescale(pos, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
    stream->need_keyframe = 1;
    // Seek to the nearest timestamp before that requested where
    // there is an I-frame
    ret = avformat_seek_file( ic, stream->ffmpeg_video_id, 0, pos, pos, 0);
    return ret;
}
