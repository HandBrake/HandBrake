// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoderHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncoderHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Utilities;

    using VideoEncoder = Model.Video.VideoEncoder;

    public class VideoEncoderHelpers
    {
        public static bool IsX264(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("x264"))
            {
                return true;
            }

            return false;
        }

        public static bool IsX265(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("x265"))
            {
                return true;
            }

            return false;
        }

        public static bool IsH264(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("264"))
            {
                return true;
            }

            return false;
        }

        public static bool IsH265(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("265"))
            {
                return true;
            }

            return false;
        }


        public static bool IsQuickSync(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("qsv"))
            {
                return true;
            }

            return false;
        }

        public static bool IsNVEnc(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("nvenc"))
            {
                return true;
            }

            return false;
        }

        public static bool IsVCN(VideoEncoder encoder)
        {
            if (EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("vce") || EnumHelper<VideoEncoder>.GetShortName(encoder).Contains("vcn"))
            {
                return true;
            }

            return false;
        }
    } 
}
