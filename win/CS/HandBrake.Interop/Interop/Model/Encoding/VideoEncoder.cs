// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoder.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The video encoder.
    /// </summary>
    public enum VideoEncoder
    {
        [DisplayName("H.264 (x264)")]
        [ShortName("x264")]
        X264 = 0,

        [DisplayName("H.264 10-bit (x264)")]
        [ShortName("x264_10bit")]
        X264_10,

        [DisplayName("H.264 (Intel QSV)")]
        [ShortName("qsv_h264")]
        QuickSync,

        [DisplayName("H.264 (AMD VCE)")]
        [ShortName("vce_h264")]
        VceH264,

        [DisplayName("H.264 (Nvidia NVEnc)")]
        [ShortName("nvenc_h264")]
        NvencH264,
        
        [DisplayName("MPEG-4")]
        [ShortName("mpeg4")]
        FFMpeg,

        [DisplayName("MPEG-2")]
        [ShortName("mpeg2")]
        FFMpeg2,

        [DisplayName("Theora")]
        [ShortName("theora")]
        Theora,

        [DisplayName("H.265 (x265)")]
        [ShortName("x265")]
        X265,

        [DisplayName("H.265 12-bit (x265)")]
        [ShortName("x265_12bit")]
        X265_12,

        [DisplayName("H.265 10-bit (x265)")]
        [ShortName("x265_10bit")]
        X265_10,

        [DisplayName("H.265 (Intel QSV)")]
        [ShortName("qsv_h265")]
        QuickSyncH265,

        [DisplayName("H.265 10-bit (Intel QSV)")]
        [ShortName("qsv_h265_10bit")]
        QuickSyncH26510b,

        [DisplayName("H.265 (AMD VCE)")]
        [ShortName("vce_h265")]
        VceH265,

        [DisplayName("H.265 (Nvidia NVEnc)")]
        [ShortName("nvenc_h265")]
        NvencH265,

        [DisplayName("VP8")]
        [ShortName("VP8")]
        VP8,

        [DisplayName("VP9")]
        [ShortName("VP9")]
        VP9
    }
}
