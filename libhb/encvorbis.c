/* encvorbis.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"

#include "handbrake/handbrake.h"
#include "handbrake/audio_remap.h"

#include "vorbis/vorbisenc.h"

#define OGGVORBIS_FRAME_SIZE 1024

int  encvorbisInit( hb_work_object_t *, hb_job_t * );
int  encvorbisWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encvorbisClose( hb_work_object_t * );

hb_work_object_t hb_encvorbis =
{
    WORK_ENCVORBIS,
    "Vorbis encoder (libvorbis)",
    encvorbisInit,
    encvorbisWork,
    encvorbisClose
};

struct hb_work_private_s
{
    uint8_t   *buf;
    hb_job_t  *job;
    hb_list_t *list;

    vorbis_dsp_state vd;
    vorbis_comment   vc;
    vorbis_block     vb;
    vorbis_info      vi;

    unsigned  input_samples;
    uint64_t  pts;
    int64_t   prev_blocksize;
    int       out_discrete_channels;

    int       remap_table[8];
};

int encvorbisInit(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    if (pv == NULL)
    {
        hb_error("encvorbis: calloc failed");
        return -1;
    }
    hb_audio_t *audio = w->audio;
    w->private_data = pv;
    pv->job = job;

    int i;
    ogg_packet header[3];

    hb_log("encvorbis: opening libvorbis");

    /* init */
    for (i = 0; i < 3; i++)
    {
        // Zero vorbis headers so that we don't crash in mk_laceXiph
        // when vorbis_encode_setup_managed fails.
        memset(w->config->vorbis.headers[i], 0, sizeof(ogg_packet));
    }
    vorbis_info_init(&pv->vi);

    pv->out_discrete_channels =
        hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);

    if (audio->config.out.bitrate > 0)
    {
        if (vorbis_encode_setup_managed(&pv->vi, pv->out_discrete_channels,
                                        audio->config.out.samplerate, -1,
                                        audio->config.out.bitrate * 1000, -1))
        {
            hb_error("encvorbis: vorbis_encode_setup_managed() failed");
            return -1;
        }
    }
    else if (audio->config.out.quality != HB_INVALID_AUDIO_QUALITY)
    {
        // map VBR quality to Vorbis API (divide by 10)
        if (vorbis_encode_setup_vbr(&pv->vi, pv->out_discrete_channels,
                                    audio->config.out.samplerate,
                                    audio->config.out.quality / 10))
        {
            hb_error("encvorbis: vorbis_encode_setup_vbr() failed");
            return -1;
        }
    }

    if (vorbis_encode_ctl(&pv->vi, OV_ECTL_RATEMANAGE2_SET, NULL) ||
        vorbis_encode_setup_init(&pv->vi))
    {
        hb_error("encvorbis: vorbis_encode_ctl(ratemanage2_set) OR vorbis_encode_setup_init() failed");
        return -1;
    }

    /* add a comment */
    vorbis_comment_init(&pv->vc);
    vorbis_comment_add_tag(&pv->vc, "Encoder", "HandBrake");
    vorbis_comment_add_tag(&pv->vc, "LANGUAGE", w->config->vorbis.language);

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init(&pv->vd, &pv->vi);
    vorbis_block_init(&pv->vd, &pv->vb);

    /* get the 3 headers */
    vorbis_analysis_headerout(&pv->vd, &pv->vc,
                              &header[0], &header[1], &header[2]);
    ogg_packet *pheader;
    for (i = 0; i < 3; i++)
    {
        pheader = (ogg_packet*)w->config->vorbis.headers[i];
        memcpy(pheader, &header[i], sizeof(ogg_packet));
        pheader->packet = w->config->vorbis.headers[i] + sizeof(ogg_packet);
        memcpy(pheader->packet, header[i].packet, header[i].bytes );
    }

    pv->input_samples = pv->out_discrete_channels * OGGVORBIS_FRAME_SIZE;
    audio->config.out.samples_per_frame = OGGVORBIS_FRAME_SIZE;
    pv->buf = malloc(pv->input_samples * sizeof(float));
    if (pv->buf == NULL)
    {
        hb_error("encvorbis: malloc failed");
        return -1;
    }

    pv->list = hb_list_init();

    // channel remapping
    uint64_t layout = hb_ff_mixdown_xlat(audio->config.out.mixdown, NULL);
    hb_audio_remap_build_table(&hb_vorbis_chan_map,
                               audio->config.in.channel_map, layout,
                               pv->remap_table);

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encvorbisClose(hb_work_object_t * w)
{
    hb_work_private_t *pv = w->private_data;

    vorbis_block_clear(&pv->vb);
    vorbis_dsp_clear(&pv->vd);
    vorbis_info_clear(&pv->vi);
    vorbis_comment_clear(&pv->vc);

    if (pv->list)
    {
        hb_list_empty(&pv->list);
    }

    free(pv->buf);
    free(pv);
    w->private_data = NULL;
}

/***********************************************************************
 * Flush
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Flush( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    int64_t     blocksize = 0;

    if( vorbis_analysis_blockout( &pv->vd, &pv->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &pv->vb, NULL );
        vorbis_bitrate_addblock( &pv->vb );

        if( vorbis_bitrate_flushpacket( &pv->vd, &op ) )
        {
            buf = hb_buffer_init( op.bytes );
            memcpy( buf->data, op.packet, op.bytes );
            blocksize = vorbis_packet_blocksize(&pv->vi, &op);

            buf->s.type = AUDIO_BUF;
            buf->s.frametype = HB_FRAME_AUDIO;

            buf->s.start = (int64_t)(vorbis_granule_time(&pv->vd, op.granulepos) * 90000);
            buf->s.stop  = (int64_t)(vorbis_granule_time(&pv->vd, (pv->prev_blocksize + blocksize)/4 + op.granulepos) * 90000);
            buf->s.duration = buf->s.stop - buf->s.start;
            /* The stop time isn't accurate for the first ~3 packets, as the actual blocksize depends on the previous _and_ current packets. */
            pv->prev_blocksize = blocksize;
            return buf;
        }
    }

    return NULL;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t* Encode(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *buf;
    float **buffer;
    int i, j;

    /* Try to extract more data */
    if ((buf = Flush(w)) != NULL)
    {
        return buf;
    }

    /* Check if we need more data */
    if (hb_list_bytes(pv->list) < pv->input_samples * sizeof(float))
    {
        return NULL;
    }

    /* Process more samples */
    hb_list_getbytes(pv->list, pv->buf, pv->input_samples * sizeof(float),
                     &pv->pts, NULL);
    buffer = vorbis_analysis_buffer(&pv->vd, OGGVORBIS_FRAME_SIZE);
    for (i = 0; i < OGGVORBIS_FRAME_SIZE; i++)
    {
        for (j = 0; j < pv->out_discrete_channels; j++)
        {
            buffer[j][i] = ((float*)pv->buf)[(pv->out_discrete_channels * i +
                                              pv->remap_table[j])];
        }
    }

    vorbis_analysis_wrote(&pv->vd, OGGVORBIS_FRAME_SIZE);

    /* Try to extract again */
    return Flush(w);
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encvorbisWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                   hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * buf;
    hb_buffer_list_t list;

    *buf_in = NULL;
    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input - send it downstream & say we're done */
        *buf_out = in;
        return HB_WORK_DONE;
    }

    hb_list_add(pv->list, in);

    buf = Encode( w );
    while (buf)
    {
        hb_buffer_list_append(&list, buf);
        buf = Encode( w );
    }

    *buf_out = hb_buffer_list_clear(&list);
    return HB_WORK_OK;
}
