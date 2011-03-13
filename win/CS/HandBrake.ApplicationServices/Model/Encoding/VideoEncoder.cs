/*  VideoEncoder.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// The Video Encoder
    /// </summary>
    public enum VideoEncoder
    {
        [Description("H.264 (x264)")]
        X264 = 0,

        [Description("MPEG-4 (FFMpeg)")]
        FFMpeg,

        [Description("VP3 (Theora)")]
        Theora
    }
}
