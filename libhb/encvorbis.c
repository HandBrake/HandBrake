/* encvorbis.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/extradata.h"
#include "handbrake/handbrake.h"
#include "handbrake/audio_remap.h"

#include "vorbis/vorbisenc.h"

#define OGGVORBIS_FRAME_SIZE 1024

struct hb_work_private_s
{
    float     *buf;
    hb_list_t *list;

    vorbis_dsp_state vd;
    vorbis_comment   vc;
    vorbis_block     vb;
    vorbis_info      vi;

    unsigned  input_samples;
    uint64_t  pts;
    int64_t   first_pts;
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

    pv->first_pts = AV_NOPTS_VALUE;

    hb_log("encvorbis: opening libvorbis");

    vorbis_info_init(&pv->vi);

    pv->out_discrete_channels = audio->config.out.ch_layout->nb_channels;

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
    vorbis_comment_add_tag(&pv->vc, "LANGUAGE", audio->config.lang.simple);

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init(&pv->vd, &pv->vi);
    vorbis_block_init(&pv->vd, &pv->vb);

    /* get the 3 headers */
    ogg_packet header[3];
    vorbis_analysis_headerout(&pv->vd, &pv->vc,
                              &header[0], &header[1], &header[2]);

    uint8_t headers[3][HB_CONFIG_MAX_SIZE];
    for (int i = 0; i < 3; i++)
    {
        ogg_packet *pheader = (ogg_packet *)headers[i];
        memcpy(pheader, &header[i], sizeof(ogg_packet));
        pheader->packet = headers[i] + sizeof(ogg_packet);
        memcpy(pheader->packet, header[i].packet, header[i].bytes );
    }

    hb_set_xiph_extradata(w->extradata, headers);

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
    AVChannelLayout out_layout = {0};
    hb_audio_remap_map_channel_layout(&hb_vorbis_chan_map, &out_layout, audio->config.out.ch_layout);

    hb_audio_remap_build_table(&out_layout,
                               audio->config.out.ch_layout,
                               pv->remap_table);

    av_channel_layout_uninit(&out_layout);

    return 0;
}

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

static hb_buffer_t * receive(hb_work_private_t *pv)
{
    if( vorbis_analysis_blockout( &pv->vd, &pv->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &pv->vb, NULL );
        vorbis_bitrate_addblock( &pv->vb );

        if( vorbis_bitrate_flushpacket( &pv->vd, &op ) )
        {
            hb_buffer_t *buf = hb_buffer_init( op.bytes );
            memcpy( buf->data, op.packet, op.bytes );
            int64_t blocksize = vorbis_packet_blocksize(&pv->vi, &op);

            buf->s.type = AUDIO_BUF;
            buf->s.frametype = HB_FRAME_AUDIO;

            // Granule positions count samples from the first sample sent to
            // the encoder, so offset them by that sample's timestamp
            buf->s.start = pv->first_pts + (int64_t)(vorbis_granule_time(&pv->vd, op.granulepos) * 90000);
            buf->s.stop  = pv->first_pts + (int64_t)(vorbis_granule_time(&pv->vd, (pv->prev_blocksize + blocksize)/4 + op.granulepos) * 90000);
            buf->s.duration = buf->s.stop - buf->s.start;
            /* The stop time isn't accurate for the first ~3 packets, as the actual blocksize depends on the previous _and_ current packets. */
            pv->prev_blocksize = blocksize;
            return buf;
        }
    }

    return NULL;
}

/* Read nsamples interleaved samples from the input list and hand them
 * to libvorbis. nsamples must not exceed OGGVORBIS_FRAME_SIZE. */
static void send(hb_work_private_t *pv, int nsamples)
{
    float **buffer;

    hb_list_getbytes(pv->list, (uint8_t *)pv->buf,
                     nsamples * pv->out_discrete_channels * sizeof(float),
                     &pv->pts, NULL);
    if (pv->first_pts == AV_NOPTS_VALUE)
    {
        pv->first_pts = pv->pts;
    }
    buffer = vorbis_analysis_buffer(&pv->vd, nsamples);
    for (int i = 0; i < nsamples; i++)
    {
        for (int j = 0; j < pv->out_discrete_channels; j++)
        {
            buffer[j][i] = ((float*)pv->buf)[(pv->out_discrete_channels * i +
                                              pv->remap_table[j])];
        }
    }

    vorbis_analysis_wrote(&pv->vd, nsamples);
}

static hb_buffer_t * encode(hb_work_private_t *pv)
{
    hb_buffer_t *buf;

    /* Try to extract more data */
    if ((buf = receive(pv)) != NULL)
    {
        return buf;
    }

    /* Check if we need more data */
    if (hb_list_bytes(pv->list) < pv->input_samples * sizeof(float))
    {
        return NULL;
    }

    /* Process more samples */
    send(pv, OGGVORBIS_FRAME_SIZE);

    /* Try to extract again */
    return receive(pv);
}

/* Encode the samples left over at EOF (less than a full frame), signal
 * the end of the stream to libvorbis and collect the remaining packets. */
static void flush(hb_work_private_t *pv, hb_buffer_list_t *list)
{
    hb_buffer_t *buf;

    while ((buf = encode(pv)) != NULL)
    {
        hb_buffer_list_append(list, buf);
    }

    int nsamples = hb_list_bytes(pv->list) /
               (pv->out_discrete_channels * sizeof(float));
    if (nsamples > 0)
    {
        send(pv, nsamples);
    }

    /* A zero-length write marks the end of the stream, which makes
     * libvorbis release the blocks it still holds for analysis */
    vorbis_analysis_wrote(&pv->vd, 0);

    while ((buf = receive(pv)) != NULL)
    {
        hb_buffer_list_append(list, buf);
    }
}

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
        /* EOF on input - flush the encoder, then send the EOF
         * downstream & say we're done */
        flush(pv, &list);
        hb_buffer_list_append(&list, in);
        *buf_out = hb_buffer_list_clear(&list);
        return HB_WORK_DONE;
    }

    hb_list_add(pv->list, in);

    buf = encode(pv);
    while (buf)
    {
        hb_buffer_list_append(&list, buf);
        buf = encode(pv);
    }

    *buf_out = hb_buffer_list_clear(&list);
    return HB_WORK_OK;
}

hb_work_object_t hb_encvorbis =
{
    WORK_ENCVORBIS,
    "Vorbis encoder (libvorbis)",
    encvorbisInit,
    encvorbisWork,
    encvorbisClose
};
