/* common.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>

#include "common.h"
#include "lang.h"
#include "hb.h"

/**********************************************************************
 * Global variables
 *********************************************************************/
hb_rate_t hb_video_rates[] =
{
    {  "5",     5400000 },
    { "10",     2700000 },
    { "12",     2250000 },
    { "15",     1800000 },
    { "23.976", 1126125 },
    { "24",     1125000 },
    { "25",     1080000 },
    { "29.97",   900900 },
    { "30",      900000 },
    { "50",      540000 },
    { "59.94",   450450 },
    { "60",      450000 },
};
int hb_video_rates_count = sizeof(hb_video_rates) / sizeof(hb_rate_t);

hb_rate_t hb_audio_rates[] =
{
    {  "8",      8000 },
    { "11.025", 11025 },
    { "12",     12000 },
    { "16",     16000 },
    { "22.05",  22050 },
    { "24",     24000 },
    { "32",     32000 },
    { "44.1",   44100 },
    { "48",     48000 },
};
int hb_audio_rates_count = sizeof(hb_audio_rates) / sizeof(hb_rate_t);

hb_rate_t hb_audio_bitrates[] =
{
    // AC3-compatible bitrates
    {   "32",   32 },
    {   "40",   40 },
    {   "48",   48 },
    {   "56",   56 },
    {   "64",   64 },
    {   "80",   80 },
    {   "96",   96 },
    {  "112",  112 },
    {  "128",  128 },
    {  "160",  160 },
    {  "192",  192 },
    {  "224",  224 },
    {  "256",  256 },
    {  "320",  320 },
    {  "384",  384 },
    {  "448",  448 },
    {  "512",  512 },
    {  "576",  576 },
    {  "640",  640 },
    // additional bitrates
    {  "768",  768 },
    {  "960",  960 },
    { "1152", 1152 },
    { "1344", 1344 },
    { "1536", 1536 },
};
int hb_audio_bitrates_count = sizeof(hb_audio_bitrates) / sizeof(hb_rate_t);

static hb_error_handler_t *error_handler = NULL;

hb_dither_t hb_audio_dithers[] =
{
    { "default",                       "auto",          AV_RESAMPLE_DITHER_NONE - 1,      },
    { "none",                          "none",          AV_RESAMPLE_DITHER_NONE,          },
    { "rectangular",                   "rectangular",   AV_RESAMPLE_DITHER_RECTANGULAR,   },
    { "triangular",                    "triangular",    AV_RESAMPLE_DITHER_TRIANGULAR,    },
    { "triangular with high pass",     "triangular_hp", AV_RESAMPLE_DITHER_TRIANGULAR_HP, },
    { "triangular with noise shaping", "triangular_ns", AV_RESAMPLE_DITHER_TRIANGULAR_NS, },
};
int hb_audio_dithers_count = sizeof(hb_audio_dithers) / sizeof(hb_dither_t);

hb_mixdown_t hb_audio_mixdowns[] =
{
    { "None",               "HB_AMIXDOWN_NONE",      "none",       HB_AMIXDOWN_NONE      },
    { "Mono",               "HB_AMIXDOWN_MONO",      "mono",       HB_AMIXDOWN_MONO      },
    { "Mono (Left Only)",   "HB_AMIXDOWN_LEFT",      "left_only",  HB_AMIXDOWN_LEFT      },
    { "Mono (Right Only)",  "HB_AMIXDOWN_RIGHT",     "right_only", HB_AMIXDOWN_RIGHT     },
    { "Stereo",             "HB_AMIXDOWN_STEREO",    "stereo",     HB_AMIXDOWN_STEREO    },
    { "Dolby Surround",     "HB_AMIXDOWN_DOLBY",     "dpl1",       HB_AMIXDOWN_DOLBY     },
    { "Dolby Pro Logic II", "HB_AMIXDOWN_DOLBYPLII", "dpl2",       HB_AMIXDOWN_DOLBYPLII },
    { "5.1 Channels",       "HB_AMIXDOWN_5POINT1",   "5point1",    HB_AMIXDOWN_5POINT1   },
    { "6.1 Channels",       "HB_AMIXDOWN_6POINT1",   "6point1",    HB_AMIXDOWN_6POINT1   },
    { "7.1 Channels",       "HB_AMIXDOWN_7POINT1",   "7point1",    HB_AMIXDOWN_7POINT1   },
    { "7.1 (5F/2R/LFE)",    "HB_AMIXDOWN_5_2_LFE",   "5_2_lfe",    HB_AMIXDOWN_5_2_LFE   },
};
int hb_audio_mixdowns_count = sizeof(hb_audio_mixdowns) / sizeof(hb_mixdown_t);

hb_encoder_t hb_video_encoders[] =
{
    { "H.264 (x264)",    "x264",    HB_VCODEC_X264,         HB_MUX_MP4|HB_MUX_MKV },
    { "MPEG-4 (FFmpeg)", "ffmpeg4", HB_VCODEC_FFMPEG_MPEG4, HB_MUX_MP4|HB_MUX_MKV },
    { "MPEG-2 (FFmpeg)", "ffmpeg2", HB_VCODEC_FFMPEG_MPEG2, HB_MUX_MP4|HB_MUX_MKV },
    { "VP3 (Theora)",    "theora",  HB_VCODEC_THEORA,                  HB_MUX_MKV },
};
int hb_video_encoders_count = sizeof(hb_video_encoders) / sizeof(hb_encoder_t);

// note: the first encoder in the list must be AAC
hb_encoder_t hb_audio_encoders[] =
{
#ifdef __APPLE__
    { "AAC (CoreAudio)",    "ca_aac",     HB_ACODEC_CA_AAC,       HB_MUX_MP4|HB_MUX_MKV },
    { "HE-AAC (CoreAudio)", "ca_haac",    HB_ACODEC_CA_HAAC,      HB_MUX_MP4|HB_MUX_MKV },
#endif
    { "AAC (faac)",         "faac",       HB_ACODEC_FAAC,         HB_MUX_MP4|HB_MUX_MKV },
#ifdef USE_FDK_AAC
    { "AAC (FDK)",          "fdk_aac",    HB_ACODEC_FDK_AAC,      HB_MUX_MP4|HB_MUX_MKV },
    { "HE-AAC (FDK)",       "fdk_haac",   HB_ACODEC_FDK_HAAC,     HB_MUX_MP4|HB_MUX_MKV },
#endif
    { "AAC (ffmpeg)",       "ffaac",      HB_ACODEC_FFAAC,        HB_MUX_MP4|HB_MUX_MKV },
    { "AAC Passthru",       "copy:aac",   HB_ACODEC_AAC_PASS,     HB_MUX_MP4|HB_MUX_MKV },
    { "AC3 (ffmpeg)",       "ffac3",      HB_ACODEC_AC3,          HB_MUX_MP4|HB_MUX_MKV },
    { "AC3 Passthru",       "copy:ac3",   HB_ACODEC_AC3_PASS,     HB_MUX_MP4|HB_MUX_MKV },
    { "DTS Passthru",       "copy:dts",   HB_ACODEC_DCA_PASS,     HB_MUX_MP4|HB_MUX_MKV },
    { "DTS-HD Passthru",    "copy:dtshd", HB_ACODEC_DCA_HD_PASS,  HB_MUX_MP4|HB_MUX_MKV },
    { "MP3 (lame)",         "lame",       HB_ACODEC_LAME,         HB_MUX_MP4|HB_MUX_MKV },
    { "MP3 Passthru",       "copy:mp3",   HB_ACODEC_MP3_PASS,     HB_MUX_MP4|HB_MUX_MKV },
    { "Vorbis (vorbis)",    "vorbis",     HB_ACODEC_VORBIS,                  HB_MUX_MKV },
    { "FLAC (ffmpeg)",      "ffflac",     HB_ACODEC_FFFLAC,                  HB_MUX_MKV },
    { "FLAC (24-bit)",      "ffflac24",   HB_ACODEC_FFFLAC24,                HB_MUX_MKV },
    { "Auto Passthru",      "copy",       HB_ACODEC_AUTO_PASS,    HB_MUX_MP4|HB_MUX_MKV },
};
int hb_audio_encoders_count = sizeof(hb_audio_encoders) / sizeof(hb_encoder_t);

/* Expose values for PInvoke */
hb_rate_t*    hb_get_video_rates()          { return hb_video_rates;          }
int           hb_get_video_rates_count()    { return hb_video_rates_count;    }
hb_rate_t*    hb_get_audio_rates()          { return hb_audio_rates;          }
int           hb_get_audio_rates_count()    { return hb_audio_rates_count;    }
hb_rate_t*    hb_get_audio_bitrates()       { return hb_audio_bitrates;       }
int           hb_get_audio_bitrates_count() { return hb_audio_bitrates_count; }
hb_dither_t*  hb_get_audio_dithers()        { return hb_audio_dithers;        }
int           hb_get_audio_dithers_count()  { return hb_audio_dithers_count;  }
hb_mixdown_t* hb_get_audio_mixdowns()       { return hb_audio_mixdowns;       }
int           hb_get_audio_mixdowns_count() { return hb_audio_mixdowns_count; }
hb_encoder_t* hb_get_video_encoders()       { return hb_video_encoders;       }
int           hb_get_video_encoders_count() { return hb_video_encoders_count; }
hb_encoder_t* hb_get_audio_encoders()       { return hb_audio_encoders;       }
int           hb_get_audio_encoders_count() { return hb_audio_encoders_count; }

int hb_audio_dither_get_default()
{
    // "auto"
    return hb_audio_dithers[0].method;
}

int hb_audio_dither_get_default_method()
{
    /*
     * input could be s16 (possibly already dithered) converted to flt, so
     * let's use a "low-risk" dither algorithm (standard triangular).
     */
    return AV_RESAMPLE_DITHER_TRIANGULAR;
}

int hb_audio_dither_is_supported(uint32_t codec)
{
    // encoder's input sample format must be s16(p)
    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            return 1;
        default:
            return 0;
    }
}

const char* hb_audio_dither_get_description(int method)
{
    int i;
    for (i = 0; i < hb_audio_dithers_count; i++)
    {
        if (hb_audio_dithers[i].method == method)
        {
            return hb_audio_dithers[i].description;
        }
    }
    return "";
}


int hb_mixdown_is_supported(int mixdown, uint32_t codec, uint64_t layout)
{
    return (hb_mixdown_has_codec_support(mixdown, codec) &&
            hb_mixdown_has_remix_support(mixdown, layout));
}

int hb_mixdown_has_codec_support(int mixdown, uint32_t codec)
{
    // Passthru, only "None" mixdown is supported
    if (codec & HB_ACODEC_PASS_FLAG)
        return (mixdown == HB_AMIXDOWN_NONE);

    // Not passthru, "None" mixdown never supported
    if (mixdown == HB_AMIXDOWN_NONE)
        return 0;

    switch (codec)
    {
        case HB_ACODEC_VORBIS:
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return (mixdown <= HB_AMIXDOWN_7POINT1);

        case HB_ACODEC_LAME:
        case HB_ACODEC_FFAAC:
            return (mixdown <= HB_AMIXDOWN_DOLBYPLII);

        case HB_ACODEC_FAAC:
        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_CA_HAAC:
            return ((mixdown <= HB_AMIXDOWN_5POINT1) ||
                    (mixdown == HB_AMIXDOWN_5_2_LFE));

        default:
            return (mixdown <= HB_AMIXDOWN_5POINT1);
    }
}

int hb_mixdown_has_remix_support(int mixdown, uint64_t layout)
{
    switch (mixdown)
    {
        // stereo + front left/right of center
        case HB_AMIXDOWN_5_2_LFE:
            return ((layout & AV_CH_FRONT_LEFT_OF_CENTER) &&
                    (layout & AV_CH_FRONT_RIGHT_OF_CENTER) &&
                    (layout & AV_CH_LAYOUT_STEREO) == AV_CH_LAYOUT_STEREO);

        // 7.0 or better
        case HB_AMIXDOWN_7POINT1:
            return ((layout & AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0);

        // 6.0 or better
        case HB_AMIXDOWN_6POINT1:
            return ((layout & AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0 ||
                    (layout & AV_CH_LAYOUT_6POINT0) == AV_CH_LAYOUT_6POINT0 ||
                    (layout & AV_CH_LAYOUT_HEXAGONAL) == AV_CH_LAYOUT_HEXAGONAL);

        // stereo + either of front center, side or back left/right, back center
        case HB_AMIXDOWN_5POINT1:
            return ((layout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1 ||
                    (layout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2 ||
                    (layout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD ||
                    (layout & AV_CH_LAYOUT_SURROUND) == AV_CH_LAYOUT_SURROUND);

        // stereo + either of side or back left/right, back center
        // also, allow Dolby Surrounbd output if the input is already Dolby
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            return ((layout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1 ||
                    (layout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2 ||
                    (layout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD ||
                    (layout == AV_CH_LAYOUT_STEREO_DOWNMIX &&
                     mixdown == HB_AMIXDOWN_DOLBY));

        // more than 1 channel
        case HB_AMIXDOWN_STEREO:
            return (av_get_channel_layout_nb_channels(layout) > 1);

        // regular stereo (not Dolby)
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            return (layout == AV_CH_LAYOUT_STEREO);

        // mono remix always supported
        // HB_AMIXDOWN_NONE always supported (for Passthru)
        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_NONE:
            return 1;

        // unknown mixdown, should never happen
        default:
            return 0;
    }
}

int hb_mixdown_get_discrete_channel_count(int amixdown)
{
    switch (amixdown)
    {
        case HB_AMIXDOWN_5_2_LFE:
        case HB_AMIXDOWN_7POINT1:
            return 8;

        case HB_AMIXDOWN_6POINT1:
            return 7;

        case HB_AMIXDOWN_5POINT1:
            return 6;

        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            return 1;

        case HB_AMIXDOWN_NONE:
            return 0;

        default:
            return 2;
    }
}

int hb_mixdown_get_low_freq_channel_count(int amixdown)
{
    switch (amixdown)
    {
        case HB_AMIXDOWN_5POINT1:
        case HB_AMIXDOWN_6POINT1:
        case HB_AMIXDOWN_7POINT1:
        case HB_AMIXDOWN_5_2_LFE:
            return 1;

        default:
            return 0;
    }
}

int hb_mixdown_get_mixdown_from_short_name(const char *short_name)
{
    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (!strcmp(hb_audio_mixdowns[i].short_name, short_name))
        {
            return hb_audio_mixdowns[i].amixdown;
        }
    }
    return 0;
}

const char* hb_mixdown_get_short_name_from_mixdown(int amixdown)
{
    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (hb_audio_mixdowns[i].amixdown == amixdown)
        {
            return hb_audio_mixdowns[i].short_name;
        }
    }
    return "";
}

void hb_autopassthru_apply_settings( hb_job_t * job )
{
    int i, j, already_printed;
    hb_audio_t * audio;
    for( i = 0, already_printed = 0; i < hb_list_count( job->list_audio ); )
    {
        audio = hb_list_item( job->list_audio, i );
        if( audio->config.out.codec == HB_ACODEC_AUTO_PASS )
        {
            if( !already_printed )
                hb_autopassthru_print_settings( job );
            already_printed = 1;
            audio->config.out.codec = hb_autopassthru_get_encoder( audio->config.in.codec,
                                                                   job->acodec_copy_mask,
                                                                   job->acodec_fallback,
                                                                   job->mux );
            if( !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) &&
                !( audio->config.out.codec & HB_ACODEC_MASK ) )
            {
                hb_log( "Auto Passthru: passthru not possible and no valid fallback specified, dropping track %d",
                        audio->config.out.track );
                hb_list_rem( job->list_audio, audio );
                hb_audio_close( &audio );
                continue;
            }
            audio->config.out.samplerate = audio->config.in.samplerate;
            if( !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
            {
                if( audio->config.out.codec == job->acodec_fallback )
                {
                    hb_log( "Auto Passthru: passthru not possible for track %d, using fallback",
                            audio->config.out.track );
                }
                else
                {
                    hb_log( "Auto Passthru: passthru and fallback not possible for track %d, using default encoder",
                            audio->config.out.track );
                }
                audio->config.out.mixdown = hb_get_default_mixdown( audio->config.out.codec,
                                                                    audio->config.in.channel_layout );
                audio->config.out.bitrate = hb_get_default_audio_bitrate( audio->config.out.codec,
                                                                          audio->config.out.samplerate,
                                                                          audio->config.out.mixdown );
                audio->config.out.compression_level = hb_get_default_audio_compression( audio->config.out.codec );
            }
            else
            {
                for( j = 0; j < hb_audio_encoders_count; j++ )
                {
                    if( hb_audio_encoders[j].encoder == audio->config.out.codec )
                    {
                        hb_log( "Auto Passthru: using %s for track %d",
                                hb_audio_encoders[j].human_readable_name,
                                audio->config.out.track );
                        break;
                    }
                }
            }
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         * Note: out.track starts at 1, i starts at 0 */
        audio->config.out.track = ++i;
    }
}

void hb_autopassthru_print_settings( hb_job_t * job )
{
    int i, codec_len;
    char *mask = NULL, *tmp;
    const char *fallback = NULL;
    for( i = 0; i < hb_audio_encoders_count; i++ )
    {
        if( ( hb_audio_encoders[i].encoder & HB_ACODEC_PASS_FLAG ) &&
            ( hb_audio_encoders[i].encoder != HB_ACODEC_AUTO_PASS ) &&
            ( hb_audio_encoders[i].encoder & job->acodec_copy_mask ) )
        {
            if( mask )
            {
                tmp = hb_strncat_dup( mask, ", ", 2 );
                if( tmp )
                {
                    free( mask );
                    mask = tmp;
                }
            }
            // passthru name without " Passthru"
            codec_len = strlen( hb_audio_encoders[i].human_readable_name ) - 9;
            tmp = hb_strncat_dup( mask, hb_audio_encoders[i].human_readable_name, codec_len );
            if( tmp )
            {
                free( mask );
                mask = tmp;
            }
        }
        else if( !( hb_audio_encoders[i].encoder & HB_ACODEC_PASS_FLAG ) &&
                  ( hb_audio_encoders[i].encoder == job->acodec_fallback ) )
        {
            fallback = hb_audio_encoders[i].human_readable_name;
        }
    }
    if( !mask )
        hb_log( "Auto Passthru: no codecs allowed" );
    else
        hb_log( "Auto Passthru: allowed codecs are %s", mask );
    if( !fallback )
        hb_log( "Auto Passthru: no valid fallback specified" );
    else
        hb_log( "Auto Passthru: fallback is %s", fallback );
}

int hb_autopassthru_get_encoder( int in_codec, int copy_mask, int fallback, int muxer )
{
    int i;
    int out_codec = ( copy_mask & in_codec ) | HB_ACODEC_PASS_FLAG;
    // sanitize fallback encoder and selected passthru
    // note: invalid fallbacks are caught in hb_autopassthru_apply_settings
    for( i = 0; i < hb_audio_encoders_count; i++ )
    {
        if( ( hb_audio_encoders[i].encoder == fallback ) &&
           !( hb_audio_encoders[i].muxers & muxer ) )
        {
            // fallback not possible with current muxer
            // use the default audio encoder instead
            fallback = hb_get_default_audio_encoder(muxer);
            break;
        }
    }
    for( i = 0; i < hb_audio_encoders_count; i++ )
    {
        if( ( hb_audio_encoders[i].encoder == out_codec ) &&
           !( hb_audio_encoders[i].muxers & muxer ) )
        {
            // selected passthru not possible with current muxer
            out_codec = fallback;
            break;
        }
    }
    if( !( out_codec & HB_ACODEC_PASS_MASK ) )
        return fallback;
    return out_codec;
}

int hb_get_default_audio_encoder(int muxer)
{
#ifndef __APPLE__
    if (muxer == HB_MUX_MKV)
    {
        return HB_ACODEC_LAME;
    }
#endif
    return hb_audio_encoders[0].encoder;
}

// Given an input bitrate, find closest match in the set of allowed bitrates
int hb_find_closest_audio_bitrate(int bitrate)
{
    // Check if bitrate mode was disabled
    if (bitrate <= 0)
        return bitrate;

    int ii, result;
    // result is highest rate if none found during search.
    // rate returned will always be <= rate asked for.
    result = hb_audio_bitrates[0].rate;
    for (ii = hb_audio_bitrates_count - 1; ii > 0; ii--)
    {
        if (bitrate >= hb_audio_bitrates[ii].rate)
        {
            result = hb_audio_bitrates[ii].rate;
            break;
        }
    }
    return result;
}

/* Get the bitrate low and high limits for a codec/samplerate/mixdown triplet.

Encoder    1.0 channel    2.0 channels    5.1 channels    6.1 channels    7.1 channels
--------------------------------------------------------------------------------------

faac
----
supported samplerates: 8 - 48 kHz
libfaac/util.c defines the bitrate limits:
MinBitrate() -> 8000 bps (per channel, incl. LFE).
MaxBitrate() -> (6144 * samplerate / 1024) bps (per channel, incl. LFE).
But output bitrates don't go as high as the theoretical maximums:
12 kHz        43  (72)        87 (144)      260  (432)      303  (504)      342  (576)
24 kHz        87 (144)       174 (288)      514  (864)      595 (1008)      669 (1152)
48 kHz       174 (288)       347 (576)      970 (1728)     1138 (2016)     1287 (2304)
Also, faac isn't a great encoder, so you don't want to allow too low a bitrate.
Limits: minimum of  32 Kbps per channel
        maximum of 192 Kbps per channel at 32-48 kHz, adjusted for sr_shift


ffaac
-----
supported samplerates: 8 - 48 kHz
libavcodec/aacenc.c defines a maximum bitrate:
-> 6144 * samplerate / 1024 bps (per channel, incl. LFE).
But output bitrates don't go as high as the theoretical maximums:
12 kHz        61  (72)       123 (144)
24 kHz       121 (144)       242 (288)
48 kHz       236 (288)       472 (576)
Also, ffaac isn't a great encoder, so you don't want to allow too low a bitrate.
Limits: minimum of  32 Kbps per channel
        maximum of 192 Kbps per channel at 32 kHz, adjusted for sr_shift
        maximum of 256 Kbps per channel at 44.1-48 kHz, adjusted for sr_shift

vorbis
------
supported samplerates: 8 - 48 kHz
lib/modes/setup_*.h provides a range of allowed bitrates for various configurations.
for each samplerate, the highest minimums and lowest maximums are:
 8 kHz        Minimum  8 Kbps, maximum  32 Kbps (per channel, incl. LFE).
12 kHz        Minimum 14 Kbps, maximum  44 Kbps (per channel, incl. LFE).
16 kHz        Minimum 16 Kbps, maximum  86 Kbps (per channel, incl. LFE).
24 kHz        Minimum 22 Kbps, maximum  86 Kbps (per channel, incl. LFE).
32 kHz        Minimum 26 Kbps, maximum 190 Kbps (per channel, incl. LFE).
48 kHz        Minimum 28 Kbps, maximum 240 Kbps (per channel, incl. LFE).
Limits: minimum of 14/22/28 Kbps per channel (8-12, 16-24, 32-48 kHz)
        maximum of 32/86/190/240 Kbps per channel (8-12, 16-24, 32, 44.1-48 kHz)

lame
----
supported samplerates: 8 - 48 kHz
lame_init_params() allows the following bitrates:
12 kHz        Minimum  8 Kbps, maximum  64 Kbps
24 kHz        Minimum  8 Kbps, maximum 160 Kbps
48 kHz        Minimum 32 Kbps, maximum 320 Kbps
Limits: minimum of 8/8/32 Kbps (8-12, 16-24, 32-48 kHz)
        maximum of 64/160/320 Kbps (8-12, 16-24, 32-48 kHz)

ffac3
-----
supported samplerates: 32 - 48 kHz (< 32 kHz disabled for compatibility reasons)
Dolby's encoder has a min. of 224 Kbps for 5 full-bandwidth channels (5.0, 5.1)
The maximum AC3 bitrate is 640 Kbps
Limits: minimum of 224/5 Kbps per full-bandwidth channel, maximum of 640 Kbps

ca_aac
------
supported samplerates: 8 - 48 kHz
Core Audio API provides a range of allowed bitrates:
 8 kHz         8 -  24        16 -  48        40 - 112        48 - 144        56 - 160
12 kHz        12 -  32        24 -  64        64 - 160        72 - 192        96 - 224
16 kHz        12 -  48        24 -  96        64 - 224        72 - 288        96 - 320
24 kHz        16 -  64        32 - 128        80 - 320        96 - 384       112 - 448
32 kHz        24 -  96        48 - 192       128 - 448       144 - 576       192 - 640
48 kHz        32 - 256        64 - 320       160 - 768       192 - 960       224 - 960
Limits:
 8 kHz -> minimum of  8 Kbps and maximum of  24 Kbps per full-bandwidth channel
12 kHz -> minimum of 12 Kbps and maximum of  32 Kbps per full-bandwidth channel
16 kHz -> minimum of 12 Kbps and maximum of  48 Kbps per full-bandwidth channel
24 kHz -> minimum of 16 Kbps and maximum of  64 Kbps per full-bandwidth channel
32 kHz -> minimum of 24 Kbps and maximum of  96 Kbps per full-bandwidth channel
48 kHz -> minimum of 32 Kbps and maximum of 160 Kbps per full-bandwidth channel
48 kHz ->                        maximum of +96 Kbps for Mono
Note: encCoreAudioInit() will sanitize any mistake made here.

ca_haac
-------
supported samplerates: 32 - 48 kHz
Core Audio API provides a range of allowed bitrates:
32 kHz         12 - 40         24 - 80        64 - 192          N/A           96 - 256
48 kHz         16 - 40         32 - 80        80 - 192          N/A          112 - 256
Limits: minimum of 12 Kbps per full-bandwidth channel (<= 32 kHz)
        minimum of 16 Kbps per full-bandwidth channel ( > 32 kHz)
        maximum of 40 Kbps per full-bandwidth channel
Note: encCoreAudioInit() will sanitize any mistake made here.

fdk_aac
-------
supported samplerates: 8 - 48 kHz
libfdk limits the bitrate to the following values:
 8 kHz              48              96             240
12 kHz              72             144             360
16 kHz              96             192             480
24 kHz             144             288             720
32 kHz             192             384             960
48 kHz             288             576            1440
Limits: minimum of samplerate * 2/3 Kbps per full-bandwidth channel (see ca_aac)
        maximum of samplerate * 6.0 Kbps per full-bandwidth channel
 
fdk_haac
--------
supported samplerates: 16 - 48 kHz
libfdk limits the bitrate to the following values:
16 kHz         8 -  48        16 -  96        45 - 199
24 kHz         8 -  63        16 - 127        45 - 266
32 kHz         8 -  63        16 - 127        45 - 266
48 kHz        12 -  63        16 - 127        50 - 266
Limits: minimum of 12 Kbps per full-bandwidth channel  (<= 32 kHz) (see ca_haac)
        minimum of 16 Kbps per full-bandwidth channel  ( > 32 kHz) (see ca_haac)
        maximum of 48,  96 or 192 Kbps (1.0, 2.0, 5.1) (<= 16 kHz)
        maximum of 64, 128 or 256 Kbps (1.0, 2.0, 5.1) ( > 16 kHz)
*/

void hb_get_audio_bitrate_limits(uint32_t codec, int samplerate, int mixdown,
                                 int *low, int *high)
{
    if (codec & HB_ACODEC_PASS_FLAG)
    {
        // Bitrates don't apply to passthrough audio, but may apply if we
        // fallback to an encoder when the source can't be passed through.
        *low = hb_audio_bitrates[0].rate;
        *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
        return;
    }

    /* samplerate, sr_shift */
    int sr_shift;
    samplerate = hb_get_best_samplerate(codec, samplerate, &sr_shift);

    /* LFE, full-bandwidth channels */
    int lfe_count, nchannels;
    lfe_count = hb_mixdown_get_low_freq_channel_count(mixdown);
    nchannels = hb_mixdown_get_discrete_channel_count(mixdown) - lfe_count;

    switch (codec)
    {
        // Bitrates don't apply to "lossless" audio
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            *low = *high = -1;
            return;

        case HB_ACODEC_AC3:
            *low  = 224 * nchannels / 5;
            *high = 640;
            break;

        case HB_ACODEC_CA_AAC:
        {
            switch (samplerate)
            {
                case 8000:
                    *low  = nchannels *  8;
                    *high = nchannels * 24;
                    break;
                case 11025:
                case 12000:
                    *low  = nchannels * 12;
                    *high = nchannels * 32;
                    break;
                case 16000:
                    *low  = nchannels * 12;
                    *high = nchannels * 48;
                    break;
                case 22050:
                case 24000:
                    *low  = nchannels * 16;
                    *high = nchannels * 64;
                    break;
                case 32000:
                    *low  = nchannels * 24;
                    *high = nchannels * 96;
                    break;
                case 44100:
                case 48000:
                default:
                    *low  = nchannels * 32;
                    *high = nchannels * (160 + (96 * (nchannels == 1)));
                    break;
            }
        } break;

        case HB_ACODEC_CA_HAAC:
            *low  = nchannels * (12 + (4 * (samplerate >= 44100)));
            *high = nchannels * 40;
            break;

        case HB_ACODEC_FDK_AAC:
            *low  = nchannels * samplerate * 2 / 3000;
            *high = nchannels * samplerate * 6 / 1000;
            break;

        case HB_ACODEC_FDK_HAAC:
            *low  = (nchannels * (12 + (4 * (samplerate >= 44100))));
            *high = (nchannels - (nchannels > 2)) * (48 +
                                                     (16 *
                                                      (samplerate >= 22050)));
            break;

        case HB_ACODEC_FAAC:
            *low  = (nchannels + lfe_count) * 32;
            *high = (nchannels + lfe_count) * (192 >> sr_shift);
            break;

        case HB_ACODEC_FFAAC:
            *low  = ((nchannels + lfe_count) * 32);
            *high = ((nchannels + lfe_count) *
                     ((192 + (64 * ((samplerate << sr_shift) >= 44100)))
                      >> sr_shift));
            break;

        case HB_ACODEC_LAME:
            *low  =  8 + (24 * (sr_shift < 1));
            *high = 64 + (96 * (sr_shift < 2)) + (160 * (sr_shift < 1));
            break;

        case HB_ACODEC_VORBIS:
            *low  = (nchannels + lfe_count) * (14 +
                                               (8 * (sr_shift < 2)) +
                                               (6 * (sr_shift < 1)));
            *high = (nchannels + lfe_count) * (32 +
                                               ( 54 * (sr_shift < 2)) +
                                               (104 * (sr_shift < 1)) +
                                               ( 50 * (samplerate >= 44100)));
            break;

        default:
            *low  = hb_audio_bitrates[0].rate;
            *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
            break;
    }
    // sanitize max. bitrate
    if (*high < hb_audio_bitrates[0].rate)
        *high = hb_audio_bitrates[0].rate;
    if (*high > hb_audio_bitrates[hb_audio_bitrates_count-1].rate)
        *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
}

// Given an input bitrate, sanitize it.
// Check low and high limits and make sure it is in the set of allowed bitrates.
int hb_get_best_audio_bitrate(uint32_t codec, int bitrate, int samplerate,
                              int mixdown)
{
    int low, high;
    hb_get_audio_bitrate_limits(codec, samplerate, mixdown, &low, &high);
    if (bitrate > high)
        bitrate = high;
    if (bitrate < low)
        bitrate = low;
    return hb_find_closest_audio_bitrate(bitrate);
}

// Get the default bitrate for a given codec/samplerate/mixdown triplet.
int hb_get_default_audio_bitrate(uint32_t codec, int samplerate, int mixdown)
{
    if (codec & HB_ACODEC_PASS_FLAG)
    {
        return -1;
    }

    int bitrate, nchannels, sr_shift;
    /* full-bandwidth channels, sr_shift */
    nchannels = (hb_mixdown_get_discrete_channel_count(mixdown) -
                 hb_mixdown_get_low_freq_channel_count(mixdown));
    hb_get_best_samplerate(codec, samplerate, &sr_shift);

    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return -1;

        // 96, 224, 640 Kbps
        case HB_ACODEC_AC3:
            bitrate = (nchannels * 128) - (32 * (nchannels < 5));
            break;

        case HB_ACODEC_CA_HAAC:
        case HB_ACODEC_FDK_HAAC:
            bitrate = nchannels * 32;
            break;

        default:
            bitrate = nchannels * 80;
            break;
    }
    // sample_rate adjustment
    bitrate >>= sr_shift;
    return hb_get_best_audio_bitrate(codec, bitrate, samplerate, mixdown);
}

// Get limits and hints for the UIs.
//
// granularity sets the minimum step increments that should be used
// (it's ok to round up to some nice multiple if you like)
//
// direction says whether 'low' limit is highest or lowest 
// quality (direction 0 == lowest value is worst quality)
void hb_get_audio_quality_limits(uint32_t codec, float *low, float *high,
                                 float *granularity, int *direction)
{
    switch (codec)
    {
        case HB_ACODEC_LAME:
            *direction = 1;
            *granularity = 0.5;
            *low = 0.;
            *high = 10.0;
            break;

        case HB_ACODEC_VORBIS:
            *direction = 0;
            *granularity = 0.5;
            *low = -2.0;
            *high = 10.0;
            break;

        case HB_ACODEC_CA_AAC:
            *direction = 0;
            *granularity = 9;
            *low = 1.;
            *high = 127.0;
            break;

        default:
            *direction = 0;
            *granularity = 1;
            *low = *high = HB_INVALID_AUDIO_QUALITY;
            break;
    }
}

float hb_get_best_audio_quality(uint32_t codec, float quality)
{
    float low, high, granularity;
    int direction;
    hb_get_audio_quality_limits(codec, &low, &high, &granularity, &direction);
    if (quality > high)
        quality = high;
    if (quality < low)
        quality = low;
    return quality;
}

float hb_get_default_audio_quality( uint32_t codec )
{
    float quality;
    switch( codec )
    {
        case HB_ACODEC_LAME:
            quality = 2.;
            break;

        case HB_ACODEC_VORBIS:
            quality = 5.;
            break;

        case HB_ACODEC_CA_AAC:
            quality = 91.;
            break;

        default:
            quality = HB_INVALID_AUDIO_QUALITY;
            break;
    }
    return quality;
}

// Get limits and hints for the UIs.
//
// granularity sets the minimum step increments that should be used
// (it's ok to round up to some nice multiple if you like)
//
// direction says whether 'low' limit is highest or lowest 
// compression level (direction 0 == lowest value is worst compression level)
void hb_get_audio_compression_limits(uint32_t codec, float *low, float *high,
                                     float *granularity, int *direction)
{
    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            *direction = 0;
            *granularity = 1;
            *high = 12;
            *low = 0;
            break;

        case HB_ACODEC_LAME:
            *direction = 1;
            *granularity = 1;
            *high = 9;
            *low = 0;
            break;

        default:
            *direction = 0;
            *granularity = 1;
            *low = *high = -1;
            break;
    }
}

float hb_get_best_audio_compression(uint32_t codec, float compression)
{
    float low, high, granularity;
    int direction;
    hb_get_audio_compression_limits(codec, &low, &high, &granularity, &direction);
    if( compression > high )
        compression = high;
    if( compression < low )
        compression = low;
    return compression;
}

float hb_get_default_audio_compression(uint32_t codec)
{
    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return 5.;

        case HB_ACODEC_LAME:
            return 2.;

        default:
            return -1.;
    }
}

int hb_get_best_mixdown(uint32_t codec, uint64_t layout, int mixdown)
{
    // Passthru, only "None" mixdown is supported
    if (codec & HB_ACODEC_PASS_FLAG)
        return HB_AMIXDOWN_NONE;

    // caller requested the best available mixdown
    if (mixdown == HB_INVALID_AMIXDOWN)
        mixdown = hb_audio_mixdowns[hb_audio_mixdowns_count].amixdown;

    int ii;
    // test all mixdowns until an authorized, supported mixdown is found
    // stop before we reach the "worst" non-None mixdown (index == 1)
    for (ii = hb_audio_mixdowns_count; ii > 1; ii--)
        if (hb_audio_mixdowns[ii].amixdown <= mixdown &&
            hb_mixdown_is_supported(hb_audio_mixdowns[ii].amixdown, codec, layout))
            break;
    return hb_audio_mixdowns[ii].amixdown;
}

int hb_get_default_mixdown(uint32_t codec, uint64_t layout)
{
    int mixdown;
    switch (codec)
    {
        // the FLAC encoder defaults to the best mixdown up to 7.1
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            mixdown = HB_AMIXDOWN_7POINT1;
            break;
        // the AC3 encoder defaults to the best mixdown up to 5.1
        case HB_ACODEC_AC3:
            mixdown = HB_AMIXDOWN_5POINT1;
            break;
        // other encoders default to the best mixdown up to DPLII
        default:
            mixdown = HB_AMIXDOWN_DOLBYPLII;
            break;
    }
    // return the best available mixdown up to the selected default
    return hb_get_best_mixdown(codec, layout, mixdown);
}

int hb_get_best_samplerate(uint32_t codec, int samplerate, int *sr_shift)
{
    int ii, best_samplerate, samplerate_shift;
    if ((samplerate < 32000) &&
        (codec == HB_ACODEC_CA_HAAC || codec == HB_ACODEC_AC3))
    {
        // ca_haac can't do samplerates < 32 kHz
        // AC-3 < 32 kHz suffers from poor hardware compatibility
        best_samplerate  = 32000;
        samplerate_shift = 0;
    }
    else if (samplerate < 16000 && codec == HB_ACODEC_FDK_HAAC)
    {
        // fdk_haac can't do samplerates < 16 kHz
        best_samplerate  = 16000;
        samplerate_shift = 1;
    }
    else
    {
        best_samplerate = samplerate;
        for (ii = hb_audio_rates_count - 1; ii >= 0; ii--)
        {
            // valid samplerate
            if (best_samplerate == hb_audio_rates[ii].rate)
                break;

            // samplerate is higher than the next valid samplerate,
            // or lower than the lowest valid samplerate
            if (best_samplerate > hb_audio_rates[ii].rate || ii == 0)
            {
                best_samplerate = hb_audio_rates[ii].rate;
                break;
            }
        }
        /* sr_shift: 0 -> 48000, 44100, 32000 Hz
         *           1 -> 24000, 22050, 16000 Hz
         *           2 -> 12000, 11025,  8000 Hz
         *
         * also, since samplerates are sanitized downwards:
         *
         * (samplerate < 32000) implies (samplerate <= 24000)
         */
        samplerate_shift = ((best_samplerate < 16000) ? 2 :
                            (best_samplerate < 32000) ? 1 : 0);
    }
    if (sr_shift != NULL)
    {
        *sr_shift = samplerate_shift;
    }
    return best_samplerate;
}

/**********************************************************************
 * hb_reduce
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce( int *x, int *y, int num, int den )
{
    // find the greatest common divisor of num & den by Euclid's algorithm
    int n = num, d = den;
    while ( d )
    {
        int t = d;
        d = n % d;
        n = t;
    }

    // at this point n is the gcd. if it's non-zero remove it from num
    // and den. Otherwise just return the original values.
    if ( n )
    {
        *x = num / n;
        *y = den / n;
    }
    else
    {
        *x = num;
        *y = den;
    }
}

/**********************************************************************
 * hb_reduce64
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce64( int64_t *x, int64_t *y, int64_t num, int64_t den )
{
    // find the greatest common divisor of num & den by Euclid's algorithm
    int64_t n = num, d = den;
    while ( d )
    {
        int64_t t = d;
        d = n % d;
        n = t;
    }

    // at this point n is the gcd. if it's non-zero remove it from num
    // and den. Otherwise just return the original values.
    if ( n )
    {
        num /= n;
        den /= n;
    }

    *x = num;
    *y = den;

}

void hb_limit_rational64( int64_t *x, int64_t *y, int64_t num, int64_t den, int64_t limit )
{
    hb_reduce64( &num, &den, num, den );
    if ( num < limit && den < limit )
    {
        *x = num;
        *y = den;
        return;
    }

    if ( num > den )
    {
        double div = (double)limit / num;
        num = limit;
        den *= div;
    }
    else
    {
        double div = (double)limit / den;
        den = limit;
        num *= div;
    }
    *x = num;
    *y = den;
}

/**********************************************************************
 * hb_fix_aspect
 **********************************************************************
 * Given the output width (if HB_KEEP_WIDTH) or height
 * (HB_KEEP_HEIGHT) and the current crop values, calculates the
 * correct height or width in order to respect the DVD aspect ratio
 *********************************************************************/
void hb_fix_aspect( hb_job_t * job, int keep )
{
    hb_title_t * title = job->title;
    int          i;
    int  min_width;
    int min_height;
    int    modulus;

    /* don't do anything unless the title has complete size info */
    if ( title->height == 0 || title->width == 0 || title->aspect == 0 )
    {
        hb_log( "hb_fix_aspect: incomplete info for title %d: "
                "height = %d, width = %d, aspect = %.3f",
                title->index, title->height, title->width, title->aspect );
        return;
    }

    // min_width and min_height should be multiples of modulus
    min_width    = 32;
    min_height   = 32;
    modulus      = job->modulus ? job->modulus : 16;

    for( i = 0; i < 4; i++ )
    {
        // Sanity check crop values are zero or positive multiples of 2
        if( i < 2 )
        {
            // Top, bottom
            job->crop[i] = MIN( EVEN( job->crop[i] ), EVEN( ( title->height / 2 ) - ( min_height / 2 ) ) );
            job->crop[i] = MAX( 0, job->crop[i] );
        }
        else
        {
            // Left, right
            job->crop[i] = MIN( EVEN( job->crop[i] ), EVEN( ( title->width / 2 ) - ( min_width / 2 ) ) );
            job->crop[i] = MAX( 0, job->crop[i] );
        }
    }

    double par = (double)title->width / ( (double)title->height * title->aspect );
    double cropped_sar = (double)( title->height - job->crop[0] - job->crop[1] ) /
                         (double)( title->width - job->crop[2] - job->crop[3] );
    double ar = par * cropped_sar;

    // Dimensions must be greater than minimum and multiple of modulus
    if( keep == HB_KEEP_WIDTH )
    {
        job->width  = MULTIPLE_MOD( job->width, modulus );
        job->width  = MAX( min_width, job->width );
        job->height = MULTIPLE_MOD( (uint64_t)( (double)job->width * ar ), modulus );
        job->height = MAX( min_height, job->height );
    }
    else
    {
        job->height = MULTIPLE_MOD( job->height, modulus );
        job->height = MAX( min_height, job->height );
        job->width  = MULTIPLE_MOD( (uint64_t)( (double)job->height / ar ), modulus );
        job->width  = MAX( min_width, job->width );
    }
}

/**********************************************************************
 * hb_list implementation
 **********************************************************************
 * Basic and slow, but enough for what we need
 *********************************************************************/

#define HB_LIST_DEFAULT_SIZE 20

struct hb_list_s
{
    /* Pointers to items in the list */
    void ** items;

    /* How many (void *) allocated in 'items' */
    int     items_alloc;

    /* How many valid pointers in 'items' */
    int     items_count;
};

/**********************************************************************
 * hb_list_init
 **********************************************************************
 * Allocates an empty list ready for HB_LIST_DEFAULT_SIZE items
 *********************************************************************/
hb_list_t * hb_list_init()
{
    hb_list_t * l;

    l              = calloc( sizeof( hb_list_t ), 1 );
    l->items       = calloc( HB_LIST_DEFAULT_SIZE * sizeof( void * ), 1 );
    l->items_alloc = HB_LIST_DEFAULT_SIZE;

    return l;
}

/**********************************************************************
 * hb_list_count
 **********************************************************************
 * Returns the number of items currently in the list
 *********************************************************************/
int hb_list_count( const hb_list_t * l )
{
    return l->items_count;
}

/**********************************************************************
 * hb_list_add
 **********************************************************************
 * Adds an item at the end of the list, making it bigger if necessary.
 * Can safely be called with a NULL pointer to add, it will be ignored.
 *********************************************************************/
void hb_list_add( hb_list_t * l, void * p )
{
    if( !p )
    {
        return;
    }

    if( l->items_count == l->items_alloc )
    {
        /* We need a bigger boat */
        l->items_alloc += HB_LIST_DEFAULT_SIZE;
        l->items        = realloc( l->items,
                                   l->items_alloc * sizeof( void * ) );
    }

    l->items[l->items_count] = p;
    (l->items_count)++;
}

/**********************************************************************
 * hb_list_insert
 **********************************************************************
 * Adds an item at the specifiec position in the list, making it bigger
 * if necessary.
 * Can safely be called with a NULL pointer to add, it will be ignored.
 *********************************************************************/
void hb_list_insert( hb_list_t * l, int pos, void * p )
{
    if( !p )
    {
        return;
    }

    if( l->items_count == l->items_alloc )
    {
        /* We need a bigger boat */
        l->items_alloc += HB_LIST_DEFAULT_SIZE;
        l->items        = realloc( l->items,
                                   l->items_alloc * sizeof( void * ) );
    }

    if ( l->items_count != pos )
    {
        /* Shift all items after it sizeof( void * ) bytes earlier */
        memmove( &l->items[pos+1], &l->items[pos],
                 ( l->items_count - pos ) * sizeof( void * ) );
    }


    l->items[pos] = p;
    (l->items_count)++;
}

/**********************************************************************
 * hb_list_rem
 **********************************************************************
 * Remove an item from the list. Bad things will happen if called
 * with a NULL pointer or if the item is not in the list.
 *********************************************************************/
void hb_list_rem( hb_list_t * l, void * p )
{
    int i;

    /* Find the item in the list */
    for( i = 0; i < l->items_count; i++ )
    {
        if( l->items[i] == p )
        {
            /* Shift all items after it sizeof( void * ) bytes earlier */
            memmove( &l->items[i], &l->items[i+1],
                     ( l->items_count - i - 1 ) * sizeof( void * ) );

            (l->items_count)--;
            break;
        }
    }
}

/**********************************************************************
 * hb_list_item
 **********************************************************************
 * Returns item at position i, or NULL if there are not that many
 * items in the list
 *********************************************************************/
void * hb_list_item( const hb_list_t * l, int i )
{
    if( i < 0 || i >= l->items_count )
    {
        return NULL;
    }

    return l->items[i];
}

/**********************************************************************
 * hb_list_bytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, returns the total
 * number of bytes in the list
 *********************************************************************/
int hb_list_bytes( hb_list_t * l )
{
    hb_buffer_t * buf;
    int           ret;
    int           i;

    ret = 0;
    for( i = 0; i < hb_list_count( l ); i++ )
    {
        buf  = hb_list_item( l, i );
        ret += buf->size - buf->offset;
    }

    return ret;
}

/**********************************************************************
 * hb_list_seebytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>, keeping the list unmodified.
 *********************************************************************/
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    int           i;

    for( i = 0, copied = 0; copied < size; i++ )
    {
        buf     = hb_list_item( l, i );
        copying = MIN( buf->size - buf->offset, size - copied );
        memcpy( &dst[copied], &buf->data[buf->offset], copying );
        copied += copying;
    }
}

/**********************************************************************
 * hb_list_getbytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>. What's copied is removed from the list.
 * The variable pointed by <pts> is set to the PTS of the buffer the
 * first byte has been got from.
 * The variable pointed by <pos> is set to the position of that byte
 * in that buffer.
 *********************************************************************/
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, uint64_t * pos )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    uint8_t       has_pts;

    /* So we won't have to deal with NULL pointers */
     uint64_t dummy1, dummy2;

    if( !pts ) pts = &dummy1;
    if( !pos ) pos = &dummy2;

    for( copied = 0, has_pts = 0; copied < size;  )
    {
        buf     = hb_list_item( l, 0 );
        copying = MIN( buf->size - buf->offset, size - copied );
        memcpy( &dst[copied], &buf->data[buf->offset], copying );

        if( !has_pts )
        {
            *pts    = buf->s.start;
            *pos    = buf->offset;
            has_pts = 1;
        }

        buf->offset += copying;
        if( buf->offset >= buf->size )
        {
            hb_list_rem( l, buf );
            hb_buffer_close( &buf );
        }

        copied += copying;
    }
}

/**********************************************************************
 * hb_list_empty
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, close them all and
 * close the list.
 *********************************************************************/
void hb_list_empty( hb_list_t ** _l )
{
    hb_list_t * l = *_l;
    hb_buffer_t * b;

    while( ( b = hb_list_item( l, 0 ) ) )
    {
        hb_list_rem( l, b );
        hb_buffer_close( &b );
    }

    hb_list_close( _l );
}

/**********************************************************************
 * hb_list_close
 **********************************************************************
 * Free memory allocated by hb_list_init. Does NOT free contents of
 * items still in the list.
 *********************************************************************/
void hb_list_close( hb_list_t ** _l )
{
    hb_list_t * l = *_l;

    free( l->items );
    free( l );

    *_l = NULL;
}

int global_verbosity_level; //Necessary for hb_deep_log
/**********************************************************************
 * hb_valog
 **********************************************************************
 * If verbose mode is one, print message with timestamp. Messages
 * longer than 180 characters are stripped ;p
 *********************************************************************/
void hb_valog( hb_debug_level_t level, const char * prefix, const char * log, va_list args)
{
    char        string[362]; /* 360 chars + \n + \0 */
    time_t      _now;
    struct tm * now;

    if( !getenv( "HB_DEBUG" ) )
    {
        /* We don't want to print it */
        return;
    }

    if( global_verbosity_level < level )
    {
        /* Hiding message */
        return;
    }

    /* Get the time */
    _now = time( NULL );
    now  = localtime( &_now );
    if ( prefix && *prefix )
    {
        // limit the prefix length
        snprintf( string, 40, "[%02d:%02d:%02d] %s ",
                 now->tm_hour, now->tm_min, now->tm_sec, prefix );
    }
    else
    {
        sprintf( string, "[%02d:%02d:%02d] ",
                 now->tm_hour, now->tm_min, now->tm_sec );
    }
    int end = strlen( string );

    /* Convert the message to a string */
    vsnprintf( string + end, 361 - end, log, args );

    /* Add the end of line */
    strcat( string, "\n" );

    /* Print it */
    fprintf( stderr, "%s", string );
}

/**********************************************************************
 * hb_log
 **********************************************************************
 * If verbose mode is one, print message with timestamp. Messages
 * longer than 180 characters are stripped ;p
 *********************************************************************/
void hb_log( char * log, ... )
{
    va_list     args;

    va_start( args, log );
    hb_valog( 0, NULL, log, args );
    va_end( args );
}

/**********************************************************************
 * hb_deep_log
 **********************************************************************
 * If verbose mode is >= level, print message with timestamp. Messages
 * longer than 360 characters are stripped ;p
 *********************************************************************/
void hb_deep_log( hb_debug_level_t level, char * log, ... )
{
    va_list     args;

    va_start( args, log );
    hb_valog( level, NULL, log, args );
    va_end( args );
}

/**********************************************************************
 * hb_error
 **********************************************************************
 * Using whatever output is available display this error.
 *********************************************************************/
void hb_error( char * log, ... )
{
    char        string[181]; /* 180 chars + \0 */
    char        rep_string[181];
    static char last_string[181];
    static int  last_error_count = 0;
    static uint64_t last_series_error_time = 0;
    static hb_lock_t *mutex = 0;
    va_list     args;
    uint64_t time_now;

    /* Convert the message to a string */
    va_start( args, log );
    vsnprintf( string, 180, log, args );
    va_end( args );

    if( !mutex )
    {
        mutex = hb_lock_init();
    }

    hb_lock( mutex );

    time_now = hb_get_date();

    if( strcmp( string, last_string) == 0 )
    {
        /*
         * The last error and this one are the same, don't log it
         * just count it instead, unless it was more than one second
         * ago.
         */
        last_error_count++;
        if( last_series_error_time + ( 1000 * 1 ) > time_now )
        {
            hb_unlock( mutex );
            return;
        } 
    }
    
    /*
     * A new error, or the same one more than 10sec since the last one
     * did we have any of the same counted up?
     */
    if( last_error_count > 0 )
    {
        /*
         * Print out the last error to ensure context for the last 
         * repeated message.
         */
        if( error_handler )
        {
            error_handler( last_string );
        } else {
            hb_log( "%s", last_string );
        }
        
        if( last_error_count > 1 )
        {
            /*
             * Only print out the repeat message for more than 2 of the
             * same, since we just printed out two of them already.
             */
            snprintf( rep_string, 180, "Last error repeated %d times", 
                      last_error_count - 1 );
            
            if( error_handler )
            {
                error_handler( rep_string );
            } else {
                hb_log( "%s", rep_string );
            }
        }
        
        last_error_count = 0;
    }

    last_series_error_time = time_now;

    strcpy( last_string, string );

    /*
     * Got the error in a single string, send it off to be dispatched.
     */
    if( error_handler )
    {
        error_handler( string );
    } else {
        hb_log( "%s", string );
    }

    hb_unlock( mutex );
}

void hb_register_error_handler( hb_error_handler_t * handler )
{
    error_handler = handler;
}

static void hb_update_str( char **dst, const char *src )
{
    if ( dst )
    {
        free( *dst );
        *dst = NULL;
        if ( src )
        {
            *dst = strdup( src );
        }
    }
}

/**********************************************************************
 * hb_title_init
 **********************************************************************
 *
 *********************************************************************/
hb_title_t * hb_title_init( char * path, int index )
{
    hb_title_t * t;

    t = calloc( sizeof( hb_title_t ), 1 );

    t->index         = index;
    t->playlist      = -1;
    t->list_audio    = hb_list_init();
    t->list_chapter  = hb_list_init();
    t->list_subtitle = hb_list_init();
    t->list_attachment = hb_list_init();
    t->metadata      = hb_metadata_init();
    strcat( t->path, path );
    // default to decoding mpeg2
    t->video_id      = 0xE0;
    t->video_codec   = WORK_DECMPEG2;
    t->angle_count   = 1;
    t->pixel_aspect_width = 1;
    t->pixel_aspect_height = 1;

    return t;
}

/**********************************************************************
 * hb_title_close
 **********************************************************************
 *
 *********************************************************************/
void hb_title_close( hb_title_t ** _t )
{
    hb_title_t * t = *_t;
    hb_audio_t * audio;
    hb_chapter_t * chapter;
    hb_subtitle_t * subtitle;
    hb_attachment_t * attachment;

    while( ( chapter = hb_list_item( t->list_chapter, 0 ) ) )
    {
        hb_list_rem( t->list_chapter, chapter );
        hb_chapter_close( &chapter );
    }
    hb_list_close( &t->list_chapter );

    while( ( audio = hb_list_item( t->list_audio, 0 ) ) )
    {
        hb_list_rem( t->list_audio, audio );
        hb_audio_close( &audio );
    }
    hb_list_close( &t->list_audio );

    while( ( subtitle = hb_list_item( t->list_subtitle, 0 ) ) )
    {
        hb_list_rem( t->list_subtitle, subtitle );
        hb_subtitle_close( &subtitle );
    }
    hb_list_close( &t->list_subtitle );
    
    while( ( attachment = hb_list_item( t->list_attachment, 0 ) ) )
    {
        hb_list_rem( t->list_attachment, attachment );
        hb_attachment_close( &attachment );
    }
    hb_list_close( &t->list_attachment );

    hb_metadata_close( &t->metadata );

    free( t->video_codec_name );
    free(t->container_name);

#if defined(HB_TITLE_JOBS)
    hb_job_close( &t->job );
#endif

    free( t );
    *_t = NULL;
}

// The mac ui expects certain fields of the job struct to be cleaned up
// and others to remain untouched.
// e.g. picture settings like cropping, width, height, should remain untouched.
//
// So only initialize job elements that we know get set up by prepareJob and
// prepareJobForPreview.
//
// This should all get resolved in some future mac ui refactoring.
static void job_reset_for_mac_ui( hb_job_t * job, hb_title_t * title )
{
    if ( job == NULL || title == NULL )
        return;

    job->title = title;

    /* Set defaults settings */
    job->chapter_start = 1;
    job->chapter_end   = hb_list_count( title->list_chapter );
    job->list_chapter = hb_chapter_list_copy( title->list_chapter );

    job->vcodec     = HB_VCODEC_FFMPEG_MPEG4;
    job->vquality   = -1.0;
    job->vbitrate   = 1000;
    job->pass       = 0;
    job->vrate      = title->rate;
    job->vrate_base = title->rate_base;

    job->list_audio = hb_list_init();
    job->list_subtitle = hb_list_init();
    job->list_filter = hb_list_init();

    job->list_attachment = hb_attachment_list_copy( title->list_attachment );
    job->metadata = hb_metadata_copy( title->metadata );
}


static void job_setup( hb_job_t * job, hb_title_t * title )
{
    if ( job == NULL || title == NULL )
        return;

    job->title = title;

    /* Set defaults settings */
    job->chapter_start = 1;
    job->chapter_end   = hb_list_count( title->list_chapter );
    job->list_chapter = hb_chapter_list_copy( title->list_chapter );

    /* Autocrop by default. Gnark gnark */
    memcpy( job->crop, title->crop, 4 * sizeof( int ) );

    /* Preserve a source's pixel aspect, if it's available. */
    if( title->pixel_aspect_width && title->pixel_aspect_height )
    {
        job->anamorphic.par_width  = title->pixel_aspect_width;
        job->anamorphic.par_height = title->pixel_aspect_height;
    }

    if( title->aspect != 0 && title->aspect != 1. &&
        !job->anamorphic.par_width && !job->anamorphic.par_height)
    {
        hb_reduce( &job->anamorphic.par_width, &job->anamorphic.par_height,
                   (int)(title->aspect * title->height + 0.5), title->width );
    }

    job->width = title->width - job->crop[2] - job->crop[3];
    hb_fix_aspect( job, HB_KEEP_WIDTH );
    if( job->height > title->height - job->crop[0] - job->crop[1] )
    {
        job->height = title->height - job->crop[0] - job->crop[1];
        hb_fix_aspect( job, HB_KEEP_HEIGHT );
    }

    job->keep_ratio = 1;

    job->vcodec     = HB_VCODEC_FFMPEG_MPEG4;
    job->vquality   = -1.0;
    job->vbitrate   = 1000;
    job->pass       = 0;
    job->vrate      = title->rate;
    job->vrate_base = title->rate_base;

    job->mux = HB_MUX_MP4;

    job->list_audio = hb_list_init();
    job->list_subtitle = hb_list_init();
    job->list_filter = hb_list_init();

    job->list_attachment = hb_attachment_list_copy( title->list_attachment );
    job->metadata = hb_metadata_copy( title->metadata );
}

static void job_clean( hb_job_t * job )
{
    if (job)
    {
        hb_chapter_t *chapter;
        hb_audio_t *audio;
        hb_subtitle_t *subtitle;
        hb_filter_object_t *filter;
        hb_attachment_t *attachment;

        free(job->file);
        job->file = NULL;
        free(job->advanced_opts);
        job->advanced_opts = NULL;
        free(job->x264_preset);
        job->x264_preset = NULL;
        free(job->x264_tune);
        job->x264_tune = NULL;
        free(job->h264_profile);
        job->h264_profile = NULL;
        free(job->h264_level);
        job->h264_level = NULL;

        // clean up chapter list
        while( ( chapter = hb_list_item( job->list_chapter, 0 ) ) )
        {
            hb_list_rem( job->list_chapter, chapter );
            hb_chapter_close( &chapter );
        }
        hb_list_close( &job->list_chapter );

        // clean up audio list
        while( ( audio = hb_list_item( job->list_audio, 0 ) ) )
        {
            hb_list_rem( job->list_audio, audio );
            hb_audio_close( &audio );
        }
        hb_list_close( &job->list_audio );

        // clean up subtitle list
        while( ( subtitle = hb_list_item( job->list_subtitle, 0 ) ) )
        {
            hb_list_rem( job->list_subtitle, subtitle );
            hb_subtitle_close( &subtitle );
        }
        hb_list_close( &job->list_subtitle );

        // clean up filter list
        while( ( filter = hb_list_item( job->list_filter, 0 ) ) )
        {
            hb_list_rem( job->list_filter, filter );
            hb_filter_close( &filter );
        }
        hb_list_close( &job->list_filter );

        // clean up attachment list
        while( ( attachment = hb_list_item( job->list_attachment, 0 ) ) )
        {
            hb_list_rem( job->list_attachment, attachment );
            hb_attachment_close( &attachment );
        }
        hb_list_close( &job->list_attachment );

        // clean up metadata
        hb_metadata_close( &job->metadata );
    }
}

/*
 * Create a pristine job structure from a title
 * title_index is 1 based
 */
hb_job_t * hb_job_init_by_index( hb_handle_t * h, int title_index )
{
    hb_title_set_t *title_set = hb_get_title_set( h );
    hb_title_t * title = hb_list_item( title_set->list_title,
                                       title_index - 1 );
    return hb_job_init( title );
}

hb_job_t * hb_job_init( hb_title_t * title )
{
    hb_job_t * job;

    if ( title == NULL )
        return NULL;

    job = calloc( sizeof( hb_job_t ), 1 );
    job_setup(job, title);

    return job;
}

/**
 * Clean up the job structure so that is is ready for setting up a new job.
 * Should be called by front-ends after hb_add().
 */
/**********************************************************************
 * hb_job_reset
 **********************************************************************
 *
 *********************************************************************/
void hb_job_reset( hb_job_t * job )
{
    if ( job )
    {
        hb_title_t * title = job->title;
        job_clean(job);
        job_reset_for_mac_ui(job, title);
    }
}

/**********************************************************************
 * hb_job_close
 **********************************************************************
 *
 *********************************************************************/
void hb_job_close( hb_job_t ** _j )
{
    if (_j && *_j)
    {
        job_clean(*_j);
        free( *_j );
        _j = NULL;
    }
}

void hb_job_set_file( hb_job_t *job, const char *file )
{
    if ( job )
    {
        hb_update_str( &job->file, file );
    }
}

void hb_job_set_advanced_opts( hb_job_t *job, const char *advanced_opts )
{
    if ( job )
    {
        hb_update_str( &job->advanced_opts, advanced_opts );
    }
}

void hb_job_set_x264_preset( hb_job_t *job, const char *preset )
{
    if ( job )
    {
        hb_update_str( &job->x264_preset, preset );
    }
}

void hb_job_set_x264_tune( hb_job_t *job, const char *tune )
{
    if ( job )
    {
        hb_update_str( &job->x264_tune, tune );
    }
}

void hb_job_set_h264_profile( hb_job_t *job, const char *profile )
{
    if ( job )
    {
        hb_update_str( &job->h264_profile, profile );
    }
}

void hb_job_set_h264_level( hb_job_t *job, const char *level )
{
    if ( job )
    {
        hb_update_str( &job->h264_level, level );
    }
}

hb_filter_object_t * hb_filter_copy( hb_filter_object_t * filter )
{
    if( filter == NULL )
        return NULL;

    hb_filter_object_t * filter_copy = malloc( sizeof( hb_filter_object_t ) );
    memcpy( filter_copy, filter, sizeof( hb_filter_object_t ) );
    if( filter->settings )
        filter_copy->settings = strdup( filter->settings );
    return filter_copy;
}

/**********************************************************************
 * hb_filter_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_filter_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_filter_object_t *filter = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( filter = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_filter_copy(filter) );
            }
        }
    }
    return list;
}

/**
 * Gets a filter object with the given type
 * @param filter_id The type of filter to get.
 * @returns The requested filter object.
 */
hb_filter_object_t * hb_filter_init( int filter_id )
{
    hb_filter_object_t * filter;
    switch( filter_id )
    {
        case HB_FILTER_DETELECINE:
            filter = &hb_filter_detelecine;
            break;

        case HB_FILTER_DECOMB:
            filter = &hb_filter_decomb;
            break;

        case HB_FILTER_DEINTERLACE:
            filter = &hb_filter_deinterlace;
            break;

        case HB_FILTER_VFR:
            filter = &hb_filter_vfr;
            break;

        case HB_FILTER_DEBLOCK:
            filter = &hb_filter_deblock;
            break;

        case HB_FILTER_DENOISE:
            filter = &hb_filter_denoise;
            break;

        case HB_FILTER_RENDER_SUB:
            filter = &hb_filter_render_sub;
            break;

        case HB_FILTER_CROP_SCALE:
            filter = &hb_filter_crop_scale;
            break;

        case HB_FILTER_ROTATE:
            filter = &hb_filter_rotate;
            break;

        default:
            filter = NULL;
            break;
    }
    return hb_filter_copy( filter );
}

/**********************************************************************
 * hb_filter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_filter_close( hb_filter_object_t ** _f )
{
    hb_filter_object_t * f = *_f;

    if( f->settings )
        free( f->settings );

    free( f );
    *_f = NULL;
}

/**********************************************************************
 * hb_chapter_copy
 **********************************************************************
 *
 *********************************************************************/
hb_chapter_t *hb_chapter_copy(const hb_chapter_t *src)
{
    hb_chapter_t *chap = NULL;

    if ( src )
    {
        chap = calloc( 1, sizeof(*chap) );
        memcpy( chap, src, sizeof(*chap) );
        if ( src->title )
        {
            chap->title = strdup( src->title );
        }
    }
    return chap;
}

/**********************************************************************
 * hb_chapter_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_chapter_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_chapter_t *chapter = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( chapter = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_chapter_copy(chapter) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_chapter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_chapter_close(hb_chapter_t **chap)
{
    if ( chap && *chap )
    {
        free((*chap)->title);
        free(*chap);
        *chap = NULL;
    }
}

/**********************************************************************
 * hb_chapter_set_title
 **********************************************************************
 *
 *********************************************************************/
void hb_chapter_set_title( hb_chapter_t *chapter, const char *title )
{
    if ( chapter )
    {
        hb_update_str( &chapter->title, title );
    }
}

/**********************************************************************
 * hb_chapter_set_title_by_index
 **********************************************************************
 * Applies information from the given job to the official job instance.
 * @param job Handle to hb_job_t.
 * @param chapter The chapter to apply the name to (1-based).
 * @param titel to apply.
 *********************************************************************/
void hb_chapter_set_title_by_index( hb_job_t * job, int chapter_index, const char * title )
{
    hb_chapter_t * chapter = hb_list_item( job->list_chapter, chapter_index - 1 );
    hb_chapter_set_title( chapter, title );
}

/**********************************************************************
 * hb_audio_copy
 **********************************************************************
 *
 *********************************************************************/
hb_audio_t *hb_audio_copy(const hb_audio_t *src)
{
    hb_audio_t *audio = NULL;

    if( src )
    {
        audio = calloc(1, sizeof(*audio));
        memcpy(audio, src, sizeof(*audio));
        if ( src->config.out.name )
        {
            audio->config.out.name = strdup(src->config.out.name);
        }
    }
    return audio;
}

/**********************************************************************
 * hb_audio_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_audio_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_audio_t *audio = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( audio = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_audio_copy(audio) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_audio_close
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_close( hb_audio_t **audio )
{
    if ( audio && *audio )
    {
        free((*audio)->config.out.name);
        free(*audio);
        *audio = NULL;
    }
}

/**********************************************************************
 * hb_audio_new
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_config_init(hb_audio_config_t * audiocfg)
{
    /* Set read-only paramaters to invalid values */
    audiocfg->in.codec = 0;
    audiocfg->in.codec_param = 0;
    audiocfg->in.reg_desc = 0;
    audiocfg->in.stream_type = 0;
    audiocfg->in.substream_type = 0;
    audiocfg->in.version = 0;
    audiocfg->in.flags = 0;
    audiocfg->in.mode = 0;
    audiocfg->in.samplerate = -1;
    audiocfg->in.samples_per_frame = -1;
    audiocfg->in.bitrate = -1;
    audiocfg->in.channel_layout = -1;
    audiocfg->in.channel_map = NULL;
    audiocfg->lang.description[0] = 0;
    audiocfg->lang.simple[0] = 0;
    audiocfg->lang.iso639_2[0] = 0;

    /* Initalize some sensible defaults */
    audiocfg->in.track = audiocfg->out.track = 0;
    audiocfg->out.codec = hb_audio_encoders[0].encoder;
    audiocfg->out.samplerate = -1;
    audiocfg->out.samples_per_frame = -1;
    audiocfg->out.bitrate = -1;
    audiocfg->out.quality = HB_INVALID_AUDIO_QUALITY;
    audiocfg->out.compression_level = -1;
    audiocfg->out.mixdown = HB_INVALID_AMIXDOWN;
    audiocfg->out.dynamic_range_compression = 0;
    audiocfg->out.gain = 0;
    audiocfg->out.normalize_mix_level = 0;
    audiocfg->out.dither_method = hb_audio_dither_get_default();
    audiocfg->out.name = NULL;
}

/**********************************************************************
 * hb_audio_add
 **********************************************************************
 *
 *********************************************************************/
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg)
{
    hb_title_t *title = job->title;
    hb_audio_t *audio;

    audio = hb_audio_copy( hb_list_item( title->list_audio, audiocfg->in.track ) );
    if( audio == NULL )
    {
        /* We fail! */
        return 0;
    }

    if( (audiocfg->in.bitrate != -1) && (audiocfg->in.codec != 0xDEADBEEF) )
    {
        /* This most likely means the client didn't call hb_audio_config_init
         * so bail. */
        return 0;
    }

    /* Set the job's "in track" to the value passed in audiocfg.
     * HandBrakeCLI assumes this value is preserved in the jobs
     * audio list, but in.track in the title's audio list is not 
     * required to be the same. */
    audio->config.in.track = audiocfg->in.track;

    /* Really shouldn't ignore the passed out track, but there is currently no
     * way to handle duplicates or out-of-order track numbers. */
    audio->config.out.track = hb_list_count(job->list_audio) + 1;
    audio->config.out.codec = audiocfg->out.codec;
    if((audiocfg->out.codec & HB_ACODEC_PASS_FLAG) &&
       ((audiocfg->out.codec == HB_ACODEC_AUTO_PASS) ||
        (audiocfg->out.codec & audio->config.in.codec & HB_ACODEC_PASS_MASK)))
    {
        /* Pass-through, copy from input. */
        audio->config.out.samplerate = audio->config.in.samplerate;
        audio->config.out.bitrate = audio->config.in.bitrate;
        audio->config.out.mixdown = HB_AMIXDOWN_NONE;
        audio->config.out.dynamic_range_compression = 0;
        audio->config.out.gain = 0;
        audio->config.out.normalize_mix_level = 0;
        audio->config.out.compression_level = -1;
        audio->config.out.quality = HB_INVALID_AUDIO_QUALITY;
        audio->config.out.dither_method = AV_RESAMPLE_DITHER_NONE;
    }
    else
    {
        /* Non pass-through, use what is given. */
        audio->config.out.codec &= ~HB_ACODEC_PASS_FLAG;
        audio->config.out.samplerate = audiocfg->out.samplerate;
        audio->config.out.bitrate = audiocfg->out.bitrate;
        audio->config.out.compression_level = audiocfg->out.compression_level;
        audio->config.out.quality = audiocfg->out.quality;
        audio->config.out.dynamic_range_compression = audiocfg->out.dynamic_range_compression;
        audio->config.out.mixdown = audiocfg->out.mixdown;
        audio->config.out.gain = audiocfg->out.gain;
        audio->config.out.normalize_mix_level = audiocfg->out.normalize_mix_level;
        audio->config.out.dither_method = audiocfg->out.dither_method;
    }
    if (audiocfg->out.name && *audiocfg->out.name)
    {
        audio->config.out.name = strdup(audiocfg->out.name);
    }

    hb_list_add(job->list_audio, audio);
    return 1;
}

hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i)
{
    hb_audio_t *audio = NULL;

    if( (audio = hb_list_item(list, i)) )
        return &(audio->config);

    return NULL;
}

/**********************************************************************
 * hb_subtitle_copy
 **********************************************************************
 *
 *********************************************************************/
hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src)
{
    hb_subtitle_t *subtitle = NULL;

    if( src )
    {
        subtitle = calloc(1, sizeof(*subtitle));
        memcpy(subtitle, src, sizeof(*subtitle));
        if ( src->extradata )
        {
            subtitle->extradata = malloc( src->extradata_size );
            memcpy( subtitle->extradata, src->extradata, src->extradata_size );
        }
    }
    return subtitle;
}

/**********************************************************************
 * hb_subtitle_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_subtitle_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_subtitle_t *subtitle = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( subtitle = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_subtitle_copy(subtitle) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_subtitle_close
 **********************************************************************
 *
 *********************************************************************/
void hb_subtitle_close( hb_subtitle_t **sub )
{
    if ( sub && *sub )
    {
        free ((*sub)->extradata);
        free(*sub);
        *sub = NULL;
    }
}

/**********************************************************************
 * hb_subtitle_add
 **********************************************************************
 *
 *********************************************************************/
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track)
{
    hb_title_t *title = job->title;
    hb_subtitle_t *subtitle;

    subtitle = hb_subtitle_copy( hb_list_item( title->list_subtitle, track ) );
    if( subtitle == NULL )
    {
        /* We fail! */
        return 0;
    }
    subtitle->config = *subtitlecfg;
    subtitle->out_track = hb_list_count(job->list_subtitle) + 1;
    hb_list_add(job->list_subtitle, subtitle);
    return 1;
}

int hb_srt_add( const hb_job_t * job, 
                const hb_subtitle_config_t * subtitlecfg, 
                const char *lang )
{
    hb_subtitle_t *subtitle;
    iso639_lang_t *language = NULL;
    int retval = 0;

    subtitle = calloc( 1, sizeof( *subtitle ) );
    
    subtitle->id = (hb_list_count(job->list_subtitle) << 8) | 0xFF;
    subtitle->format = TEXTSUB;
    subtitle->source = SRTSUB;
    subtitle->codec = WORK_DECSRTSUB;

    language = lang_for_code2( lang );

    if( language )
    {

        strcpy( subtitle->lang, language->eng_name );
        strncpy( subtitle->iso639_2, lang, 4 );
        
        subtitle->config = *subtitlecfg;
        subtitle->config.dest = PASSTHRUSUB;

        hb_list_add(job->list_subtitle, subtitle);
        retval = 1;
    }
    return retval;
}

int hb_subtitle_can_force( int source )
{
    return source == VOBSUB || source == PGSSUB;
}

int hb_subtitle_can_burn( int source )
{
    return source == VOBSUB || source == PGSSUB || source == SSASUB;
}

int hb_subtitle_can_pass( int source, int mux )
{
    if ( mux == HB_MUX_MKV )
    {
        switch( source )
        {
            case PGSSUB:
            case VOBSUB:
            case SSASUB:
            case SRTSUB:
            case UTF8SUB:
            case TX3GSUB:
            case CC608SUB:
            case CC708SUB:
                return 1;

            default:
                return 0;
        }
    }
    else if ( mux == HB_MUX_MP4 )
    {
        switch( source )
        {
            case VOBSUB:
            case SSASUB:
            case SRTSUB:
            case UTF8SUB:
            case TX3GSUB:
            case CC608SUB:
            case CC708SUB:
                return 1;

            default:
                return 0;
        }
    }
    else
    {
        // Internal error. Should never get here.
        hb_error("internel error.  Bad mux %d\n", mux);
        return 0;
    }
}

/**********************************************************************
 * hb_metadata_init
 **********************************************************************
 *
 *********************************************************************/
hb_metadata_t *hb_metadata_init()
{
    hb_metadata_t *metadata = calloc( 1, sizeof(*metadata) );
    return metadata;
}

/**********************************************************************
 * hb_metadata_copy
 **********************************************************************
 *
 *********************************************************************/
hb_metadata_t *hb_metadata_copy( const hb_metadata_t *src )
{
    hb_metadata_t *metadata = NULL;

    if ( src )
    {
        metadata = calloc( 1, sizeof(*metadata) );
        if ( src->name )
        {
            metadata->name = strdup(src->name);
        }
        if ( src->artist )
        {
            metadata->artist = strdup(src->artist);
        }
        if ( src->album_artist )
        {
            metadata->album_artist = strdup(src->album_artist);
        }
        if ( src->composer )
        {
            metadata->composer = strdup(src->composer);
        }
        if ( src->release_date )
        {
            metadata->release_date = strdup(src->release_date);
        }
        if ( src->comment )
        {
            metadata->comment = strdup(src->comment);
        }
        if ( src->album )
        {
            metadata->album = strdup(src->album);
        }
        if ( src->genre )
        {
            metadata->genre = strdup(src->genre);
        }
        if ( src->description )
        {
            metadata->description = strdup(src->description);
        }
        if ( src->long_description )
        {
            metadata->long_description = strdup(src->long_description);
        }
        if ( src->list_coverart )
        {
            int ii;
            for ( ii = 0; ii < hb_list_count( src->list_coverart ); ii++ )
            {
                hb_coverart_t *art = hb_list_item( src->list_coverart, ii );
                hb_metadata_add_coverart(
                        metadata, art->data, art->size, art->type );
            }
        }
    }
    return metadata;
}

/**********************************************************************
 * hb_metadata_close
 **********************************************************************
 *
 *********************************************************************/
void hb_metadata_close( hb_metadata_t **_m )
{
    if ( _m && *_m )
    {
        hb_metadata_t *m = *_m;
        hb_coverart_t *art;

        free( m->name );
        free( m->artist );
        free( m->composer );
        free( m->release_date );
        free( m->comment );
        free( m->album );
        free( m->album_artist );
        free( m->genre );
        free( m->description );
        free( m->long_description );

        if ( m->list_coverart )
        {
            while( ( art = hb_list_item( m->list_coverart, 0 ) ) )
            {
                hb_list_rem( m->list_coverart, art );
                free( art->data );
                free( art );
            }
            hb_list_close( &m->list_coverart );
        }

        free( m );
        *_m = NULL;
    }
}

/**********************************************************************
 * hb_metadata_set_*
 **********************************************************************
 *
 *********************************************************************/
void hb_metadata_set_name( hb_metadata_t *metadata, const char *name )
{
    if ( metadata )
    {
        hb_update_str( &metadata->name, name );
    }
}

void hb_metadata_set_artist( hb_metadata_t *metadata, const char *artist )
{
    if ( metadata )
    {
        hb_update_str( &metadata->artist, artist );
    }
}

void hb_metadata_set_composer( hb_metadata_t *metadata, const char *composer )
{
    if ( metadata )
    {
        hb_update_str( &metadata->composer, composer );
    }
}

void hb_metadata_set_release_date( hb_metadata_t *metadata, const char *release_date )
{
    if ( metadata )
    {
        hb_update_str( &metadata->release_date, release_date );
    }
}

void hb_metadata_set_comment( hb_metadata_t *metadata, const char *comment )
{
    if ( metadata )
    {
        hb_update_str( &metadata->comment, comment );
    }
}

void hb_metadata_set_genre( hb_metadata_t *metadata, const char *genre )
{
    if ( metadata )
    {
        hb_update_str( &metadata->genre, genre );
    }
}

void hb_metadata_set_album( hb_metadata_t *metadata, const char *album )
{
    if ( metadata )
    {
        hb_update_str( &metadata->album, album );
    }
}

void hb_metadata_set_album_artist( hb_metadata_t *metadata, const char *album_artist )
{
    if ( metadata )
    {
        hb_update_str( &metadata->album_artist, album_artist );
    }
}

void hb_metadata_set_description( hb_metadata_t *metadata, const char *description )
{
    if ( metadata )
    {
        hb_update_str( &metadata->description, description );
    }
}

void hb_metadata_set_long_description( hb_metadata_t *metadata, const char *long_description )
{
    if ( metadata )
    {
        hb_update_str( &metadata->long_description, long_description );
    }
}

void hb_metadata_add_coverart( hb_metadata_t *metadata, const uint8_t *data, int size, int type )
{
    if ( metadata )
    {
        if ( metadata->list_coverart == NULL )
        {
            metadata->list_coverart = hb_list_init();
        }
        hb_coverart_t *art = calloc( 1, sizeof(hb_coverart_t) );
        art->data = malloc( size );
        memcpy( art->data, data, size );
        art->size = size;
        art->type = type;
        hb_list_add( metadata->list_coverart, art );
    }
}

void hb_metadata_rem_coverart( hb_metadata_t *metadata, int idx )
{
    if ( metadata )
    {
        hb_coverart_t *art = hb_list_item( metadata->list_coverart, idx );
        if ( art )
        {
            hb_list_rem( metadata->list_coverart, art );
            free( art->data );
            free( art );
        }
    }
}

char * hb_strdup_printf( const char * fmt, ... )
{
    int       len;
    va_list   ap;
    int       size = 256;
    char    * str;
    char    * tmp;

    str = malloc( size );
    if ( str == NULL )
        return NULL;

    while (1) 
    {
        /* Try to print in the allocated space. */
        va_start( ap, fmt );
        len = vsnprintf( str, size, fmt, ap );
        va_end( ap );

        /* If that worked, return the string. */
        if ( len > -1 && len < size )
        {
            return str;
        }

        /* Else try again with more space. */
        if ( len > -1 )     /* glibc 2.1 */
            size = len + 1; /* precisely what is needed */
        else                /* glibc 2.0 */
            size *= 2;      /* twice the old size */
        tmp = realloc( str, size );
        if ( tmp == NULL )
        {
            free( str );
            return NULL;
        }
        else
            str = tmp;
    }
}

char * hb_strncat_dup( const char * s1, const char * s2, size_t n )
{
    size_t len;
    char * str;

    len = 0;
    if( s1 )
        len += strlen( s1 );
    if( s2 )
        len += MAX( strlen( s2 ), n );
    if( !len )
        return NULL;

    str = malloc( len + 1 );
    if( !str )
        return NULL;

    if( s1 )
        strcpy( str, s1 );
    else
        strcpy( str, "" );
    strncat( str, s2, n );
    return str;
}

/**********************************************************************
 * hb_attachment_copy
 **********************************************************************
 *
 *********************************************************************/
hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src)
{
    hb_attachment_t *attachment = NULL;

    if( src )
    {
        attachment = calloc(1, sizeof(*attachment));
        memcpy(attachment, src, sizeof(*attachment));
        if ( src->name )
        {
            attachment->name = strdup( src->name );
        }
        if ( src->data )
        {
            attachment->data = malloc( src->size );
            memcpy( attachment->data, src->data, src->size );
        }
    }
    return attachment;
}

/**********************************************************************
 * hb_attachment_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_attachment_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_attachment_t *attachment = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( attachment = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_attachment_copy(attachment) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_attachment_close
 **********************************************************************
 *
 *********************************************************************/
void hb_attachment_close( hb_attachment_t **attachment )
{
    if ( attachment && *attachment )
    {
        free((*attachment)->data);
        free((*attachment)->name);
        free(*attachment);
        *attachment = NULL;
    }
}

/**********************************************************************
 * hb_yuv2rgb
 **********************************************************************
 * Converts a YCrCb pixel to an RGB pixel.
 * 
 * This conversion is lossy (due to rounding and clamping).
 * 
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_yuv2rgb(int yuv)
{
    double y, Cr, Cb;
    int r, g, b;

    y  = (yuv >> 16) & 0xff;
    Cr = (yuv >>  8) & 0xff;
    Cb = (yuv      ) & 0xff;

    r = 1.164 * (y - 16)                      + 1.596 * (Cr - 128);
    g = 1.164 * (y - 16) - 0.392 * (Cb - 128) - 0.813 * (Cr - 128);
    b = 1.164 * (y - 16) + 2.017 * (Cb - 128);
    
    r = (r < 0) ? 0 : r;
    g = (g < 0) ? 0 : g;
    b = (b < 0) ? 0 : b;
    
    r = (r > 255) ? 255 : r;
    g = (g > 255) ? 255 : g;
    b = (b > 255) ? 255 : b;
    
    return (r << 16) | (g << 8) | b;
}

/**********************************************************************
 * hb_rgb2yuv
 **********************************************************************
 * Converts an RGB pixel to a YCrCb pixel.
 * 
 * This conversion is lossy (due to rounding and clamping).
 * 
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_rgb2yuv(int rgb)
{
    double r, g, b;
    int y, Cr, Cb;
    
    r = (rgb >> 16) & 0xff;
    g = (rgb >>  8) & 0xff;
    b = (rgb      ) & 0xff;

    y  =  16. + ( 0.257 * r) + (0.504 * g) + (0.098 * b);
    Cb = 128. + (-0.148 * r) - (0.291 * g) + (0.439 * b);
    Cr = 128. + ( 0.439 * r) - (0.368 * g) - (0.071 * b);
    
    y = (y < 0) ? 0 : y;
    Cb = (Cb < 0) ? 0 : Cb;
    Cr = (Cr < 0) ? 0 : Cr;
    
    y = (y > 255) ? 255 : y;
    Cb = (Cb > 255) ? 255 : Cb;
    Cr = (Cr > 255) ? 255 : Cr;
    
    return (y << 16) | (Cr << 8) | Cb;
}

const char * hb_subsource_name( int source )
{
    switch (source)
    {
        case VOBSUB:
            return "VOBSUB";
        case SRTSUB:
            return "SRT";
        case CC608SUB:
            return "CC";
        case CC708SUB:
            return "CC";
        case UTF8SUB:
            return "UTF-8";
        case TX3GSUB:
            return "TX3G";
        case SSASUB:
            return "SSA";
        case PGSSUB:
            return "PGS";
        default:
            return "Unknown";
    }
}

void hb_hexdump( hb_debug_level_t level, const char * label, const uint8_t * data, int len )
{
    int ii;
    char line[80], ascii[19], *p;

    ascii[18] = 0;
    ascii[0] = '|';
    ascii[17] = '|';
    memset(&ascii[1], '.', 16);
    p = line;
    if( label )
        hb_deep_log(level, "++++ %s ++++", label);
    else
        hb_deep_log(level, "++++++++++++");
    for( ii = 0; ii < len; ii++ )
    {
        if( ( ii & 0x0f ) == 0x0f )
        {
            p += sprintf( p, "%02x", data[ii] );
            hb_deep_log( level, "    %-50s%20s", line, ascii );
            memset(&ascii[1], '.', 16);
            p = line;
        }
        else if( ( ii & 0x07 ) == 0x07 )
        {
            p += sprintf( p, "%02x  ", data[ii] );
        }
        else
        {
            p += sprintf( p, "%02x ", data[ii] );
        }
        if( isgraph( data[ii] ) )
            ascii[(ii & 0x0f) + 1] = data[ii];
        else
            ascii[(ii & 0x0f) + 1] = '.';
    }
    ascii[ii] = 0;
    if( p != line )
    {
        hb_deep_log( level, "    %-50s%20s", line, ascii );
    }
}

int hb_use_dxva( hb_title_t * title )
{                
    return ( (title->video_codec_param == AV_CODEC_ID_MPEG2VIDEO 
              || title->video_codec_param == AV_CODEC_ID_H264
              || title->video_codec_param == AV_CODEC_ID_VC1 
              || title->video_codec_param == AV_CODEC_ID_WMV3 
              || title->video_codec_param == AV_CODEC_ID_MPEG4 )
             && title->opaque_priv );
}

