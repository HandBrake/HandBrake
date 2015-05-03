// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceAudioTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// An audio track from the source video.
    /// </summary>
    public class SourceAudioTrack
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

        /// <summary>
        /// Gets or sets the codec.
        /// </summary>
        public int Codec { get; set; }
    }
}