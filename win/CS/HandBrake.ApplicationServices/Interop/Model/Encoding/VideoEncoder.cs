// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The video encoder.
    /// </summary>
    public enum VideoEncoder
    {
        [Display(Name = "H.264 (x264)")]
        [ShortName("x264")]
        X264 = 0,

        [Display(Name = "H.264 (Intel QSV)")]
        [ShortName("qsv_h264")]
        QuickSync,

        [Display(Name = "MPEG-4")]
        [ShortName("mpeg4")]
        FFMpeg,

        [Display(Name = "MPEG-2")]
        [ShortName("mpeg2")]
        FFMpeg2,

        [Display(Name = "Theora")]
        [ShortName("theora")]
        Theora,

        [Display(Name = "H.265 (x265)")]
        [ShortName("x265")]
        X265,

        [Display(Name = "VP8")]
        [ShortName("vp8")]
        VP8
    }
}
