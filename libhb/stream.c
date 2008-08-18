/* $Id$

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "lang.h"
#include "a52dec/a52.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include <string.h>
#include <ctype.h>

#define min(a, b) a < b ? a : b

/*
 * This table defines how ISO MPEG stream type codes map to HandBrake
 * codecs. It is indexed by the 8 bit stream type and contains the codec
 * worker object id and a parameter for that worker proc (ignored except
 * for the ffmpeg-based codecs in which case it is the ffmpeg codec id).
 *
 * Entries with a worker proc id of 0 or a kind of 'U' indicate that HB
 * doesn't handle the stream type.
 */
typedef struct {
    enum { U, A, V } kind; /* unknown / audio / video */
    int codec;          /* HB worker object id of codec */
    int codec_param;    /* param for codec (usually ffmpeg codec id) */
    const char* name;   /* description of type */
} stream2codec_t;

#define st(id, kind, codec, codec_param, name) \
 [id] = { kind, codec, codec_param, name }

static const stream2codec_t st2codec[256] = {
    st(0x01, V, WORK_DECMPEG2,     0,              "MPEG1"),
    st(0x02, V, WORK_DECMPEG2,     0,              "MPEG2"),
    st(0x03, A, HB_ACODEC_MPGA,    CODEC_ID_MP2,   "MPEG1"),
    st(0x04, A, HB_ACODEC_MPGA,    CODEC_ID_MP2,   "MPEG2"),
    st(0x05, U, 0,                 0,              "ISO 13818-1 private section"),
    st(0x06, U, 0,                 0,              "ISO 13818-1 PES private data"),
    st(0x07, U, 0,                 0,              "ISO 13522 MHEG"),
    st(0x08, U, 0,                 0,              "ISO 13818-1 DSM-CC"),
    st(0x09, U, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0a, U, 0,                 0,              "ISO 13818-6 encap"),
    st(0x0b, U, 0,                 0,              "ISO 13818-6 DSM-CC U-N msgs"),
    st(0x0c, U, 0,                 0,              "ISO 13818-6 Stream descriptors"),
    st(0x0d, U, 0,                 0,              "ISO 13818-6 Sections"),
    st(0x0e, U, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0f, A, HB_ACODEC_MPGA,    CODEC_ID_AAC,   "ISO 13818-7 AAC Audio"),
    st(0x10, V, WORK_DECAVCODECV,  CODEC_ID_MPEG4, "MPEG4"),
    st(0x11, A, HB_ACODEC_MPGA,    CODEC_ID_AAC_LATM, "MPEG4 LATM AAC"),
    st(0x12, U, 0,                 0,              "MPEG4 generic"),

    st(0x14, U, 0,                 0,              "ISO 13818-6 DSM-CC download"),

    st(0x1b, V, WORK_DECAVCODECV,  CODEC_ID_H264,  "H.264"),

    st(0x80, U, 0,                 0,              "DigiCipher II Video"),
    st(0x81, A, HB_ACODEC_AC3,     0,              "AC-3"),
    st(0x82, A, HB_ACODEC_MPGA,    CODEC_ID_DTS,   "HDMV DTS"),
    st(0x83, A, HB_ACODEC_LPCM,    0,              "LPCM"),
    st(0x84, A, 0,                 0,              "SDDS"),
    st(0x85, U, 0,                 0,              "ATSC Program ID"),
    st(0x86, U, 0,                 0,              "SCTE 35 splice info"),
    st(0x87, A, 0,                 0,              "E-AC-3"),

    st(0x8a, A, HB_ACODEC_DCA,     0,              "DTS"),

    st(0x91, A, HB_ACODEC_AC3,     0,              "AC-3"),
    st(0x92, U, 0,                 0,              "Subtitle"),

    st(0x94, A, 0,                 0,              "SDDS"),
    st(0xa0, V, 0,                 0,              "MSCODEC"),

    st(0xea, V, WORK_DECAVCODECV,  CODEC_ID_VC1,   "VC1"),
};
#undef st

typedef enum {
    hb_stream_type_unknown = 0,
    transport,
    program,
    dvd_program,
    ffmpeg
} hb_stream_type_t;

#define kMaxNumberVideoPIDS 1
#define kMaxNumberAudioPIDS 15
#define kMaxNumberDecodeStreams (kMaxNumberVideoPIDS+kMaxNumberAudioPIDS)
#define kMaxNumberPMTStreams 32


struct hb_stream_s
{
    int     frames;             /* video frames so far */
    int     errors;             /* total errors so far */
    int     last_error_frame;   /* frame # at last error message */
    int     last_error_count;   /* # errors at last error message */
    int     packetsize;         /* Transport Stream packet size */

    int64_t ts_lastpcr;         /* the last pcr we found in the TS stream */
    int64_t ts_nextpcr;         /* the next pcr to put in a PS packet */

    uint8_t *ts_packet;         /* buffer for one TS packet */
    uint8_t *ts_buf[kMaxNumberDecodeStreams];
    int     ts_pos[kMaxNumberDecodeStreams];
    int8_t  ts_foundfirst[kMaxNumberDecodeStreams];
    int8_t  ts_skipbad[kMaxNumberDecodeStreams];
    int8_t  ts_streamcont[kMaxNumberDecodeStreams];
    int8_t  ts_start[kMaxNumberDecodeStreams];

    uint8_t *fwrite_buf;        /* PS buffer (set by hb_ts_stream_decode) */
    uint8_t *fwrite_buf_orig;   /* PS buffer start (set by hb_ts_stream_decode) */

    /*
     * Stuff before this point is dynamic state updated as we read the
     * stream. Stuff after this point is stream description state that
     * we learn during the initial scan but cache so it can be
     * reused during the conversion read.
     */
    uint8_t ts_number_video_pids;
    uint8_t ts_number_audio_pids;

    int16_t ts_video_pids[kMaxNumberVideoPIDS];
    int16_t ts_audio_pids[kMaxNumberAudioPIDS];

    uint8_t ts_streamid[kMaxNumberDecodeStreams];
    uint8_t ts_stream_type[kMaxNumberDecodeStreams];

    char    *path;
    FILE    *file_handle;
    hb_stream_type_t hb_stream_type;
    hb_title_t *title;

    AVFormatContext *ffmpeg_ic;

    struct {
        int lang_code;
        int flags;
        int rate;
        int bitrate;
    } a52_info[kMaxNumberAudioPIDS];

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

        int section_length;
        int program_number;
        unsigned int PCR_PID;
        int program_info_length;
        unsigned char *progam_info_descriptor_data;
        struct
        {
            unsigned char stream_type;
            unsigned short elementary_PID;
            unsigned short ES_info_length;
            unsigned char *es_info_descriptor_data;
        } pmt_stream_info[kMaxNumberPMTStreams];
    } pmt_info;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle);
static void hb_ts_stream_init(hb_stream_t *stream);
static void hb_ts_stream_find_pids(hb_stream_t *stream);
static int hb_ts_stream_decode(hb_stream_t *stream, uint8_t *obuf);
static void hb_ts_stream_reset(hb_stream_t *stream);
static hb_audio_t *hb_ts_stream_set_audio_id_and_codec(hb_stream_t *stream,
                                                       int aud_pid_index);
static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title);
static off_t align_to_next_packet(hb_stream_t *stream);

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title );
static void ffmpeg_close( hb_stream_t *d );
static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream );
static int ffmpeg_read( hb_stream_t *stream, hb_buffer_t *buf );
static int ffmpeg_seek( hb_stream_t *stream, float frac );

/*
 * streams have a bunch of state that's learned during the scan. We don't
 * want to throw away the state when scan does a close then relearn
 * everything when reader does an open. So we save the stream state on
 * the close following a scan and reuse it when 'reader' does an open.
 */
static hb_list_t *stream_state_list;

static hb_stream_t *hb_stream_lookup( const char *path )
{
    if ( stream_state_list == NULL )
        return NULL;

    hb_stream_t *ss;
    int i = 0;

    while ( ( ss = hb_list_item( stream_state_list, i++ ) ) != NULL )
    {
        if ( strcmp( path, ss->path ) == 0 )
        {
            break;
        }
    }
    return ss;
}

static void hb_stream_state_delete( hb_stream_t *ss )
{
    hb_list_rem( stream_state_list, ss );
    free( ss->path );
    free( ss );
}

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

static void ts_warn( hb_stream_t *stream, char *log, ... )
{
    va_list     args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );
}

static void ts_err( hb_stream_t *stream, int curstream, char *log, ... )
{
    va_list     args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );

    stream->ts_skipbad[curstream] = 1;
    stream->ts_pos[curstream] = 0;
    stream->ts_streamcont[curstream] = -1;
}

static int check_ps_sync(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack header in the
    // first four bytes.
    return (buf[0] == 0x00) && (buf[1] == 0x00) &&
           (buf[2] == 0x01) && (buf[3] == 0xba);
}

static int check_ps_sys(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack followed by a
    // SYS. If we've already verified the pack, this skips over it and checks
    // for the sys header.
    int pos = 14 + ( buf[13] & 0x7 );   // skip over the PACK
    return (buf[pos+0] == 0x00) && (buf[pos+1] == 0x00) &&
           (buf[pos+2] == 0x01) && (buf[pos+3] == 0xbb);
}

static int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte, no scrambling & a legal adaptation ctrl
    return (buf[0] == 0x47) && ((buf[3] >> 6) == 0) && ((buf[3] >> 4) > 0);
}

static int have_ts_sync(const uint8_t *buf, int psize)
{
    return check_ts_sync(&buf[0*psize]) && check_ts_sync(&buf[1*psize]) &&
           check_ts_sync(&buf[2*psize]) && check_ts_sync(&buf[3*psize]) &&
           check_ts_sync(&buf[4*psize]) && check_ts_sync(&buf[5*psize]) &&
           check_ts_sync(&buf[6*psize]) && check_ts_sync(&buf[7*psize]);
}

static int hb_stream_check_for_ts(const uint8_t *buf)
{
    // transport streams should have a sync byte every 188 bytes.
    // search the first 8KB of buf looking for at least 8 consecutive
    // correctly located sync patterns.
    int offset = 0;

    for ( offset = 0; offset < 8*1024-8*188; ++offset )
    {
        if ( have_ts_sync( &buf[offset], 188) )
            return 188 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 192) )
            return 192 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 204) )
            return 204 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 208) )
            return 208 | (offset << 8);
    }
    return 0;
}

static int hb_stream_check_for_ps(const uint8_t *buf)
{
    // program streams should start with a PACK then a SYS header.
    return check_ps_sync(buf) && check_ps_sys(buf);
}

static int hb_stream_check_for_dvd_ps(const uint8_t *buf)
{
    // DVD program streams should have a Pack header every 2048 bytes.
    // check that we have 4 of these in a row.
    return check_ps_sync(&buf[0*2048]) && check_ps_sync(&buf[1*2048]) &&
           check_ps_sync(&buf[2*2048]) && check_ps_sync(&buf[3*2048]);
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
            hb_ts_stream_init(stream);
            return 1;
        }
        if ( hb_stream_check_for_dvd_ps(buf) != 0 )
        {
            hb_log("file is MPEG DVD Program Stream");
            stream->hb_stream_type = dvd_program;
            return 1;
        }
        if ( hb_stream_check_for_ps(buf) != 0 )
        {
            hb_log("file is MPEG Program Stream");
            stream->hb_stream_type = program;
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

    if ( d->ts_packet )
    {
        free( d->ts_packet );
        d->ts_packet = NULL;
    }
	for (i = 0; i < kMaxNumberDecodeStreams; i++)
	{
		if (d->ts_buf[i])
		{
			free(d->ts_buf[i]);
			d->ts_buf[i] = NULL;
		}
	}
}

static void hb_stream_delete( hb_stream_t *d )
{
    hb_stream_delete_dynamic( d );
    free( d->path );
    free( d );
}

/***********************************************************************
 * hb_stream_open
 ***********************************************************************
 *
 **********************************************************************/
hb_stream_t * hb_stream_open( char *path, hb_title_t *title )
{
    FILE *f = fopen( path, "r" );
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

    /*
     * if we're opening the stream to read & convert, we need
     * the state we saved when we scanned the stream. if we're
     * opening the stream to scan it we want to rebuild the state
     * (even if we have saved state, the stream may have changed).
     */
    hb_stream_t *ss = hb_stream_lookup( path );
    if ( title && ss && ss->hb_stream_type != ffmpeg )
    {
        /*
         * copy the saved state since we might be encoding the same stream
         * multiple times.
         */
        memcpy( d, ss, sizeof(*d) );
        d->file_handle = f;
        d->title = title;
        d->path = strdup( path );

        if ( d->hb_stream_type == transport )
        {
            d->ts_packet = malloc( d->packetsize );

            int i = 0;
            for ( ; i < d->ts_number_video_pids + d->ts_number_audio_pids; i++)
            {
                d->ts_buf[i] = malloc( HB_DVD_READ_BUFFER_SIZE );
            }
            hb_stream_seek( d, 0. );
        }
        return d;
    }

    /*
     * opening for scan - delete any saved state then (re)scan the stream.
     * If it's something we can deal with (MPEG2 PS or TS) return a stream
     * reference structure & null otherwise.
     */
    if ( ss != NULL )
    {
        hb_stream_state_delete( ss );
    }
    d->file_handle = f;
    d->title = title;
    d->path = strdup( path );
    if (d->path != NULL )
    {
        if ( hb_stream_get_type( d ) != 0 )
        {
            return d;
        }
        fclose( d->file_handle );
		d->file_handle = NULL;
        if ( ffmpeg_open( d, title ) )
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

/***********************************************************************
 * hb_stream_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_stream_close( hb_stream_t ** _d )
{
    hb_stream_t *stream = * _d;

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

    /*
     * if the stream was opened for a scan, cache the result, otherwise delete
     * the state.
     */
    if ( stream->title == NULL )
    {
        hb_stream_delete_dynamic( stream );
        if ( stream_state_list == NULL )
        {
            stream_state_list = hb_list_init();
        }
        hb_list_add( stream_state_list, stream );
    }
    else
    {
        hb_stream_delete( stream );
    }
    *_d = NULL;
}

/* when the file was first opened we made entries for all the audio elementary
 * streams we found in it. Streams that were later found during the preview scan
 * now have an audio codec, type, rate, etc., associated with them. At the end
 * of the scan we delete all the audio entries that weren't found by the scan
 * or don't have a format we support. This routine deletes audio entry 'indx'
 * by copying all later entries down one slot. */
static void hb_stream_delete_audio_entry(hb_stream_t *stream, int indx)
{
    int i;

    for (i = indx+1; i < stream->ts_number_audio_pids; ++i)
    {
        stream->ts_audio_pids[indx] = stream->ts_audio_pids[i];
        stream->ts_stream_type[1 + indx] = stream->ts_stream_type[1+i];
        stream->ts_streamid[1 + indx] = stream->ts_streamid[1 + i];
        ++indx;
    }
    --stream->ts_number_audio_pids;
}

static int index_of_pid(int pid, hb_stream_t *stream)
{
    int i;

    if ( pid == stream->ts_video_pids[0] )
        return 0;

    for ( i = 0; i < stream->ts_number_audio_pids; ++i )
        if ( pid == stream->ts_audio_pids[i] )
            return i + 1;

    return -1;
}

/***********************************************************************
 * hb_ps_stream_title_scan
 ***********************************************************************
 *
 **********************************************************************/
hb_title_t * hb_stream_title_scan(hb_stream_t *stream)
{
	if ( stream->hb_stream_type == ffmpeg )
        return ffmpeg_title_scan( stream );

    // 'Barebones Title'
    hb_title_t *aTitle = hb_title_init( stream->path, 0 );
    aTitle->index = 1;

	// Copy part of the stream path to the title name
	char *sep = strrchr(stream->path, '/');
	if (sep)
		strcpy(aTitle->name, sep+1);
	char *dot_term = strrchr(aTitle->name, '.');
	if (dot_term)
		*dot_term = '\0';

    // Height, width,  rate and aspect ratio information is filled in when the previews are built

    hb_stream_duration(stream, aTitle);

    // One Chapter
    hb_chapter_t * chapter;
    chapter = calloc( sizeof( hb_chapter_t ), 1 );
    chapter->index = 1;
    chapter->duration = aTitle->duration;
    chapter->hours = aTitle->hours;
    chapter->minutes = aTitle->minutes;
    chapter->seconds = aTitle->seconds;
    hb_list_add( aTitle->list_chapter, chapter );

    // Figure out how many audio streams we really have:
    // - For transport streams, for each PID listed in the PMT (whether
    //   or not it was an audio stream type) read the bitstream until we
    //   find an packet from that PID containing a PES header and see if
    //   the elementary stream is an audio type.
    // - For program streams read the first 4MB and take every unique
    //   audio stream we find.
	if (stream->hb_stream_type == transport)
	{
        int i;

        for (i=0; i < stream->ts_number_audio_pids; i++)
        {
            hb_audio_t *audio = hb_ts_stream_set_audio_id_and_codec(stream, i);
            if (audio->config.in.codec)
                hb_list_add( aTitle->list_audio, audio );
            else
            {
                free(audio);
                hb_stream_delete_audio_entry(stream, i);
                --i;
            }
        }

        // add the PCR PID if we don't already have it
        if ( index_of_pid( stream->pmt_info.PCR_PID, stream ) < 0 )
        {
            stream->ts_audio_pids[stream->ts_number_audio_pids++] =
                stream->pmt_info.PCR_PID;
        }

        // set up the video codec to use for this title
        aTitle->video_codec = st2codec[stream->ts_stream_type[0]].codec;
        aTitle->video_codec_param = st2codec[stream->ts_stream_type[0]].codec_param;
	}
    else
    {
        hb_ps_stream_find_audio_ids(stream, aTitle);
    }

  return aTitle;
}

/*
 * read the next transport stream packet from 'stream'. Return NULL if
 * we hit eof & a pointer to the sync byte otherwise.
 */
static const uint8_t *next_packet( hb_stream_t *stream )
{
    uint8_t *buf = stream->ts_packet + stream->packetsize - 188;

    while ( 1 )
    {
        if ( fread(stream->ts_packet, 1, stream->packetsize, stream->file_handle) !=
             stream->packetsize )
        {
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
            hb_log( "next_packet: eof while re-establishing sync @ %lld", pos );
            return NULL;
        }
        ts_warn( stream, "next_packet: sync lost @ %lld, regained after %lld bytes",
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

/*
 * scan the next MB of 'stream' to find the next start packet for
 * the Packetized Elementary Stream associated with TS PID 'pid'.
 */
static const uint8_t *hb_ts_stream_getPEStype(hb_stream_t *stream, uint32_t pid)
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

        /*
         * The PES header is only in TS packets with 'start' set so we check
         * that first then check for the right PID.
         */
        if ((buf[1] & 0x40) == 0 || (buf[1] & 0x1f) != (pid >> 8) ||
            buf[2] != (pid & 0xff))
        {
            // not a start packet or not the pid we want
            continue;
        }

        /* skip over the TS hdr to return a pointer to the PES hdr */
        int udata = 4;
        switch (buf[3] & 0x30)
        {
            case 0x00: // illegal
            case 0x20: // fill packet
                continue;

            case 0x30: // adaptation
                if (buf[4] > 182)
                {
                    hb_log("hb_ts_stream_getPEStype: invalid adaptation field length %d for PID 0x%x", buf[4], pid);
                    continue;
                }
                udata += buf[4] + 1;
                break;
        }
        /* PES hdr has to begin with an mpeg start code */
        if (buf[udata+0] == 0x00 && buf[udata+1] == 0x00 && buf[udata+2] == 0x01)
        {
            return &buf[udata];
        }
    }

    /* didn't find it */
    return 0;
}

static uint64_t hb_ps_stream_getVideoPTS(hb_stream_t *stream)
{
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
    hb_list_t *list = hb_list_init();
    // how many blocks we read while searching for a video PES header
    int blksleft = 1024;
    uint64_t pts = 0;

    while (--blksleft >= 0 && hb_stream_read(stream, buf) == 1)
    {
        hb_buffer_t *es;

        // 'buf' contains an MPEG2 PACK - get a list of all it's elementary streams
        hb_demux_ps( buf, list, 0 );

        while ( ( es = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, es );
            if ( es->id == 0xe0 )
            {
                // this PES contains video - if there's a PTS we're done
                // hb_demux_ps left the PTS in buf_es->start.
                if ( es->start != ~0 )
                {
                    pts = es->start;
                    blksleft = 0;
                    break;
                }
            }
            hb_buffer_close( &es );
        }
    }
    hb_list_empty( &list );
    hb_buffer_close(&buf);
    return pts;
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

#define NDURSAMPLES 16

// get one (position, timestamp) sampple from a transport or program
// stream.
static struct pts_pos hb_sample_pts(hb_stream_t *stream, uint64_t fpos)
{
    struct pts_pos pp = { 0, 0 };

    if ( stream->hb_stream_type == transport )
    {
        const uint8_t *buf;
        fseeko( stream->file_handle, fpos, SEEK_SET );
        align_to_next_packet( stream );
        buf = hb_ts_stream_getPEStype( stream, stream->ts_video_pids[0] );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %llu", fpos);
            return pp;
        }
        if ( ( buf[7] >> 7 ) != 1 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %llu", fpos);
            return pp;
        }
        pp.pts = ( ( (uint64_t)buf[9] >> 1 ) & 7 << 30 ) |
                 ( (uint64_t)buf[10] << 22 ) |
                 ( ( (uint64_t)buf[11] >> 1 ) << 15 ) |
                 ( (uint64_t)buf[12] << 7 ) |
                 ( (uint64_t)buf[13] >> 1 );
    }
    else
    {
        // round address down to nearest dvd sector start
        fpos &=~ ( HB_DVD_READ_BUFFER_SIZE - 1 );
        fseeko( stream->file_handle, fpos, SEEK_SET );
        if ( stream->hb_stream_type == program )
        {
            skip_to_next_pack( stream );
        }
        pp.pts = hb_ps_stream_getVideoPTS( stream );
    }
    pp.pos = ftello(stream->file_handle);
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
    double rates[NDURSAMPLES * NDURSAMPLES / 2];
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
        int ns = i + ( n >> 1 );
        if ( ns > n )
            ns = n;
        for ( j = i+1; j < ns; ++j )
        {
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
int hb_stream_read( hb_stream_t * src_stream, hb_buffer_t * b )
{
	if ( src_stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_read( src_stream, b );
    }
    if ( src_stream->hb_stream_type == dvd_program )
    {
        size_t amt_read = fread(b->data, HB_DVD_READ_BUFFER_SIZE, 1,
                                src_stream->file_handle);
        return (amt_read > 0);
    }
    if ( src_stream->hb_stream_type == program )
    {
        // a general program stream has arbitrary sized pack's. we're
        // currently positioned at the start of a pack so read up to but
        // not including the start of the next, expanding the buffer
        // as necessary.
        uint8_t *cp = b->data;
        uint8_t *ep = cp + b->alloc;
        uint32_t strt_code = -1;
        int c;

        // consume the first byte of the initial pack so we don't match on
        // it in the loop below.
        if ( ( c = getc( src_stream->file_handle ) ) == EOF )
            return 0;

        *cp++ = c;

        flockfile( src_stream->file_handle );
        while ( ( c = getc_unlocked( src_stream->file_handle ) ) != EOF )
        {
            strt_code = ( strt_code << 8 ) | c;
            if ( strt_code == 0x000001ba )
                // we found the start of the next pack
                break;
            if ( cp >= ep )
            {
                // need to expand the buffer
                int curSize = cp - b->data;
                hb_buffer_realloc( b, curSize * 2 );
                cp = b->data + curSize;
                ep = b->data + b->alloc;
            }
            *cp++ = c;
        }
        funlockfile( src_stream->file_handle );

        // if we didn't terminate on an eof back up so the next read
        // starts on the pack boundary.
        b->size = cp - b->data;
        if ( c != EOF )
        {
            fseeko( src_stream->file_handle, -4, SEEK_CUR );
            b->size -= 4;
        }
        return 1;
    }
    return hb_ts_stream_decode( src_stream, b->data );
}

/***********************************************************************
 * hb_stream_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_seek( hb_stream_t * src_stream, float f )
{
	if ( src_stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek( src_stream, f );
    }
    off_t stream_size, cur_pos, new_pos;
    double pos_ratio = f;
    cur_pos = ftello( src_stream->file_handle );
    fseeko( src_stream->file_handle, 0, SEEK_END );
    stream_size = ftello( src_stream->file_handle );
    new_pos = (off_t) ((double) (stream_size) * pos_ratio);
    new_pos &=~ (HB_DVD_READ_BUFFER_SIZE - 1);

    int r = fseeko( src_stream->file_handle, new_pos, SEEK_SET );
    if (r == -1)
    {
        fseeko( src_stream->file_handle, cur_pos, SEEK_SET );
        return 0;
    }

    if ( src_stream->hb_stream_type == transport )
    {
        // We need to drop the current decoder output and move
        // forwards to the next transport stream packet.
        hb_ts_stream_reset(src_stream);
    }
    else if ( src_stream->hb_stream_type == program )
    {
        skip_to_next_pack( src_stream );
    }

    return 1;
}

static const char* make_upper( const char* s )
{
    static char name[8];
    char *cp = name;
    char *ep = cp + sizeof(name)-1;

    while ( *s && cp < ep )
    {
        *cp++ = islower(*s)? toupper(*s) : *s;
        ++s;
    }
    *cp = 0;
    return name;
}

static void set_audio_description( hb_audio_t *audio, iso639_lang_t *lang )
{
    /* XXX
     * This is a duplicate of code in dvd.c - it should get factored out
     * into a common routine. We probably should only be putting the lang
     * code or a lang pointer into the audio config & let the common description
     * formatting routine in scan.c do all the stuff below.
     */
    const char *codec_name;
    AVCodecContext *cc;

    if ( audio->config.in.codec == HB_ACODEC_FFMPEG &&
         ( cc = hb_ffmpeg_context( audio->config.in.codec_param ) ) &&
         avcodec_find_decoder( cc->codec_id ) )
    {
        codec_name = make_upper( avcodec_find_decoder( cc->codec_id )->name );
        if ( !strcmp( codec_name, "LIBFAAD" ) )
        {
            codec_name = "AAC";
        }
    }
    else if ( audio->config.in.codec == HB_ACODEC_MPGA &&
              avcodec_find_decoder( audio->config.in.codec_param ) )
    {
        codec_name = avcodec_find_decoder( audio->config.in.codec_param )->name;
    }
    else
    {
        codec_name = audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" :
                     audio->config.in.codec == HB_ACODEC_DCA ? "DTS" :
                     audio->config.in.codec == HB_ACODEC_MPGA ? "MPEG" : 
                     audio->config.in.codec == HB_ACODEC_LPCM ? "LPCM" : 
                     audio->config.in.codec == HB_ACODEC_FFMPEG ? "FFMPEG" :
                     "Unknown";
    }
    snprintf( audio->config.lang.description,
              sizeof( audio->config.lang.description ), "%s (%s)",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name,
              codec_name );
    snprintf( audio->config.lang.simple, sizeof( audio->config.lang.simple ), "%s",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2, sizeof( audio->config.lang.iso639_2 ),
              "%s", lang->iso639_2);
}

static hb_audio_t *hb_ts_stream_set_audio_id_and_codec(hb_stream_t *stream,
                                                       int aud_pid_index)
{
    off_t cur_pos = ftello(stream->file_handle);
    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );
    const uint8_t *buf;

    fseeko(stream->file_handle, 0, SEEK_SET);
    align_to_next_packet(stream);
    buf = hb_ts_stream_getPEStype(stream, stream->ts_audio_pids[aud_pid_index]);

    /* check that we found a PES header */
    uint8_t stype = 0;
    if (buf && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01)
    {
        // 0xbd is the normal container for AC3/DCA/PCM/etc. 0xfd indicates an
        // extended stream id (ISO 13818-1(2007)). If we cared about the
        // real id we'd have to look inside the PES extension to find it.
        // But since we remap stream id's when we generate PS packets from
        // the TS packets we can just ignore the actual id.
        if ( buf[3] == 0xbd || buf[3] == 0xfd )
        {
            audio->id = 0x80bd | (aud_pid_index << 8);
            stype = stream->ts_stream_type[1 + aud_pid_index];
            if ( st2codec[stype].kind == U )
            {
                // XXX assume unknown stream types are AC-3 (if they're not
                // audio we'll find that out during the scan but if they're
                // some other type of audio we'll end up ignoring them).
                stype = 0x81;
                stream->ts_stream_type[1 + aud_pid_index] = 0x81;
            }
            stream->ts_streamid[1 + aud_pid_index] = 0xbd;
        }
        else if ((buf[3] & 0xe0) == 0xc0)
        {
            audio->id = 0xc0 | aud_pid_index;
            stype = stream->ts_stream_type[1 + aud_pid_index];
            if ( st2codec[stype].kind == U )
            {
                // XXX assume unknown stream types are MPEG audio
                stype = 0x03;
                stream->ts_stream_type[1 + aud_pid_index] = 0x03;
            }
        }
    }
    // if we found an audio stream type & HB has a codec that can decode it
    // finish configuring the audio so we'll add it to the title's list.
    if ( st2codec[stype].kind == A && st2codec[stype].codec )
    {
        stream->ts_streamid[1 + aud_pid_index] = audio->id;
        audio->config.in.codec = st2codec[stype].codec;
        audio->config.in.codec_param = st2codec[stype].codec_param;
		set_audio_description( audio,
                  lang_for_code( stream->a52_info[aud_pid_index].lang_code ) );
        hb_log("transport stream pid 0x%x (type 0x%x) is %s audio id 0x%x",
               stream->ts_audio_pids[aud_pid_index],
               stype, st2codec[stype].name, audio->id);
    }
    else
    {
        if ( buf )
        {
            hb_log("transport stream pid 0x%x (type 0x%x, substream 0x%x) "
                    "isn't audio", stream->ts_audio_pids[aud_pid_index],
                    stream->ts_stream_type[1 + aud_pid_index], buf[3]);
        }
        else
        {
            hb_log("transport stream pid 0x%x (type 0x%x) isn't audio",
                    stream->ts_audio_pids[aud_pid_index],
                    stream->ts_stream_type[1 + aud_pid_index]);
        }
	}
    fseeko(stream->file_handle, cur_pos, SEEK_SET);
    return audio;
}

static void add_audio_to_title(hb_title_t *title, int id)
{
    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = id;
    switch ( id >> 12 )
    {
        case 0x0:
            audio->config.in.codec = HB_ACODEC_MPGA;
            hb_log("add_audio_to_title: added MPEG audio stream 0x%x", id);
            break;
        case 0x2:
            // type 2 is a DVD subtitle stream - just ignore it */
            free( audio );
            return;
        case 0x8:
            audio->config.in.codec = HB_ACODEC_AC3;
            hb_log("add_audio_to_title: added AC3 audio stream 0x%x", id);
            break;
        case 0xa:
            audio->config.in.codec = HB_ACODEC_LPCM;
            hb_log("add_audio_to_title: added LPCM audio stream 0x%x", id);
            break;
        default:
            hb_log("add_audio_to_title: unknown audio stream type 0x%x", id);
            free( audio );
            return;

    }
    set_audio_description( audio, lang_for_code( 0 ) );
    hb_list_add( title->list_audio, audio );
}

static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title)
{
    off_t cur_pos = ftello(stream->file_handle);
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
    hb_list_t *list = hb_list_init();
    // how many blocks we read while searching for audio streams
    int blksleft = 4096;
    // there can be at most 16 unique streams in an MPEG PS (8 in a DVD)
    // so we use a bitmap to keep track of the ones we've already seen.
    // Bit 'i' of smap is set if we've already added the audio for
    // audio substream id 'i' to the title's audio list.
    uint32_t smap = 0;

    // start looking 20% into the file since there's occasionally no
    // audio at the beginning (particularly for vobs).
    hb_stream_seek(stream, 0.2f);

    while (--blksleft >= 0 && hb_stream_read(stream, buf) == 1)
    {
        hb_buffer_t *es;

        // 'buf' contains an MPEG2 PACK - get a list of all it's elementary streams
        hb_demux_ps( buf, list, 0 );

        while ( ( es = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, es );
            if ( (es->id & 0xff) == 0xbd || (es->id & 0xe0) == 0xc0 )
            {
                // this PES contains some kind of audio - get the substream id
                // and check if we've seen it already.
                int ssid = (es->id > 0xff ? es->id >> 8 : es->id) & 0xf;
                if ( (smap & (1 << ssid)) == 0 )
                {
                    // we haven't seen this stream before - add it to the
                    // title's list of audio streams.
                    smap |= (1 << ssid);
                    add_audio_to_title(title, es->id);
                }
            }
            hb_buffer_close( &es );
        }
    }
    hb_list_empty( &list );
    hb_buffer_close(&buf);
    fseeko(stream->file_handle, cur_pos, SEEK_SET);
}

/***********************************************************************
 * hb_ts_stream_init
 ***********************************************************************
 *
 **********************************************************************/

static void hb_ts_stream_init(hb_stream_t *stream)
{
	int i;

	for (i=0; i < kMaxNumberDecodeStreams; i++)
	{
		stream->ts_streamcont[i] = -1;
	}
	stream->ts_video_pids[0] = -1;
    for ( i = 0; i < stream->ts_number_audio_pids; i++ )
    {
        stream-> ts_audio_pids[i] = -1;
    }

    stream->ts_packet = malloc( stream->packetsize );

	// Find the audio and video pids in the stream
	hb_ts_stream_find_pids(stream);

	for (i = 0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
	{
        // demuxing buffer for TS to PS conversion
		stream->ts_buf[i] = malloc( HB_DVD_READ_BUFFER_SIZE );
	}

    stream->ts_streamid[0] = 0xE0;		// stream 0 must be video
}

#define MAX_HOLE 208*80

static off_t align_to_next_packet(hb_stream_t *stream)
{
    uint8_t buf[MAX_HOLE];
	off_t pos = 0;
    off_t start = ftello(stream->file_handle);

    if ( start >= stream->packetsize ) {
        start -= stream->packetsize;
        fseeko(stream->file_handle, start, SEEK_SET);
    }

    if (fread(buf, sizeof(buf), 1, stream->file_handle) == 1)
	{
        const uint8_t *bp = buf;
        int i;

        for ( i = sizeof(buf); --i >= 0; ++bp )
        {
            if ( have_ts_sync( bp, stream->packetsize ) )
            {
                break;
            }
        }
        if ( i >= 0 )
        {
            pos = ( bp - buf ) - stream->packetsize + 188;
            if ( pos < 0 )
                pos = 0;
        }
	}
    fseeko(stream->file_handle, start+pos, SEEK_SET);
	return pos;
}


typedef struct {
    uint8_t *buf;
    uint32_t val;
    int pos;
} bitbuf_t;

static const unsigned int bitmask[] = {
	0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
	0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
	0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
	0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};

static inline void set_buf(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear)
{
	bb->pos = 0;
	bb->buf = buf;
	bb->val = (bb->buf[0] << 24) | (bb->buf[1] << 16) |
              (bb->buf[2] << 8) | bb->buf[3];
	if (clear)
		memset(bb->buf, 0, bufsize);
}

static inline int buf_size(bitbuf_t *bb)
{
	return bb->pos >> 3;
}

static inline unsigned int get_bits(bitbuf_t *bb, int bits)
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
		bb->val = (bb->buf[pos] << 24) | (bb->buf[pos + 1] << 16) | (bb->buf[pos + 2] << 8) | bb->buf[pos + 3];

		if (bits > 0)
		{
			val |= (bb->val >> (32 - bits)) & bitmask[bits];
			bb->pos += bits;
		}
	}

	return val;
}

// extract what useful information we can from the elementary stream
// descriptor list at 'dp' and add it to the stream at 'esindx'.
// Descriptors with info we don't currently use are ignored.
// The descriptor list & descriptor item formats are defined in
// ISO 13818-1 (2000E) section 2.6 (pg. 62).
static void decode_element_descriptors(hb_stream_t* stream, int esindx,
                                       const uint8_t *dp, uint8_t dlen)
{
    const uint8_t *ep = dp + dlen;

    while (dp < ep)
    {
        switch (dp[0])
        {
            case 10:    // ISO_639_language descriptor
                stream->a52_info[esindx].lang_code = lang_to_code(lang_for_code2((const char *)&dp[2]));
                break;

            default:
                break;
        }
        dp += dp[1] + 2;
    }
}

static const char *stream_type_name (uint8_t stream_type)
{
    return st2codec[stream_type].name? st2codec[stream_type].name : "Unknown";
}

int decode_program_map(hb_stream_t* stream)
{
    bitbuf_t bb;
	set_buf(&bb, stream->pmt_info.tablebuf, stream->pmt_info.tablepos, 0);

    get_bits(&bb, 8);  // table_id
    get_bits(&bb, 4);
    unsigned int section_length = get_bits(&bb, 12);
    stream->pmt_info.section_length = section_length;

    unsigned int program_number = get_bits(&bb, 16);
    stream->pmt_info.program_number = program_number;
    get_bits(&bb, 2);
    get_bits(&bb, 5);  // version_number
    get_bits(&bb, 1);
    get_bits(&bb, 8);  // section_number
    get_bits(&bb, 8);  // last_section_number
    get_bits(&bb, 3);
    unsigned int PCR_PID = get_bits(&bb, 13);
    stream->pmt_info.PCR_PID = PCR_PID;
    get_bits(&bb, 4);
    unsigned int program_info_length = get_bits(&bb, 12);
    stream->pmt_info.program_info_length = program_info_length;

	int i=0;
	unsigned char *descriptor_buf = (unsigned char *) malloc(program_info_length);
	for (i = 0; i < program_info_length; i++)
	{
	  descriptor_buf[i] = get_bits(&bb, 8);
	}

	int cur_pos =  9 /* data after the section length field*/ + program_info_length;
	int done_reading_stream_types = 0;
	while (!done_reading_stream_types)
    {
        unsigned char stream_type = get_bits(&bb, 8);
        get_bits(&bb, 3);
        unsigned int elementary_PID = get_bits(&bb, 13);
        get_bits(&bb, 4);
        unsigned int ES_info_length = get_bits(&bb, 12);

        int i=0;
        unsigned char *ES_info_buf = (unsigned char *) malloc(ES_info_length);
        for (i=0; i < ES_info_length; i++)
        {
            ES_info_buf[i] = get_bits(&bb, 8);
        }


        if (stream->ts_number_video_pids == 0 && st2codec[stream_type].kind == V )
        {
            stream->ts_video_pids[0] = elementary_PID;
            stream->ts_stream_type[0] = stream_type;
            stream->ts_number_video_pids = 1;
        }
        else
        {
            // Defined audio stream types are 0x81 for AC-3/A52 audio and 0x03
            // for mpeg audio. But content producers seem to use other
            // values (0x04 and 0x06 have both been observed) so at this point
            // we say everything that isn't a video pid is audio then at the end
            // of hb_stream_title_scan we'll figure out which are really audio
            // by looking at the PES headers.
            i = stream->ts_number_audio_pids;
            if (i < kMaxNumberAudioPIDS)
            {
                stream->ts_audio_pids[i] = elementary_PID;
                stream->ts_stream_type[1 + i] = stream_type;
                if (ES_info_length > 0)
                {
                    decode_element_descriptors(stream, i, ES_info_buf,
                                               ES_info_length);
                }
                ++stream->ts_number_audio_pids;
            }
        }

        cur_pos += 5 /* stream header */ + ES_info_length;

        free(ES_info_buf);

        if (cur_pos >= section_length - 4 /* stop before the CRC */)
        done_reading_stream_types = 1;
    }

	free(descriptor_buf);
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
	if (start && stream->pmt_info.reading)
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
                    set_buf(&bb, tablebuf + pos, tablepos - pos, 0);

                    unsigned char section_id	= get_bits(&bb, 8);
                    get_bits(&bb, 4);
                    unsigned int section_len	= get_bits(&bb, 12);
                    get_bits(&bb, 16); // transport_id
                    get_bits(&bb, 2);
                    get_bits(&bb, 5);  // version_num
                    get_bits(&bb, 1);  // current_next
                    get_bits(&bb, 8);  // section_num
                    get_bits(&bb, 8);  // last_section

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
                            unsigned int pkt_program_num = get_bits(&bb, 16);
							stream->pat_info[stream->ts_number_pat_entries].program_number = pkt_program_num;

                            get_bits(&bb, 3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              get_bits(&bb, 13); // pkt_network_id
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = get_bits(&bb, 13);
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

                    pos += 3 + section_len;
            }

            tablepos = 0;
    }
    return 1;
}

static void hb_ts_stream_find_pids(hb_stream_t *stream)
{
	// align to first packet
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
			// There are some streams where the PAT table has multiple entries as if their are
			// multiple programs in the same transport stream, and yet there's actually only one
			// program really in the stream. This seems to be true for transport streams that
			// originate in the HDHomeRun but have been output by EyeTV's export utility. What I think
			// is happening is that the HDHomeRun is sending the entire transport stream as broadcast,
			// but the EyeTV is only recording a single (selected) program number and not rewriting the
			// PAT info on export to match what's actually on the stream.
			// Until we have a way of handling multiple programs per transport stream elegantly we'll match
			// on the first pat entry for which we find a matching program map PID.  The ideal solution would
			// be to build a title choice popup from the PAT program number details and then select from
			// their - but right now the API's not capable of that.
			if (pid == stream->pat_info[pat_index].program_map_PID)
			{
			  if (build_program_map(buf, stream) > 0)
				break;
			}
		}
		// Keep going  until we have a complete set of PIDs
		if ((stream->ts_number_video_pids > 0) && (stream->ts_number_audio_pids > 0))
		  break;
	}

	hb_log("hb_ts_stream_find_pids - found the following PIDS");
	hb_log("    Video PIDS : ");
    int i;
	for (i=0; i < stream->ts_number_video_pids; i++)
	{
        hb_log( "      0x%x type %s (0x%x)", 
                stream->ts_video_pids[i],
                stream_type_name(stream->ts_stream_type[i]),
                stream->ts_stream_type[i]);
	}
	hb_log("    Audio PIDS : ");
	for (i = 0; i < stream->ts_number_audio_pids; i++)
	{
        hb_log( "      0x%x type %s (0x%x)", 
                stream->ts_audio_pids[i],
                stream_type_name(stream->ts_stream_type[i+1]),
                stream->ts_stream_type[i+1] );
	}
 }


static void fwrite64( hb_stream_t *stream, void *buf, int size )
{
    if ( (stream->fwrite_buf - stream->fwrite_buf_orig) + size > 2048 )
    {
        hb_log( "steam fwrite64 buffer overflow - writing %d with %d already",
                size, stream->fwrite_buf - stream->fwrite_buf_orig );
        return;
    }
    memcpy( stream->fwrite_buf, buf, size );
    stream->fwrite_buf += size;
}

static void write_pack(hb_stream_t* stream, uint64_t time, int stuffing)
{
	uint8_t buf[24];

    buf[0] = 0x00;      // pack id
    buf[1] = 0x00;
    buf[2] = 0x01;
    buf[3] = 0xba;

    buf[4] = 0x44 |     // SCR
             ( ( ( time >> 30 ) & 7 ) << 3 ) |
             ( ( time >> 28 ) & 3 );
    buf[5] = time >> 20;
    buf[6] = 0x04 |
             ( ( ( time >> 15 ) & 0x1f ) << 3 ) |
             ( ( time >> 13 ) & 3 );
    buf[7] = time >> 5;
    buf[8] = 0x04 | ( time << 3 );

    buf[9] = 0x01;      // SCR extension

    buf[10] = 384000 >> (22 - 8);     // program mux rate
    buf[11] = (uint8_t)( 384000 >> (22 - 16) );
    buf[12] = (uint8_t)( 384000 << 2 ) | 0x03;

    buf[13] = 0xf8 | stuffing;

    int i;
    for (i = 0; i < stuffing; ++i )
        buf[14+i] = 0xff;

	fwrite64(stream, buf, 14 + stuffing );
}

static void pad_buffer(hb_stream_t* stream, int pad)
{
	pad -= 6;

	uint8_t buf[6];
	buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0xbe;
	buf[4] = pad >> 8;
    buf[5] = pad;

	fwrite64(stream, buf, 6);

	buf[0] = 0xff;
    while ( --pad >= 0 )
    {
		fwrite64(stream, buf, 1);
	}
}

static void make_pes_header(hb_stream_t* stream, int len, uint8_t streamid)
{
	uint8_t buf[9];

    memset(buf, 0, sizeof(buf) );
    buf[2] = 1;
    buf[3] = streamid;
    buf[4] = ( len + 3 ) >> 8;
    buf[5] = len + 3;
    buf[6] = 0x88;

    fwrite64(stream, buf, 9);
}

static void generate_output_data(hb_stream_t *stream, int curstream)
{
    uint8_t *tdat = stream->ts_buf[curstream];
    int len;

    // we always ship a PACK header plus all the data in our demux buf.
    // AC3 audio also always needs its substream header.
    len = 14 + stream->ts_pos[curstream];
    if ( stream->ts_stream_type[curstream] == 0x81)
    {
        len += 4;
    }

    if ( ! stream->ts_start[curstream] )
    {
        // we're in the middle of a chunk of PES data - we need to add
        // a 'continuation' PES header after the PACK header.
        len += 9;
    }

    // Write out pack header
    // If we don't have 2048 bytes we need to pad to 2048. We can
    // add a padding frame after our data but we need at least 7
    // bytes of space to do it (6 bytes of header & 1 of pad). If
    // we have fewer than 7 bytes left we need to fill the excess
    // space with stuffing bytes added to the pack header.
    int stuffing = 0;
    if ( len > HB_DVD_READ_BUFFER_SIZE )
    {
        hb_log( "stream ts length botch %d", len );
    }
    if ( HB_DVD_READ_BUFFER_SIZE - len < 8)
    {
        stuffing = HB_DVD_READ_BUFFER_SIZE - len;
    }
    write_pack(stream, stream->ts_nextpcr, stuffing );
    stream->ts_nextpcr += 10;

    if ( stream->ts_start[curstream] )
    {
        // Start frames already have a PES header but we have modify it
        // to map from TS PID to PS stream id. Also, if the stream is AC3
        // audio we have to insert an AC3 stream header between the end of
        // the PES header and the start of the stream data.

        stream->ts_start[curstream] = 0;
        tdat[3] = stream->ts_streamid[curstream];

        uint16_t plen = stream->ts_pos[curstream] - 6;
        if ( stream->ts_stream_type[curstream] == 0x81)
        {
            // We have to add an AC3 header in front of the data. Add its
            // size to the PES packet length.
            plen += 4;
            tdat[4] = plen >> 8;
            tdat[5] = plen;

            // Write out the PES header
            int hdrsize = 9 + tdat[8];
            fwrite64(stream, tdat, hdrsize);

            // add a four byte DVD ac3 stream header
            uint8_t ac3_substream_id[4];
            int ssid = (curstream - stream->ts_number_video_pids) & 0xf;
            ac3_substream_id[0] = 0x80 | ssid;  // substream id
            ac3_substream_id[1] = 0x01;         // number of sync words
            ac3_substream_id[2] = 0x00;         // first offset (16 bits)
            ac3_substream_id[3] = 0x02;
            fwrite64(stream, ac3_substream_id, 4);

            // add the rest of the data
            fwrite64(stream, tdat + hdrsize, stream->ts_pos[curstream] - hdrsize);
        }
        else
        {
            // not audio - don't need to modify the stream so write what we've got
            tdat[4] = plen >> 8;
            tdat[5] = plen;
            fwrite64( stream,  tdat, stream->ts_pos[curstream] );
        }
    }
    else
    {
        // data without a PES start header needs a simple 'continuation'
        // PES header. AC3 audio also needs its substream header.
        if ( stream->ts_stream_type[curstream] != 0x81)
        {
            make_pes_header(stream, stream->ts_pos[curstream],
                            stream->ts_streamid[curstream]);
        }
        else
        {
            make_pes_header(stream, stream->ts_pos[curstream] + 4,
                            stream->ts_streamid[curstream]);

            // add a four byte DVD ac3 stream header
            uint8_t ac3_substream_id[4];
            int ssid = (curstream - stream->ts_number_video_pids) & 0xf;
            ac3_substream_id[0] = 0x80 | ssid;  // substream id
            ac3_substream_id[1] = 0x01;         // number of sync words
            ac3_substream_id[2] = 0x00;         // first offset (16 bits)
            ac3_substream_id[3] = 0x02;
            fwrite64(stream, ac3_substream_id, 4);
        }
        fwrite64( stream, tdat, stream->ts_pos[curstream] );
    }

    // Write padding
    int left = HB_DVD_READ_BUFFER_SIZE - len;
    if ( left >= 8 )
    {
        pad_buffer(stream, left);
    }

    stream->ts_pos[curstream] = 0;
}

static int isIframe( hb_stream_t *stream, const uint8_t *buf, int adapt_len )
{
    // For mpeg2: look for a gop start or i-frame picture start
    // for h.264: look for idr nal type or a slice header for an i-frame
    // for vc1:   ???
    int i;
    uint32_t strid = 0;


    if ( stream->ts_stream_type[0] <= 2 )
    {
        // This section of the code handles MPEG-1 and MPEG-2 video streams
        for (i = 13 + adapt_len; i < 188; i++)
        {
            strid = (strid << 8) | buf[i];
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
                        if (i<185)
                        {
                            // check if picture_coding_type == 1
                            if ((buf[i+2] & (0x7 << 3)) == (1 << 3))
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
    if ( stream->ts_stream_type[0] == 0x1b )
    {
        // we have an h.264 stream 
        for (i = 13 + adapt_len; i < 188; i++)
        {
            strid = (strid << 8) | buf[i];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code - remove the ref_idc from the nal type
                uint8_t nal_type = strid & 0x1f;
                if ( nal_type == 0x05 )
                    // h.264 IDR picture start
                    return 1;

                if ( nal_type == 0x01 )
                {
                    // h.264 slice: has to be start MB 0 & type I (2, 4, 7 or 9)
                    uint8_t id = buf[i+1];
                    if ( ( id >> 4 ) == 0x0b || ( id >> 2 ) == 0x25 ||
                         id == 0x88 || id == 0x8a )
                    {
                        return 1;
                    }
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }

    // we don't understand the stream type so just say "yes" otherwise
    // we'll discard all the video.
    return 1;
}

/***********************************************************************
 * hb_ts_stream_decode
 ***********************************************************************
 *
 **********************************************************************/
static int hb_ts_stream_decode( hb_stream_t *stream, uint8_t *obuf )
{
    /*
     * stash the output buffer pointer in our stream so we don't have to
     * pass it & its original value to everything we call.
     */
    stream->fwrite_buf = obuf;
    stream->fwrite_buf_orig = obuf;

	// spin until we get a packet of data from some stream or hit eof
	while ( 1 )
	{
        int64_t pcr = stream->ts_lastpcr;
        int curstream;

        const uint8_t *buf = next_packet(stream);
        if ( buf == NULL )
        {
            // end of file - we didn't finish filling our ps write buffer
            // so just discard the remainder (the partial buffer is useless)
            hb_log("hb_ts_stream_decode - eof");
            return 0;
		}

        /* This next section validates the packet */

		// Get pid and use it to find stream state.
		int pid = ((buf[1] & 0x1F) << 8) | buf[2];
        if ( ( curstream = index_of_pid( pid, stream ) ) < 0 )
            continue;

		// Get error
		int errorbit = (buf[1] & 0x80) != 0;
		if (errorbit)
		{
			ts_err( stream, curstream,  "packet error bit set");
			continue;
		}

		// Get adaption header info
		int adaption = (buf[3] & 0x30) >> 4;
		int adapt_len = 0;
		if (adaption == 0)
		{
			ts_err( stream, curstream,  "adaptation code 0");
			continue;
		}
		else if (adaption == 0x2)
			adapt_len = 184;
		else if (adaption == 0x3)
		{
			adapt_len = buf[4] + 1;
			if (adapt_len > 184)
			{
				ts_err( stream, curstream,  "invalid adapt len %d", adapt_len);
                continue;
			}
		}

        // if there's an adaptation header & PCR_flag is set
        // get the PCR (Program Clock Reference)
        if ( adapt_len > 7 && ( buf[5] & 0x10 ) != 0 )
        {
            pcr = ( (uint64_t)buf[6] << (33 - 8) ) |
                  ( (uint64_t)buf[7] << (33 - 16) ) |
                  ( (uint64_t)buf[8] << (33 - 24) ) |
                  ( (uint64_t)buf[9] << (33 - 32) ) |
                  ( buf[10] >> 7 );
            stream->ts_nextpcr = pcr;

            // remember the pcr across calls to this routine
            stream->ts_lastpcr = pcr;
        }

		if ( pcr == -1 )
		{
            // don't accumulate data until we get a pcr
		    continue;
		}

		// Get continuity
        // Continuity only increments for adaption values of 0x3 or 0x01
        // and is not checked for start packets.

		int start = (buf[1] & 0x40) != 0;

        if ( (adaption & 0x01) != 0 )
		{
            int continuity = (buf[3] & 0xF);
            if ( continuity == stream->ts_streamcont[curstream] )
            {
                // we got a duplicate packet (usually used to introduce
                // a PCR when one is needed). The only thing that can
                // change in the dup is the PCR which we grabbed above
                // so ignore the rest.
                continue;
            }
            if ( !start && (stream->ts_streamcont[curstream] != -1) &&
                 (continuity != ( (stream->ts_streamcont[curstream] + 1) & 0xf ) ) )
			{
				ts_err( stream, curstream,  "continuity error: got %d expected %d",
                        (int)continuity,
                        (stream->ts_streamcont[curstream] + 1) & 0xf );
                stream->ts_streamcont[curstream] = continuity;
				continue;
			}
			stream->ts_streamcont[curstream] = continuity;
		}

        /* If we get here the packet is valid - process its data */

        if ( start )
        {
            // Found a random access point (now we can start a frame/audio packet..)

			// If we were skipping a bad packet, start fresh on this new PES packet..
			if (stream->ts_skipbad[curstream] == 1)
			{
                // video skips to an iframe after a bad packet to minimize
                // screen corruption
                if ( curstream == 0 && !isIframe( stream, buf, adapt_len ) )
                {
                    continue;
                }
				stream->ts_skipbad[curstream] = 0;
			}

			// If we don't have video yet, check to see if this is an
            // i_frame (group of picture start)
			if ( curstream == 0 )
            {
                if ( !stream->ts_foundfirst[0] )
                {
                    if ( !isIframe( stream, buf, adapt_len ) )
                    {
                        // didn't find an I frame
                        continue;
                    }
                    stream->ts_foundfirst[0] = 1;
                }
                ++stream->frames;
            }
			else if ( ! stream->ts_foundfirst[curstream] )
            {
                // start other streams only after first video frame found.
                if ( ! stream->ts_foundfirst[0] )
                {
                    continue;
                }
                stream->ts_foundfirst[curstream] = 1;
			}

            // If we have some data already on this stream, turn it into
            // a program stream packet. Then add the payload for this
            // packet to the current pid's buffer.
            if ( stream->ts_pos[curstream] )
            {
                generate_output_data(stream, curstream);
                stream->ts_start[curstream] = 1;
                memcpy(stream->ts_buf[curstream],
                       buf + 4 + adapt_len, 184 - adapt_len);
                stream->ts_pos[curstream] = 184 - adapt_len;
                return 1;
            }
            stream->ts_start[curstream] = 1;
        }

		// Add the payload for this packet to the current buffer
		if (!stream->ts_skipbad[curstream] && stream->ts_foundfirst[curstream] &&
            (184 - adapt_len) > 0)
		{
			memcpy(stream->ts_buf[curstream] + stream->ts_pos[curstream],
                   buf + 4 + adapt_len, 184 - adapt_len);
			stream->ts_pos[curstream] += 184 - adapt_len;

            // if the next TS packet could possibly overflow our 2K output buffer
            // we need to generate a packet now. Overflow would be 184 bytes of
            // data + the 9 byte PES hdr + the 14 byte PACK hdr = 211 bytes.
            if ( stream->ts_pos[curstream] >= (HB_DVD_READ_BUFFER_SIZE - 216) )
            {
                // we have enough data to make a PS packet
                generate_output_data(stream, curstream);
                return 1;
            }
		}
	}
}

static void hb_ts_stream_reset(hb_stream_t *stream)
{
	int i;

	for (i=0; i < kMaxNumberDecodeStreams; i++)
	{
		stream->ts_pos[i] = 0;
		stream->ts_foundfirst[i] = 0;
		stream->ts_skipbad[i] = 0;
		stream->ts_streamcont[i] = -1;
		stream->ts_start[i] = 0;
	}

    stream->ts_lastpcr = -1;
    stream->ts_nextpcr = -1;

    stream->frames = 0;
    stream->errors = 0;
    stream->last_error_frame = -10000;
    stream->last_error_count = 0;

    align_to_next_packet(stream);
}

// ------------------------------------------------------------------
// Support for reading media files via the ffmpeg libraries.

static void ffmpeg_add_codec( hb_stream_t *stream, int stream_index )
{
    // add a codec to the context here so it will be there when we
    // read the first packet.
    AVCodecContext *context = stream->ffmpeg_ic->streams[stream_index]->codec;
    context->workaround_bugs = FF_BUG_AUTODETECT;
    context->error_resilience = 1;
    context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;
    AVCodec *codec = avcodec_find_decoder( context->codec_id );
    avcodec_open( context, codec );
}

// The ffmpeg stream reader / parser shares a lot of state with the 
// decoder via a codec context kept in the AVStream of the reader's
// AVFormatContext. Since decoding is done in a different thread we
// have to somehow pass this codec context to the decoder and we have
// to do it before the first packet is read (so we can't put the info
// in the buf we'll send downstream). Decoders don't have any way to
// get to the stream directly (they're not passed the title or job
// pointers during a scan) so this is a back door for the decoder to
// get the codec context. We just stick the stream pointer in the next
// slot an array of pointers maintained as a circular list then return
// the index into the list combined with the ffmpeg stream index as the
// codec_param that will be passed to the decoder init routine. We make
// the list 'big' (enough for 1024 simultaneously open ffmpeg streams)
// so that we don't have to do a complicated allocator or worry about
// deleting entries on close. 
//
// Entries can only be added to this list during a scan and are never
// deleted so the list access doesn't require locking.
static hb_stream_t **ffmpeg_streams;    // circular list of stream pointers
static int ffmpeg_stream_cur;           // where we put the last stream pointer
#define ffmpeg_sl_bits (10)             // log2 stream list size (in entries)
#define ffmpeg_sl_size (1 << ffmpeg_sl_bits)

// add a stream to the list & return the appropriate codec_param to access it
static int ffmpeg_codec_param( hb_stream_t *stream, int stream_index )
{
    if ( !ffmpeg_streams )
    {
        ffmpeg_streams = calloc( ffmpeg_sl_size, sizeof(stream) );
    }

    // the title scan adds all the ffmpeg media streams at once so we
    // only add a new entry to our stream list if the stream is different
    // than last time.
    int slot = ffmpeg_stream_cur;
    if ( ffmpeg_streams[slot] != stream )
    {
        // new stream - put it in the next slot of the stream list
        slot = ++ffmpeg_stream_cur & (ffmpeg_sl_size - 1);
        ffmpeg_streams[slot] = stream;
    }

    ffmpeg_add_codec( stream, stream_index );

    return ( stream_index << ffmpeg_sl_bits ) | slot;
}

// we're about to open 'title' to convert it - remap the stream associated
// with the video & audio codec params of the title to refer to 'stream'
// (the original scan stream was closed and no longer exists).
static void ffmpeg_remap_stream( hb_stream_t *stream, hb_title_t *title )
{
    // all the video & audio came from the same stream so remapping
    // the video's stream slot takes care of everything.
    int slot = title->video_codec_param & (ffmpeg_sl_size - 1);
    ffmpeg_streams[slot] = stream;

    // add codecs for all the streams used by the title
    ffmpeg_add_codec( stream, title->video_codec_param >> ffmpeg_sl_bits );

    int i;
    hb_audio_t *audio;
    for ( i = 0; ( audio = hb_list_item( title->list_audio, i ) ); ++i )
    {
        if ( audio->config.in.codec == HB_ACODEC_FFMPEG )
        {
            ffmpeg_add_codec( stream,
                              audio->config.in.codec_param >> ffmpeg_sl_bits );
        }
    }
}

void *hb_ffmpeg_context( int codec_param )
{
    int slot = codec_param & (ffmpeg_sl_size - 1);
    int stream_index = codec_param >> ffmpeg_sl_bits;
    return ffmpeg_streams[slot]->ffmpeg_ic->streams[stream_index]->codec;
}

void *hb_ffmpeg_avstream( int codec_param )
{
    int slot = codec_param & (ffmpeg_sl_size - 1);
    int stream_index = codec_param >> ffmpeg_sl_bits;
    return ffmpeg_streams[slot]->ffmpeg_ic->streams[stream_index];
}

static AVFormatContext *ffmpeg_deferred_close;

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title )
{
    if ( ffmpeg_deferred_close )
    {
        av_close_input_file( ffmpeg_deferred_close );
        ffmpeg_deferred_close = NULL;
    }
    AVFormatContext *ic;

    av_log_set_level( AV_LOG_ERROR );
    if ( av_open_input_file( &ic, stream->path, NULL, 0, NULL ) < 0 )
    {
        return 0;
    }
    if ( av_find_stream_info( ic ) < 0 )
        goto fail;

    stream->ffmpeg_ic = ic;
    stream->hb_stream_type = ffmpeg;

    if ( title )
    {
        // we're opening for read. scan passed out codec params that
        // indexed its stream so we need to remap them so they point
        // to this stream.
        ffmpeg_remap_stream( stream, title );
        ffmpeg_seek( stream, 0. );
        av_log_set_level( AV_LOG_ERROR );
    }
    else
    {
        // we're opening for scan. let ffmpeg put some info into the
        // log about what we've got.
        av_log_set_level( AV_LOG_INFO );
        dump_format( ic, 0, stream->path, 0 );
        av_log_set_level( AV_LOG_ERROR );

        // accept this file if it has at least one video stream we can decode
        int i;
        for (i = 0; i < ic->nb_streams; ++i )
        {
            if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
            {
                break;
            }
        }
        if ( i >= ic->nb_streams )
            goto fail;
    }
    return 1;

  fail:
    av_close_input_file( ic );
    return 0;
}

static void ffmpeg_close( hb_stream_t *d )
{
    // XXX since we're sharing the CodecContext with the downstream
    // decoder proc we can't close the stream. We need to reference count
    // this so we can close it when both are done with their instance but
    // for now just defer the close until the next stream open or close.
    if ( ffmpeg_deferred_close )
    {
        av_close_input_file( ffmpeg_deferred_close );
    }
    ffmpeg_deferred_close = d->ffmpeg_ic;
}

static void add_ffmpeg_audio( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;

    // scan will ignore any audio without a bitrate. Since we've already
    // typed the audio in order to determine its codec we set up the audio
    // paramters here.
    if ( codec->bit_rate || codec->sample_rate )
    {
        static const int chan2layout[] = {
            HB_INPUT_CH_LAYOUT_MONO,  // We should allow no audio really.
            HB_INPUT_CH_LAYOUT_MONO,   
            HB_INPUT_CH_LAYOUT_STEREO,
            HB_INPUT_CH_LAYOUT_2F1R,   
            HB_INPUT_CH_LAYOUT_2F2R,
            HB_INPUT_CH_LAYOUT_3F2R,   
            HB_INPUT_CH_LAYOUT_4F2R,
            HB_INPUT_CH_LAYOUT_STEREO, 
            HB_INPUT_CH_LAYOUT_STEREO,
        };

        hb_audio_t *audio = calloc( 1, sizeof(*audio) );;

        audio->id = id;
        if ( codec->codec_id == CODEC_ID_AC3 )
        {
            audio->config.in.codec = HB_ACODEC_AC3;
        }
        else
        {
            audio->config.in.codec = HB_ACODEC_FFMPEG;
            audio->config.in.codec_param = ffmpeg_codec_param( stream, id );

            audio->config.in.bitrate = codec->bit_rate? codec->bit_rate : 1;
            audio->config.in.samplerate = codec->sample_rate;
            audio->config.in.channel_layout = chan2layout[codec->channels & 7];
        }

        set_audio_description( audio, lang_for_code2( st->language ) );

        hb_list_add( title->list_audio, audio );
    }
}

static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream )
{
    AVFormatContext *ic = stream->ffmpeg_ic;

    // 'Barebones Title'
    hb_title_t *title = hb_title_init( stream->path, 0 );
    title->index = 1;

	// Copy part of the stream path to the title name
	char *sep = strrchr(stream->path, '/');
	if (sep)
		strcpy(title->name, sep+1);
	char *dot_term = strrchr(title->name, '.');
	if (dot_term)
		*dot_term = '\0';

    uint64_t dur = ic->duration * 90000 / AV_TIME_BASE;
    title->duration = dur;
    dur /= 90000;
    title->hours    = dur / 3600;
    title->minutes  = ( dur % 3600 ) / 60;
    title->seconds  = dur % 60;

    // One Chapter
    hb_chapter_t * chapter;
    chapter = calloc( sizeof( hb_chapter_t ), 1 );
    chapter->index = 1;
    chapter->duration = title->duration;
    chapter->hours = title->hours;
    chapter->minutes = title->minutes;
    chapter->seconds = title->seconds;
    hb_list_add( title->list_chapter, chapter );

    // set the title to decode the first video stream in the file
    title->demuxer = HB_NULL_DEMUXER;
    title->video_codec = 0;
    int i;
    for (i = 0; i < ic->nb_streams; ++i )
    {
        if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO &&
             avcodec_find_decoder( ic->streams[i]->codec->codec_id ) &&
             title->video_codec == 0 )
        {
            title->video_id = i;

            // We have to use the 'internal' avcodec decoder because
            // it needs to share the codec context from this video
            // stream. The parser internal to av_read_frame
            // passes a bunch of state info to the decoder via the context.
            title->video_codec = WORK_DECAVCODECVI;
            title->video_codec_param = ffmpeg_codec_param( stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO &&
                  avcodec_find_decoder( ic->streams[i]->codec->codec_id ) )
        {
            add_ffmpeg_audio( title, stream, i );
        }
    }

    title->container_name = strdup( ic->iformat->name );
    title->data_rate = ic->bit_rate;

    return title;
}

static int64_t av_to_hb_pts( int64_t pts, double conv_factor )
{
    if ( pts == AV_NOPTS_VALUE )
        return -1;
    return (int64_t)( (double)pts * conv_factor );
}

static int ffmpeg_read( hb_stream_t *stream, hb_buffer_t *buf )
{
    AVPacket pkt;

    if ( av_read_frame( stream->ffmpeg_ic, &pkt ) < 0 )
    {
        return 0;
    }
    if ( pkt.size > buf->alloc )
    {
        // need to expand buffer
        hb_buffer_realloc( buf, pkt.size );
    }
    memcpy( buf->data, pkt.data, pkt.size );
    buf->id = pkt.stream_index;
    buf->size = pkt.size;
    int64_t pts = pkt.pts != AV_NOPTS_VALUE? pkt.pts : 
                         pkt.dts != AV_NOPTS_VALUE? pkt.dts : -1;
    buf->start = av_to_hb_pts( pts,
                  av_q2d(stream->ffmpeg_ic->streams[pkt.stream_index]->time_base)*90000. );
    buf->renderOffset = av_to_hb_pts( pkt.pts,
                  av_q2d(stream->ffmpeg_ic->streams[pkt.stream_index]->time_base)*90000. );
    av_free_packet( &pkt );
    return 1;
}

static int ffmpeg_seek( hb_stream_t *stream, float frac )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    int64_t pos = (double)ic->duration * (double)frac;
    av_seek_frame( ic, -1, pos, pos? 0 : AVSEEK_FLAG_BACKWARD );
    return 1;
}
