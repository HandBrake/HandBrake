// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncoder type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    public enum VideoEncoder
    {
        [DisplayString("H.264 (x264)")]
        X264 = 0,

        [DisplayString("MPEG-4 (FFMpeg)")]
        FFMpeg,

        [DisplayString("VP3 (Theora)")]
        Theora
    }
}
