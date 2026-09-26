// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_filter_ids.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    public enum hb_audio_filter_ids
    {
        HB_AUDIO_FILTER_INVALID = 0,
        HB_AUDIO_FILTER_FIRST = 10001,

        HB_AUDIO_FILTER_ADECLICK,
        HB_AUDIO_FILTER_ADECLIP,
        HB_AUDIO_FILTER_AFFTDN,
        HB_AUDIO_FILTER_ANLMDN,
        HB_AUDIO_FILTER_AGATE,
        HB_AUDIO_FILTER_ACOMPRESSOR,
        HB_AUDIO_FILTER_ALIMITER,
        HB_AUDIO_FILTER_DIALOGUENHANCE,
        HB_AUDIO_FILTER_CROSSFEED,
        HB_AUDIO_FILTER_STEREOWIDEN,
        HB_AUDIO_FILTER_LOUDNORM,

        // Finally filters that don't care what order they are in,
        // except that they must be after the above filters
        HB_AUDIO_FILTER_AVFILTER,

        HB_AUDIO_FILTER_LAST
    }
}
