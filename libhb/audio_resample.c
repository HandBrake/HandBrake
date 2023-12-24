/* audio_resample.c
 *
 * Copyright (c) 2003-2023 HandBrake Team
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
     * When downmixing, Dual Mono to Mono is a special case:
     * the audio must remain 2-channel until all conversions are done.
     */
    if (hb_amixdown == HB_AMIXDOWN_LEFT || hb_amixdown == HB_AMIXDOWN_RIGHT)
    {
        channel_layout                 = AV_CH_LAYOUT_STEREO;
        resample->dual_mono_downmix    = 1;
        resample->dual_mono_right_only = (hb_amixdown == HB_AMIXDOWN_RIGHT);
    }
    else
    {
        resample->dual_mono_downmix = 0;
    }

    // requested output channel_layout, sample_fmt
    resample->out.channels            = hb_layout_get_discrete_channel_count(channel_layout);
    resample->out.channel_layout      = channel_layout;
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
    resample->in.channel_layout     = resample->out.channel_layout;
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

static int is_mono(uint64_t layout)
{
    int ii, channel_count;
    uint64_t mask;

    if (layout == AV_CH_LAYOUT_NATIVE)
    {
        return 0;
    }
    for (ii = 0, channel_count = 0, mask = 1;
         ii < 64 && channel_count < 2; ii++, mask <<= 1)
    {
        if (layout & mask)
        {
            channel_count++;
        }
    }
    return channel_count == 1;
}

void hb_audio_resample_set_channel_layout(hb_audio_resample_t *resample,
                                          uint64_t channel_layout)
{
    if (resample != NULL)
    {
        if (channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
        {
            // Dolby Surround is Stereo when it comes to remixing
            channel_layout = AV_CH_LAYOUT_STEREO;
        }
        // swresample can't remap a single-channel layout to
        // another single-channel layout
        if (resample->out.channel_layout == AV_CH_LAYOUT_MONO &&
            is_mono(channel_layout))
        {
            channel_layout = AV_CH_LAYOUT_MONO;
        }
        resample->in.channel_layout = channel_layout;
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
         resample->out.channel_layout != resample->in.channel_layout);

    resample_changed =
        (resample->resample_needed &&
         (resample->resample.sample_fmt != resample->in.sample_fmt ||
          resample->resample.sample_rate != resample->in.sample_rate ||
          resample->resample.channel_layout != resample->in.channel_layout ||
          resample->resample.lfe_mix_level != resample->in.lfe_mix_level ||
          resample->resample.center_mix_level != resample->in.center_mix_level ||
          resample->resample.surround_mix_level != resample->in.surround_mix_level));

    if (resample_changed || (resample->resample_needed &&
                             resample->swresample == NULL))
    {
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
            av_opt_set_int(resample->swresample, "out_channel_layout",
                           resample->out.channel_layout, 0);
            av_opt_set_int(resample->swresample, "matrix_encoding",
                           resample->out.matrix_encoding, 0);
            av_opt_set_double(resample->swresample, "rematrix_maxval",
                              resample->out.maxval, 0);
        }

        av_opt_set_int(resample->swresample, "in_sample_fmt",
                       resample->in.sample_fmt, 0);
        av_opt_set_int(resample->swresample, "in_sample_rate",
                       resample->in.sample_rate, 0);
        av_opt_set_int(resample->swresample, "in_channel_layout",
                       resample->in.channel_layout, 0);
        av_opt_set_double(resample->swresample, "lfe_mix_level",
                          resample->in.lfe_mix_level, 0);
        av_opt_set_double(resample->swresample, "center_mix_level",
                          resample->in.center_mix_level, 0);
        av_opt_set_double(resample->swresample, "surround_mix_level",
                          resample->in.surround_mix_level, 0);

        if ((ret = swr_init(resample->swresample)))
        {
            char err_desc[64];
            av_strerror(ret, err_desc, 63);
            hb_error("hb_audio_resample_update: swr_init() failed (%s)",
                     err_desc);
            // swresample won't open, start over
            swr_free(&resample->swresample);
            return ret;
        }

        resample->resample.sample_fmt         = resample->in.sample_fmt;
        resample->resample.sample_rate        = resample->in.sample_rate;
        resample->resample.channel_layout     = resample->in.channel_layout;
        resample->resample.channels           = hb_layout_get_discrete_channel_count(resample->in.channel_layout);
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
        out_samples = nsamples  * resample->out.sample_rate /
                                  resample->in.sample_rate + 1;
        out_size = av_samples_get_buffer_size(NULL, resample->out.channels,
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
                     resample->out.sample_size * resample->out.channels);
    }
    else
    {
        out_samples = nsamples;
        out_size = (out_samples *
                    resample->out.sample_size * resample->out.channels);
        out = hb_buffer_init(out_size);
        memcpy(out->data, samples[0], out_size);
    }

    /*
     * Dual Mono to Mono.
     *
     * Copy all left or right samples to the first half of the buffer and halve
     * the buffer size.
     */
    if (resample->dual_mono_downmix)
    {
        int ii, jj = !!resample->dual_mono_right_only;
        int sample_size = resample->out.sample_size;
        uint8_t *audio_samples = out->data;
        for (ii = 0; ii < out_samples; ii++)
        {
            memcpy(audio_samples + (ii * sample_size),
                   audio_samples + (jj * sample_size), sample_size);
            jj += 2;
        }
        out->size = out_samples * sample_size;
    }
    out->s.duration = 90000. * out_samples / resample->out.sample_rate;

    return out;
}
