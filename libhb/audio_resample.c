/* audio_resample.c
 *
 * Copyright (c) 2003-2026 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/audio_resample.h"

hb_audio_resample_t* hb_audio_resample_init(enum AVSampleFormat sample_fmt,
                                            int sample_rate,
                                            int hb_amixdown, int normalize_mix)
{
    hb_audio_resample_t *resample = calloc(1, sizeof(hb_audio_resample_t));
    if (resample == NULL)
    {
        hb_error("hb_audio_resample_init: failed to allocate resample");
        goto fail;
    }

    // swresample context, initialized in hb_audio_resample_update()
    resample->swresample = NULL;

    // we don't support planar output yet
    if (av_sample_fmt_is_planar(sample_fmt))
    {
        hb_error("hb_audio_resample_init: planar output not supported ('%s')",
                 av_get_sample_fmt_name(sample_fmt));
        goto fail;
    }

    // convert mixdown to channel_layout/matrix_encoding combo
    int matrix_encoding;
    uint64_t channel_layout = hb_ff_mixdown_xlat(hb_amixdown, &matrix_encoding);

    /*
     * Mono (Left Only) and Mono (Right Only) output a Mono channel_layout,
     * use a custom remix matrix which selects a single input channel.
     */
    switch (hb_amixdown)
    {
        case HB_AMIXDOWN_LEFT:
            resample->mono_source_channel = AV_CHAN_FRONT_LEFT;
            break;
        case HB_AMIXDOWN_RIGHT:
            resample->mono_source_channel = AV_CHAN_FRONT_RIGHT;
            break;
        default:
            resample->mono_source_channel = AV_CHAN_NONE;
            break;
    }

    // requested output channel_layout, sample_fmt
    av_channel_layout_from_mask(&resample->out.ch_layout, channel_layout);
    resample->out.matrix_encoding     = matrix_encoding;
    resample->out.sample_fmt          = sample_fmt;
    resample->out.sample_rate         = sample_rate;
    if (normalize_mix)
    {
        resample->out.maxval = 1.0;
    }
    else
    {
        resample->out.maxval = 1000;
    }
    resample->out.sample_size         = av_get_bytes_per_sample(sample_fmt);

    // set default input characteristics
    resample->in.sample_fmt         = resample->out.sample_fmt;
    resample->in.sample_rate        = resample->out.sample_rate;
    av_channel_layout_copy(&resample->in.ch_layout, &resample->out.ch_layout);
    resample->in.lfe_mix_level      = HB_MIXLEV_ZERO;
    resample->in.center_mix_level   = HB_MIXLEV_DEFAULT;
    resample->in.surround_mix_level = HB_MIXLEV_DEFAULT;

    // by default, no conversion needed
    resample->resample_needed = 0;
    return resample;

fail:
    hb_audio_resample_free(resample);
    return NULL;
}

void hb_audio_resample_set_ch_layout(hb_audio_resample_t *resample,
                                     const AVChannelLayout *ch_layout)
{
    if (resample != NULL)
    {
        AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
        AVChannelLayout stereo_dowmix = AV_CHANNEL_LAYOUT_STEREO_DOWNMIX;
        if (av_channel_layout_compare(ch_layout, &stereo_dowmix) == 0)
        {
            // Dolby Surround is Stereo when it comes to remixing
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            av_channel_layout_copy(&resample->in.ch_layout, &stereo);
        }
        // swresample can't remap a single-channel layout to
        // another single-channel layout
        else if (av_channel_layout_compare(ch_layout, &mono) == 0 &&
                 ch_layout->nb_channels == 1)
        {
            av_channel_layout_copy(&resample->in.ch_layout, &mono);
        }
        else
        {
            av_channel_layout_copy(&resample->in.ch_layout, ch_layout);
        }
    }
}

void hb_audio_resample_set_mix_levels(hb_audio_resample_t *resample,
                                      double surround_mix_level,
                                      double center_mix_level,
                                      double lfe_mix_level)
{
    if (resample != NULL)
    {
        resample->in.lfe_mix_level      = lfe_mix_level;
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

void hb_audio_resample_set_sample_rate(hb_audio_resample_t *resample,
                                       int sample_rate)
{
    if (resample != NULL)
    {
        resample->in.sample_rate = sample_rate;
    }
}

/*
 * Mono (Left Only) / Mono (Right Only): special Stereo-only mixdowns that
 * keep only the front left (or front right) channel via a custom remix matrix.
 */
static int hb_audio_resample_set_mono_source_matrix(hb_audio_resample_t *resample)
{
    const AVChannelLayout *in = &resample->in.ch_layout;
    double *matrix;
    int index, ret;

    index = av_channel_layout_index_from_channel(in,
                                                 resample->mono_source_channel);
    if (index < 0)
    {
        if (in->nb_channels > 1)
        {
            char description[256];
            av_channel_layout_describe(in, description, sizeof(description));
            hb_log("hb_audio_resample_set_mono_source_matrix: no %s channel "
                   "in input layout %s, downmixing to Mono",
                   resample->mono_source_channel == AV_CHAN_FRONT_RIGHT ?
                   "front right" : "front left", description);
        }
        return 0;
    }

    matrix = av_calloc(in->nb_channels, sizeof(*matrix));
    if (matrix == NULL)
    {
        hb_error("hb_audio_resample_set_mono_source_matrix: "
                 "failed to allocate matrix");
        return AVERROR(ENOMEM);
    }
    matrix[index] = 1.0;

    ret = swr_set_matrix(resample->swresample, matrix, in->nb_channels);
    av_free(matrix);
    if (ret < 0)
    {
        hb_error("hb_audio_resample_set_mono_source_matrix: "
                 "swr_set_matrix() failed (%s)", av_err2str(ret));
    }
    return ret;
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
        (resample->out.sample_fmt != resample->in.sample_fmt ||
         resample->out.sample_rate != resample->in.sample_rate ||
         av_channel_layout_compare(&resample->out.ch_layout, &resample->in.ch_layout));

    resample_changed =
        (resample->resample_needed &&
         (resample->resample.sample_fmt != resample->in.sample_fmt ||
          resample->resample.sample_rate != resample->in.sample_rate ||
          av_channel_layout_compare(&resample->resample.ch_layout, &resample->in.ch_layout) ||
          resample->resample.lfe_mix_level != resample->in.lfe_mix_level ||
          resample->resample.center_mix_level != resample->in.center_mix_level ||
          resample->resample.surround_mix_level != resample->in.surround_mix_level));

    if (resample_changed || (resample->resample_needed &&
                             resample->swresample == NULL))
    {
        if (resample->mono_source_channel != AV_CHAN_NONE)
        {
            swr_free(&resample->swresample);
        }
        if (resample->swresample == NULL)
        {
            resample->swresample = swr_alloc();
            if (resample->swresample == NULL)
            {
                hb_error("hb_audio_resample_update: swr_alloc() failed");
                return 1;
            }

            av_opt_set_int(resample->swresample, "out_sample_fmt",
                           resample->out.sample_fmt, 0);
            av_opt_set_int(resample->swresample, "out_sample_rate",
                           resample->out.sample_rate, 0);
            av_opt_set_chlayout(resample->swresample, "out_chlayout",
                           &resample->out.ch_layout, 0);
            av_opt_set_int(resample->swresample, "matrix_encoding",
                           resample->out.matrix_encoding, 0);
            av_opt_set_double(resample->swresample, "rematrix_maxval",
                              resample->out.maxval, 0);
        }

        av_opt_set_int(resample->swresample, "in_sample_fmt",
                       resample->in.sample_fmt, 0);
        av_opt_set_int(resample->swresample, "in_sample_rate",
                       resample->in.sample_rate, 0);
        av_opt_set_chlayout(resample->swresample, "in_chlayout",
                       &resample->in.ch_layout, 0);
        av_opt_set_double(resample->swresample, "lfe_mix_level",
                          resample->in.lfe_mix_level, 0);
        av_opt_set_double(resample->swresample, "center_mix_level",
                          resample->in.center_mix_level, 0);
        av_opt_set_double(resample->swresample, "surround_mix_level",
                          resample->in.surround_mix_level, 0);

        ret = 0;
        if (resample->mono_source_channel != AV_CHAN_NONE)
        {
            ret = hb_audio_resample_set_mono_source_matrix(resample);
        }
        if (ret == 0 && (ret = swr_init(resample->swresample)))
        {
            hb_error("hb_audio_resample_update: swr_init() failed (%s)",
                     av_err2str(ret));
        }
        if (ret)
        {
            // swresample won't open, start over
            swr_free(&resample->swresample);
            return ret;
        }

        resample->resample.sample_fmt         = resample->in.sample_fmt;
        resample->resample.sample_rate        = resample->in.sample_rate;
        av_channel_layout_copy(&resample->resample.ch_layout, &resample->in.ch_layout);
        resample->resample.lfe_mix_level      = resample->in.lfe_mix_level;
        resample->resample.center_mix_level   = resample->in.center_mix_level;
        resample->resample.surround_mix_level = resample->in.surround_mix_level;
    }

    return 0;
}

void hb_audio_resample_free(hb_audio_resample_t *resample)
{
    if (resample != NULL)
    {
        av_channel_layout_uninit(&resample->in.ch_layout);
        av_channel_layout_uninit(&resample->resample.ch_layout);
        av_channel_layout_uninit(&resample->out.ch_layout);

        if (resample->swresample != NULL)
        {
            swr_free(&resample->swresample);
        }
        free(resample);
    }
}

hb_buffer_t* hb_audio_resample(hb_audio_resample_t *resample,
                               const uint8_t **samples, int nsamples)
{
    if (resample == NULL)
    {
        hb_error("hb_audio_resample: resample is NULL");
        return NULL;
    }
    if (resample->resample_needed && resample->swresample == NULL)
    {
        hb_error("hb_audio_resample: resample needed but libswresample context "
                 "is NULL");
        return NULL;
    }

    hb_buffer_t *out;
    int out_size, out_samples;

    if (resample->resample_needed)
    {
        // Compute in 64 bit, nsamples * sample_rate can overflow an int
        // (e.g. 16384 samples FLAC frames at 192 kHz)
        out_samples = (int64_t)nsamples * resample->out.sample_rate /
                                          resample->in.sample_rate + 1;
        out_size = av_samples_get_buffer_size(NULL, resample->out.ch_layout.nb_channels,
                                              out_samples,
                                              resample->out.sample_fmt, 0);
        out = hb_buffer_init(out_size);
        out_samples = swr_convert(resample->swresample, &out->data, out_samples,
                                                        samples,    nsamples);

        if (out_samples <= 0)
        {
            if (out_samples < 0)
            {
                hb_log("hb_audio_resample: swr_convert() failed");
            }
            // don't send empty buffers downstream (EOF)
            hb_buffer_close(&out);
            return NULL;
        }
        out->size = (out_samples *
                     resample->out.sample_size * resample->out.ch_layout.nb_channels);
    }
    else
    {
        out_samples = nsamples;
        out_size = (out_samples *
                    resample->out.sample_size * resample->out.ch_layout.nb_channels);
        out = hb_buffer_init(out_size);
        memcpy(out->data, samples[0], out_size);
    }

    out->s.duration = 90000. * out_samples / resample->out.sample_rate;

    return out;
}
