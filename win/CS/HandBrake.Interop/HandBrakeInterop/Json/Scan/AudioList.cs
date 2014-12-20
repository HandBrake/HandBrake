// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Scan
{
    /// <summary>
    /// The audio list.
    /// </summary>
    internal class AudioList
    {
        /// <summary>
        /// Gets or sets the bit rate.
        /// </summary>
        public int BitRate { get; set; }

        /// <summary>
        /// Gets or sets the channel layout.
        /// </summary>
        public int ChannelLayout { get; set; }

        /// <summary>
        /// Gets or sets the description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the language.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets the sample rate.
        /// </summary>
        public int SampleRate { get; set; }
    }
}