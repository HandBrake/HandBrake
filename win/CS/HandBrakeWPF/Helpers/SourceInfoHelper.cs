// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceInfoHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Scan.Model;
    using System;

    internal class SourceInfoHelper
    {
        public static string GenerateSourceInfo(Title title)
        {
            if (title != null)
            {
                int parW = title.ParVal.Width;
                int parH = title.ParVal.Height;
                int displayW = title.Resolution.Width * parW / parH;

                string dynamicRange = "SDR";
                if (title.ColorInformation.HDR10plus)
                {
                    dynamicRange = "HDR10+";
                } 
                else if (title.ColorInformation.HDR10)
                {
                    dynamicRange = "HDR10";
                }

                if (title.ColorInformation.DBV)
                {
                    dynamicRange = title.ColorInformation.DBVProfileStr;
                }

                dynamicRange += string.Format(" ({0}-bit {1}, {2})", title.ColorInformation.BitDepth ?? 8, title.ColorInformation.ChromaSubsampling, title.ColorInformation.ColourInfoStr);

                return string.Format("{0}x{1} ({2}x{3}), {4} FPS, {5}, {6} {7}, {8} {9}",
                    title.Resolution.Width,
                    title.Resolution.Height,
                    displayW,
                    title.Resolution.Height,
                    Math.Round(title.Fps, 2),
                    dynamicRange,
                    title.AudioTracks.Count,
                    Resources.MainView_AudioTrackCount,
                    title.Subtitles.Count,
                    Resources.MainView_SubtitleTracksCount);
            }

            return string.Empty;
        }
    }
}
