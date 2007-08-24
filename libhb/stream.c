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
#define kMaxNumberAudioPIDS 32
//#define kVideoStream 0
//#define kAudioStream 1
#define kNumDecodeBuffers 2

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
	int				 ts_selected_audio_pid_index;
	
	unsigned char*	 ts_packetbuf[kMaxNumberDecodeStreams];
	int				 ts_packetpos[kMaxNumberDecodeStreams];
//	int				 ts_bufpackets[kMaxNumberDecodeStreams];
	int				 ts_foundfirst[kMaxNumberDecodeStreams];
	int				 ts_skipbad[kMaxNumberDecodeStreams];
	int				 ts_streamcont[kMaxNumberDecodeStreams];
	int				 ts_streamid[kMaxNumberDecodeStreams];
	int				 ts_audio_stream_type[kMaxNumberAudioPIDS];
	
	FILE			 *debug_output;
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
static void hb_stream_set_audio_id_and_codec(hb_stream_t *stream, hb_audio_t *audio);

int hb_stream_is_stream_type( char * path )
{
  if ((strstr(path,".mpg") != NULL) ||
      (strstr(path,".vob") != NULL) || 
      (strstr(path, ".VOB") != NULL) ||
      (strstr(path, ".mpeg") != NULL) ||
      (strstr(path,".ts") != NULL) || 
      (strstr(path, ".m2t") != NULL) ||
      (strstr(path, ".TS") != NULL))
  {
    return 1;
  }
  else
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
    if( !( d->file_handle = fopen( path, "rb" ) ) )
    {
        hb_log( "hb_stream_open: fopen failed (%s)", path );
        goto fail;
    }

    d->path = strdup( path );

	if ( ( strstr(d->path,".ts") != NULL) || ( strstr(d->path,".m2t") != NULL) || ( strstr(d->path,".TS") != NULL) )
	{
		d->stream_type = hb_stream_type_transport;
		hb_ts_stream_init(d);
	}
	else if (( strstr(d->path,".mpg") != NULL) || ( strstr(d->path,".vob") != NULL) || ( strstr(d->path,".mpeg") != NULL) || ( strstr(d->path,".VOB") != NULL))
	{
		d->stream_type = hb_stream_type_program;
	}
	
    return d;

fail:
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

	if (d->debug_output)
	{
		fclose(d->debug_output);
		d->debug_output = NULL;
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
	
    free( d );
    *_d = NULL;
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
    
	int i=0, num_audio_tracks = 1;
	
	if (stream->stream_type == hb_stream_type_transport)
	{
		num_audio_tracks = stream->ts_number_audio_pids;
	}
	
	for (i=0; i < num_audio_tracks ; i++)
	{
		// Basic AC-3 Audio track
		hb_audio_t * audio;
		audio = calloc( sizeof( hb_audio_t ), 1 );

		audio->source_pid = stream->ts_audio_pids[i];
		
		hb_stream_set_audio_id_and_codec(stream, audio);
		
		hb_list_add( aTitle->list_audio, audio );
	}
	
  return aTitle;
}

/***********************************************************************
 * hb_stream_duration
 ***********************************************************************
 *
 **********************************************************************/
void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle)
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

    unsigned char *buf = (unsigned char *) malloc(4096);
    int done = 0;
    off_t cur_pos;
    int64_t first_pts = 0, last_pts = 0;
    
    // To calculate the duration we look for the first and last presentation time stamps in the stream for video data
    // and then use the delta
    while (!done)
    {
      cur_pos = ftello(stream->file_handle);
      if (fread(buf, 4096, 1, stream->file_handle) == 1)
      {
        int i=0;
        for (i=0; (i <= 4092) && !done; i++)
        {
          if ((buf[i] == 0x00) && (buf[i+1] == 0x00) && (buf[i+2] == 0x01) && (buf[i+3]  == 0xe0))    // Found a Video Stream
          {
              // Now look for a PTS field - we need to make sure we have enough space so we back up a little and read
              // some more data
              fseeko(stream->file_handle, cur_pos + i, SEEK_SET);
              if (fread(buf, 4096, 1, stream->file_handle) == 1)
              {
                  int has_pts             = ( ( buf[7] >> 6 ) & 0x2 ) ? 1 : 0;
                  if (has_pts)
                  {
                    first_pts = ( ( ( (uint64_t) buf[9] >> 1 ) & 0x7 ) << 30 ) +
                          ( buf[10] << 22 ) +
                          ( ( buf[11] >> 1 ) << 15 ) +
                          ( buf[12] << 7 ) +
                          ( buf[13] >> 1 );
                    done = 1;
                  }
                  else
                  {
                    fseeko(stream->file_handle, cur_pos, SEEK_SET);
                    fread(buf, 4096, 1, stream->file_handle);
                  }
              }
          }
        }
      }
      else
        done = 1;    // End of data;
    }

    // Now work back from the end of the stream
    fseeko(stream->file_handle,0 ,SEEK_END);
    
    done = 0;
    while (!done)
    {
      // Back up a little
      if (fseeko(stream->file_handle, -4096, SEEK_CUR) < 0)
      {
        done = 1;
        break;
      }
      
      cur_pos = ftello(stream->file_handle);
      if (fread(buf, 4096, 1, stream->file_handle) == 1)
      {
        int i=0;
        for (i=4092; (i >= 0) && !done; i--)
        {
          if ((buf[i] == 0x00) && (buf[i+1] == 0x00) && (buf[i+2] == 0x01) && (buf[i+3] == 0xe0))    // Found a Video Stream
          {
              // Now look for a PTS field - we need to make sure we have enough space so we back up a little and read
              // some more data
              fseeko(stream->file_handle, cur_pos + i, SEEK_SET);
              fread(buf, 1, 4096, stream->file_handle);

              unsigned char pts_dts_flag = buf[7];
              
              int has_pts             = ( ( buf[7] >> 6 ) & 0x2 ) ? 1 : 0;
              if (has_pts)
              {
                last_pts = ( ( ( (uint64_t) buf[9] >> 1 ) & 0x7 ) << 30 ) +
                      ( buf[10] << 22 ) +
                      ( ( buf[11] >> 1 ) << 15 ) +
                      ( buf[12] << 7 ) +
                      ( buf[13] >> 1 );
                
                done = 1;
              }
              else
              {
                // Re Read the original data and carry on (i is still valid in the old buffer)
                fseeko(stream->file_handle, cur_pos, SEEK_SET);
                fread(buf, 4096, 1, stream->file_handle);
              }
          }
        }
        fseeko(stream->file_handle, -4096, SEEK_CUR);   // 'Undo' the last read
      }
      else
        done = 1;    // End of data;
    }
    free(buf);
    
    int64_t duration = last_pts - first_pts;
    inTitle->duration = duration; //90LL * dvdtime2msec( &d->pgc->playback_time );
    inTitle->hours    = inTitle->duration / 90000 / 3600;
    inTitle->minutes  = ( ( inTitle->duration / 90000 ) % 3600 ) / 60;
    inTitle->seconds  = ( inTitle->duration / 90000 ) % 60;
    
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

/***********************************************************************
 * hb_stream_set_audio_id_and_codec
 ***********************************************************************
 *
 **********************************************************************/
void hb_stream_set_audio_id_and_codec(hb_stream_t *stream, hb_audio_t *audio)
{
	off_t cur_pos;
	cur_pos = ftello(stream->file_handle);
	int done = 0;
	hb_buffer_t *buf  = NULL;

        int cur_audio_pid_index = stream->ts_selected_audio_pid_index;

	if (stream->stream_type == hb_stream_type_transport)
	{
		int i=0;
		for (i=0; i < stream->ts_number_audio_pids; i++)
		{
			if (stream->ts_audio_pids[i] == audio->source_pid)
			{
				stream->ts_selected_audio_pid_index = i;
				break;
			}
		}
	}

      //Start at the beginning of the stream
      hb_stream_seek(stream, 0.0f);
//		fseeko(stream->file_handle,0 ,SEEK_SET);
		
      // Now we must scan forwards for a valid audio start code (0x000001xx)
      buf = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
      while (!done)
      {
//			if (fread(buf->data,4096,1,stream->file_handle) == 1)
              if (hb_stream_read(stream, buf) == 1)
              {
                int i=0;
                for (i=0; (i <= HB_DVD_READ_BUFFER_SIZE-4) && (!done); i++)
                {
                      if ((buf->data[i] == 0x00) && (buf->data[i+1] == 0x00) && (buf->data[i+2] == 0x01))
                      {
                        if (buf->data[i+3] == 0xbd)
                        {
                              audio->id = 0x80bd;
                              audio->codec = HB_ACODEC_AC3;
                              done = 1;
                        }
                        else if ((buf->data[i+3] & 0xe0) == 0xc0)
                        {
                              audio->id = buf->data[i+3];
                              audio->codec = HB_ACODEC_MPGA;
                              done = 1;
                        } 
                      }
                }
              }
              else
                done = 1;    // End of data;
      }
      hb_buffer_close(&buf);

      fseeko(stream->file_handle, cur_pos, SEEK_SET);
      
      stream->ts_selected_audio_pid_index = cur_audio_pid_index;
}

/***********************************************************************
 * hb_stream_update_audio
 ***********************************************************************
 *
 **********************************************************************/
void hb_stream_update_audio(hb_stream_t *stream, hb_audio_t *audio)
{
	iso639_lang_t *lang;
	
	if (stream->stream_type == hb_stream_type_program)
	{
		lang = lang_for_code(0x0000);
	}
	else if (stream->stream_type == hb_stream_type_transport)
	{
		// Find the audio stream info for this PID
		int i=0;
		for (i=0; i < stream->ts_number_audio_pids; i++)
		{
			if (stream->ts_audio_pids[i] == audio->source_pid)
				break;
		}
		if (i == stream->ts_number_audio_pids)
		{
			hb_log("hb_stream_update_audio - cannot find PID 0x%x (%d) in ts_audio_pids list !", audio->source_pid, audio->source_pid);
			return;
		}
		
		lang = lang_for_code(stream->a52_info[i].lang_code);
		audio->rate = stream->a52_info[i].rate;
		audio->bitrate = stream->a52_info[i].bitrate;
		audio->config.a52.ac3flags = audio->ac3flags = stream->a52_info[i].flags;

	}
	
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

void		 hb_stream_set_selected_audio_pid_index(hb_stream_t *stream, int i)
{
	stream->ts_selected_audio_pid_index = i;
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
		  fprintf(stderr, "hb_stream_put_back - trying to step beyond the start of the buffer, read_pos = %d amt to put back = %d\n", stream->ps_decode_buffer[read_buffer_index].read_pos, i);
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
	
	// This is the index (in ts_audio_pids) of the selected
	// output stream. It should not be set until after all the 
	// pids in the stream have been discovered.
	stream->ts_selected_audio_pid_index = -1;
	
	stream->debug_output = fopen("/Users/awk/Desktop/hb_debug.mpg", "wb");
	
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

off_t align_to_next_packet(FILE* f)
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

	fseek(f, start+pos, SEEK_SET);

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

// ------------------------------------------------------------------------------------

int decode_program_map(unsigned char *buf, hb_stream_t *stream)
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
                    hb_log("decode_program_map - Bad program section length (> 1024)");
                    return 0;
            }
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + pointer_len, 184 - adapt_len - pointer_len);
            tablepos += 184 - adapt_len - pointer_len;
    }

    if (start && reading)
    {
            int done_reading_stream_types = 0;
            
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + 1, pointer_len - 1);

            unsigned int pos = 0;
            set_buf(tablebuf + pos, tablepos - pos, 0);

            unsigned char section_id	= get_bits(8);
                                                                      get_bits(4);
            unsigned int section_length = get_bits(12);
            unsigned int program_number = get_bits(16);
                                                                        get_bits(2);
            unsigned char version_number = get_bits(5);
                                                                      get_bits(1);
            unsigned char section_number = get_bits(8);
            unsigned char last_section_number = get_bits(8);
                                                                      get_bits(3);
            unsigned int PCR_PID = get_bits(13);
                                                                      get_bits(4);
            unsigned int program_info_length = get_bits(12);
            int i=0;
            unsigned char *descriptor_buf = (unsigned char *) malloc(program_info_length);
            for (i = 0; i < program_info_length; i++)
            {
              descriptor_buf[i] = get_bits(8);
            }                                 
            
            int cur_pos =  9 /* data so far */ + program_info_length;
            done_reading_stream_types = 0;
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
              if ((stream_type == 0x04) || (stream_type == 0x81) || (stream_type == 0x03) || (stream_type == 0x06))    // ATSC Defines stream type 0x81 for AC-3/A52 audio, there's also some evidence of streams using type 6 for AC-3 audio too
              {
                if (stream->ts_number_audio_pids <= kMaxNumberAudioPIDS)
                  stream->ts_number_audio_pids++;
                stream->ts_audio_pids[stream->ts_number_audio_pids-1] =  elementary_PID;

                stream->a52_info[stream->ts_number_audio_pids-1].lang_code = 'e' << 8 | 'n';
				stream->ts_audio_stream_type[stream->ts_number_audio_pids-1] = stream_type;
				
                if (ES_info_length > 0)
                {
                  hb_log("decode_program_map - Elementary Stream Info Present, decode language codes ?");
                }

              }

              cur_pos += 5 /* stream header */ + ES_info_length;
              
              free(ES_info_buf);
              
              if (cur_pos >= section_length - 4 /* stop before the CRC */)
                done_reading_stream_types = 1;
            }
                                     
            free(descriptor_buf);
    }
    
    return 1;
}

int decode_PAT(unsigned char *buf, unsigned int *program_num, unsigned int *network_PID, unsigned int *program_map_PID)
{
//    int maxchannels = 8;
//    static ATSC_CHANNEL_INFO* channels;
//
//    if (channels == NULL)
//      channels = (ATSC_CHANNEL_INFO*) malloc(maxchannels * sizeof(ATSC_CHANNEL_INFO));
//      
//    int numchannels;

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
//                    unsigned int protocol_ver	= get_bits(8);

                    switch (section_id)
                    {
                      case 0x00:
                        {
                          // Program Association Section
                          section_len -= 5;    // Already read transport stream ID, version num, section num, and last section num
                          section_len -= 4;   // Ignore the CRC
                          int curr_pos = 0;
                          while (curr_pos < section_len)
                          {
                            unsigned int pkt_program_num = get_bits(16);
                            if (program_num)
                              *program_num = pkt_program_num;
                              
                            get_bits(3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              unsigned int pkt_network_PID = get_bits(13);
//                              printf("PAT - Transport ID = 0x%x (%d) program_num 0x%x (%d) network_PID = 0x%x (%d)\n", transport_id, transport_id, pkt_program_num, pkt_program_num, pkt_network_PID, pkt_network_PID);
                              if (network_PID)
                                *network_PID = pkt_network_PID;
                                
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = get_bits(13);
//                              printf("PAT - Transport ID = 0x%x (%d) program_num 0x%x (%d) program_map_PID = 0x%x (%d)\n", transport_id, transport_id, pkt_program_num, pkt_program_num, pkt_program_map_PID, pkt_program_map_PID);
                              if (program_map_PID)
                                *program_map_PID = pkt_program_map_PID;
                            }
                            curr_pos += 4;
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

	if (stream->debug_output)
	{
		fwrite(stream->ps_decode_buffer[stream->ps_current_write_buffer_index].data, stream->ps_decode_buffer[stream->ps_current_write_buffer_index].len, 1, stream->debug_output);
	}
	
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
				// Make a four byte ac3 streamid
				ac3_substream_id[0] = 0x80;	// Four byte AC3 CODE??
				ac3_substream_id[1] = 0x01;
				ac3_substream_id[2] = 0x00;	// WHY???  OH WHY??
				ac3_substream_id[3] = 0x02;
				ac3len = 4;
			}
			
			int written = 0;	// Bytes we've written to output file
			int pos = 0;		// Position in PES packet buffer
			
			for (;;)
			{
//				int64_t fpos = ftell64(fout);
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

//				if (pid == stream->ts_audio_pids[0])
//					stream->ts_packetbuf[curstream][pos + 3] = stream->ts_streamid[kAudioStream];
//				else
//					stream->ts_packetbuf[curstream][pos + 3] = stream->ts_streamid[kVideoStream];
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
	int curstream = 0;

	// Stream ID info
	unsigned int program_num = 0;
	unsigned int network_PID = 0;
	unsigned int program_map_PID = 0;

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
//			curfilepos += bytesRead;
			bytesReadInPacket = 0;
		}

		// Check sync byte
		if ((buf[0] != 0x47) && (buf[0] != 0x72) && (buf[0] != 0x29))
		{
//			__int64 pos = ftell64(fin);
			hb_log("hb_ts_stream_find_pids - Bad transport packet (no sync byte 0x47)!");
			int i = 0;
			for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
				stream->ts_skipbad[i] = 1;
//			stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 1;
			continue;
		}

		// Get pid
		int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;
                
                if ((pid == 0x0000) && (program_num == 0))
                {
                  decode_PAT(buf, &program_num, &network_PID, &program_map_PID);
                  continue;
                }
                
                if (pid == 0x1ffb)
                {
                  printf("Need to decode PSIP data !\n");
                  continue;
                }
                
                if ((network_PID > 0) && (pid == network_PID))
                {
                  printf("Need to Decode network PID section !\n");
                  continue;
                }
                
                if ((program_map_PID > 0) && (pid == program_map_PID))
                {
                  decode_program_map(buf, stream);
                  break;;
                }
                
                // Skip until we have a complete set of PIDs
                if ((stream->ts_number_video_pids == 0) || (stream->ts_number_audio_pids == 0))
                  continue;
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

	// If a selected audio pid index has been set it indicates
	// which of the potential pids we need to output so only return
	// that index for the appropriate pid. Other pids should just
	// be ignored.
	if (stream->ts_selected_audio_pid_index >= 0) 
	{
		if (pid == stream->ts_audio_pids[stream->ts_selected_audio_pid_index])
			return stream->ts_selected_audio_pid_index;
		else
			return -1;
	}
	
	// If no specific pid index is set then we're probably just gathering
	// pid and/or stream information (during DecodePreviews for example)
	// so return the appropriate index
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
	
	int bytesReadInPacket = 0;
	int curr_write_buffer_index = stream->ps_current_write_buffer_index;
	
	// Write output data until a buffer switch occurs.
	while (curr_write_buffer_index == stream->ps_current_write_buffer_index)
	{
		// Try to read packet..
		int bytesRead;
		if ((bytesRead = fread(buf+bytesReadInPacket, 1, 188-bytesReadInPacket, stream->file_handle)) != 188-bytesReadInPacket)
		{
			if (bytesRead < 0)
				bytesRead = 0;
			bytesReadInPacket += bytesRead;

			// Flush any outstanding output data - we're done here.
			flushbuf(stream);
			break;
		}
		else
		{
//			curfilepos += bytesRead;
			bytesReadInPacket = 0;
		}

		// Check sync byte
		if ((buf[0] != 0x47) && (buf[0] != 0x72) && (buf[0] != 0x29))
		{
//			__int64 pos = ftell64(fin);
			hb_log("hb_ts_stream_decode - Bad transport packet (no sync byte 0x47)!");
			for (i=0; i < stream->ts_number_video_pids + stream->ts_number_audio_pids; i++)
			{
		//	stream->ts_skipbad[kAudioStream] = stream->ts_skipbad[kVideoStream] = 1;
				stream->ts_skipbad[i] = 1;
			}
			continue;
		}

		// Get pid
		int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;

		// Skip this block
		if (index_of_pid(pid, stream) < 0)
			continue;
//		if (pid != stream->ts_audio_pids[0] && pid != stream->ts_video_pids[0])
//			continue;

		// Get the pos and buf - we organize our streams as 'n' video streams then 'm' audio streams
		int index_of_selected_pid = -1;
		if ((index_of_selected_pid = index_of_video_pid(pid,stream)) < 0)
		{
			// Not a video PID perhaps audio ?
			if ((index_of_selected_pid = index_of_audio_pid(pid,stream)) < 0)
			{
				hb_log("hb_ts_stream_decode - Unknown pid 0x%x (%d)", pid, pid);
				continue;
			}
			else
			{
				curstream = stream->ts_number_video_pids + index_of_selected_pid;
				if (curstream > kMaxNumberDecodeStreams)
				{
					hb_log("hb_ts_stream_decode - Too many streams %d", curstream);
					continue;
				}
			}
		}
		else
			curstream = index_of_selected_pid;
		
//		if (pid == stream->ts_video_pids[0])
//			curstream = 0;
//		else
//			curstream = 1;

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
		int continuity = (buf[3] & 0xF);
		if ((stream->ts_streamcont[curstream] != -1) && (adaption & 0x01 == 0x01))		// Continuity only increments for adaption values of 0x3 or 0x01
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

		// Get PCR
		if (start && (adaption & 0x2) && (buf[5] & 0x10))
		{
			int64_t PCR_base = ((int64_t)buf[6] << 25) | ((int64_t)buf[7] << 17) | 
				  ((int64_t)buf[8] << 9) | ((int64_t)buf[9] << 1) | ((int64_t)buf[10] >> 7);
			int64_t PCR_ext = ((int64_t)(buf[10] & 0x1) << 8) | ((int64_t)buf[11]);
			int64_t PCR = PCR_base * 300 + PCR_ext;
		}

		// Get random
//		bool random = false;
//		if (start && (adaption & 0x2))
//			random = (buf[5] & 0x40) != 0;		// BUG: SOME TS STREAMS DON'T HAVE THE RANDOM BIT (ABC!! ALIAS)

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
				if ((stream->ts_audio_stream_type[curstream] == 0x04) || (stream->ts_audio_stream_type[curstream] == 0x81))
				{
					write_ac3 = hb_ts_handle_ac3_audio(stream, curstream, buf, adapt_len);
				}
				else if (stream->ts_audio_stream_type[curstream] == 0x03)
				{
					hb_ts_handle_mpeg_audio(stream, curstream, buf, adapt_len);
				}
				else
				{
					hb_log("hb_ts_stream_decode - Unknown Audio Stream type ! 0x%x (%d)", stream->ts_audio_stream_type[curstream], stream->ts_audio_stream_type[curstream]);
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

