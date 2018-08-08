// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoPresetFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video preset factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    /// <summary>
    /// The video tune factory.
    /// </summary>
    public class VideoPresetFactory
    {
        /// <summary>
        /// The get display name for a given short name.
        /// LibHB doesn't currently support this.
        /// </summary>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetDisplayName(string shortName)
        {
            string presetName = GetNvencPresetName(shortName);
            if (presetName != shortName)
            {
                return presetName;
            }

            switch (shortName)
            {
                case "ultrafast":
                    return "Ultrafast";
                case "superfast":
                    return "Superfast";
                case "veryfast":
                    return "Veryfast";
                case "faster":
                    return "Faster";
                case "fast":
                    return "Fast";
                case "medium":
                    return "Medium";
                case "slow":
                    return "Slow";
                case "slower":
                    return "Slower";
                case "veryslow":
                    return "VerySlow";
                case "placebo":
                    return "Placebo";

                case "balanced":
                    return "Balanced";
                case "speed":
                    return "Speed";
                case "quality":
                    return "Quality";
            }

            return shortName;
        }

        public static string GetNvencPresetName(string shortName)
        {
            switch (shortName)
            {
                case "losslesshp":
                    return "High Performance Lossless";
                case "lossless":
                    return "Lossless";
                case "llhp":
                    return "High Performance Low Latency";
                case "llhq":
                    return "High Quality Low Latency";
                case "ll":
                    return "Low Latency";
                case "bd":
                    return "Bluray Disk";
                case "hq":
                    return "High Quality";
                case "hp":
                    return "High Performance";
                case "fast":
                    return "Fast";
                case "medium":
                    return "Medium";
                case "slow":
                    return "Slow";
                case "default":
                    return "Default";
                case null:
                    return "Automatic";
            }

            return shortName;
        }
    }
}
