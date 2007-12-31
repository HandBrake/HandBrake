/* $Id$

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "lang.h"
#include "a52dec/a52.h"

#include <string.h>

#define min(a, b) a < b ? a : b

typedef enum { hb_stream_type_unknown = 0, hb_stream_type_transport, hb_stream_type_program } hb_stream_type_t;
 
#define kMaxNumberDecodeStreams 8
#define kMaxNumberVideoPIDS 16
#define kMaxNumberAudioPIDS 16
//#define kVideoStream 0
//#define kAudioStream 1
#define kNumDecodeBuffers 2
#define kMaxNumberPMTStreams 32

#define CLOCKRATE		((int64_t)27000000)			// MPEG System clock rate
#define STREAMRATE		((int64_t)2401587)			// Original HD stream rate 19.2 Mbps
#define DEMUX			(((int)STREAMRATE * 8) / 50)// Demux value for HD content STREAMRATE / 50

struct hb_stream_s
{
    char         * path;
	FILE		 * file_handle;
	hb_stream_type_t stream_type;
	
	int				 ps_current_write_buffer_index;
	int				 ps_current_read_buffer_index;

	struct {
		int				 size;
		int				 len;
		int				 read_pos;
		int				 write_pos;
		unsigned char *  data;
	} ps_decode_buffer[kNumDecodeBuffers];
	
	struct {
		int lang_code;
		int flags;
		int rate;
		int bitrate;
	} a52_info[kMaxNumberAudioPIDS];
	
	int				 ts_video_pids[kMaxNumberVideoPIDS];
	int				 ts_audio_pids[kMaxNumberAudioPIDS];
	
	int				 ts_number_video_pids;
	int				 ts_number_audio_pids;
	
	unsigned char*	 ts_packetbuf[kMaxNumberDecodeStreams];
	int				 ts_packetpos[kMaxNumberDecodeStreams];
//	int				 ts_bufpackets[kMaxNumberDecodeStreams];
	int				 ts_foundfirst[kMaxNumberDecodeStreams];
	int				 ts_skipbad[kMaxNumberDecodeStreams];
	int				 ts_streamcont[kMaxNumberDecodeStreams];
	int				 ts_streamid[kMaxNumberDecodeStreams];
	int				 ts_audio_stream_type[kMaxNumberAudioPIDS];
	
	struct 
	{
		unsigned short program_number;
		unsigned short program_map_PID;
	} pat_info[kMaxNumberPMTStreams];
	int	 ts_number_pat_entries;
	
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
static void hb_ts_stream_decode(hb_stream_t *stream);
static void hb_ts_stream_reset(hb_stream_t *stream);
static void hb_stream_put_back(hb_stream_t *stream, int i);
static hb_audio_t *hb_ts_stream_set_audio_id_and_codec(hb_stream_t *stream,
                                                       int aud_pid_index);
static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title);
static off_t align_to_next_packet(FILE* f);


static inline int check_ps_sync(const uint8_t *buf)
{
    // must have a Pack header
    // (note: the operator '&' instead of '&&' is used deliberately for better
    // performance with a multi-issue pipeline.)
    return (buf[0] == 0x00) & (buf[1] == 0x00) & (buf[2] == 0x01) & (buf[3] == 0xba);
}

static inline int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte, no scrambling & a legal adaptation ctrl
    return (buf[0] == 0x47) & ((buf[3] >> 6) == 0) & ((buf[3] >> 4) > 0);
}

static inline int have_ts_sync(const uint8_t *buf)
{
    return check_ts_sync(&buf[0*188]) & check_ts_sync(&buf[1*188]) &
           check_ts_sync(&buf[2*188]) & check_ts_sync(&buf[3*188]) &
           check_ts_sync(&buf[4*188]) & check_ts_sync(&buf[5*188]) &
           check_ts_sync(&buf[6*188]) & check_ts_sync(&buf[7*188]);
}

static int hb_stream_check_for_ts(const uint8_t *buf)
{
    // transport streams should have a sync byte every 188 bytes.
    // search the first KB of buf looking for at least 8 consecutive
    // correctly located sync patterns.
    int offset = 0;

    for ( offset = 0; offset < 1024; ++offset )
    {
        if ( have_ts_sync( &buf[offset]) )
            return 1;
    }
    return 0;
}

static int hb_stream_check_for_ps(const uint8_t *buf)
{
    // program streams should have a Pack header every 2048 bytes.
    // check that we have 4 of these.
    return check_ps_sync(&buf[0*2048]) & check_ps_sync(&buf[1*2048]) &
           check_ps_sync(&buf[2*2048]) & check_ps_sync(&buf[3*2048]);
}

static int hb_stream_get_type(hb_stream_t *stream)
{
    uint8_t buf[2048*4];

    if ( fread(buf, 1, sizeof(buf), stream->file_handle) == sizeof(buf) )
    {
        if ( hb_stream_check_for_ts(buf) != 0 )
        {
            hb_log("file is MPEG Transport Stream");
            stream->stream_type = hb_stream_type_transport;
            hb_ts_stream_init(stream);
            return 1;
        }
        if ( hb_stream_check_for_ps(buf) != 0 )
        {
            hb_log("file is MPEG Program Stream");
            stream->stream_type = hb_stream_type_program;
            return 1;
        }
    }
    return 0;
}

/***********************************************************************
 * hb_stream_open
 ***********************************************************************
 *
 **********************************************************************/
hb_stream_t * hb_stream_open( char * path )
{
    hb_stream_t * d;

    d = calloc( sizeof( hb_stream_t ), 1 );

    /* Open device */
    if( ( d->file_handle = fopen( path, "rb" ) ) )
    {
        d->path = strdup( path );
        if ( hb_stream_get_type( d ) != 0 )
        {
            return d;
        }
        fclose( d->file_handle );
        free( d->path );
    }
    else
    {
        hb_log( "hb_stream_open: fopen failed (%s)", path );
    }
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
    hb_stream_t * d = *_d;

    if( d->file_handle )
    {
        fclose( d->file_handle );
		d->file_handle = NULL;
    }

	int i=0;
	for (i = 0; i < kNumDecodeBuffers; i++)
	{
		if (d->ps_decode_buffer[i].data)
		{
			free(d->ps_decode_buffer[i].data);
			d->ps_decode_buffer[i].data = NULL;
		}
	}
	
	for (i = 0; i < kMaxNumberDecodeStreams; i++)
	{
		if (d->ts_packetbuf[i])
		{
			free(d->ts_packetbuf[i]);
			d->ts_packetbuf[i] = NULL;
		}
	}
    free( d->path );
    free( d );
    *_d = NULL;
}

static void hb_stream_delete_audio_entry(hb_stream_t *stream, int indx)
{
    int i;

    for (i = indx+1; i < stream->ts_number_audio_pids; ++i)
    {
        stream->ts_audio_pids[indx] = stream->ts_audio_pids[i];
        stream->ts_audio_stream_type[indx] = stream->ts_audio_stream_type[i];
        ++indx;
    }
    --stream->ts_number_audio_pids;
}

/***********************************************************************
 * hb_ps_stream_title_scan
 ***********************************************************************
 *
 **********************************************************************/
hb_title_t * hb_stream_title_scan(hb_stream_t *stream)
{
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
	if (stream->stream_type == hb_stream_type_transport)
	{
        int i;

        for (i=0; i < stream->ts_number_audio_pids; i++)
        {
            hb_audio_t *audio = hb_ts_stream_set_audio_id_and_codec(stream, i);
            if (audio->codec)
                hb_list_add( aTitle->list_audio, audio );
            else
            {
                free(audio);
                hb_stream_delete_audio_entry(stream, i);
                --i;
            }
        }
	}
    else
    {
        hb_ps_stream_find_audio_ids(stream, aTitle);
    }

  return aTitle;
}

/*
 * scan the next MB of 'stream' to find the next start packet for
 * the Packetized Elementary Stream associated with TS PID 'pid'.
 */
static const uint8_t *hb_ts_stream_getPEStype(hb_stream_t *stream, uint32_t pid)
{
    static uint8_t buf[188];
    int npack = 100000; // max packets to read

    while (--npack >= 0)
    {
        if (fread(buf, 1, 188, stream->file_handle) != 188)
        {
            hb_log("hb_ts_stream_getPEStype: EOF while searching for PID 0x%x", pid);
            return 0;
        }
        if (buf[0] != 0x47)
        {
            hb_log("hb_ts_stream_getPEStype: lost sync while searching for PID 0x%x", pid);
            align_to_next_packet(stream->file_handle);
            continue;
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
        return &buf[udata];
    }

    /* didn't find it */
    return 0;
}

/***********************************************************************
 * hb_stream_duration
 ***********************************************************************
 *
 **********************************************************************/
static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle)
{
	// VOB Files often have exceedingly unusual PTS values in them - they will progress for a while
	// and then reset without warning ! 
	if  (strstr(stream->path,".vob") != NULL) 
	{
		// So we'll use a 'fake duration' that should give enough time !
		int64_t duration = 4 * 3600 * 90000;
		inTitle->duration = duration; //90LL * dvdtime2msec( &d->pgc->playback_time );
		inTitle->hours    = inTitle->duration / 90000 / 3600;
		inTitle->minutes  = ( ( inTitle->duration / 90000 ) % 3600 ) / 60;
		inTitle->seconds  = ( inTitle->duration / 90000 ) % 60;
		return;
	}

    uint64_t first_pts, last_pts;
    const uint8_t *buf;
    
    // To calculate the duration we get video presentation time stamps
    // at a couple places in the file then use their difference scaled
    // by the ratio of the distance between our measurement points &
    // the size of the file. The issue is that our video file may have
    // chunks from several different program fragments (main feature,
    // commercials, station id, trailers, etc.) all with their on base
    // pts value. We need to difference two pts's from the same program
    // fragment. Since extraneous material is very likely at the beginning
    // and end of the content, for now we take a time stamp from 25%
    // into the file & 75% into the file then double their difference
    // to get the total duration.
    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    fseeko(stream->file_handle, fsize >> 2, SEEK_SET);
    align_to_next_packet(stream->file_handle);
    buf = hb_ts_stream_getPEStype(stream, stream->ts_video_pids[0]);
    if (buf == NULL)
    {
        hb_log("hb_stream_duration: couldn't find video start packet");
        return;
    }
    if ((buf[7] >> 7) != 1)
    {
        hb_log("hb_stream_duration: no PTS in initial video packet");
        return;
    }
    first_pts = ( ( (uint64_t)buf[9] >> 1 ) & 7 << 30 ) |
                ( (uint64_t)buf[10] << 22 ) |
                ( ( (uint64_t)buf[11] >> 1 ) << 15 ) |
                ( (uint64_t)buf[12] << 7 ) |
                ( (uint64_t)buf[13] >> 1 );
    hb_log("hb_stream_duration: first pts %lld", first_pts);

    // now get a pts from a frame near the end of the file.
    fseeko(stream->file_handle, fsize - (fsize >> 2), SEEK_SET);
    align_to_next_packet(stream->file_handle);
    buf = hb_ts_stream_getPEStype(stream, stream->ts_video_pids[0]);
    if (buf == NULL)
    {
        hb_log("hb_stream_duration: couldn't find video end packet");
        return;
    }
    if ((buf[7] >> 7) != 1)
    {
        hb_log("hb_stream_duration: no PTS in final video packet");
        inTitle->duration = 0;
        return;
    }
    last_pts = ( ( (uint64_t)buf[9] >> 1 ) & 7 << 30 ) |
                ( (uint64_t)buf[10] << 22 ) |
                ( ( (uint64_t)buf[11] >> 1 ) << 15 ) |
                ( (uint64_t)buf[12] << 7 ) |
                ( (uint64_t)buf[13] >> 1 );
    hb_log("hb_stream_duration: last pts %lld", last_pts);

    inTitle->duration = (last_pts - first_pts) * 2 + 90000 * 60;
    inTitle->hours    = inTitle->duration / 90000 / 3600;
    inTitle->minutes  = ( ( inTitle->duration / 90000 ) % 3600 ) / 60;
    inTitle->seconds  = ( inTitle->duration / 90000 ) % 60;

    rewind(stream->file_handle);
}

/***********************************************************************
 * hb_stream_read
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_read( hb_stream_t * src_stream, hb_buffer_t * b )
{
  if (src_stream->stream_type == hb_stream_type_program)
  {
	  size_t amt_read;
	  amt_read = fread(b->data, HB_DVD_READ_BUFFER_SIZE, 1, src_stream->file_handle);
	  if (amt_read > 0)
		return 1;
	  else
		return 0;
  }
  else if  (src_stream->stream_type == hb_stream_type_transport)
  {
	int read_buffer_index = src_stream->ps_current_read_buffer_index;

	// Transport streams are a little more complex  - we might be able to just
	// read from the transport stream conversion buffer (if there's enough data)
	// or we may need to transfer what's left and fill it again.
	if (src_stream->ps_decode_buffer[read_buffer_index].len - src_stream->ps_decode_buffer[read_buffer_index].read_pos > HB_DVD_READ_BUFFER_SIZE)
	{
		memcpy(b->data, src_stream->ps_decode_buffer[read_buffer_index].data + src_stream->ps_decode_buffer[read_buffer_index].read_pos,HB_DVD_READ_BUFFER_SIZE);
		src_stream->ps_decode_buffer[read_buffer_index].read_pos += HB_DVD_READ_BUFFER_SIZE;
		return 1;
	}
	else
	{
		// Not quite enough data in the buffer - transfer what is present, fill the buffer and then 
		// transfer what's still needed.
		int transfer_size = HB_DVD_READ_BUFFER_SIZE;
		int amt_avail_to_transfer = src_stream->ps_decode_buffer[read_buffer_index].len - src_stream->ps_decode_buffer[read_buffer_index].read_pos;
		memcpy(b->data, src_stream->ps_decode_buffer[read_buffer_index].data + src_stream->ps_decode_buffer[read_buffer_index].read_pos, amt_avail_to_transfer);
		transfer_size -= amt_avail_to_transfer;
		src_stream->ps_decode_buffer[read_buffer_index].read_pos += amt_avail_to_transfer;
		
		// Give up this buffer - decoding may well need it, and we're done
		src_stream->ps_decode_buffer[read_buffer_index].write_pos = 0;
		src_stream->ps_decode_buffer[read_buffer_index].len = 0;
		
		// Fill the buffer
		hb_ts_stream_decode(src_stream);
		
		// Decoding will almost certainly have changed the current read buffer index
		read_buffer_index = src_stream->ps_current_read_buffer_index;
		
		if (src_stream->ps_decode_buffer[read_buffer_index].len == 0)
		{
			hb_log("hb_stream_read - buffer after decode has zero length data");
			return 0;
		}
		
		// Read the bit we still need
		memcpy(b->data+amt_avail_to_transfer, src_stream->ps_decode_buffer[read_buffer_index].data + src_stream->ps_decode_buffer[read_buffer_index].read_pos,transfer_size);
		src_stream->ps_decode_buffer[read_buffer_index].read_pos += transfer_size;
		
		return 1;
	}	
  }
  else
	return 0;
}

/***********************************************************************
 * hb_stream_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_seek( hb_stream_t * src_stream, float f )
{
  off_t stream_size, cur_pos, new_pos;
  double pos_ratio = f;
  cur_pos = ftello(src_stream->file_handle);
  fseeko(src_stream->file_handle,0 ,SEEK_END);
  stream_size = ftello(src_stream->file_handle);
  new_pos = (off_t) ((double) (stream_size) * pos_ratio);
  int r = fseeko(src_stream->file_handle, new_pos, SEEK_SET);
  
  if (r == -1)
  {
    fseeko(src_stream->file_handle, cur_pos, SEEK_SET);
    return 0;
  }
  
  if (src_stream->stream_type == hb_stream_type_transport)
  {
	// We need to drop the current decoder output and move
	// forwards to the next transport stream packet.
	hb_ts_stream_reset(src_stream);
  }
  
  // Now we must scan forwards for a valid start code (0x000001BA)
  int done = 0;
  hb_buffer_t *buf = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
  while (!done)
  {
    if (hb_stream_read(src_stream,buf) == 1)
    {
      int i=0;
      for (i=0; (i <= HB_DVD_READ_BUFFER_SIZE-4) && (!done); i++)
      {
        if ((buf->data[i] == 0x00) && (buf->data[i+1] == 0x00) && (buf->data[i+2] == 0x01) && (buf->data[i+3] == 0xba))
        {
          done = 1;
		  // 'Put Back' the data we've just read (up to this point)
		  hb_stream_put_back(src_stream, i);
        }
      }
    }
    else
      done = 1;    // End of data;
  }
  hb_buffer_close(&buf);
  return 1;
}

static hb_audio_t *hb_ts_stream_set_audio_id_and_codec(hb_stream_t *stream,
                                                       int aud_pid_index)
{
    off_t cur_pos = ftello(stream->file_handle);
    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );
    const uint8_t *buf;

    fseeko(stream->file_handle, 0, SEEK_SET);
    align_to_next_packet(stream->file_handle);
    buf = hb_ts_stream_getPEStype(stream, stream->ts_audio_pids[aud_pid_index]);

    /* check that we found a PES header */
    if (buf && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01)
    {
        if (buf[3] == 0xbd)
        {
            audio->id = 0x80bd | (aud_pid_index << 8);
            audio->codec = HB_ACODEC_AC3;
            hb_log("transport stream pid 0x%x (type 0x%x) is AC-3 audio id 0x%x",
                   stream->ts_audio_pids[aud_pid_index],
                   stream->ts_audio_stream_type[aud_pid_index],
                   audio->id);
            stream->ts_audio_stream_type[aud_pid_index] = 0x81;
        }
        else if ((buf[3] & 0xe0) == 0xc0)
        {
            audio->id = buf[3];
            audio->codec = HB_ACODEC_MPGA;
            hb_log("transport stream pid 0x%x (type 0x%x) is MPEG audio id 0x%x",
                   stream->ts_audio_pids[aud_pid_index],
                   stream->ts_audio_stream_type[aud_pid_index],
                   audio->id);
            stream->ts_audio_stream_type[aud_pid_index] = 0x03;
        }
    }
    fseeko(stream->file_handle, cur_pos, SEEK_SET);
    if (! audio->codec)
    {
        hb_log("transport stream pid 0x%x (type 0x%x) isn't audio",
                stream->ts_audio_pids[aud_pid_index],
                stream->ts_audio_stream_type[aud_pid_index]);
	}
    return audio;
}

static void add_audio_to_title(hb_title_t *title, int id)
{
    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = id;
    switch ( id >> 12 )
    {
        case 0x0:
            audio->codec = HB_ACODEC_MPGA;
            hb_log("hb_ps_stream_find_audio_ids: added MPEG audio stream 0x%x", id);
            break;
        case 0x2:
            audio->codec = HB_ACODEC_LPCM;
            hb_log("hb_ps_stream_find_audio_ids: added LPCM audio stream 0x%x", id);
            break;
        case 0x4:
            audio->codec = HB_ACODEC_DCA;
            hb_log("hb_ps_stream_find_audio_ids: added DCA audio stream 0x%x", id);
            break;
        case 0x8:
            audio->codec = HB_ACODEC_AC3;
            hb_log("hb_ps_stream_find_audio_ids: added AC3 audio stream 0x%x", id);
            break;
    }
    hb_list_add( title->list_audio, audio );
}

static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title)
{
    off_t cur_pos = ftello(stream->file_handle);
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
    hb_list_t *list = hb_list_init();
    // how many blocks we read while searching for audio streams
    int blksleft = 2048;
    // there can be at most 16 unique streams in an MPEG PS (8 in a DVD)
    // so we use a bitmap to keep track of the ones we've already seen.
    // Bit 'i' of smap is set if we've already added the audio for
    // audio substream id 'i' to the title's audio list.
    uint32_t smap = 0;

    hb_stream_seek(stream, 0.0f);
		
    while (--blksleft >= 0 && hb_stream_read(stream, buf) == 1)
    {
        hb_buffer_t *es;

        // 'buf' contains an MPEG2 PACK - get a list of all it's elementary streams
        hb_demux_ps(buf, list);

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
 * hb_stream_update_audio
 ***********************************************************************
 *
 **********************************************************************/
void hb_stream_update_audio(hb_stream_t *stream, hb_audio_t *audio)
{
	iso639_lang_t *lang;
	
    if (stream->stream_type == hb_stream_type_transport)
	{
		// Find the audio stream info for this PID
        int i = (audio->id >> 8) & 0xf;
        if (i >= stream->ts_number_audio_pids)
		{
            hb_log("hb_stream_update_audio: no PID for audio stream 0x%x",
                    audio->id);
			return;
		}
		
		lang = lang_for_code(stream->a52_info[i].lang_code);
		if (!audio->rate)
			audio->rate = stream->a52_info[i].rate;
		if (!audio->bitrate)
			audio->bitrate = stream->a52_info[i].bitrate;
		if (!audio->config.a52.ac3flags)
			audio->config.a52.ac3flags = audio->ac3flags = stream->a52_info[i].flags;

	}
    else
    {
        // XXX should try to get language code from the AC3 bitstream
        lang = lang_for_code(0x0000);
    }
	
	if (!audio->input_channel_layout)
	{
		switch( audio->ac3flags & A52_CHANNEL_MASK )
		{
			/* mono sources */
			case A52_MONO:
			case A52_CHANNEL1:
			case A52_CHANNEL2:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_MONO;
				break;
			/* stereo input */
			case A52_CHANNEL:
			case A52_STEREO:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
				break;
			/* dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input */
			case A52_DOLBY:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_DOLBY;
				break;
			/* 3F/2R input */
			case A52_3F2R:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F2R;
				break;
			/* 3F/1R input */
			case A52_3F1R:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F1R;
				break;
			/* other inputs */
			case A52_3F:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F;
				break;
			case A52_2F1R:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F1R;
				break;
			case A52_2F2R:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F2R;
				break;
			/* unknown */
			default:
				audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
		}

		/* add in our own LFE flag if the source has LFE */
		if (audio->ac3flags & A52_LFE)
		{
			audio->input_channel_layout = audio->input_channel_layout | HB_INPUT_CH_LAYOUT_HAS_LFE;
		}
	}
	
	snprintf( audio->lang, sizeof( audio->lang ), "%s (%s)", strlen(lang->native_name) ? lang->native_name : lang->eng_name,
	  audio->codec == HB_ACODEC_AC3 ? "AC3" : ( audio->codec == HB_ACODEC_MPGA ? "MPEG" : "LPCM" ) );
	snprintf( audio->lang_simple, sizeof( audio->lang_simple ), "%s", strlen(lang->native_name) ? lang->native_name : lang->eng_name );
	snprintf( audio->iso639_2, sizeof( audio->iso639_2 ), "%s", lang->iso639_2);

	if ( (audio->ac3flags & A52_CHANNEL_MASK) == A52_DOLBY ) {
		sprintf( audio->lang + strlen( audio->lang ),
			 " (Dolby Surround)" );
	} else {
		sprintf( audio->lang + strlen( audio->lang ),
			 " (%d.%d ch)",
			HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(audio->input_channel_layout) +
			HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(audio->input_channel_layout),
			HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(audio->input_channel_layout));
	}

	hb_log( "hb_stream_update_audio: id=%x, lang=%s, 3cc=%s, rate = %d, bitrate = %d, flags = 0x%x (%d)", audio->id, audio->lang, audio->iso639_2, audio->rate, audio->bitrate, audio->ac3flags, audio->ac3flags );

}

/***********************************************************************
 * hb_stream_put_back
 ***********************************************************************
 *
 **********************************************************************/
static void hb_stream_put_back(hb_stream_t *stream, int i)
{
	if (stream->stream_type == hb_stream_type_program)
	{
		// Program streams are pretty easy - we just reposition the source file
		// pointer
		fseeko(stream->file_handle, -(HB_DVD_READ_BUFFER_SIZE-i), SEEK_CUR);
	}
	else if (stream->stream_type == hb_stream_type_transport)
	{
		int read_buffer_index = stream->ps_current_read_buffer_index;
		
		// Transport streams are a little more tricky - so long as the 
		// amount to back up is still within the current decode buffer
		// we can just adjust the read pos.
		if (stream->ps_decode_buffer[read_buffer_index].read_pos - i > 0)
		{
			stream->ps_decode_buffer[read_buffer_index].read_pos -= i;
		}
		else
		  hb_error("hb_stream_put_back - trying to step beyond the start of the buffer, read_pos = %d amt to put back = %d\n", stream->ps_decode_buffer[read_buffer_index].read_pos, i);
	}
}


/***********************************************************************
 * hb_ts_stream_init
 ***********************************************************************
 *
 **********************************************************************/
 #define PS_DECODE_BUFFER_SIZE ( 1024 * 1024 * 4)
 
static void hb_ts_stream_init(hb_stream_t *stream)
{
	// Output Program Stream
	int i=0;
	for (i=0; i < kNumDecodeBuffers; i++)
	{
		stream->ps_decode_buffer[i].data = (unsigned char *) malloc(PS_DECODE_BUFFER_SIZE);
		stream->ps_decode_buffer[i].read_pos = 0;
		stream->ps_decode_buffer[i].size = PS_DECODE_BUFFER_SIZE;
		stream->ps_decode_buffer[i].len = 0;
		stream->ps_decode_buffer[i].write_pos = 0;
	}
	
	for (i=0; i < kMaxNumberDecodeStreams; i++)
	{
		stream->ts_streamcont[i] = -1;
	}
	
	stream->ps_current_write_buffer_index = 0;
	stream->ps_current_read_buffer_index = 1;
	
	// Find the audio and video pids in the stream
	hb_ts_stream_find_pids(stream);
	
	for (i=0; i < stream->ts_number_video_pids; i++)
	{
		// In progress audio/video data during the transport stream -> program stream processing
		stream->ts_packetbuf[i] = (unsigned char *) malloc(1024 * 1024);
		stream->ts_streamid[i] = 0xE0;		// Stream is Video
	}
	
	for (i = stream->ts_number_video_pids; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
	{
		stream->ts_packetbuf[i] = (unsigned char *) malloc(1024 * 1024);
		stream->ts_streamid[i] = 0xBD;		// Stream 1 is AC-3 Audio
	}
}

// ------------------------------------------------------------------------------------

static off_t align_to_next_packet(FILE* f)
{
	unsigned char buf[188*20];

	off_t start = ftello(f);
	off_t pos = 0;

	if (fread(buf, 188*20, 1, f) == 1)
	{
		int found = 0;
		while (!found && (pos < 188))
		{
			found = 1;
			int i = 0;
			for (i = 0; i < 188*20; i += 188)
			{
				unsigned char c = buf[pos+i];
				// Check sync byte
				if ((c != 0x47) && (c != 0x72) && (c != 0x29))
				{
					// this offset failed, try next
					found = 0;
					pos++;
					break;
				}
			}
		}
	}

	if (pos == 188)
		pos = 0;		// failed to find anything!!!!!?

    fseeko(f, start+pos, SEEK_SET);

	return pos;
}

// ------------------------------------------------------------------------------------

int bitpos = 0;
unsigned int bitval = 0;
unsigned char* bitbuf = NULL;
unsigned int bitmask[] = {
	0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
	0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
	0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
	0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};

static inline void set_buf(unsigned char* buf, int bufsize, int clear)
{
	bitpos = 0;
	bitbuf = buf;
	bitval = (bitbuf[0] << 24) | (bitbuf[1] << 16) | (bitbuf[2] << 8) | bitbuf[3];
	if (clear)
		memset(bitbuf, 0, bufsize);
}

static inline int buf_size()
{
	return bitpos >> 3;
}

static inline void set_bits(unsigned int val, int bits)
{
	val &= bitmask[bits];

	while (bits > 0)
	{
		int bitsleft = (8 - (bitpos & 7));
		if (bits >= bitsleft)
		{
			bitbuf[bitpos >> 3] |= val >> (bits - bitsleft);
			bitpos += bitsleft;
			bits -= bitsleft;
			val &= bitmask[bits];
		}
		else
		{
			bitbuf[bitpos >> 3] |= val << (bitsleft - bits);
			bitpos += bits;
			bits = 0;
		}
	}
}

static inline unsigned int get_bits(int bits)
{
	unsigned int val;
	int left = 32 - (bitpos & 31);

	if (bits < left)
	{
		val = (bitval >> (left - bits)) & bitmask[bits];
		bitpos += bits;
	}
	else
	{
		val = (bitval & bitmask[left]) << (bits - left);
		bitpos += left;
		bits -= left;

		int pos = bitpos >> 3;
		bitval = (bitbuf[pos] << 24) | (bitbuf[pos + 1] << 16) | (bitbuf[pos + 2] << 8) | bitbuf[pos + 3];
		
		if (bits > 0)
		{
			val |= (bitval >> (32 - bits)) & bitmask[bits];
			bitpos += bits;
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

int decode_program_map(hb_stream_t* stream)
{
	set_buf(stream->pmt_info.tablebuf, stream->pmt_info.tablepos, 0);

	unsigned char table_id	= get_bits(8);
															  get_bits(4);
	unsigned int section_length = get_bits(12);
	stream->pmt_info.section_length = section_length;
	
	unsigned int program_number = get_bits(16);
	stream->pmt_info.program_number = program_number;
																get_bits(2);
	unsigned char version_number = get_bits(5);
															  get_bits(1);
	unsigned char section_number = get_bits(8);
	unsigned char last_section_number = get_bits(8);
															  get_bits(3);
	unsigned int PCR_PID = get_bits(13);
	stream->pmt_info.PCR_PID = PCR_PID;
															  get_bits(4);
	unsigned int program_info_length = get_bits(12);
	stream->pmt_info.program_info_length = program_info_length;

	int i=0;
	unsigned char *descriptor_buf = (unsigned char *) malloc(program_info_length);
	for (i = 0; i < program_info_length; i++)
	{
	  descriptor_buf[i] = get_bits(8);
	}                                 
	
	int cur_pos =  9 /* data after the section length field*/ + program_info_length;
	int done_reading_stream_types = 0;
	while (!done_reading_stream_types)
	{
	  unsigned char stream_type = get_bits(8);
		  get_bits(3);
	  unsigned int elementary_PID = get_bits(13);
		  get_bits(4);
	  unsigned int ES_info_length = get_bits(12);
	  
	  int i=0;
	  unsigned char *ES_info_buf = (unsigned char *) malloc(ES_info_length);
	  for (i=0; i < ES_info_length; i++)
	  {
		ES_info_buf[i] = get_bits(8);
	  }
	
	  if (stream_type == 0x02)
	  {
		if (stream->ts_number_video_pids <= kMaxNumberVideoPIDS)
		  stream->ts_number_video_pids++;
		stream->ts_video_pids[stream->ts_number_video_pids-1] = elementary_PID;
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
            stream->ts_number_audio_pids++;
        stream->ts_audio_pids[i] = elementary_PID;
        stream->ts_audio_stream_type[i] = stream_type;
		
		if (ES_info_length > 0)
		{
            decode_element_descriptors(stream, i, ES_info_buf, ES_info_length);
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

// ------------------------------------------------------------------------------------

int build_program_map(unsigned char *buf, hb_stream_t *stream)
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

int decode_PAT(unsigned char *buf, hb_stream_t *stream)
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
                    set_buf(tablebuf + pos, tablepos - pos, 0);

                    unsigned char section_id	= get_bits(8);
                                                                              get_bits(4);
                    unsigned int section_len	= get_bits(12);
                    unsigned int transport_id	= get_bits(16);
                                                                              get_bits(2);
                    unsigned int version_num	= get_bits(5);
                    unsigned int current_next	= get_bits(1);
                    unsigned int section_num	= get_bits(8);
                    unsigned int last_section	= get_bits(8);

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
                            unsigned int pkt_program_num = get_bits(16);
							stream->pat_info[stream->ts_number_pat_entries].program_number = pkt_program_num;
                              
                            get_bits(3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              unsigned int pkt_network_PID = get_bits(13);
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = get_bits(13);
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

static int flushbuf(hb_stream_t *stream)
{
	int old_write_index = stream->ps_current_write_buffer_index;

	// Flip the buffers and start moving on to the next
	stream->ps_current_write_buffer_index++;
	if (stream->ps_current_write_buffer_index > kNumDecodeBuffers-1)
		stream->ps_current_write_buffer_index = 0;
	
	if ( (stream->ps_decode_buffer[stream->ps_current_write_buffer_index].len != 0) || (stream->ps_decode_buffer[stream->ps_current_write_buffer_index].write_pos != 0) )
	{
		hb_log("flushbuf - new buffer (index %d) has non zero length and write position !", stream->ps_current_write_buffer_index);
		return 0;
	}
	
	stream->ps_current_read_buffer_index = old_write_index;
	stream->ps_decode_buffer[stream->ps_current_read_buffer_index].read_pos = 0;
	
	return 1;
}

static int fwrite64(void* buf, int elsize, int elnum, hb_stream_t* stream)
{
	int size = elsize;
	if (elnum > 1)
		size *= elnum;
	
	int written = 0;
	int current_write_index = stream->ps_current_write_buffer_index;
	
	if (size <= stream->ps_decode_buffer[current_write_index].size - stream->ps_decode_buffer[current_write_index].write_pos)
	{
		memcpy(stream->ps_decode_buffer[current_write_index].data + stream->ps_decode_buffer[current_write_index].write_pos, buf, size);
		stream->ps_decode_buffer[current_write_index].write_pos += size;
		stream->ps_decode_buffer[current_write_index].len = stream->ps_decode_buffer[current_write_index].write_pos;
		written = size;
	}
	else
	{
		memcpy(stream->ps_decode_buffer[current_write_index].data + stream->ps_decode_buffer[current_write_index].write_pos, buf, stream->ps_decode_buffer[current_write_index].size - stream->ps_decode_buffer[current_write_index].write_pos);
		written += stream->ps_decode_buffer[current_write_index].size - stream->ps_decode_buffer[current_write_index].write_pos;
		stream->ps_decode_buffer[current_write_index].write_pos += stream->ps_decode_buffer[current_write_index].size - stream->ps_decode_buffer[current_write_index].write_pos;
		stream->ps_decode_buffer[current_write_index].len = stream->ps_decode_buffer[current_write_index].write_pos;

		if (flushbuf(stream))
		{
			// FLushing the buffer will have change the current write buffer
			current_write_index = stream->ps_current_write_buffer_index;
			
			memcpy(stream->ps_decode_buffer[current_write_index].data, (unsigned char*)buf + written, size - written);
			stream->ps_decode_buffer[current_write_index].write_pos += size - written;
			stream->ps_decode_buffer[current_write_index].len = stream->ps_decode_buffer[current_write_index].write_pos;
			written += size - written;
		}
	}


	if (elnum == 1 && written == size)
		return 1;
	else
		return written / elsize;
}

static int write_pack(hb_stream_t* stream, int64_t time)
{
	unsigned char buf[64];
	set_buf(buf, 64, 1);						// clear buffer

	int64_t ext_time = time % 300;
	time = time / 300;

	set_bits(0x000001ba, 32);					// pack id								32
	set_bits(1, 2);								// 0x01									2
	set_bits((unsigned int)(time >> 30), 3);	// system_clock_reference_base			3
	set_bits(1, 1);								// marker_bit							1
	set_bits((unsigned int)(time >> 15), 15);	// system_clock_reference_base			15
	set_bits(1, 1);								// marker_bit							1
	set_bits((unsigned int)time, 15);			// system_clock_reference_base1			15
	set_bits(1, 1);								// marker_bit							1
	set_bits((unsigned int)ext_time, 9);		// system_clock_reference_extension		9
	set_bits(1, 1);								// marker_bit							1
	set_bits(DEMUX, 22);						// program_mux_rate						22
	set_bits(1, 1);								// marker_bit							1
	set_bits(1, 1);								// marker_bit							1
	set_bits(31, 5);							// reserved								5
	set_bits(0, 3);								// pack_stuffing_length					3

	return fwrite64(buf, buf_size(), 1, stream) == 1;
}

static int pad_buffer(hb_stream_t *stream, int pad)
{
	pad -= 6;

	char buf[6];
	buf[0] = '\x0'; buf[1] = '\x0'; buf[2] = '\x1'; buf[3] = '\xbe';
	buf[4] = pad >> 8; buf[5] = pad & 0xff;

	if (fwrite64(buf, 6, 1, stream) != 1)
		return 0;

	unsigned char padbyte = 0xff;
	int i=0;
	for (i = 0; i < pad; i++)
	{
		if (fwrite64(&padbyte, 1, 1, stream) != 1)
			return 0;
	}

	return 1;
}

int make_pes_header(unsigned char* buf, int streamid, int len, int64_t PTS, int64_t DTS)
{
	int hdrlen = 0;
	int PTS_DTS_flags = 0;
	if (PTS != -1)
	{
		if (DTS != -1)
		{
			PTS_DTS_flags = 3;
			hdrlen += 10;
		}
		else
		{
			PTS_DTS_flags = 2;
			hdrlen += 5;
		}
	}

	set_buf(buf, 9 + hdrlen, 1);				// clear the buffer

	set_bits(0x000001, 24);						// packet_start_code_prefix				24
	set_bits((unsigned int)streamid, 8);		// directory_stream_id					8
	set_bits(len, 16);							// PES_packet_length					16
	set_bits(0x2, 2);							// '10'									2
	set_bits(0, 2);								// PES_scrambling_control				2
	set_bits(1, 1);								// PES_priority							1
	set_bits(0, 1);								// data_alignment_indicator				1
	set_bits(0, 1);								// copyright							1
	set_bits(0, 1);								// original_or_copy						1
	set_bits(PTS_DTS_flags, 2);					// PTS_DTS_flags						2
	set_bits(0, 1);								// ESCR_flag							1
	set_bits(0, 1);								// ES_rate_flag							1
	set_bits(0, 1);								// DSM_trick_mode_flag					1
	set_bits(0, 1);								// additional_copy_info_flag			1
	set_bits(0, 1);								// PES_CRC_flag							1
	set_bits(0, 1);								// PES_extension_flag					1
	set_bits(hdrlen, 8);						// PES_header_data_length				8
	
	if (PTS_DTS_flags == 2)
	{
		set_bits(2, 4);								// '0010'							4
		set_bits((unsigned int)(PTS >> 30), 3);		// PTS [32..30]						3
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)(PTS >> 15), 15);	// PTS [29..15]						15
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)PTS, 15);			// PTS [14..0]						15
		set_bits(1, 1);								// marker bit						1
	}
	else if (PTS_DTS_flags == 3)
	{
		set_bits(3, 4);								// '0011'							4
		set_bits((unsigned int)(PTS >> 30), 3);		// PTS [32..30]						3
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)(PTS >> 15), 15);	// PTS [29..15]						15
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)PTS, 15);			// PTS [14..0]						15
		set_bits(1, 1);								// marker bit						1
		set_bits(1, 4);								// '0001'							4
		set_bits((unsigned int)(DTS >> 30), 3);		// DTS [32..30]						3
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)(DTS >> 15), 15);	// DTS [29..15]						15
		set_bits(1, 1);								// marker bit						1
		set_bits((unsigned int)DTS, 15);			// DTS [14..0]						15
		set_bits(1, 1);								// marker bit						1
	}

	return buf_size();
}

int generate_output_data(hb_stream_t *stream, int write_ac3, int curstream, int pid)
{
			unsigned char ac3_substream_id[4];
			int ac3len = 0;
			
			if (write_ac3)
			{
                // Make a four byte DVD ac3 stream header
                int ssid = (curstream - stream->ts_number_video_pids) & 0xf;
                ac3_substream_id[0] = 0x80 | ssid;  // substream id
                ac3_substream_id[1] = 0x01;         // number of sync words
                ac3_substream_id[2] = 0x00;         // first offset (16 bits)
                ac3_substream_id[3] = 0x02;
                ac3len = 4;
			}
			
			int written = 0;	// Bytes we've written to output file
			int pos = 0;		// Position in PES packet buffer
			
			for (;;)
			{
				if ((stream->ps_decode_buffer[stream->ps_current_write_buffer_index].len % HB_DVD_READ_BUFFER_SIZE) != 0)
				{
					hb_log("write_output_stream - Packet's not falling on read buffer size boundries!");
					return 1;
				}

				// Get total length of this pack
				int len = min(14 + ac3len + stream->ts_packetpos[curstream] - pos, HB_DVD_READ_BUFFER_SIZE);

				// Figure out stuffing (if we have less than 16 bytes left)
				int stuffing = 0;
				if (len < HB_DVD_READ_BUFFER_SIZE && HB_DVD_READ_BUFFER_SIZE - len < 16)
				{
					stuffing = HB_DVD_READ_BUFFER_SIZE - len;
					len += stuffing;
				}

				// Write out pack header
				off_t file_offset = ftello(stream->file_handle);
				int64_t packet_time = (file_offset * CLOCKRATE / STREAMRATE) + 0 /*file_time*/;
				if (!write_pack(stream, packet_time))
				{
					hb_log("write_output_stream - Couldn't write pack header!");
					return 1;
				}

				int index_of_selected_pid = -1;
				if ((index_of_selected_pid = index_of_video_pid(pid,stream)) < 0)
				{
					if ((index_of_selected_pid = index_of_audio_pid(pid,stream)) < 0)
					{
						hb_log("generate_output_data - cannot find pid 0x%x (%d) in selected audio or video pids", pid, pid);
						return 0;
					}
					else
					{
						stream->ts_packetbuf[curstream][pos + 3] = stream->ts_streamid[stream->ts_number_video_pids + index_of_selected_pid];
					}
				}
				else
					stream->ts_packetbuf[curstream][pos + 3] = stream->ts_streamid[index_of_selected_pid];

				// Packet length..
				// Subtract pack size (14) and pes id and len (6) from lenth
				stream->ts_packetbuf[curstream][pos + 4] = (len - 6 - 14) >> 8; stream->ts_packetbuf[curstream][pos + 5] = (len - 6 - 14) & 0xFF;

				// Add any stuffing bytes to header extra len
				int hdrsize = 9 + stream->ts_packetbuf[curstream][pos + 8];
				stream->ts_packetbuf[curstream][pos + 8] += stuffing;					// Add stuffing to header bytes

				// Write out id, streamid, len
				if (fwrite64(stream->ts_packetbuf[curstream] + pos, hdrsize, 1, stream) != 1)	// Write pes id, streamid, and len
				{
					hb_log("write_output_stream - Failed to write output file!");
					return 1;
				}
				
				// Write stuffing
				int i=0;
				for (i = 0; i < stuffing; i++)				// Write any stuffing bytes
				{
					unsigned char stuff = 0xff;
					if (fwrite64(&stuff, 1, 1, stream) != 1)
					{
						hb_log("write_output_stream - Failed to write output file!");
						return 1;
					}
				}

				// Write ac3 streamid
				if (ac3len != 0)
				{
					if (fwrite64(ac3_substream_id, ac3len, 1, stream) != 1)
					{
						hb_log("write_output_stream - Failed to write output file!");
						return 1;
					}
				}

				// Write rest of data len minus headersize (9) stuffing, and pack size (14)
				if (fwrite64(stream->ts_packetbuf[curstream] + pos + hdrsize, len - hdrsize - 14 - stuffing - ac3len, 1, stream) != 1)	// Write data bytes
				{
					hb_log("write_output_stream - Failed to write output file!");
					return 1;
				}
				written += len;

				// Add len minus stuff we added like the pack (14) and the stuffing.
				pos += len - 14 - stuffing - ac3len;
				if (pos == stream->ts_packetpos[curstream])
					break;

				// Add pes header for next packet
				pos -= 9;
//				make_pes_header(stream->ts_packetbuf[curstream] + pos, (pid == stream->ts_video_pids[0] ? stream->ts_streamid[kVideoStream] : stream->ts_streamid[kAudioStream]), 0, -1, -1);
				make_pes_header(stream->ts_packetbuf[curstream] + pos, stream->ts_streamid[curstream], 0, -1, -1);
			}

			// Write padding
			if ((written % HB_DVD_READ_BUFFER_SIZE) != 0)
			{
				int left = HB_DVD_READ_BUFFER_SIZE - (written % HB_DVD_READ_BUFFER_SIZE);

				// Pad out to HB_DVD_READ_BUFFER_SIZE bytes
				if (!pad_buffer(stream, left))
				{
					hb_log("write_output_stream - Couldn't write pad buffer!");
					return 1;
				}
			}

			stream->ts_packetpos[curstream] = 0;
			stream->ts_streamcont[curstream] = -1;

	return 0;
}

static void hb_ts_handle_mpeg_audio(hb_stream_t *stream, int curstream, unsigned char* buf, int adapt_len )
{
	// Although we don't have AC3/A52 audio here we can still use the same structure to record this useful information.
	
	stream->a52_info[curstream - stream->ts_number_video_pids].flags = A52_STEREO;
	stream->a52_info[curstream - stream->ts_number_video_pids].rate = 48000 /*Hz*/;
	stream->a52_info[curstream - stream->ts_number_video_pids].bitrate = 384000 /*Bps*/;
}

static int hb_ts_handle_ac3_audio(hb_stream_t *stream, int curstream, unsigned char* buf, int adapt_len )
{
	int spos, dpos;

	// Make sure we start with 0x0b77
	if (stream->ts_packetbuf[curstream][9 + stream->ts_packetbuf[curstream][8]] != 0x0b || stream->ts_packetbuf[curstream][9 + stream->ts_packetbuf[curstream][8] + 1] != 0x77)
	{
		spos = 9 + stream->ts_packetbuf[curstream][8];
		dpos = 9 + stream->ts_packetbuf[curstream][8];
		while (spos <= stream->ts_packetpos[curstream] - 2 && !(stream->ts_packetbuf[curstream][spos] == 0x0b && stream->ts_packetbuf[curstream][spos + 1] == 0x77))
			spos++;

		if (!(stream->ts_packetbuf[curstream][spos] == 0x0b && stream->ts_packetbuf[curstream][spos + 1] == 0x77))
		{
			hb_log("hb_ts_stream_decode - Couldn't sync AC3 packet!");
			stream->ts_skipbad[curstream] = 1;
			return 0;
		}

		while (spos < stream->ts_packetpos[curstream])
		{
			stream->ts_packetbuf[curstream][dpos] = stream->ts_packetbuf[curstream][spos];
			spos++;
			dpos++;
		}
		stream->ts_packetpos[curstream] = dpos;
	}

	// Check the next packet to make sure IT starts with a 0x0b77
	int plen = 0;
//					if (buf[4 + adapt_len] == 0 && buf[4 + adapt_len + 1] == 0 &&		// Starting with an mpeg header?
//						buf[4 + adapt_len + 2] == 1 && buf[4 + adapt_len + 3] == 0xBD)
			plen = 9 + buf[4 + adapt_len + 8];
	int pstart = 4 + adapt_len + plen;
	if (buf[pstart] != 0x0b || buf[pstart + 1] != 0x77)
	{
		spos = pstart;
		while (spos < 188 - 2 && !(buf[spos] == 0x0b && buf[spos + 1] == 0x77))
		{
			stream->ts_packetbuf[curstream][stream->ts_packetpos[curstream]] = buf[spos];
			stream->ts_packetpos[curstream]++;
			spos++;
		}

		if (!(buf[spos] == 0x0b && buf[spos + 1] == 0x77))
		{
			hb_log("hb_ts_stream_decode - Couldn't sync AC3 packet!");
			stream->ts_skipbad[curstream] = 1;
			return 0;
		}

		adapt_len = spos - 4 - plen;

		dpos = spos - 1;
		spos = pstart - 1;
		while (spos >= pstart - plen)
		{
			buf[dpos] = buf[spos];
			spos--;
			dpos--;
		}
	}

	int flags, rate, bitrate;
	if( a52_syncinfo( &buf[pstart], &flags, &rate, &bitrate ) )
	{
		stream->a52_info[curstream - stream->ts_number_video_pids].flags = flags;
		stream->a52_info[curstream - stream->ts_number_video_pids].rate = rate;
		stream->a52_info[curstream - stream->ts_number_video_pids].bitrate = bitrate;
	}
	return 1;
}

static void hb_ts_stream_find_pids(hb_stream_t *stream)
{
	unsigned char buf[188];

	// align to first packet
	align_to_next_packet(stream->file_handle);

	// Read the Transport Stream Packets (188 bytes each) looking at first for PID 0 (the PAT PID), then decode that
	// to find the program map PID and then decode that to get the list of audio and video PIDs
	
	int bytesReadInPacket = 0;
	for (;;)
	{
		// Try to read packet..
		int bytesRead;
		if ((bytesRead = fread(buf+bytesReadInPacket, 1, 188-bytesReadInPacket, stream->file_handle)) != 188-bytesReadInPacket)
		{
			if (bytesRead < 0)
				bytesRead = 0;
			bytesReadInPacket += bytesRead;

			hb_log("hb_ts_stream_find_pids - end of file");
			break;	
		}
		else
		{
			bytesReadInPacket = 0;
		}

		// Check sync byte
		if ((buf[0] != 0x47) && (buf[0] != 0x72) && (buf[0] != 0x29))
		{
			hb_log("hb_ts_stream_find_pids - Bad transport packet (no sync byte 0x47)!");
			int i = 0;
			for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
				stream->ts_skipbad[i] = 1;
			continue;
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
	int i=0;
	for (i=0; i < stream->ts_number_video_pids; i++)
	{
		hb_log("      0x%x (%d)", stream->ts_video_pids[i], stream->ts_video_pids[i]);
	}
	hb_log("    Audio PIDS : ");
	for (i = 0; i < stream->ts_number_audio_pids; i++)
	{
		hb_log("      0x%x (%d)", stream->ts_audio_pids[i], stream->ts_audio_pids[i]);
	}
 }

int index_of_video_pid(int pid, hb_stream_t *stream)
{
	int found_pid = -1, i = 0;
	
	for (i = 0; (i < stream->ts_number_video_pids) && (found_pid < 0); i++)
	{
		if (pid == stream->ts_video_pids[i])
			found_pid = i;
	}
	return found_pid;
}

int index_of_audio_pid(int pid, hb_stream_t *stream)
{
	int i = 0, found_pid = -1;

	for (i = 0; (i < stream->ts_number_audio_pids) && (found_pid < 0); i++)
	{
		if (pid == stream->ts_audio_pids[i])
			found_pid = i;
	}
	return found_pid;
}

int index_of_pid(int pid, hb_stream_t *stream)
{
	int found_pid = -1;
	
	if ((found_pid = index_of_video_pid(pid, stream)) >= 0)
		return found_pid;
	
	if ((found_pid = index_of_audio_pid(pid, stream)) >= 0)
		return found_pid;
		
	return found_pid;
}

/***********************************************************************
 * hb_ts_stream_decode
 ***********************************************************************
 *
 **********************************************************************/
static void hb_ts_stream_decode(hb_stream_t *stream)
{
	unsigned char buf[188];
	int curstream;
	int doing_iframe;
	
	int i = 0;
	for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
	{
//	stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 0;
		stream->ts_skipbad[i] = 0;
	}
	
	doing_iframe = 0;
	
	if ((stream->ts_number_video_pids == 0) || (stream->ts_number_audio_pids == 0))
	{
		hb_log("hb_ts_stream_decode  - no Video or Audio PID selected, cannot decode transport stream");
		return;
	}
	
	int curr_write_buffer_index = stream->ps_current_write_buffer_index;
	
	// Write output data until a buffer switch occurs.
	while (curr_write_buffer_index == stream->ps_current_write_buffer_index)
	{
      if ((fread(buf, 188, 1, stream->file_handle)) != 1)
		{
            // end of file - we didn't finish filling our ps write buffer
            // so just discard the remainder (the partial buffer is useless)
            hb_log("hb_ts_stream_decode - eof");
            stream->ps_decode_buffer[stream->ps_current_write_buffer_index].len = 0;
         return;
		}

		// Check sync byte
		if ((buf[0] != 0x47) && (buf[0] != 0x72) && (buf[0] != 0x29))
		{
            off_t pos = ftello(stream->file_handle);
            hb_log("hb_ts_stream_decode - no sync byte 0x47 @ %lld",  pos);
			for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
			{
		//	stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 1;
				stream->ts_skipbad[i] = 1;
			}
			continue;
		}

		// Get pid
		int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;

		// Get the pos and buf - we organize our streams as 'n' video streams then 'm' audio streams
      int index_of_selected_pid;
		if ((index_of_selected_pid = index_of_video_pid(pid,stream)) < 0)
		{
			// Not a video PID perhaps audio ?
			if ((index_of_selected_pid = index_of_audio_pid(pid,stream)) < 0)
			{
            // not a pid we want
				continue;
			}
			else
			{
				curstream = stream->ts_number_video_pids + index_of_selected_pid;
			}
		}
		else
			curstream = index_of_selected_pid;
		
		// Get start code
		int start;
		start = (buf[1] & 0x40) != 0;
                  
		if (!start && stream->ts_skipbad[curstream])
			continue;

		// Get error
		int errorbit = (buf[1] & 0x80) != 0;
		if (errorbit)
		{
			hb_log("hb_ts_stream_decode - Error bit set in packet");
			stream->ts_skipbad[curstream] = 1;
			continue;
		}

		// Get adaption header info
		int adaption = (buf[3] & 0x30) >> 4;
		int adapt_len = 0;

		// Get continuity
        // Continuity only increments for adaption values of 0x3 or 0x01
		int continuity = (buf[3] & 0xF);
        if ((stream->ts_streamcont[curstream] != -1) && ((adaption & 0x01) != 0))
		{
			if (continuity != ((stream->ts_streamcont[curstream] + 1) & 0xF))
			{
				hb_log("hb_ts_stream_decode - Bad continuity code in packet");
				stream->ts_skipbad[curstream] = 1;
				continue;
			}
			stream->ts_streamcont[curstream] = continuity;
		}
			
		// Get adaption header size
		if (adaption == 0)
		{
			hb_log("hb_ts_stream_decode - Bad adaption code (code was 0)!");
			for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
			{
				stream->ts_skipbad[i] = 1;
			}
		//	stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 1;
			continue;
		}
		else if (adaption == 0x2)
			adapt_len = 184;
		else if (adaption == 0x3)
		{
			adapt_len = buf[4] + 1;
			if (adapt_len > 184)
			{
				hb_log("hb_ts_stream_decode - Invalid adapt len (was > 183)!");
				for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
				{
					stream->ts_skipbad[i] = 1;
				}
//				stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 1;
			}
		}

		// HBO is slick, it doesn't bother to sync AC3 packets with PES elementary stream packets.. so
		// we have to swizzle them together!  (ARGHH!)
//		if (pid == stream->ts_audio_pids[0] && start)
		if ((index_of_audio_pid(pid, stream) >= 0) && start)
		{
			// Is there an AC3 packet start 0b77 code in this packet??
			int sync_found = 0;
			unsigned char *p = buf + 4 + adapt_len;
			while (p <= buf + 186)
			{
				if (p[0] == 0x0b && p[1] == 0x77)
				{
					sync_found = 1;
					break;
				}
				p++;
			}

			// Couldn't find an AC3 sync start in this packet.. don't make a PES packet!
			if (!sync_found)
			{
//					int pos = ftell(fin);
//					error("AC3 packet sync not found in start frame");
//					return 1;
				adapt_len += 9 + buf[4 + adapt_len + 8];	
				start = 0;
			}
		}

		// Found a random access point (now we can start a frame/audio packet..)
		if (start)
		{
			// Check to see if this is an i_frame (group of picture start)
			if (pid == stream->ts_video_pids[0])
			{
//                                printf("Found Video Start for pid 0x%x\n", pid);
				// Look for the Group of Pictures packet.. indicates this is an I-Frame packet..
				doing_iframe = 0;
				unsigned int strid = 0;
				int i = 4;
				for (i = 4 + adapt_len; i < 188; i++)
				{
					strid = (strid << 8) | buf[i];
					if (strid == 0x000001B8) // group_start_code
					{
						// found a Group of Pictures header, subsequent picture must be an I-frame
						doing_iframe = 1;
					}
					else if (strid == 0x000001B3) // sequence_header code
					{
						doing_iframe = 1;
					}
					else if (strid == 0x00000100) // picture_start_code
					{
						// picture_header, let's see if it's an I-frame
						if (i<187)
						{
//							int pic_start_code = (buf[i+2] >> 3) & 0x07;
//							hb_log("hb_ts_stream_decode - picture_start_code header value = 0x%x (%d)", pic_start_code, pic_start_code);
							// check if picture_coding_type == 1
							if ((buf[i+2] & (0x7 << 3)) == (1 << 3))
							{
								// found an I-frame picture
								doing_iframe = 1;
							}
						}
					}

					if (doing_iframe)
					{
						if (!stream->ts_foundfirst[curstream])
						{
							stream->ts_foundfirst[curstream] = 1;
//							first_video_PCR = PCR;
						}
						break;
					}
				}
			}
			else if (index_of_audio_pid(pid, stream) >= 0)
			{
			    if (stream->ts_foundfirst[0])  // Set audio found first ONLY after first video frame found. There's an assumption here that stream '0' is a video stream
				{
					stream->ts_foundfirst[curstream] |= 1;
				}
			}
			
			// If we were skipping a bad packet, start fresh on this new PES packet..
			if (stream->ts_skipbad[curstream] == 1)
			{
				stream->ts_skipbad[curstream] = 0;
				stream->ts_packetpos[curstream] = 0;
			}

			// Get the continuity code of this packet
			stream->ts_streamcont[curstream] = continuity;
		}

		// Write a 2048 byte program stream packet..
		if (start && stream->ts_packetpos[curstream] > 0 && stream->ts_foundfirst[curstream] && !stream->ts_skipbad[curstream])
		{
			// Save the substream id block so we can added it to subsequent blocks
			int write_ac3 = 0;
//			if (pid == stream->ts_audio_pids[0] /*&& audstreamid == 0xBD*/)
			if (index_of_audio_pid(pid, stream) >= 0)
			{
				// Curstream is a zero based index of streams and includes both video and audio streams, so we must subtract the numver of video streams
				// from the indes value used here since ts_audio_stream_type is indexed only by audio streams.
                if (stream->ts_audio_stream_type[curstream - stream->ts_number_video_pids] == 0x03)
				{
					hb_ts_handle_mpeg_audio(stream, curstream, buf, adapt_len);
				}
				else
				{
                    write_ac3 = hb_ts_handle_ac3_audio(stream, curstream, buf, adapt_len);
				}
			}
		
		if (generate_output_data(stream, write_ac3, curstream, pid) != 0)
			return ;
		}

		// Add the payload for this packet to the current buffer
		if (stream->ts_foundfirst[curstream] && (184 - adapt_len) > 0)
		{
			memcpy(stream->ts_packetbuf[curstream] + stream->ts_packetpos[curstream], buf + 4 + adapt_len, 184 - adapt_len);
			stream->ts_packetpos[curstream] += 184 - adapt_len;
		}
	}
}

/***********************************************************************
 * hb_ts_stream_reset
 ***********************************************************************
 *
 **********************************************************************/
static void hb_ts_stream_reset(hb_stream_t *stream)
{
	int i=0;
	for (i=0; i < kNumDecodeBuffers; i++)
	{
		stream->ps_decode_buffer[i].read_pos = 0;
		stream->ps_decode_buffer[i].write_pos = 0;
		stream->ps_decode_buffer[i].len = 0;
	}

	for (i=0; i < kMaxNumberDecodeStreams; i++)
	{
		stream->ts_streamcont[i] = -1;
	}

	stream->ps_current_write_buffer_index = 0;
	stream->ps_current_read_buffer_index = 1;

	align_to_next_packet(stream->file_handle);
}

