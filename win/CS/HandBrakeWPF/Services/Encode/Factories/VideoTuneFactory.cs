// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoTuneFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video tune factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    /// <summary>
    /// The video tune factory.
    /// </summary>
    public class VideoTuneFactory
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
            switch (shortName)
            {
                case "auto":
                    return "Auto";
                case "film":
                    return "Film";
                case "animation":
                    return "Animation";
                case "grain":
                    return "Grain";
                case "stillimage":
                    return "Still Image";
                case "vq":
                    return "VQ";
                case "psnr":
                    return "PSNR";
                case "ssim":
                    return "SSIM";
                case "screen":
                    return "Screen";
                case "fastdecode":
                    return "Fast Decode";
                case "zerolatency":
                    return "Zero Latency";
            }

            return shortName;
        }
    }
}
