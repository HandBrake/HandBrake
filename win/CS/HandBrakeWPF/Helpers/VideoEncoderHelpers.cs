// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoderHelpers.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncoderHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Configuration;

    using HandBrake.Interop.Interop.Model.Encoding;

    public class VideoEncoderHelpers
    {
        public static bool IsH264(VideoEncoder encoder)
        {
            if (encoder == VideoEncoder.X264 || encoder == VideoEncoder.X264_10 || encoder == VideoEncoder.QuickSync || encoder == VideoEncoder.NvencH264)
            {
                return true;
            }

            return false;
        }
    } 
}
