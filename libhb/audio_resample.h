/* audio_resample.h
 *
 * Copyright (c) 2003-2015 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* Implements a libavresample wrapper for convenience.
 *
 * Supports sample_fmt and channel_layout conversion.
 *
 * sample_rate conversion will come later (libavresample doesn't support
 * sample_rate conversion with float samples yet). */

#ifndef AUDIO_RESAMPLE_H
#define AUDIO_RESAMPLE_H

#include <math.h>
#include <stdint.h>
#include "libavutil/channel_layout.h"
#include "libavresample/avresample.h"

/* Default mix level for center and surround channels */
#define HB_MIXLEV_DEFAULT ((double)M_SQRT1_2)
/* Default mix level for LFE channel */
#define HB_MIXLEV_ZERO    ((double)0.0)

typedef struct
{
    int dual_mono_downmix;
    int dual_mono_right_only;

    int resample_needed;
    AVAudioResampleContext *avresample;

    struct
    {
        uint64_t channel_layout;
        double lfe_mix_level;
        double center_mix_level;
        double surround_mix_level;
        enum AVSampleFormat sample_fmt;
    } in;

    struct
    {
        int channels;
        uint64_t channel_layout;
        double lfe_mix_level;
        double center_mix_level;
        double surround_mix_level;
        enum AVSampleFormat sample_fmt;
    } resample;

    struct
    {
        int channels;
        int sample_size;
        int normalize_mix_level;
        uint64_t channel_layout;
        enum AVSampleFormat sample_fmt;
        enum AVMatrixEncoding matrix_encoding;
    } out;
} hb_audio_resample_t;

/* Initialize an hb_audio_resample_t for converting audio to the requested
 * sample_fmt and mixdown.
 *
 * Also sets the default audio input characteristics, so that they are the same
 * as the output characteristics (no conversion needed).
 */
hb_audio_resample_t* hb_audio_resample_init(enum AVSampleFormat sample_fmt,
                                            int hb_amixdown, int normalize_mix);

/* The following functions set the audio input characteristics.
 *
 * They should be called whenever the relevant characteristic(s) differ from the
 * requested output characteristics, or if they may have changed in the source.
 */

void                 hb_audio_resample_set_channel_layout(hb_audio_resample_t *resample,
                                                          uint64_t channel_layout);

void                 hb_audio_resample_set_mix_levels(hb_audio_resample_t *resample,
                                                      double surround_mix_level,
                                                      double center_mix_level,
                                                      double lfe_mix_level);

void                 hb_audio_resample_set_sample_fmt(hb_audio_resample_t *resample,
                                                      enum AVSampleFormat sample_fmt);

/* Update an hb_audio_resample_t.
 *
 * Must be called after using any of the above functions.
 */
int                  hb_audio_resample_update(hb_audio_resample_t *resample);

/* Free an hb_audio_remsample_t. */
void                 hb_audio_resample_free(hb_audio_resample_t *resample);

/* Convert input samples to the requested output characteristics
 * (sample_fmt and channel_layout + matrix_encoding).
 *
 * Returns an hb_buffer_t with the converted output.
 *
 * resampling is only done when necessary.
 */
hb_buffer_t*         hb_audio_resample(hb_audio_resample_t *resample,
                                       uint8_t **samples, int nsamples);

#endif /* AUDIO_RESAMPLE_H */
