/* audio_resample.h
 *
 * Copyright (c) 2003-2012 HandBrake Team
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
#include "libavutil/audioconvert.h"
#include "libavresample/avresample.h"

/* Default mix level for center and surround channels */
#define HB_MIXLEV_DEFAULT ((double)M_SQRT1_2)

typedef struct
{
    int resample_needed;
    AVAudioResampleContext *avresample;

    struct
    {
        int channels;
        int linesize;
        uint64_t channel_layout;
        double center_mix_level;
        double surround_mix_level;
        enum AVSampleFormat sample_fmt;
    } in;

    struct
    {
        int channels;
        int linesize;
        int sample_size;
        int normalize_mix_level;
        uint64_t channel_layout;
        enum AVSampleFormat sample_fmt;
        enum AVMatrixEncoding matrix_encoding;
    } out;
} hb_audio_resample_t;

/* Initialize an hb_audio_resample_t for converting audio to the requested
 * sample_fmt and channel_layout, using the specified matrix_encoding.
 *
 * Input characteristics are set via hb_audio_resample_update().
 */
hb_audio_resample_t* hb_audio_resample_init(enum AVSampleFormat output_sample_fmt,
                                            uint64_t output_channel_layout,
                                            enum AVMatrixEncoding matrix_encoding,
                                            int normalize_mix_level);

/* Update an hb_audio_resample_t, setting the input sample characteristics.
 *
 * Can be called whenever the input type may change.
 *
 * new_channel_layout is sanitized automatically.
 */
int                  hb_audio_resample_update(hb_audio_resample_t *resample,
                                              enum AVSampleFormat new_sample_fmt,
                                              uint64_t new_channel_layout,
                                              double new_surroundmixlev,
                                              double new_centermixlev,
                                              int new_channels);

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
                                       void *samples, int nsamples);

#endif /* AUDIO_RESAMPLE_H */
