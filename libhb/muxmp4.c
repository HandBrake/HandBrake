/* $Id: muxmp4.c,v 1.24 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <ffmpeg/avformat.h>

#include "hb.h"

int64_t ff_gcd(int64_t a, int64_t b);
static inline int ff_get_fourcc(const char *s)
{
    return (s[0]) + (s[1]<<8) + (s[2]<<16) + (s[3]<<24);
}

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    AVFormatContext * format;
};

struct hb_mux_data_s
{
    int track;
};

/**********************************************************************
 * MP4Init
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int MP4Init( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;
    
    hb_audio_t    * audio;
    int i;
    AVFormatContext * oc;
    AVStream *st;
    AVFormatParameters params;

    register_protocol(&file_protocol);
    movenc_init();

    oc = av_alloc_format_context();

    if( job->mux & HB_MUX_PSP )
    {
        oc->oformat = guess_format( "psp", NULL, NULL );
    }
    else
    {
        oc->oformat = guess_format( "mp4", NULL, NULL );
    }
    if( !oc->oformat )
    {
        hb_log( "guess_format failed" );
        return 1;
    }
    snprintf( oc->filename, sizeof( oc->filename ),
              "%s", job->file );

    st = av_new_stream( oc, oc->nb_streams );
    if( !st )
    {
        hb_log( "av_new_stream failed" );
        return 1;
    }
    st->stream_copy = 1;
    st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    st->codec->codec_type = CODEC_TYPE_VIDEO;
    st->codec->codec_id = ( job->vcodec == HB_VCODEC_X264 ) ?
                            CODEC_ID_H264 : CODEC_ID_MPEG4;
    st->codec->extradata= job->config.mpeg4.bytes;
    st->codec->extradata_size= job->config.mpeg4.length;
    st->codec->bit_rate = 1000 * job->vbitrate;
    i = ff_gcd( job->vrate_base, job->vrate );
    st->codec->time_base = (AVRational){ job->vrate_base / i, job->vrate / i };

    st->codec->pix_fmt = PIX_FMT_YUV420P;
    st->codec->width = job->width;
    st->codec->height = job->height;
    st->codec->has_b_frames = 0;

    job->mux_data = malloc( sizeof( hb_mux_data_t ) );
    job->mux_data->track = 0;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );

        audio->mux_data = malloc( sizeof( hb_mux_data_t ) );
        audio->mux_data->track = i + 1;

        st = av_new_stream( oc, oc->nb_streams );
        if( !st )
        {
            hb_log( "av_new_stream failed" );
            return 1;
        }
        st->stream_copy = 1;
        st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        st->codec->codec_type = CODEC_TYPE_AUDIO;
        st->codec->codec_id = CODEC_ID_AAC;
        st->codec->bit_rate = 1000 * job->abitrate;
        st->codec->extradata= audio->config.aac.bytes;
        st->codec->extradata_size= audio->config.aac.length;
        st->codec->time_base = (AVRational){ 1, job->arate };
        st->codec->channels = 2;
        st->codec->sample_rate = job->arate;
        st->codec->frame_size = 1024;
        st->codec->block_align = 0;
    }

    oc->timestamp = 0;
    if( url_fopen( &oc->pb, job->file, URL_WRONLY ) < 0 )
    {
        hb_log( "url_fopen failed (%s)", job->file );
        return 1;
    }

    memset( &params, 0, sizeof( params ) );
    if( av_set_parameters( oc, &params ) < 0 )
    {
        hb_log( "av_set_parameters failed" );
        return 1;
    }

    oc->packet_size= 0;
    oc->mux_rate= 0;
    oc->preload= (int)(0.5*AV_TIME_BASE);
    oc->max_delay= (int)(0.7*AV_TIME_BASE);
    oc->loop_output = AVFMT_NOOUTPUTLOOP;

    if( av_write_header( oc ) < 0 )
    {
        hb_log( "av_write_header failed" );
        return 1;
    }

    m->format = oc;

    return 0;
}

static int MP4Mux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.stream_index = mux_data->track;
    pkt.data         = buf->data;
    pkt.size         = buf->size;
    pkt.pts          = buf->start;

    if( buf->key )
    {
        pkt.flags |= PKT_FLAG_KEY;
    }

    av_interleaved_write_frame( m->format, &pkt );

    return 0;
}

static int MP4End( hb_mux_object_t * m )
{
    av_write_trailer( m->format );
    url_fclose( &m->format->pb );
    av_free( m->format );

    return 0;
}

hb_mux_object_t * hb_mux_mp4_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = MP4Init;
    m->mux       = MP4Mux;
    m->end       = MP4End;
    m->job       = job;
    return m;
}

