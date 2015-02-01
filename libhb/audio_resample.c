/* audio_resample.c
 *
 * Copyright (c) 2003-2015 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "common.h"
#include "hbffmpeg.h"
#include "audio_resample.h"

hb_audio_resample_t* hb_audio_resample_init(enum AVSampleFormat sample_fmt,
                                            int hb_amixdown, int normalize_mix)
{
    hb_audio_resample_t *resample = calloc(1, sizeof(hb_audio_resample_t));
    if (resample == NULL)
    {
        hb_error("hb_audio_resample_init: failed to allocate resample");
        goto fail;
    }

    // avresample context, initialized in hb_audio_resample_update()
    resample->avresample = NULL;

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
    resample->out.channels = av_get_channel_layout_nb_channels(channel_layout);
    resample->out.channel_layout      = channel_layout;
    resample->out.matrix_encoding     = matrix_encoding;
    resample->out.normalize_mix_level = normalize_mix;
    resample->out.sample_fmt          = sample_fmt;
    resample->out.sample_size         = av_get_bytes_per_sample(sample_fmt);

    // set default input characteristics
    resample->in.sample_fmt         = resample->out.sample_fmt;
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
         resample->out.channel_layout != resample->in.channel_layout);

    resample_changed =
        (resample->resample_needed &&
         (resample->resample.sample_fmt != resample->in.sample_fmt ||
          resample->resample.channel_layout != resample->in.channel_layout ||
          resample->resample.lfe_mix_level != resample->in.lfe_mix_level ||
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
        av_opt_set_double(resample->avresample, "lfe_mix_level",
                          resample->in.lfe_mix_level, 0);
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
        if (resample->avresample != NULL)
        {
            avresample_free(&resample->avresample);
        }
        free(resample);
    }
}

hb_buffer_t* hb_audio_resample(hb_audio_resample_t *resample,
                               uint8_t **samples, int nsamples)
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

    hb_buffer_t *out;
    int out_size, out_samples;

    if (resample->resample_needed)
    {
        int in_linesize, out_linesize;
        // set in/out linesize and out_size
        av_samples_get_buffer_size(&in_linesize,
                                   resample->resample.channels, nsamples,
                                   resample->resample.sample_fmt, 0);
        out_size = av_samples_get_buffer_size(&out_linesize,
                                              resample->out.channels, nsamples,
                                              resample->out.sample_fmt, 0);
        out = hb_buffer_init(out_size);

        out_samples = avresample_convert(resample->avresample,
                                         &out->data, out_linesize, nsamples,
                                         samples,     in_linesize, nsamples);

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

    return out;
}
