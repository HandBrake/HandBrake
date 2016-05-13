// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoLevelFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video level factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    /// <summary>
    /// The video tune factory.
    /// </summary>
    public class VideoLevelFactory
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
            }

            return shortName;
        }
    }
}
