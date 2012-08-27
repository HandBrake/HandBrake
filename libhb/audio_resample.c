/* audio_resample.c
 *
 * Copyright (c) 2003-2012 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "common.h"
#include "hbffmpeg.h"
#include "audio_resample.h"

hb_audio_resample_t* hb_audio_resample_init(enum AVSampleFormat output_sample_fmt,
                                            uint64_t output_channel_layout,
                                            enum AVMatrixEncoding matrix_encoding,
                                            int normalize_mix_level)
{
    hb_audio_resample_t *resample = malloc(sizeof(hb_audio_resample_t));
    if (resample == NULL)
    {
        hb_error("hb_audio_resample_init: failed to allocate resample");
        return NULL;
    }

    resample->out.sample_fmt          = output_sample_fmt;
    resample->out.sample_size         =
        av_get_bytes_per_sample(output_sample_fmt);
    resample->out.channel_layout      = output_channel_layout;
    resample->out.channels            =
        av_get_channel_layout_nb_channels(output_channel_layout);
    resample->out.matrix_encoding     = matrix_encoding;
    resample->out.normalize_mix_level = normalize_mix_level;
    resample->resample_needed         = 0;
    resample->avresample              = NULL;

    // set default input characteristics
    resample->in.sample_fmt         = resample->out.sample_fmt;
    resample->in.channel_layout     = resample->out.channel_layout;
    resample->in.center_mix_level   = HB_MIXLEV_DEFAULT;
    resample->in.surround_mix_level = HB_MIXLEV_DEFAULT;

    return resample;
}

void hb_audio_resample_set_channel_layout(hb_audio_resample_t *resample,
                                          uint64_t channel_layout,
                                          int channels)
{
    if (resample != NULL)
    {
        channel_layout = hb_ff_layout_xlat(channel_layout, channels);
        if (channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
        {
            // Dolby Surround is Stereo when it comes to remixing
            channel_layout = AV_CH_LAYOUT_STEREO;
        }
        resample->in.channel_layout = channel_layout;
    }
}

void hb_audio_resample_set_mix_levels(hb_audio_resample_t *resample,
                                      double surround_mix_level,
                                      double center_mix_level)
{
    if (resample != NULL)
    {
        resample->in.center_mix_level   = center_mix_level;
        resample->in.surround_mix_level = surround_mix_level;
    }
}

void hb_audio_resample_set_sample_fmt(hb_audio_resample_t *resample,
                                      enum AVSampleFormat sample_fmt)
{
    if (resample != NULL)
    {
        resample->in.sample_fmt = sample_fmt;
    }
}

int hb_audio_resample_update(hb_audio_resample_t *resample)
{
    if (resample == NULL)
    {
        hb_error("hb_audio_resample_update: resample is NULL");
        return 1;
    }

    int ret, resample_changed;

    resample->resample_needed =
        (resample->resample_needed ||
         resample->out.sample_fmt != resample->in.sample_fmt ||
         resample->out.channel_layout != resample->in.channel_layout);

    resample_changed =
        (resample->resample_needed &&
         (resample->resample.sample_fmt != resample->in.sample_fmt ||
          resample->resample.channel_layout != resample->in.channel_layout ||
          resample->resample.center_mix_level != resample->in.center_mix_level ||
          resample->resample.surround_mix_level != resample->in.surround_mix_level));

    if (resample_changed || (resample->resample_needed &&
                             resample->avresample == NULL))
    {
        if (resample->avresample == NULL)
        {
            resample->avresample = avresample_alloc_context();
            if (resample->avresample == NULL)
            {
                hb_error("hb_audio_resample_update: avresample_alloc_context() failed");
                return 1;
            }

            av_opt_set_int(resample->avresample, "out_sample_fmt",
                           resample->out.sample_fmt, 0);
            av_opt_set_int(resample->avresample, "out_channel_layout",
                           resample->out.channel_layout, 0);
            av_opt_set_int(resample->avresample, "matrix_encoding",
                           resample->out.matrix_encoding, 0);
            av_opt_set_int(resample->avresample, "normalize_mix_level",
                           resample->out.normalize_mix_level, 0);
        }
        else if (resample_changed)
        {
            avresample_close(resample->avresample);
        }

        av_opt_set_int(resample->avresample, "in_sample_fmt",
                       resample->in.sample_fmt, 0);
        av_opt_set_int(resample->avresample, "in_channel_layout",
                       resample->in.channel_layout, 0);
        av_opt_set_double(resample->avresample, "center_mix_level",
                          resample->in.center_mix_level, 0);
        av_opt_set_double(resample->avresample, "surround_mix_level",
                          resample->in.surround_mix_level, 0);

        if ((ret = avresample_open(resample->avresample)))
        {
            char err_desc[64];
            av_strerror(ret, err_desc, 63);
            hb_error("hb_audio_resample_update: avresample_open() failed (%s)",
                     err_desc);
            // avresample won't open, start over
            avresample_free(&resample->avresample);
            return ret;
        }

        resample->resample.sample_fmt         = resample->in.sample_fmt;
        resample->resample.channel_layout     = resample->in.channel_layout;
        resample->resample.channels           =
            av_get_channel_layout_nb_channels(resample->in.channel_layout);
        resample->resample.center_mix_level   = resample->in.center_mix_level;
        resample->resample.surround_mix_level = resample->in.surround_mix_level;
    }

    return 0;
}

void hb_audio_resample_free(hb_audio_resample_t *resample)
{
    if (resample != NULL)
    {
        if (resample->avresample != NULL)
        {
            avresample_free(&resample->avresample);
        }
        free(resample);
    }
}

hb_buffer_t* hb_audio_resample(hb_audio_resample_t *resample,
                               void *samples, int nsamples)
{
    if (resample == NULL)
    {
        hb_error("hb_audio_resample: resample is NULL");
        return NULL;
    }
    if (resample->resample_needed && resample->avresample == NULL)
    {
        hb_error("hb_audio_resample: resample needed but libavresample context "
                 "is NULL");
        return NULL;
    }

    int out_size;
    hb_buffer_t *out;

    if (resample->resample_needed)
    {
        int in_linesize, out_linesize, out_samples;
        // set in/out linesize and out_size
        av_samples_get_buffer_size(&in_linesize,
                                   resample->resample.channels, nsamples,
                                   resample->resample.sample_fmt, 0);
        out_size = av_samples_get_buffer_size(&out_linesize,
                                              resample->out.channels, nsamples,
                                              resample->out.sample_fmt, 0);
        out = hb_buffer_init(out_size);

        out_samples = avresample_convert(resample->avresample,
                                         (void**)&out->data, out_linesize, nsamples,
                                         (void**)&samples,    in_linesize, nsamples);

        if (out_samples <= 0)
        {
            if (out_samples < 0)
                hb_log("hb_audio_resample: avresample_convert() failed");
            // don't send empty buffers downstream (EOF)
            hb_buffer_close(&out);
            return NULL;
        }
        out->size = (out_samples *
                     resample->out.sample_size * resample->out.channels);
    }
    else
    {
        out_size = (nsamples *
                    resample->out.sample_size * resample->out.channels);
        out = hb_buffer_init(out_size);
        memcpy(out->data, samples, out_size);
    }

    return out;
}
